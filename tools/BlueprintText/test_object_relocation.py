"""Offline checks for moving reusable UObject BPT without rewriting user data."""
import copy
import unittest

from asset_text_codec import compact, native, relocate
from test_asset_text_codec import fixture


def source():
    result = compact(fixture())
    result.pop('root')
    result['parent'] = '/Script/CoreUObject.Object'
    result['properties'] = {}
    result['variables'] = [dict(VarName='"Literal"', VarType='(PinCategory="string")'),
                           dict(VarName='"Type"', VarType='(PinCategory="class",PinSubCategoryObject="/Script/Engine.BlueprintGeneratedClass\'/Game/BP_Test.BP_Test_C\'")')]
    result['defaults'] = {'Literal': '"/Game/BP_Test.BP_Test_C"',
                          'Type': '/Script/Engine.BlueprintGeneratedClass\'/Game/BP_Test.BP_Test_C\''}
    result['graphs'][0]['body']['nodes'][0]['properties'] = {
        'FunctionReference': '(MemberParent="/Script/Engine.BlueprintGeneratedClass\'/Game/BP_Test.BP_Test_C\'",MemberName="Read")',
        'NodeComment': '"/Game/BP_Test.BP_Test_C"'}
    return result


class RelocationTests(unittest.TestCase):
    def test_references_move_literals_and_original_stay_unchanged(self):
        original = source()
        before = copy.deepcopy(original)
        moved = relocate(original, '/Game/Mods/Example/Settings/BP_Test')
        self.assertEqual(original, before)
        self.assertEqual(moved['defaults']['Literal'], original['defaults']['Literal'])
        self.assertEqual(moved['graphs'][0]['body']['nodes'][0]['properties']['NodeComment'],
                         original['graphs'][0]['body']['nodes'][0]['properties']['NodeComment'])
        self.assertIn('/Game/Mods/Example/Settings/BP_Test.BP_Test_C', moved['defaults']['Type'])
        self.assertIn('/Game/Mods/Example/', moved['variables'][1]['VarType'])
        self.assertIn('/Game/Mods/Example/', moved['graphs'][0]['body']['nodes'][0]['properties']['FunctionReference'])
        self.assertEqual(moved, compact(native(moved)))

    def test_different_package_leaf_preserves_object_name(self):
        moved = relocate(source(), '/Game/Mods/Example/RenamedPackage')
        self.assertEqual(moved['objectName'], 'BP_Test')
        self.assertIn('RenamedPackage.BP_Test_C', moved['defaults']['Type'])

    def test_external_prefix_dependency_is_untouched(self):
        original = source()
        original['dependencies'] = ['/Game/BP_TestExtra.BP_TestExtra_C', '/Game/BP_Test/Other.Other_C']
        self.assertEqual(relocate(original, '/Game/Mods/Other/BP_Test')['dependencies'], original['dependencies'])

    def test_pin_class_and_string_defaults_are_distinguished(self):
        original = source()
        node = original['graphs'][0]['body']['nodes'][0]
        alias, pin = next(iter(node['pins'].items()))
        original['pinTemplates'][pin['type']]['PinType.PinCategory'] = '"string"'
        node['defaults'][alias] = '/Game/BP_Test.BP_Test_C'
        pin['DefaultObject'] = '"/Game/BP_Test.BP_Test_C"'
        moved = relocate(original, '/Game/Mods/Other/BP_Test')
        actual = moved['graphs'][0]['body']['nodes'][0]
        self.assertEqual(actual['defaults'][alias], node['defaults'][alias])
        self.assertEqual(actual['pins'][alias]['DefaultObject'], '"/Game/Mods/Other/BP_Test.BP_Test_C"')

    def test_unknown_reference_is_rejected(self):
        original = source()
        original['properties']['FutureReference'] = '"/Game/BP_Test.BP_Test_C"'
        with self.assertRaisesRegex(ValueError, 'properties.FutureReference'):
            relocate(original, '/Game/Mods/Other/BP_Test')

    def test_local_variable_class_default_and_string_metadata(self):
        original = source()
        node = original['graphs'][0]['body']['nodes'][0]
        node['properties']['LocalVariables(0)'] = '(VarName="Local",VarType=(PinCategory="class"),DefaultValue="/Game/BP_Test.BP_Test_C",MetaDataArray=((DataKey="Hint",DataValue="/Game/BP_Test.BP_Test_C")))'
        moved = relocate(original, '/Game/Mods/Other/BP_Test')
        text = moved['graphs'][0]['body']['nodes'][0]['properties']['LocalVariables(0)']
        self.assertIn('DefaultValue="/Game/Mods/Other/BP_Test.BP_Test_C"', text)
        self.assertIn('DataValue="/Game/BP_Test.BP_Test_C"', text)

    def test_invalid_paths_and_non_object_sources_are_rejected(self):
        for path in ('/Game/BP_Test', '/Plugin/BP_Test', '/Game//Other', '/Game/Other.Object', '/Game/../Other'):
            with self.subTest(path=path), self.assertRaises(ValueError):
                relocate(source(), path)
        with self.assertRaisesRegex(ValueError, 'direct UObject'):
            relocate(compact(fixture()), '/Game/Mods/Other/BP_Test')

    def test_user_defined_pin_defaults_keep_string_but_move_class(self):
        for category in ('string', 'class'):
            original = source()
            node = original['graphs'][0]['body']['nodes'][0]
            node['custom'] = [f'CustomProperties UserDefinedPin (PinName="Argument",PinType=(PinCategory="{category}"),PinDefaultValue="/Game/BP_Test.BP_Test_C")']
            actual = relocate(original, '/Game/Mods/Other/BP_Test')['graphs'][0]['body']['nodes'][0]['custom'][0]
            if category == 'string':
                self.assertEqual(actual, node['custom'][0])
            else:
                self.assertIn('PinDefaultValue="/Game/Mods/Other/BP_Test.BP_Test_C"', actual)


if __name__ == '__main__':
    unittest.main()
