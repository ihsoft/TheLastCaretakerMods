"""Native CDO source preservation, inspection and legacy transaction boundaries."""
import copy
import unittest
import asset_text_codec as codec
from test_asset_text_codec import fixture
from graph_context import component_detail
from preview_references import property_references


def source():
    data = codec.compact(fixture())
    data['nativeDefaults'] = {'objects': [
        {'id': 'Default__BP_Test_C:ModuleComponent', 'name': 'ModuleComponent',
         'outer': '$cdo', 'class': '/Script/Fixture.ModuleComponent', 'flags': 262144,
         'properties': {'ItemAsset': "Object'/Game/Test/DA_Item.DA_Item'",
                        'ConfigData': '(Enabled=True,Rate=1000.000000,Limit=1)'}},
        {'id': 'Default__BP_Test_C:ModuleComponent.Nested', 'name': 'Nested',
         'outer': 'Default__BP_Test_C:ModuleComponent', 'class': '/Script/Fixture.Child',
         'flags': 262144, 'properties': {'Value': '41'}}]}
    return data


class NativeDefaultsTests(unittest.TestCase):
    def test_codec_roundtrip_preserves_complete_native_state(self):
        data = source()
        self.assertEqual(codec.compact(codec.native(data))['nativeDefaults'], data['nativeDefaults'])
        self.assertFalse(codec.compare(data, copy.deepcopy(data)))

    def test_absence_only_matches_empty_not_nonempty_state(self):
        empty = codec.compact(fixture())
        declared = copy.deepcopy(empty)
        declared['nativeDefaults'] = {'objects': []}
        self.assertEqual(codec.compare(empty, declared), [])
        self.assertTrue(codec.compare(empty, source()))
        changed = source()
        changed['nativeDefaults']['objects'][0]['properties'].pop('ItemAsset')
        self.assertTrue(any('ItemAsset' in row['path'] for row in codec.compare(source(), changed)))

    def test_duplicate_and_unresolved_native_owners_rejected(self):
        for mutation in ('duplicate', 'owner', 'cycle'):
            data = source(); rows = data['nativeDefaults']['objects']
            if mutation == 'duplicate': rows.append(copy.deepcopy(rows[0]))
            elif mutation == 'owner': rows[0]['outer'] = '$class'
            else: rows[0]['outer'] = rows[1]['id']
            with self.subTest(mutation=mutation), self.assertRaises(ValueError): codec.native(data)

    def test_native_component_detail_and_asset_reference(self):
        data = source()
        detail = component_detail(data, 'ModuleComponent')
        self.assertEqual(detail['roles'], ['native-default-subobject'])
        self.assertEqual(detail['ownedObjects'][0]['name'], 'Nested')
        self.assertIn('/Game/Test/DA_Item.DA_Item', [target for _, target in property_references(data)])
        detail['object']['properties'].clear()
        self.assertIn('ItemAsset', data['nativeDefaults']['objects'][0]['properties'])

    def test_relocation_refuses_uncovered_native_state(self):
        data = source(); data['parent'] = '/Script/CoreUObject.Object'; data.pop('root', None)
        with self.assertRaisesRegex(ValueError, 'relocate currently supports'):
            codec.relocate(data, '/Game/Other')


if __name__ == '__main__':
    unittest.main()
