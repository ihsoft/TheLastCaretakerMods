"""Offline contract tests using a small synthetic package, never extracted game data."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).with_name('item_patch.py')


def prop(name, kind, value, **extra):
    return {'Name': name, '$type': 'Test.' + kind, 'Value': value, **extra}


def fixture():
    generator = '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default'
    return {
        'Imports': [
            {'ObjectName': '/Script/Voyage', 'OuterIndex': 0, 'ClassName': 'Package'},
            {'ObjectName': 'VoyageItem', 'OuterIndex': -1, 'ClassName': 'Class'},
            {'ObjectName': generator, 'OuterIndex': 0, 'ClassName': 'Package'},
            {'ObjectName': 'BP_CraftingLocationGenerator_Default_C', 'OuterIndex': -3, 'ClassName': 'BlueprintGeneratedClass'},
        ],
        'Exports': [
            {'ObjectName': 'BP_CraftingLocationGenerator_Default_C_0', 'ClassIndex': -4, 'OuterIndex': 2,
             'TemplateIndex': 0, 'ObjectFlags': 'RF_Public', 'Extras': '', 'Data': [prop('AllowWithWater', 'BoolPropertyData', False)]},
            {'ObjectName': 'DA_Test', 'ClassIndex': -2, 'OuterIndex': 0, 'TemplateIndex': 0,
             'ObjectFlags': 'RF_Public, RF_Standalone', 'Extras': '', 'Data': [
                 prop('DroppedActor', 'SoftObjectPropertyData', {'AssetPath': {'PackageName': '/Game/Test/Original', 'AssetName': 'Original_C'}, 'SubPathString': ''}),
                 prop('CraftingLocationGenerator', 'ObjectPropertyData', 1),
                 prop('Price', 'IntPropertyData', 17),
                 prop('Title', 'TextPropertyData', 'Key', HistoryType='Base', Flags='Immutable', Namespace='Test', CultureInvariantString='Title')
             ]},
        ],
        'CustomVersionContainer': [{'Key': 'test-version', 'Version': 3}],
        'ObjectVersion': 1, 'ObjectVersionUE5': 2, 'IsUnversioned': True, 'PackageFlags': 'Test',
    }


class RecipeTests(unittest.TestCase):
    def setUp(self):
        scratch = TOOL.parents[2] / 'Tmp/VoyageItemPatch/tests'
        scratch.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=scratch)
        self.root = Path(self.tmp.name)
        self.source = fixture()
        self.recipe = {'schemaVersion': 1, 'profile': 'voyage-item-default-generator-v1', 'asset': '/Game/Test/DA_Test',
                       'patches': [{'property': 'DroppedActor', 'expect': '/Game/Test/Original.Original_C', 'value': '/Game/Mods/Test/Child.Child_C'}]}
        self.schema = {'UberGraphFrame': 'StructProperty<PointerToUberGraphFrame>', 'AllowWithWater': 'BoolProperty'}

    def tearDown(self):
        self.tmp.cleanup()

    def run_tool(self, mode, cooked=None):
        for name, value in [('source', self.source), ('recipe', self.recipe), ('schema', self.schema), ('cooked', cooked)]:
            (self.root / (name + '.json')).write_text(json.dumps(value))
        result = subprocess.run([sys.executable, '-B', str(TOOL), mode,
            '--source', str(self.root/'source.json'), '--recipe', str(self.root/'recipe.json'),
            '--schema', str(self.root/'schema.json'), '--cooked', str(self.root/'cooked.json'),
            '--output', str(self.root/'result.json')], capture_output=True, text=True)
        return result

    def patched(self):
        cooked = copy.deepcopy(self.source)
        cooked['Exports'][1]['Data'][0]['Value']['AssetPath'] = {'PackageName': '/Game/Mods/Test/Child', 'AssetName': 'Child_C'}
        return cooked

    def test_non_gyro_name_and_replacement(self):
        result = self.run_tool('prepare')
        self.assertEqual(result.returncode, 0, result.stderr)
        spec = json.loads((self.root/'result.json').read_text())
        self.assertEqual(spec['properties'][1]['DroppedActor'], '"/Game/Mods/Test/Child.Child_C"')
        self.assertIn('/Game/Test/DA_Test.DA_Test:', spec['properties'][1]['CraftingLocationGenerator'])

    def test_baseline_roundtrip(self):
        self.recipe['patches'] = []
        self.assertEqual(self.run_tool('compare', self.source).returncode, 0)

    def test_only_recipe_change_passes(self):
        self.assertEqual(self.run_tool('compare', self.patched()).returncode, 0)

    def test_missing_patch_fails(self):
        self.assertNotEqual(self.run_tool('compare', self.source).returncode, 0)

    def test_unrelated_change_fails(self):
        cooked = self.patched()
        cooked['Exports'][1]['Data'][2]['Value'] = 18
        self.assertNotEqual(self.run_tool('compare', cooked).returncode, 0)

    def test_text_history_flags_are_not_display_text(self):
        cooked = self.patched()
        cooked['Exports'][1]['Data'][3]['Flags'] = 0
        self.assertNotEqual(self.run_tool('compare', cooked).returncode, 0)

    def test_version_contract_change_fails(self):
        cooked = self.patched()
        cooked['CustomVersionContainer'][0]['Version'] += 1
        self.assertNotEqual(self.run_tool('compare', cooked).returncode, 0)

    def test_wrong_expected_value_rejected_by_both_paths(self):
        self.recipe['patches'][0]['expect'] = '/Game/Test/Wrong.Wrong_C'
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)
        self.assertNotEqual(self.run_tool('compare', self.patched()).returncode, 0)

    def test_reject_unknown_profile_and_property(self):
        self.recipe['profile'] = 'all-assets'
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)
        self.recipe['profile'] = 'voyage-item-default-generator-v1'
        self.recipe['patches'][0]['property'] = 'Price'
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)

    def test_reject_unsupported_inner_layout(self):
        self.schema['Unknown'] = 'DelegateProperty'
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)

    def test_reject_unknown_serialized_data(self):
        self.source['Exports'][1]['Data'].append(prop('Unknown', 'NewPropertyData', 9))
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)

    def test_reject_unsupported_text_flags_before_editor(self):
        self.source['Exports'][1]['Data'][3]['Flags'] = 0
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)

    def test_reject_wrong_outer(self):
        self.source['Exports'][0]['OuterIndex'] = 0
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)

    def test_reject_path_traversal(self):
        self.recipe['asset'] = '/Game/../DA_Test'
        self.assertNotEqual(self.run_tool('prepare').returncode, 0)


if __name__ == '__main__':
    unittest.main()
