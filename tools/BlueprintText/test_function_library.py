"""Function-library identity, bounded state, relocation and preview navigation."""
import copy
import unittest
import json
from pathlib import Path
from asset_text_codec import compact, native, compare, relocate
from test_asset_text_codec import fixture
from test_preview_graph_links import entry, call
from preview_graph_links import build_catalog, build_callers, resolve_calls
from preview_references import is_implicit_target, with_type_reference_details


def library():
    doc = compact(fixture())
    doc.pop('root')
    doc['parent'] = '/Script/Engine.BlueprintFunctionLibrary'
    doc['properties'] = {'BlueprintType': 'BPTYPE_FunctionLibrary'}
    doc['variables'] = []
    doc['defaults'] = {}
    doc['graphs'][0]['body']['nodes'][0]['properties']['ExtraFlags'] = str(0x10002000)
    return doc


class FunctionLibraryTests(unittest.TestCase):
    def test_hidden_static_cdo_stays_in_details(self):
        node = call('Check', '(MemberParent="/Game/Types.Types_C",MemberName="Check")')
        node['pins'] = {'in:target': {'type': 'hidden', 'name': 'self', 'DefaultObject': '"/Game/Types.Default__Types_C"'}}
        templates = {'hidden': {'bHidden': 'True'}}
        field = 'pins / in:target / DefaultObject'
        self.assertTrue(is_implicit_target(node, templates, field))
        self.assertTrue(any(r['field'] == field for r in with_type_reference_details(node, templates)['_typeReferences']))
        templates['hidden']['bHidden'] = 'False'
        self.assertFalse(is_implicit_target(node, templates, field))

    def test_empty_starter_is_valid_and_relocatable(self):
        doc = json.loads((Path(__file__).parent / 'Library/BFL_FunctionLibrary.json').read_text())
        moved = relocate(doc, '/Game/Mods/Example/BFL_Types')
        self.assertEqual(compare(moved, compact(native(moved))), [])
        self.assertEqual(moved['objectName'], 'BFL_FunctionLibrary')
        self.assertEqual(moved['graphs'], [])

    def test_type_flags_and_external_references_roundtrip(self):
        doc = library()
        doc['dependencies'] = ['/Game/External.External_C']
        doc['graphs'][0]['body']['nodes'][0]['properties']['FunctionReference'] = '(MemberParent="/Game/External.External_C",MemberName="Check")'
        actual = compact(native(doc))
        self.assertEqual(compare(doc, actual), [])
        self.assertEqual(actual['properties']['BlueprintType'], 'BPTYPE_FunctionLibrary')
        self.assertEqual(actual['graphs'][0]['body']['nodes'][0]['properties']['ExtraFlags'], str(0x10002000))

    def test_mismatched_parent_and_type_rejected(self):
        for field, value in [('parent', '/Script/CoreUObject.Object'), ('properties', {})]:
            doc = library(); doc[field] = value
            with self.assertRaisesRegex(ValueError, 'exact parent and BlueprintType'):
                native(doc)

    def test_instance_and_event_state_rejected(self):
        for key, value in [('variables', [{'VarName': '"X"'}]), ('defaults', {'X': '0'}),
                           ('root', {}), ('components', {}), ('widget', {})]:
            doc = library(); doc[key] = value
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, 'functions only'):
                native(doc)
        doc = library(); doc['graphs'][0]['kind'] = 'event'
        with self.assertRaisesRegex(ValueError, 'functions only'):
            native(doc)

    def test_relocation_keeps_type_and_rewrites_only_owned_reference(self):
        doc = library()
        node = doc['graphs'][0]['body']['nodes'][0]
        node['properties']['FunctionReference'] = '(MemberParent="/Game/BP_Test.BP_Test_C",MemberName="Check")'
        node['properties']['NodeComment'] = '"/Game/BP_Test.BP_Test_C"'
        moved = relocate(doc, '/Game/Shared/Types')
        props = moved['graphs'][0]['body']['nodes'][0]['properties']
        self.assertIn('/Game/Shared/Types.BP_Test_C', props['FunctionReference'])
        self.assertEqual(props['NodeComment'], node['properties']['NodeComment'])
        self.assertEqual(moved['properties'], doc['properties'])

    def test_two_static_callers_navigate_without_target_instances(self):
        lib = library()
        docs = [entry(lib)]
        for name in ('First', 'Second'):
            caller = copy.deepcopy(lib)
            caller['asset'] = '/Game/' + name
            caller['parent'] = '/Script/Engine.Actor'
            caller['properties'] = {}
            node = call('Check', '(MemberParent="/Game/BP_Test.BP_Test_C",MemberName="Calculate")')
            node['pins'] = {}
            caller['graphs'][0]['body'] = dict(nodes=[node], links=[])
            docs.append(entry(caller))
        catalog = build_catalog(docs)
        for row in docs[1:]:
            links = resolve_calls(row['docs']['source'], 'Calculate', catalog['source'])
            self.assertIn('Check', links)
        callers = build_callers(docs, catalog)['source']
        self.assertEqual(len(callers[(lib['asset'], '4'*32)]), 2)


if __name__ == '__main__':
    unittest.main()
