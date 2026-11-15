"""Existing DataAsset properties in the shared BPT workspace transaction."""
import copy
import shutil
from pathlib import Path

from blueprint_workspace import Workspace, inside, read, target, write


def validate(document):
    if set(document) != {'schema', 'kind', 'asset', 'objectName', 'class', 'objects'} or document['schema'] != 2 or document['kind'] != 'dataAsset':
        raise ValueError('Expected DataAsset schema 2; export the saved project into a new workspace')
    if not all(isinstance(document[k], str) and document[k] for k in ('asset', 'class', 'objectName')):
        raise ValueError('DataAsset identities must be nonempty strings')
    if not isinstance(document['objects'], dict) or '.' not in document['objects']:
        raise ValueError('objects must include the root object "."')
    for key, obj in document['objects'].items():
        if not isinstance(key, str) or not key or not isinstance(obj, dict) or set(obj) != {'class', 'properties'}:
            raise ValueError('Invalid DataAsset object')
        if not isinstance(obj['class'], str) or not obj['class'] or not isinstance(obj['properties'], dict):
            raise ValueError('Object class/properties are required: ' + key)
        for name, row in obj['properties'].items():
            if not isinstance(name, str) or not name or not isinstance(row, dict) or set(row) != {'type', 'editable', 'value'}:
                raise ValueError('Invalid property: ' + str(name))
            if not isinstance(row['type'], str) or not row['type'] or type(row['editable']) is not bool:
                raise ValueError('Invalid property metadata: ' + name)
            value = row['value']
            if not isinstance(value, str) and not (isinstance(value, list) and value and all(isinstance(v, str) for v in value)):
                raise ValueError('Property value must be Unreal text, or text elements for a fixed array: ' + name)
    if document['objects']['.']['class'] != document['class']:
        raise ValueError('Root object class disagrees with DataAsset class')


def changes(before, after):
    validate(before); validate(after)
    identity = copy.deepcopy(after)
    identity['objects'] = before['objects']
    if identity != before or before['objects'].keys() != after['objects'].keys():
        raise ValueError('DataAsset identity or owned object inventory changed; export again')
    result = []
    for key, old_obj in before['objects'].items():
        new_obj = after['objects'][key]
        if old_obj['class'] != new_obj['class'] or old_obj['properties'].keys() != new_obj['properties'].keys():
            raise ValueError('Object class or property inventory changed: ' + key)
        for name, old in old_obj['properties'].items():
            new = new_obj['properties'][name]
            if old['type'] != new['type'] or old['editable'] != new['editable']:
                raise ValueError('Property metadata is read-only: ' + key + ' / ' + name)
            if type(old['value']) != type(new['value']) or isinstance(old['value'], list) and len(old['value']) != len(new['value']):
                raise ValueError('Fixed property dimensions changed: ' + name)
            if old['value'] != new['value']:
                if not old['editable']:
                    raise ValueError('Property is read-only: ' + key + ' / ' + name)
                result.append(dict(object=key, property=name, before=old['value'], after=new['value']))
    return result


def check_preserved(original, actual, desired):
    """Check hidden state as well as visible values, including untouched fields."""
    if actual['source'] != desired or actual['preserved'] != original['preserved']:
        raise ValueError('DataAsset changed beyond requested properties')
    for key, obj in original['source']['objects'].items():
        for name, row in obj['properties'].items():
            if row == desired['objects'][key]['properties'][name] and original['propertyState'][key][name] != actual['propertyState'][key][name]:
                raise ValueError('Untouched property serialization changed: ' + key + ' / ' + name)


class DataAssetWorkspace(Workspace):
    def load(self):
        data = read(self.manifest)
        if set(data) != {'schema', 'kind', 'project', 'assets'} or data['schema'] != 1 or data['kind'] != 'dataAssets':
            raise ValueError('Invalid DataAsset workspace manifest')
        self.project = Path(data['project']).resolve()
        if self.project.suffix != '.uproject' or not self.project.is_file():
            raise ValueError('Workspace project is missing')
        if not isinstance(data['assets'], list) or not data['assets']:
            raise ValueError('DataAsset workspace selection must be nonempty')
        entries, seen = [], set()
        for row in data['assets']:
            if set(row) != {'source'}:
                raise ValueError('DataAsset selection accepts only source paths')
            source = inside(inside(self.root, 'data-assets'), str(Path(row['source']).relative_to('data-assets')))
            document = read(source); validate(document)
            target(self.project, document['asset'])
            if document['asset'].casefold() in seen:
                raise ValueError('Duplicate DataAsset selection')
            seen.add(document['asset'].casefold()); entries.append((row, source, document))
        return data, entries

    def export_state(self, asset, name):
        output = self.run / (name + '.json')
        self.engine_run('DataAssetExport', name, Asset=asset, Output=output)
        state = read(output); validate(state['source'])
        if state['source']['asset'] != asset or not state.get('preserved') or not state.get('propertyState'):
            raise ValueError('Incomplete DataAsset preservation snapshot: ' + asset)
        return state, output

    def export(self, project, assets):
        if self.manifest.exists() or self.root.exists() and any(p.name != '.workspace' for p in self.root.iterdir()):
            raise ValueError('ExportDataAsset requires a new or empty workspace')
        self.project = Path(project).resolve()
        if not assets or len(set(a.casefold() for a in assets)) != len(assets):
            raise ValueError('ExportDataAsset requires unique selected package identities')
        for asset in assets:
            if not target(self.project, asset).is_file():
                raise ValueError('Selected DataAsset is missing: ' + asset)
        self.prepare(self.project)
        records, baseline = [], {}
        for i, asset in enumerate(assets):
            file = target(self.project, asset); original = file.read_bytes()
            state, native = self.export_state(asset, f'export-{i}')
            if file.read_bytes() != original:
                raise ValueError('DataAsset changed during export: ' + asset)
            source = inside(self.root, 'data-assets/' + asset[6:] + '.json')
            write(source, state['source'])
            before = self.internal / 'before' / f'{i}.json'; write(before, state['source'])
            saved = self.internal / 'baseline' / f'{i}.uasset'
            saved.parent.mkdir(parents=True, exist_ok=True); saved.write_bytes(original)
            baseline[asset] = dict(source=str(source.relative_to(self.root)), bytes=str(saved.relative_to(self.internal)),
                before=str(before.relative_to(self.internal)), accepted=str(before.relative_to(self.internal)),
                native=str(native.relative_to(self.internal)), beforeNative=str(native.relative_to(self.internal)))
            records.append(dict(source=str(source.relative_to(self.root))))
        write(self.internal / 'baseline.json', baseline)
        write(self.manifest, dict(schema=1, kind='dataAssets', project=str(self.project), assets=records))
        return dict(status='exported', kind='dataAssets', assets=len(records))

    def apply(self):
        _, entries = self.load(); baseline = self.check(entries)
        selected = []
        for _, source, document in entries:
            asset = document['asset']
            if asset not in baseline:
                raise ValueError('DataAsset creation is unsupported; export the existing asset first')
            original = read(inside(self.internal, baseline[asset]['native']))
            delta = changes(original['source'], document)
            if delta:
                selected.append((source, document, original, delta))
        if not selected:
            return dict(status='unchanged', assets=len(entries), changed=0, added=0)
        self.prepare(self.project)
        self.check(entries)
        records, proposals, requests = [], {}, []
        for i, (source, document, original, _) in enumerate(selected):
            asset = document['asset']; proposals[str(source)] = source.read_bytes()
            backup = self.run / 'original' / (asset[6:] + '.uasset')
            backup.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(target(self.project, asset), backup)
            if backup.read_bytes() != inside(self.internal, baseline[asset]['bytes']).read_bytes():
                raise ValueError('DataAsset backup differs from accepted project state')
            records.append(dict(asset=asset, backup=str(backup.relative_to(self.internal))))
            request = self.run / f'edit-{i}.json'; write(request, dict(before=original, after=document)); requests.append(request)
        journal = self.internal / 'recovery.json'
        recovery = dict(project=str(self.project), assets=records, baseline=baseline, restoreRequired=False)
        write(journal, recovery)
        try:
            self.check(entries)
            recovery['restoreRequired'] = True
            write(journal, recovery)
            for i, request in enumerate(requests):
                self.engine_run('DataAssetApply', f'apply-{i}', Input=request, Output=self.run / f'expected-{i}.json')
            updated = copy.deepcopy(baseline)
            for i, (source, document, original, _) in enumerate(selected):
                asset = document['asset']; actual, native = self.export_state(asset, f'reopen-{i}')
                check_preserved(original, actual, document)
                if actual != read(self.run / f'expected-{i}.json'):
                    raise ValueError('DataAsset serialization changed after fresh reopen: ' + str(native))
                if source.read_bytes() != proposals[str(source)]:
                    raise ValueError('DataAsset proposal changed during Apply: ' + str(source))
                accepted = self.run / f'accepted-{i}.json'; write(accepted, actual['source'])
                saved = self.run / f'accepted-{i}.uasset'; shutil.copyfile(target(self.project, asset), saved)
                updated[asset].update(bytes=str(saved.relative_to(self.internal)), accepted=str(accepted.relative_to(self.internal)),
                                      native=str(native.relative_to(self.internal)))
            write(self.internal / 'baseline.json', updated); journal.unlink()
        except BaseException:
            self.recover(); raise
        return dict(status='applied', assets=len(entries), changed=len(selected), added=0,
                    changes=[dict(asset=d['asset'], properties=delta) for _, d, _, delta in selected])

    def preview(self, comparison_workspace=None, comparison_label=None):
        from data_asset_preview import render
        _, entries = self.load(); baseline = self.check(entries)
        if bool(comparison_workspace) != bool(comparison_label and comparison_label.strip()):
            raise ValueError('ComparisonWorkspace and a nonempty ComparisonLabel must be supplied together')
        reference = None
        if comparison_workspace:
            reference = Path(comparison_workspace).resolve()
            manifest = read(reference / 'workspace.json')
            if manifest.get('schema') != 1 or manifest.get('kind') != 'dataAssets':
                raise ValueError('ComparisonWorkspace must be an exported DataAsset workspace')
            comparison_records = read(reference / '.workspace/baseline.json')
        else:
            comparison_label = 'Before this edit (workspace export)'
        items = []
        for i, (_, _, document) in enumerate(entries):
            asset = document['asset']
            if asset not in baseline:
                raise ValueError('Export the existing DataAsset before preview')
            old = baseline[asset]
            # Validate against accepted identity even after an earlier Apply.
            pending = changes(read(inside(self.internal, old['accepted'])), document)
            if reference:
                record = comparison_records.get(asset)
                if not record or not record.get('before'):
                    raise ValueError('Comparison workspace has no exported baseline for: ' + asset)
                before = read(inside(reference / '.workspace', record['before']))
            else:
                before = read(inside(self.internal, old['before'])) if old.get('before') else None
            if before is not None:
                validate(before)
                write(self.run / 'comparison' / f'{i}.json', before)
            items.append(dict(document=document, before=before, pending=bool(pending)))
        page, count = render(items, comparison_label)
        output = self.run / 'preview/index.html'; output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(page, encoding='utf-8')
        comparison = (dict(workspace=str(reference or self.root), label=comparison_label, snapshot='initial-export')
                      if any(item['before'] is not None for item in items) else None)
        write(self.run / 'comparison.json', comparison)
        return dict(status='previewed', assets=len(entries), changedProperties=count, comparison=comparison, previewUrl=output.as_uri())
