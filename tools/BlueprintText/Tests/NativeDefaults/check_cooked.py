"""Check UAssetGUI's cooked-package readback independently of BPT serialization."""
import json
import sys
from pathlib import Path


def reference(data, index):
    names = []
    while index:
        row = data['Exports'][index - 1] if index > 0 else data['Imports'][-index - 1]
        names.append(row['ObjectName'])
        index = row['OuterIndex']
    return list(reversed(names))


def properties(row):
    return {prop['Name']: prop['Value'] for prop in row['Data']}


def check(folder):
    expected_config = {'bAutoStartModule': True, 'ResourceBandwidthInput': 1000.0, 'MaxResourceAmount': 1}
    for side, suffix, nested_value in (('parent', '', 41), ('child', 'Child', 47)):
        data = json.loads((folder / ('cooked-' + side + '.json')).read_text(encoding='utf-8-sig'))
        assert not any('RawExport' in row['$type'] for row in data['Exports'])
        def export(name):
            matches = [(i + 1, row) for i, row in enumerate(data['Exports']) if row['ObjectName'] == name]
            assert len(matches) == 1, (side, name, len(matches))
            return matches[0]
        component_id, component = export('ModuleComponent')
        nested_id, nested = export('Nested')
        _, other = export('OtherComponent')
        cdo_name = 'Default__BP_NativeDefaults' + suffix + '_C'
        assert reference(data, component['OuterIndex']) == [cdo_name]
        assert other['OuterIndex'] == component['OuterIndex']
        assert nested['OuterIndex'] == component_id
        assert properties(nested)['Value'] == nested_value
        values = properties(component)
        assert values['Nested'] == nested_id
        if side == 'parent':
            assert reference(data, values['ItemAsset']) == ['/Game/DA_NativeItem', 'DA_NativeItem']
            assert {p['Name']: p['Value'] for p in values['ConfigData']} == expected_config
        else:
            # The child inherits these properties from the cooked parent template.
            assert reference(data, component['TemplateIndex']) == [
                '/Game/BP_NativeDefaults', 'Default__BP_NativeDefaults_C', 'ModuleComponent']
            assert 'ItemAsset' not in values and 'ConfigData' not in values
    (folder / 'cooked-checked.json').write_text(json.dumps(dict(status='passed', assets=2,
        reader='UAssetGUI', itemAsset='/Game/DA_NativeItem.DA_NativeItem', configData=expected_config,
        nestedValues=[41, 47]), indent=2) + '\n')


if __name__ == '__main__':
    check(Path(sys.argv[1]))
