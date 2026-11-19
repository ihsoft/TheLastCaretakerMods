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


def replace_bytes(path, content):
    temporary = path.with_name(path.name + '.writing')
    temporary.write_bytes(content)
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
        for path in (project.parent / 'Plugins').glob('*/Source/' + identity + '/' + identity + '.Build.cs'):
            # Ready providers carry UBT discovery rules, not an editable source module.
            manifest = path.parents[2] / 'editor-support.json'
            if not (manifest.is_file() and read(manifest).get('kind') == 'Unreal editor support binaries'):
                providers.append(path)
    return providers


@contextmanager
def project_operation(project):
    require_working_project(project)
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


def require_working_project(project):
    if not Path(project).resolve().is_relative_to(HERE.parents[1] / 'Tmp'):
        raise ValueError('BPT engine operations require a disposable project below repository Tmp; copy maintained inputs first')


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


def engine_error_detail(text):
    """Keep the first compiler failure visible alongside the command's stop reason."""
    errors = []
    for line in text.splitlines():
        match = re.search(r'Log(Temp|Blueprint): Error:\s*(.*)', line)
        if match:
            error = (match[1], match[2].strip())
            if error not in errors:
                errors.append(error)
    if not errors:
        return ''
    compiler = next((message for channel, message in errors
                     if channel == 'Blueprint' and '[Compiler]' in message), None)
    if compiler:
        stop = next((message for channel, message in reversed(errors)
                     if channel == 'Temp'), errors[-1][1])
        details = [('first compiler error', compiler)]
        if stop != compiler:
            details.append(('stop', stop))
    else:
        details = [('', message) for _, message in errors[-2:]]
    # Bound each message separately so a long compiler error cannot hide the stop.
    return ': ' + ' | '.join((label + ': ' if label else '') +
                            (message if len(message) <= 300 else message[:297] + '...')
                            for label, message in details)


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
            detail = engine_error_detail((unreal_log if unreal_log.is_file() else log)
                                        .read_text(encoding='utf-8', errors='replace'))
            raise RuntimeError(f'{name} failed ({result.returncode}){detail}; diagnostic: {log}')

    def engine_run(self, mode, name, **fields):
        closed()
        self.command([self.engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', self.project,
                      '-run=BlueprintAssetText', '-Mode=' + mode, '-unattended', '-nop4', '-nosplash', '-nullrhi',
                      '-abslog=' + str(self.run / (name + '-unreal.log')),
                      *['-' + key + '=' + str(value) for key, value in fields.items()]], name)

    def model_preview(self, assets):
        if not assets or len(assets) != 1:
            raise ValueError('ModelPreview requires exactly one Asset (Static Mesh or Actor Blueprint)')
        asset = assets[0]
        # Uncooked Static Mesh bulk data is valid here; the Blueprint codec's
        # single-package restriction does not apply to model export.
        if not re.fullmatch(r'/Game/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)*', asset):
            raise ValueError('ModelPreview requires an explicit /Game package path')
        if not (self.project.parent / 'Content' / (asset[6:] + '.uasset')).is_file():
            raise ValueError('ModelPreview asset is missing from the project: ' + asset)
        from render_model import render_model
        self.prepare(self.project)
        output = self.run / 'model-preview'
        output.mkdir(parents=True)
        write(output / 'request.json', dict(asset=asset))
        script = output / 'export.py'
        script.write_bytes((HERE / 'model_export.py').read_bytes())
        closed()
        try:
            self.command([self.engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', self.project,
                          '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GLTFExporter',
                          '-run=pythonscript', '-script=' + str(script), '-unattended', '-nop4',
                          '-nosplash', '-nullrhi', '-ddc=NoZenLocalFallback',
                          '-abslog=' + str(self.run / 'model-export-unreal.log')], 'model-export')
        except RuntimeError as exc:
            failure = output / 'export-error.json'
            if failure.is_file():
                raise ValueError(read(failure)['error'] + '; diagnostic: ' + str(failure)) from exc
            raise
        evidence = read(output / 'export.json')
        if evidence.get('status') != 'exported' or evidence.get('asset') != asset:
            raise ValueError('ModelPreview export did not confirm the selected asset')
        report = render_model(output / 'model.glb', output / 'index.html', asset)
        if report['meshInstances'] != evidence['meshInstances']:
            raise ValueError('ModelPreview export omitted mesh instances; see export.json')
        write(output / 'geometry.json', report)
        return dict(status='previewed', asset=asset, source='saved-project',
                    previewUrl=(output / 'index.html').as_uri(), **report)

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

    def recover_edit(self):
        """Rollback an interrupted offline source/manifest pair before any next operation."""
        journal = self.internal / 'edit-recovery.json'
        if not journal.exists():
            return
        entries = read(journal)['files']
        resolved = [(inside(self.root, row['path']), row) for row in entries]
        for path, row in resolved:
            if path.read_bytes() not in (row['before'].encode('utf-8'), row['after'].encode('utf-8')):
                raise ValueError('Interrupted offline edit conflicts with subsequent changes: ' + str(path))
        for path, row in resolved:
            temporary = path.with_name(path.name + '.writing')
            temporary.write_bytes(row['before'].encode('utf-8'))
            temporary.replace(path)
        journal.unlink()

    def collapse(self, asset, graph, nodes, name, boundary_names=None):
        from collapse_graph import collapse, review_marks
        data, entries = self.load()
        matches = [entry for entry in entries if asset is None or entry[2]['asset'] == asset]
        if len(matches) != 1:
            raise ValueError('Collapse requires exactly one selected Asset')
        item, source, document = matches[0]
        result_document, result = collapse(document, graph, nodes, name, boundary_names)
        baseline = self.baseline().get(document['asset'])
        if baseline and baseline.get('before'):
            item['reviews'] = review_marks(read(inside(self.internal, baseline['before'])), result_document)
        else:
            item.pop('reviews', None)  # A wholly new asset has no Before view.
        files = [dict(path=str(path.relative_to(self.root)), before=path.read_bytes().decode('utf-8'),
                      after=(json.dumps(value, indent=2, ensure_ascii=False) + '\n').replace('\n', os.linesep))
                 for path, value in ((source, result_document), (self.manifest, data))]
        journal = self.internal / 'edit-recovery.json'
        write(journal, dict(files=files))
        try:
            write(source, result_document)
            write(self.manifest, data)
            journal.unlink()
        except Exception:
            self.recover_edit()
            raise
        result['source'] = str(source)
        return result

    def add_source(self, source, asset, project=None):
        """Stage a relocated library source; Apply owns all actual asset writes."""
        document = codec.relocate(read(source), asset)
        if self.manifest.exists():
            data, entries = self.load()
            if project and Path(project).resolve() != self.project:
                raise ValueError('ProjectFile disagrees with the existing workspace')
            self.check(entries)
            if any(d['asset'].casefold() == asset.casefold() for _, _, d in entries):
                raise ValueError('Destination is already selected: ' + asset)
        else:
            if not project:
                raise ValueError('New source workspace requires ProjectFile')
            self.project = Path(project).resolve()
            if self.project.suffix != '.uproject' or not self.project.is_file():
                raise ValueError('Workspace project is missing')
            if self.root.exists() and any(self.root.iterdir()):
                raise ValueError('New source workspace must be empty')
            data = dict(schema=1, project=str(self.project), assets=[])
        if target(self.project, asset).exists():
            raise ValueError('Destination asset already exists; use Export to edit it: ' + asset)
        relative = 'blueprints/' + asset[6:] + '.json'
        output = inside(self.root, relative)
        if output.exists():
            raise ValueError('Destination source already exists: ' + str(output))
        write(output, document)
        if not self.manifest.exists():
            write(self.internal / 'baseline.json', {})
        data['assets'].append(dict(source=relative))
        write(self.manifest, data)
        return dict(status='source-added', asset=asset, source=str(output), applied=False)

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
        # Older journals (including MarkStubs) describe an active write operation.
        restore_required = journal.get('restoreRequired', True)
        if not isinstance(restore_required, bool):
            raise ValueError('Invalid recovery restoreRequired flag')
        if not restore_required:
            # Only preparation ran; current assets may belong to an external editor.
            journal_path.unlink()
            return False
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
        for row in journal.get('argumentSources', []):
            source = inside(self.root, row['source'])
            original = inside(self.internal, row['original']).read_bytes()
            refreshed = inside(self.internal, row['refreshed']).read_bytes()
            if source.read_bytes() == refreshed:
                replace_bytes(source, original)
            # A subsequent human edit is a pending proposal, not ours to overwrite.
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
        reopen_request = self.run / 'reopen-request.json'
        write(reopen_request, dict(assets=[dict(asset=d['asset']) for _, _, d in entries]))
        reopened = self.run / 'reopened-native'
        reopened.mkdir()
        self.prepare(self.project)
        records = []
        legacy = [doc for _, _, doc in entries if 'nativeDefaults' not in doc and doc['parent'] not in
                  ('/Script/Engine.Actor', '/Script/CoreUObject.Object', '/Script/UMG.UserWidget', '/Script/Engine.BlueprintFunctionLibrary')]
        if legacy:
            request = self.run / 'legacy-defaults-request.json'
            write(request, dict(assets=legacy))
            self.engine_run('ValidateLegacyDefaults', 'legacy-defaults', Input=request)
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
        recovery = dict(project=str(self.project), assets=records, baseline=baseline, restoreRequired=False)
        proposal_bytes = {str(source): source.read_bytes() for _, source, _ in entries}
        write(journal, recovery)
        try:
            self.check(entries)
            # Persist the write phase before the first deletion, so interruption
            # requires recovery even if ImportBatch has not launched yet.
            recovery['restoreRequired'] = True
            write(journal, recovery)
            for record in records:
                target(self.project, record['asset']).unlink(missing_ok=True)
            for i, path in enumerate(inputs):
                self.engine_run('ImportBatch', f'import-{i}', Input=path)
            # Reopen all saved packages in one new process, after all import levels.
            self.engine_run('ExportBatch', 'reopen', Input=reopen_request, Output=reopened)
            for i, (_, _, document) in enumerate(entries):
                actual = codec.compact(read(reopened / f'{i}.native.json'))
                write(self.run / f'reopened-{i}.json', actual)
                differences = codec.compare_materialized(document, actual)
                if differences:
                    diagnostic = self.run / f'comparison-{i}.json'
                    write(diagnostic, differences)
                    raise ValueError('Represented fields differ after fresh-process export: ' + str(diagnostic))
            updated = {}
            refreshes = []
            for i, (_, source, document) in enumerate(entries):
                asset = document['asset']
                byte_path = self.run / 'accepted' / f'{i}.uasset'
                byte_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(target(self.project, asset), byte_path)
                updated[asset] = dict(source=str(source.relative_to(self.root)), bytes=str(byte_path.relative_to(self.internal)),
                                      before=baseline.get(asset, {}).get('before'), accepted=str((self.run / f'reopened-{i}.json').relative_to(self.internal)))
                if document != read(self.run / f'reopened-{i}.json'):
                    if source.read_bytes() != proposal_bytes[str(source)]:
                        raise ValueError('Source changed during Apply: ' + str(source))
                    original = self.run / f'source-snapshot-{i}.json'
                    original.write_bytes(proposal_bytes[str(source)])
                    refreshes.append(dict(source=str(source.relative_to(self.root)), original=str(original.relative_to(self.internal)),
                                          refreshed=str((self.run / f'reopened-{i}.json').relative_to(self.internal))))
            recovery['argumentSources'] = refreshes
            write(journal, recovery)
            for row in refreshes:
                replace_bytes(inside(self.root, row['source']), inside(self.internal, row['refreshed']).read_bytes())
            write(self.internal / 'baseline.json', updated)
            journal.unlink()
        except BaseException:
            self.recover()
            raise
        return dict(status='applied', assets=len(entries), changed=changed, added=added)

    def export(self, project, assets, *, prepared=False):
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
        if not prepared:
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

    def call_templates(self, operations, asset=None):
        if not operations or len(set(operations)) != len(operations) or any(
                not re.fullmatch(r'/(?:Script/[A-Za-z0-9_]+|Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+)\.[A-Za-z0-9_]+::[A-Za-z0-9_]+', op) for op in operations):
            raise ValueError('CallTemplates requires distinct native or /Game library class::Function operations')
        _, entries = self.load()
        selected = [doc for _, _, doc in entries if asset is None or doc['asset'] == asset]
        if len(selected) != 1:
            raise ValueError('CallTemplates requires one selected Asset when the workspace has several sources')
        document = selected[0]
        self.prepare(self.project)
        request, output = self.run / 'call-request.json', self.run / 'call-templates.json'
        write(request, dict(operations=operations, contextParent=document['parent']))
        self.engine_run('CallTemplates', 'call-templates', Input=request, Output=output)
        catalog = read(output)
        if catalog.get('contextParent') != document['parent'] or [e['identity'] for e in catalog.get('entries', [])] != operations:
            raise ValueError('CallTemplates returned a different operation set or parent context')
        # Validate factory consumption before advertising a usable catalogue.
        for entry in catalog['entries']:
            codec.template(catalog, entry['identity'], 'TemplateCheck')
        return dict(status='call-templates', catalog=str(output), asset=document['asset'],
                    contextParent=document['parent'], engineVersion=catalog['engineVersion'],
                    operations=[{k: e[k] for k in ('identity', 'pure', 'requiredConnections')} for e in catalog['entries']])

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
        # The catalog skips exporting the packages in this list. Include selected
        # widgets in its exports so their images are current, independently of Apply.
        catalog = self.preview_catalog([d['asset'] for _, _, d in entries if 'widget' not in d])
        baseline, records = self.baseline(), []
        from call_arguments import preview_documents
        from graph_review import review_marks
        views = {d['asset']: d for d in preview_documents([d for _, _, d in entries])}
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

        # Preview is a review of the selected work, not a project inventory.
        # Include forward context from both snapshots (also before Apply), but
        # never pull in unrelated assets merely because they refer to this work.
        from preview_references import node_references, property_references, package
        visible = set(selected)
        for _, _, document in entries:
            old = baseline.get(document['asset'])
            snapshots = [document]
            if old and old.get('before'):
                snapshots.append(read(inside(self.internal, old['before'])))
            for snapshot in snapshots:
                visible.update(package(ref) for _, ref in property_references(snapshot))
                for graph in snapshot['graphs']:
                    for node in graph['body']['nodes']:
                        visible.update(package(ref) for _, ref in node_references(node, snapshot['pinTemplates']))
        pending = list(visible)
        while pending:
            asset = pending.pop()
            for dependency in catalog.get(asset, {}).get('dependencies', []):
                name = dependency['asset']
                if name not in visible:
                    visible.add(name)
                    pending.append(name)
        catalog = {a: row for a, row in catalog.items() if a in visible}
        missing = {a: row for a, row in missing.items() if a in catalog}
        referenced_by = {a: [r for r in refs if r in catalog] for a, refs in referenced_by.items() if a in catalog}

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
            document = views[asset]
            if any('arguments' in n for g in document['graphs'] for n in g['body']['nodes']):
                view_source = self.run / 'argument-views' / (asset[6:] + '.json')
                write(view_source, document)
                record['source'] = str(view_source)
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
            if old and 'widget' in document:
                native_path = catalog[asset].get('native')
                if not native_path:
                    raise ValueError('Preview catalog omitted current widget resources: ' + asset)
                resources = inside(self.run / 'catalog', native_path).with_suffix('.resources') / 'manifest.json'
                if not resources.is_file():
                    raise ValueError('Preview catalog omitted current widget resources: ' + asset)
                record['resources'] = str(resources)
            before = inside(self.internal, old['before']) if old and old.get('before') else None
            if before and (codec.compare(read(before), document) or review_marks(read(before), document)):
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
        require_working_project(self.project)
        if self.project.suffix != '.uproject' or not self.project.is_file():
            raise ValueError('ProjectFile must be an existing .uproject')
        providers = shared_module_providers(self.project)
        if len(providers) > 1:
            raise ValueError('Multiple Blueprint text module providers; resolve before Prepare')
        project_data = read(self.project)
        if any(p.get('Name') == 'BlueprintTextWorkspace' and p.get('Enabled') is False for p in project_data.get('Plugins', [])):
            raise ValueError('Project explicitly disables BlueprintTextWorkspace; enable it before Prepare')
        # Clean checkouts retain the required plugin reference but not its ready
        # binaries. Dependency probes precede installation; exclude only this
        # absent provider there, then let its own attach verify the full project.
        pending_bpt = (not providers and not (self.project.parent / 'Plugins/BlueprintTextWorkspace').exists()
                       and any(p.get('Name') == 'BlueprintTextWorkspace' for p in project_data.get('Plugins', [])))
        support_results = prepare_project(self.project, self.engine, self.run / 'project-support',
                                          disabled_plugins=('BlueprintTextWorkspace',) if pending_bpt else ())
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
        # K2 nodes belong to an UncookedOnly module, also in legacy source providers.
        descriptors = [self.project, *module.parent.parent.glob('*.uplugin')]
        for descriptor in descriptors:
            data = read(descriptor)
            owned = [m for m in data.get('Modules', []) if m.get('Name') in MODULE_NAMES]
            if any(m.get('Type') == 'Editor' for m in owned):
                write(self.run / ('original-' + descriptor.name), data)
                for row in owned:
                    if row.get('Type') == 'Editor':
                        row['Type'] = 'UncookedOnly'
                write(descriptor, data)
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
    parser.add_argument('action', choices=['Prepare', 'Export', 'ExportDataAsset', 'Apply', 'Preview', 'ModelPreview', 'MarkStubs', 'Recover', 'AddSource', 'CopyAsset', 'Collapse', 'CallTemplates'])
    parser.add_argument('--workspace', required=True)
    parser.add_argument('--project')
    parser.add_argument('--asset', action='append')
    parser.add_argument('--source')
    parser.add_argument('--graph')
    parser.add_argument('--node', action='append')
    parser.add_argument('--name')
    parser.add_argument('--boundary-names', type=json.loads)
    parser.add_argument('--operation', action='append')
    parser.add_argument('--comparison-workspace')
    parser.add_argument('--comparison-label')
    parser.add_argument('--engine', default=r'K:\Epic Games\UE_5.8')
    args = parser.parse_args()
    workspace = None
    try:
        workspace = Workspace(args.workspace, args.engine)
        # Lock outside the workspace so Export can require an empty directory.
        lock_path = workspace.root.parent / (workspace.root.name + '.workspace.lock')
        with ExitStack() as stack:
            stack.enter_context(lock(lock_path))
            data_assets = args.action == 'ExportDataAsset' or (workspace.manifest.exists() and read(workspace.manifest).get('kind') == 'dataAssets')
            if (args.comparison_workspace or args.comparison_label) and (not data_assets or args.action != 'Preview'):
                raise ValueError('ComparisonWorkspace/ComparisonLabel are supported only by DataAsset Preview')
            if data_assets:
                from data_asset_workspace import DataAssetWorkspace
                if args.action not in ('ExportDataAsset', 'Apply', 'Preview', 'Recover', 'Prepare', 'ModelPreview'):
                    raise ValueError('This action requires a Blueprint workspace, not a DataAsset workspace')
                workspace = DataAssetWorkspace(args.workspace, args.engine)
            workspace.recover_edit()
            if args.action == 'Collapse':
                if not args.graph or not args.node or not args.name or (args.asset and len(args.asset) != 1):
                    raise ValueError('Collapse requires Graph, Node IDs, Name and at most one Asset')
                if (workspace.internal / 'recovery.json').exists():
                    raise ValueError('Interrupted asset write requires Recover before another operation')
                result = workspace.collapse(args.asset[0] if args.asset else None, args.graph,
                                            args.node, args.name, args.boundary_names)
                result.update(workspace=str(workspace.root), manifest=str(workspace.manifest))
                print(json.dumps(result))
                return 0
            if args.action == 'AddSource':
                if not args.source or not args.asset or len(args.asset) != 1:
                    raise ValueError('AddSource requires SourcePath and exactly one destination Asset')
                if (workspace.internal / 'recovery.json').exists():
                    raise ValueError('Interrupted asset write requires Recover before another operation')
                result = workspace.add_source(args.source, args.asset[0], args.project)
                result.update(workspace=str(workspace.root), manifest=str(workspace.manifest))
                print(json.dumps(result))
                return
            if args.action == 'ModelPreview':
                if not args.asset or len(args.asset) != 1:
                    raise ValueError('ModelPreview requires exactly one Asset')
                saved_project = read(workspace.manifest)['project'] if workspace.manifest.exists() else None
                if args.project and saved_project and Path(args.project).resolve() != Path(saved_project).resolve():
                    raise ValueError('ProjectFile disagrees with the existing workspace project')
                if not (args.project or saved_project):
                    raise ValueError('ModelPreview requires ProjectFile or an existing workspace')
                workspace.project = Path(args.project or saved_project).resolve()
                if workspace.project.suffix != '.uproject' or not workspace.project.is_file():
                    raise ValueError('ProjectFile must be an existing .uproject')
            elif args.action in ('Export', 'ExportDataAsset', 'Prepare', 'CopyAsset'):
                if not args.project:
                    raise ValueError('ProjectFile is required for ' + args.action)
                workspace.project = Path(args.project).resolve()
            elif args.action == 'Recover':
                state = workspace.manifest if workspace.manifest.exists() else workspace.internal / 'recovery.json'
                workspace.project = Path(read(state)['project']).resolve()
            else:
                workspace.load()
                if data_assets and args.project and Path(args.project).resolve() != workspace.project:
                    raise ValueError('ProjectFile disagrees with the DataAsset workspace project')
            if not (data_assets and args.action == 'Preview'):
                stack.enter_context(project_operation(workspace.project))
            if args.action != 'Recover' and (workspace.internal / 'recovery.json').exists():
                raise ValueError('Interrupted asset write requires Recover before another operation')
            if args.action == 'Prepare':
                result = workspace.prepare(args.project)
            elif args.action == 'CopyAsset':
                if not args.source or not args.asset or len(args.asset) != 1:
                    raise ValueError('CopyAsset requires SourcePath and exactly one destination Asset')
                from native_library import copy_asset
                result = copy_asset(workspace, args.source, args.asset[0])
            elif args.action in ('Export', 'ExportDataAsset'):
                result = workspace.export(args.project, args.asset)
            elif args.action == 'Apply':
                result = workspace.apply()
            elif args.action == 'Preview':
                result = workspace.preview(args.comparison_workspace, args.comparison_label) if data_assets else workspace.preview()
            elif args.action == 'ModelPreview':
                result = workspace.model_preview(args.asset)
            elif args.action == 'CallTemplates':
                if args.asset and len(args.asset) != 1:
                    raise ValueError('CallTemplates accepts at most one Asset')
                result = workspace.call_templates(args.operation, args.asset[0] if args.asset else None)
            elif args.action == 'MarkStubs':
                result = workspace.mark_stubs(args.asset)
            else:
                restored = workspace.recover()
                result = dict(status='recovered' if restored else 'no-recovery-needed',
                              recoveredCandidates=str(workspace.recovery_candidates) if restored else None)
            result.update(workspace=str(workspace.root),
                          manifest=(str(workspace.manifest) if args.action != 'ModelPreview' or workspace.manifest.exists() else None),
                          evidence=str(workspace.run))
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
    sys.modules.setdefault('blueprint_workspace', sys.modules[__name__])
    sys.exit(main())
