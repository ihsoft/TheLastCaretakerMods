"""Shared, transactional schema-2 editing in an existing Unreal project."""
import argparse
from contextlib import ExitStack, contextmanager
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid
import time

import asset_text_codec as codec

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from unreal_coordination import engine_slot
from UnrealEditorSupport.project_support import prepare_project, prepare_provider

HERE = Path(__file__).resolve().parent
OWNER = HERE.parents[1] / 'Tmp' / 'BlueprintText'
# Existing workspaces/projects keep their original identity; new providers use BlueprintText.
LEGACY_OWNER = HERE.parents[1] / 'Tmp' / 'BlueprintTextPrototype'
MODULE_NAMES = ('BlueprintText', 'BlueprintTextPrototype')


def module_template(identity):
    """Current shared source, adapted only at the legacy Unreal module boundary."""
    if identity not in MODULE_NAMES:
        raise ValueError('Unknown shared Blueprint module identity')
    template = HERE / 'ProjectTemplate/Source/BlueprintText'
    files = {p.relative_to(template): p.read_bytes() for p in template.rglob('*') if p.is_file()}
    if identity != 'BlueprintText':
        build = files.pop(Path('BlueprintText.Build.cs')).decode('utf-8')
        build = build.replace('class BlueprintText :', 'class ' + identity + ' :').replace('public BlueprintText(', 'public ' + identity + '(')
        files[Path(identity + '.Build.cs')] = build.encode('utf-8')
        implementation = files.pop(Path('BlueprintText.cpp')).decode('utf-8')
        implementation = implementation.replace('IMPLEMENT_MODULE(FDefaultModuleImpl, BlueprintText)', 'IMPLEMENT_MODULE(FDefaultModuleImpl, ' + identity + ')')
        files[Path(identity + '.cpp')] = implementation.encode('utf-8')
    return files


def write_tree(root, files):
    for relative, content in files.items():
        path = inside(root, relative)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.writing')
    temporary.write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    temporary.replace(path)


def inside(root, value):
    root, path = Path(root).resolve(), (Path(root) / value).resolve()
    if not path.is_relative_to(root) or path == root:
        raise ValueError('Path escapes its owner: ' + str(value))
    return path


def target(project, asset):
    if not isinstance(asset, str) or not re.fullmatch(r'/Game/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)*', asset):
        raise ValueError('Only explicit /Game Blueprint package identities are supported: ' + str(asset))
    content = inside(project.parent, 'Content')
    result = inside(content, asset[6:] + '.uasset')
    if any(result.with_suffix(ext).exists() for ext in ('.uexp', '.ubulk')):
        raise ValueError('Split packages are unsupported: ' + asset)
    return result


@contextmanager
def lock(path, *, wait=False):
    """Kernel-held lock; a crashed owner releases it. Never delete another lock."""
    path.parent.mkdir(parents=True, exist_ok=True)
    stream = path.open('a+b')
    stream.seek(0)
    if path.stat().st_size == 0:
        stream.write(b' '); stream.flush(); stream.seek(0)
    while True:
        try:
            if os.name == 'nt':
                import msvcrt
                msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            break
        except (BlockingIOError, PermissionError):
            if not wait:
                stream.close()
                raise ValueError('Competing Blueprint workspace operation: ' + str(path))
            time.sleep(1)
    try:
        yield
    finally:
        stream.close()


def closed():
    if os.name == 'nt':
        process = subprocess.run(['tasklist.exe', '/FO', 'CSV', '/NH'], capture_output=True, text=True, check=True)
        if re.search(r'"UnrealEditor(?:-Cmd)?\.exe"', process.stdout, re.I):
            raise ValueError('Close Unreal Editor and UnrealEditor-Cmd before this operation')


def shared_module_providers(project):
    providers = []
    for identity in MODULE_NAMES:
        providers.extend((project.parent / 'Source').glob('*/' + identity + '.Build.cs'))
        providers.extend((project.parent / 'Plugins').glob('*/Source/' + identity + '/' + identity + '.Build.cs'))
    return providers


@contextmanager
def project_operation(project):
    # closed() observes all editor processes, so serialize entire engine cycles,
    # including the gaps between subprocesses and rollback after a failed import.
    # Fixed order also coordinates with older tools still using the legacy gate.
    with engine_slot('BlueprintText: '+str(project)), lock(LEGACY_OWNER / '.engine-operation.lock', wait=True):
        with lock(OWNER / '.engine-operation.lock', wait=True):
            with lock(project.parent / 'Saved/BlueprintTextWorkspace.lock', wait=True):
                if len(shared_module_providers(project)) > 1:
                    raise ValueError('Multiple Blueprint text module providers; resolve before engine work')
                closed()
                yield


def groups(sources):
    """Keep peer references together; parents must be imported before children."""
    pending = {s['asset']: s for s in sources}
    if len(pending) != len(sources):
        raise ValueError('Duplicate asset identity')
    result = []
    while pending:
        level = [s for s in pending.values() if s['parent'].split('.', 1)[0] not in pending]
        if not level:
            raise ValueError('Cyclic selected Blueprint inheritance')
        result.append(level)
        for s in level:
            del pending[s['asset']]
    for index, level in enumerate(result):
        later = [s['asset'] for group in result[index + 1:] for s in group]
        for source in level:
            text = json.dumps(source)
            if any(re.search(re.escape(asset) + r'[.\\"]', text) for asset in later):
                raise ValueError('Selected parent references a later child; this inheritance/reference cycle is unsupported: ' + source['asset'])
    return result


class Workspace:
    def __init__(self, root, engine):
        self.root = Path(root).resolve()
        if not any(self.root.is_relative_to(owner.resolve()) and self.root != owner.resolve() for owner in (OWNER, LEGACY_OWNER)):
            raise ValueError('Workspace must be below Tmp/BlueprintText')
        self.manifest = self.root / 'workspace.json'
        self.internal = inside(self.root, '.workspace')
        self.engine = Path(engine).resolve()
        self.run = self.internal / 'runs' / uuid.uuid4().hex[:12]
        self.project = None
        self.recovery_candidates = None

    def command(self, args, name):
        self.run.mkdir(parents=True, exist_ok=True)
        log = self.run / (name + '.log')
        with log.open('w', encoding='utf-8') as stream:
            result = subprocess.run([str(a) for a in args], stdout=stream, stderr=subprocess.STDOUT)
        if result.returncode:
            unreal_log = self.run / (name + '-unreal.log')
            detail = ''
            if unreal_log.is_file():
                errors = [line.split('Error:', 1)[1].strip() for line in unreal_log.read_text(encoding='utf-8', errors='replace').splitlines()
                          if re.search(r'Log(?:Temp|Blueprint): Error:', line)]
                detail = ': ' + ' | '.join(errors[-2:])[:500] if errors else ''
            raise RuntimeError(f'{name} failed ({result.returncode}){detail}; diagnostic: {log}')

    def engine_run(self, mode, name, **fields):
        closed()
        self.command([self.engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', self.project,
                      '-run=BlueprintAssetText', '-Mode=' + mode, '-unattended', '-nop4', '-nosplash', '-nullrhi',
                      '-abslog=' + str(self.run / (name + '-unreal.log')),
                      *['-' + key + '=' + str(value) for key, value in fields.items()]], name)

    def export_one(self, asset, output, name, resources=False):
        native_path = self.run / (name + '.native.json')
        self.engine_run('Export', name, Asset=asset, Output=native_path)
        document = codec.compact(read(native_path))
        write(output, document)
        if resources and 'widget' in document:
            self.engine_run('Export', name + '-resources', Asset=asset, Output=native_path,
                            PreviewResources=output.with_suffix('.resources'))
        return document

    def load(self):
        data = read(self.manifest)
        if set(data) - {'schema', 'project', 'assets', 'title', 'description'} or data.get('schema') != 1:
            raise ValueError('Invalid workspace manifest schema or fields')
        self.project = Path(data['project']).resolve()
        if self.project.suffix != '.uproject' or not self.project.is_file():
            raise ValueError('Workspace project is missing')
        if not isinstance(data['assets'], list) or not data['assets']:
            raise ValueError('Workspace assets must be nonempty')
        entries, identities, paths = [], set(), set()
        for item in data['assets']:
            if not isinstance(item, dict) or set(item) - {'source', 'placeholder', 'note', 'reviews'}:
                raise ValueError('Asset records require source and optional placeholder/note/reviews')
            source = inside(inside(self.root, 'blueprints'), str(Path(item['source']).relative_to('blueprints')))
            document = read(source)
            codec.native(document)
            asset = document['asset']
            target(self.project, asset)
            if asset.casefold() in identities or str(source).casefold() in paths:
                raise ValueError('Duplicate asset or source in workspace')
            identities.add(asset.casefold()); paths.add(str(source).casefold())
            entries.append((item, source, document))
        return data, entries

    def baseline(self):
        return read(self.internal / 'baseline.json')

    def check(self, entries):
        baseline = self.baseline()
        selected = {source['asset'] for _, _, source in entries}
        if not set(baseline).issubset(selected):
            raise ValueError('Removing exported assets from the manifest is unsupported; restore their entries')
        for _, source, document in entries:
            asset = document['asset']
            file = target(self.project, asset)
            old = baseline.get(asset)
            if old:
                if source != inside(self.root, old['source']):
                    raise ValueError('Exported source identity or mapping changed: ' + asset)
                if not file.is_file() or file.read_bytes() != inside(self.internal, old['bytes']).read_bytes():
                    raise ValueError('Project asset changed since export/apply; export to a new workspace: ' + asset)
            elif file.exists():
                raise ValueError('New source would overwrite an unselected asset: ' + asset)
        return baseline

    def recover(self):
        journal_path = self.internal / 'recovery.json'
        if not journal_path.exists():
            return False
        journal = read(journal_path)
        if journal['project'] != str(self.project):
            raise ValueError('Recovery project mismatch')
        # Retain every current candidate before changing any selected path. These
        # may contain partial engine saves or post-interruption manual edits.
        candidates = self.run / 'recovery-candidates'
        inventory = []
        for record in journal['assets']:
            file = target(self.project, record['asset'])
            if record['backup'] is not None:
                original = inside(self.internal, record['backup']).read_bytes()
                baseline_record = journal['baseline'].get(record['asset'])
                if baseline_record and original != inside(self.internal, baseline_record['bytes']).read_bytes():
                    raise ValueError('Recovery backup differs from accepted original; preserve evidence: ' + record['asset'])
            retained = candidates / (record['asset'][6:] + '.uasset')
            if file.exists():
                retained.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(file, retained)
                if file.read_bytes() != retained.read_bytes():
                    raise RuntimeError('Recovery candidate preservation failed: ' + str(file))
            inventory.append(dict(asset=record['asset'], retained=str(retained) if file.exists() else None))
        write(candidates / 'manifest.json', dict(assets=inventory))
        self.recovery_candidates = candidates
        for record in journal['assets']:
            file = target(self.project, record['asset'])
            if record['backup'] is None:
                file.unlink(missing_ok=True)
            else:
                backup = inside(self.internal, record['backup'])
                file.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(backup, file)
                if file.read_bytes() != backup.read_bytes():
                    raise RuntimeError('Recovery copy verification failed: ' + str(file))
        write(self.internal / 'baseline.json', journal['baseline'])
        journal_path.unlink()
        return True

    def apply(self):
        _, entries = self.load()
        baseline = self.check(entries)
        changed = sum(1 for _, _, d in entries if d['asset'] in baseline and
                      codec.compare(read(inside(self.internal, baseline[d['asset']]['accepted'])), d))
        added = sum(1 for _, _, d in entries if d['asset'] not in baseline)
        if not changed and not added:
            return dict(status='unchanged', assets=len(entries), changed=0, added=0)
        levels = groups([d for _, _, d in entries])
        # Convert and validate all desired sources before touching any package.
        inputs = []
        for i, level in enumerate(levels):
            path = self.run / f'group-{i}.native.json'
            write(path, {'assets': [codec.native(d) for d in level]})
            inputs.append(path)
        self.prepare(self.project)
        records = []
        for _, _, document in entries:
            file = target(self.project, document['asset'])
            backup = self.run / 'original' / (document['asset'][6:] + '.uasset')
            if file.exists():
                backup.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(file, backup)
                if file.read_bytes() != backup.read_bytes():
                    raise RuntimeError('Backup verification failed')
            records.append(dict(asset=document['asset'], backup=str(backup.relative_to(self.internal)) if file.exists() else None))
        journal = self.internal / 'recovery.json'
        write(journal, dict(project=str(self.project), assets=records, baseline=baseline))
        try:
            self.check(entries)
            for record in records:
                target(self.project, record['asset']).unlink(missing_ok=True)
            for i, path in enumerate(inputs):
                self.engine_run('ImportBatch', f'import-{i}', Input=path)
            for i, (_, _, document) in enumerate(entries):
                actual = self.export_one(document['asset'], self.run / f'reopened-{i}.json', f'reopen-{i}', resources=True)
                differences = codec.compare(document, actual)
                if differences:
                    diagnostic = self.run / f'comparison-{i}.json'
                    write(diagnostic, differences)
                    raise ValueError('Represented fields differ after fresh-process export: ' + str(diagnostic))
            updated = {}
            for i, (_, source, document) in enumerate(entries):
                asset = document['asset']
                byte_path = self.run / 'accepted' / f'{i}.uasset'
                byte_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(target(self.project, asset), byte_path)
                resources = (self.run / f'reopened-{i}.json').with_suffix('.resources')
                if resources.exists():
                    destination = source.with_suffix('.resources')
                    shutil.copytree(resources, destination, dirs_exist_ok=True)
                updated[asset] = dict(source=str(source.relative_to(self.root)), bytes=str(byte_path.relative_to(self.internal)),
                                      before=baseline.get(asset, {}).get('before'), accepted=str((self.run / f'reopened-{i}.json').relative_to(self.internal)))
            write(self.internal / 'baseline.json', updated)
            journal.unlink()
        except BaseException:
            self.recover()
            raise
        return dict(status='applied', assets=len(entries), changed=changed, added=added)

    def export(self, project, assets):
        if self.manifest.exists() or self.root.exists() and any(p.name != '.workspace' for p in self.root.iterdir()):
            raise ValueError('Export requires a new or empty workspace')
        self.project = Path(project).resolve()
        if self.project.suffix != '.uproject' or not self.project.is_file():
            raise ValueError('ProjectFile must be an existing .uproject')
        if not assets or len({a.casefold() for a in assets}) != len(assets):
            raise ValueError('Export requires unique selected asset packages')
        for asset in assets:
            if not target(self.project, asset).is_file():
                raise ValueError('Selected asset is missing: ' + asset)
        self.prepare(self.project)
        baseline, records = {}, []
        for i, asset in enumerate(assets):
            file = target(self.project, asset)
            original = file.read_bytes()
            source = self.root / 'blueprints' / (asset[6:] + '.json')
            self.export_one(asset, source, f'export-{i}', resources=True)
            if file.read_bytes() != original:
                raise ValueError('Asset changed during export: ' + asset)
            before = self.internal / 'before' / f'{i}.json'
            byte_path = self.internal / 'baseline' / f'{i}.uasset'
            write(before, read(source)); byte_path.parent.mkdir(parents=True, exist_ok=True); byte_path.write_bytes(original)
            resources = source.with_suffix('.resources')
            if resources.exists():
                shutil.copytree(resources, before.with_suffix('.resources'))
            baseline[asset] = dict(source=str(source.relative_to(self.root)), bytes=str(byte_path.relative_to(self.internal)), before=str(before.relative_to(self.internal)), accepted=str(before.relative_to(self.internal)))
            records.append(dict(source=str(source.relative_to(self.root))))
        write(self.internal / 'baseline.json', baseline)
        write(self.manifest, dict(schema=1, project=str(self.project), assets=records))
        return dict(status='exported', assets=len(records))

    def preview_catalog(self, selected):
        request, output = self.run / 'catalog-request.json', self.run / 'catalog/catalog.json'
        write(request, dict(selected=selected))
        self.engine_run('PreviewCatalog', 'preview-catalog', Input=request, Output=output)
        catalog = read(output)
        if catalog.get('schema') != 1 or not isinstance(catalog.get('assets'), list):
            raise ValueError('Invalid project preview catalog')
        rows, identities = {}, set()
        for row in catalog['assets']:
            asset = row['asset']
            if asset.casefold() in identities or row.get('role') not in ('stub', 'unclassified'):
                raise ValueError('Duplicate asset or unknown role in preview catalog: ' + asset)
            identities.add(asset.casefold())
            rows[asset] = row
        return rows

    def mark_stubs(self, assets):
        # Explicit migration for existing dependency assets. Future creators must
        # set BlueprintText.Role on creation; preview never guesses this role.
        _, entries = self.load()
        selected = {d['asset'] for _, _, d in entries}
        if not assets or len({a.casefold() for a in assets}) != len(assets):
            raise ValueError('MarkStubs requires unique existing dependency assets')
        if selected.intersection(assets):
            raise ValueError('MarkStubs cannot change selected workspace assets; use editorRole in their BPT source and Apply')
        files = [(a, target(self.project, a)) for a in assets]
        if any(not p.is_file() for _, p in files):
            raise ValueError('MarkStubs requires existing project assets')
        self.prepare(self.project)
        records = []
        for asset, file in files:
            backup = self.run / 'original' / (asset[6:] + '.uasset')
            backup.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(file, backup)
            if file.read_bytes() != backup.read_bytes():
                raise RuntimeError('Stub metadata backup verification failed')
            records.append(dict(asset=asset, backup=str(backup.relative_to(self.internal))))
        journal = self.internal / 'recovery.json'
        write(journal, dict(project=str(self.project), assets=records, baseline=self.baseline()))
        request = self.run / 'mark-stubs.json'
        write(request, dict(assets=assets))
        try:
            self.engine_run('MarkStubs', 'mark-stubs', Input=request)
            rows = self.preview_catalog(sorted(selected))
            if any(rows.get(a, {}).get('role') != 'stub' for a in assets):
                raise ValueError('Stub metadata did not survive fresh-process reopen')
            journal.unlink()
        except BaseException:
            self.recover()
            raise
        return dict(status='stubs-marked', assets=len(assets))

    def preview(self):
        from render_preview_index import generate
        data, entries = self.load()
        self.check(entries)
        self.prepare(self.project)
        catalog = self.preview_catalog([d['asset'] for _, _, d in entries])
        baseline, records = self.baseline(), []
        selected = {d['asset'] for _, _, d in entries}
        referenced_by = {a: [] for a in catalog}
        missing = {}
        for asset, row in catalog.items():
            for dependency in row['dependencies']:
                name = dependency['asset']
                referenced_by.setdefault(name, []).append(asset)
                if dependency['missing']:
                    if name in catalog:
                        raise ValueError('Catalog reports an existing asset as missing: ' + name)
                    missing[name] = dict(asset=name, role='unclassified', **{'class': ''}, dependencies=[],
                                         reason='Missing dependency package in this editor project.')
        catalog.update(missing)

        def context(asset):
            row = catalog[asset]
            return dict(placeholder=row['role'] == 'stub', context=dict(
                asset=asset, assetClass=row['class'], role=row['role'],
                reason=row.get('reason', ''), selected=asset in selected,
                dependencies=[r['asset'] for r in row['dependencies']],
                referencedBy=sorted(set(referenced_by.get(asset, [])))))

        for item, source, document in entries:
            record = dict(item, source=str(source))
            asset = document['asset']
            if asset not in catalog:
                if asset in baseline or target(self.project, asset).exists():
                    raise ValueError('Project catalog omitted selected asset: ' + asset)
                # Proposed, not yet applied assets are explicitly distinguished.
                catalog[asset] = dict(asset=asset, role=document.get('editorRole', 'unclassified'),
                                     **{'class': 'Proposed Blueprint'}, dependencies=[], reason='Not yet applied to the project.')
            record.update(context(asset))
            proposed_role = document.get('editorRole', 'unclassified')
            if proposed_role != record['context']['role']:
                project_role = record['context']['role']
                record['placeholder'] = proposed_role == 'stub'
                record['context'].update(role=proposed_role,
                    reason=f'Editor role change proposed in workspace; current project role: {project_role}.')
            if item.get('placeholder', False) and not record['placeholder']:
                raise ValueError('Workspace placeholder disagrees with project metadata; mark the actual asset as a stub: ' + asset)
            old = baseline.get(document['asset'])
            before = inside(self.internal, old['before']) if old and old.get('before') else None
            if before and codec.compare(read(before), document):
                record.update(before=str(before), status='changed')
            elif not before:
                record['status'] = 'added'
            records.append(record)
        for asset, row in sorted(catalog.items()):
            if asset in selected:
                continue
            record = context(asset)
            if row.get('native'):
                native = inside(self.run / 'catalog', row['native'])
                source = native.with_suffix('.compact.json')
                write(source, codec.compact(read(native)))
                record['source'] = str(source)
                resources = native.with_suffix('.resources') / 'manifest.json'
                if resources.is_file():
                    record['resources'] = str(resources)
            records.append(record)
        preview_manifest = self.run / 'preview.json'
        write(preview_manifest, dict(title=data.get('title', 'Blueprint workspace'), description=data.get('description', ''), assets=records))
        result = generate(preview_manifest, self.run / 'preview')
        return dict(status='previewed', preview=result['index'], previewUrl=Path(result['index']).as_uri(),
                    assets=result['assets'], graphs=result['graphs'], selectedAssets=len(entries),
                    editableAssets=result['editableAssets'], listedAssets=result['listedAssets'],
                    stubs=sum(r['placeholder'] for r in records), missingDependencies=len(missing),
                    informationalAssets=sum('source' not in r for r in records),
                    unsupportedBlueprints=[a for a, row in catalog.items()
                                           if row['class'].endswith('Blueprint') and row.get('reason') and a not in selected])

    def refresh_module(self, module):
        """Refresh only the shared module, retaining its complete previous tree."""
        module = module.resolve()
        if module.name not in MODULE_NAMES or not module.is_relative_to(self.project.parent.resolve()):
            raise ValueError('Shared module must be inside the selected project: ' + str(module))
        def snapshot(folder):
            entries = list(folder.rglob('*'))
            if any(p.is_symlink() or getattr(p, 'is_junction', lambda: False)() for p in entries):
                raise ValueError('Shared module contains linked paths; resolve before Prepare: ' + str(folder))
            return {p.relative_to(folder): p.read_bytes() for p in entries if p.is_file()}

        current, desired = snapshot(module), module_template(module.name)
        if current == desired:
            return None
        backup = self.run / 'module-before' / module.name
        shutil.copytree(module, backup)
        if snapshot(backup) != current:
            raise RuntimeError('Shared module backup verification failed; module unchanged')
        saved = inside(self.project.parent, 'Saved')
        staging = inside(saved, 'BlueprintTextWorkspace-prepare-' + uuid.uuid4().hex)
        replacement, previous = staging / 'replacement', staging / 'previous'
        write_tree(replacement, desired)
        if snapshot(replacement) != desired:
            raise RuntimeError('Shared module staging verification failed; module unchanged')
        module.rename(previous)
        try:
            replacement.rename(module)
            if snapshot(module) != desired:
                raise RuntimeError('Shared module replacement verification failed')
        except BaseException:
            if module.exists():
                module.rename(staging / 'rejected')
            previous.rename(module)
            raise
        # The resolved staging target was checked under this project's Saved;
        # the old contents also remain in the verified run-evidence backup.
        shutil.rmtree(staging)
        return backup

    def prepare(self, project):
        self.project = Path(project).resolve()
        if self.project.suffix != '.uproject' or not self.project.is_file():
            raise ValueError('ProjectFile must be an existing .uproject')
        providers = shared_module_providers(self.project)
        if len(providers) > 1:
            raise ValueError('Multiple Blueprint text module providers; resolve before Prepare')
        project_data = read(self.project)
        if any(p.get('Name') == 'BlueprintTextWorkspace' and p.get('Enabled') is False for p in project_data.get('Plugins', [])):
            raise ValueError('Project explicitly disables BlueprintTextWorkspace; enable it before Prepare')
        support_results = prepare_project(self.project, self.engine, self.run / 'project-support')
        if not providers:
            result = prepare_provider(self.project, self.engine, self.run / 'bpt-support',
                                      HERE / 'EditorSupport/BlueprintTextWorkspace.uplugin',
                                      [dict(source='../ProjectTemplate/Source/BlueprintText', destination='Source/BlueprintText')])
            return dict(status='prepared', project=str(self.project), moduleIdentity='BlueprintText',
                        binaryProvider=result, projectSupport=support_results)
        # Preserve existing explicit source-module projects and their legacy identity.
        # New consumers and migrated projects use the ready provider above.
        module = providers[0].parent
        module_backup = self.refresh_module(module)
        editor_targets = []
        for path in (self.project.parent / 'Source').glob('*.Target.cs'):
            if re.search(r'Type\s*=\s*TargetType\.Editor', path.read_text(encoding='utf-8-sig')):
                editor_targets.append(path.name.removesuffix('.Target.cs'))
        preferred = self.project.stem + 'Editor'
        selected = preferred if preferred in editor_targets else editor_targets[0] if len(editor_targets) == 1 else 'UnrealEditor' if not editor_targets else None
        if not selected:
            raise ValueError('Ambiguous Editor target; use the project convention <Project>Editor')
        self.command(['cmd.exe', '/d', '/c', str(self.engine / 'Engine/Build/BatchFiles/Build.bat'), selected,
                      'Win64', 'Development', '-Project=' + str(self.project), '-WaitMutex', '-NoHotReloadFromIDE'], 'prepare-build')
        return dict(status='prepared', project=str(self.project), reusedModule=bool(providers),
                    refreshedModule=module_backup is not None, moduleBackup=str(module_backup) if module_backup else None, moduleIdentity=module.name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['Prepare', 'Export', 'Apply', 'Preview', 'MarkStubs', 'Recover'])
    parser.add_argument('--workspace', required=True)
    parser.add_argument('--project')
    parser.add_argument('--asset', action='append')
    parser.add_argument('--engine', default=r'K:\Epic Games\UE_5.8')
    args = parser.parse_args()
    workspace = None
    try:
        workspace = Workspace(args.workspace, args.engine)
        # Lock outside the workspace so Export can require an empty directory.
        lock_path = workspace.root.parent / (workspace.root.name + '.workspace.lock')
        with ExitStack() as stack:
            stack.enter_context(lock(lock_path))
            if args.action in ('Export', 'Prepare'):
                if not args.project:
                    raise ValueError('ProjectFile is required for Export and Prepare')
                workspace.project = Path(args.project).resolve()
            elif args.action == 'Recover':
                workspace.project = Path(read(workspace.manifest)['project']).resolve()
            else:
                workspace.load()
            stack.enter_context(project_operation(workspace.project))
            if args.action != 'Recover' and (workspace.internal / 'recovery.json').exists():
                raise ValueError('Interrupted asset write requires Recover before another operation')
            if args.action == 'Prepare':
                result = workspace.prepare(args.project)
            elif args.action == 'Export':
                result = workspace.export(args.project, args.asset)
            elif args.action == 'Apply':
                result = workspace.apply()
            elif args.action == 'Preview':
                result = workspace.preview()
            elif args.action == 'MarkStubs':
                result = workspace.mark_stubs(args.asset)
            else:
                restored = workspace.recover()
                result = dict(status='recovered' if restored else 'no-recovery-needed',
                              recoveredCandidates=str(workspace.recovery_candidates) if restored else None)
            result.update(workspace=str(workspace.root), manifest=str(workspace.manifest), evidence=str(workspace.run))
            try:
                write(workspace.run / 'result.json', result)
            except OSError as evidence_error:
                result['evidenceWriteWarning'] = str(evidence_error)[:300]
            print(json.dumps(result))
    except Exception as exc:
        diagnostic = workspace.run / 'failure.json' if workspace else None
        if diagnostic:
            try:
                write(diagnostic, dict(error=str(exc), type=type(exc).__name__))
            except OSError:
                diagnostic = None
        print(json.dumps(dict(status='failed', reason=str(exc)[:1000], diagnostic=str(diagnostic) if diagnostic else None,
                              recoveredCandidates=str(workspace.recovery_candidates) if workspace and workspace.recovery_candidates else None)))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
