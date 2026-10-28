"""Offline schema-2 preservation and rejection checks; synthetic native text only."""
import copy
import unittest

from asset_text_codec import add_call, compact, compare, native, parse_nodes, split_fields
from render_graph import render


def pin(guid, name, output=False, target=None):
    values = [f'PinId={guid}', f'PinName="{name}"', 'PinType.PinCategory="real"',
              'PinType.PinSubCategory="double"', 'FuturePinMetadata=(Text="a,b",Values=(1,2))']
    if output:
        values.append('Direction="EGPD_Output"')
    if target:
        values.append(f'LinkedTo=({target},)')
    values.append('DefaultValue="1.500000"')
    return 'CustomProperties Pin (' + ','.join(values) + ',)'


def fixture():
    left, right = '1' * 32, '2' * 32
    text = '\n'.join([
        'Begin Object Class=/Script/BlueprintGraph.K2Node_FunctionEntry Name="Entry"',
        'LocalVariables(0)=(VarName="Local",DefaultValue="a,b")',
        'FutureNodeField=(Nested=(Value="x,y"))',
        'CustomProperties UserDefinedPin (PinName="Result",DesiredPinDirection=EGPD_Output)',
        pin(left, 'Result', True, f'Sink {right}'),
        'End Object',
        'Begin Object Class=/Script/BlueprintGraph.K2Node_VariableSet Name="Sink"',
        'VariableReference=(MemberName="Amount",bSelfContext=True)',
        pin(right, 'Amount', target=f'Entry {left}'),
        'End Object'])
    return {'schema': 2, 'format': 'native-asset', 'asset': '/Game/BP_Test',
            'parent': '/Script/Engine.Actor', 'properties': {'FutureBlueprintField': '(A=1,B=2)'},
            'variables': ['(VarName="Amount",DefaultValue="1.5",MetaDataArray=((DataKey="Hint",DataValue="a,b")))'],
            'defaults': {'Amount': '1.500000'}, 'root': {'node': {'VariableGuid': '3' * 32}, 'template': {}},
            'dependencies': [], 'graphs': [{'name': 'Calculate', 'kind': 'function',
                'properties': {'GraphGuid': '4' * 32}, 'nodeCount': 2, 'text': text}]}


class AssetTextTests(unittest.TestCase):
    def test_component_tree_preservation_and_invalid_owners(self):
        source = fixture()
        del source['root']
        source['parent'] = '/Game/BP_Parent.BP_Parent_C'
        source['components'] = {'scs': 'BP_Test_C:SCS', 'templates': [],
            'generatedTemplates': ['BP_Test_C:Mesh'], 'objects': [
                {'id': 'BP_Test_C:SCS', 'name': 'SCS', 'class': '/Script/Engine.SimpleConstructionScript',
                 'outer': '$class', 'flags': 1, 'properties': {'RootNodes': '()'}},
                {'id': 'BP_Test_C:Mesh', 'name': 'Mesh', 'class': '/Script/Engine.StaticMeshComponent',
                 'outer': '$class', 'flags': 1, 'properties': {'RelativeLocation': '(X=0,Y=1,Z=0)'}}]}
        result = native(compact(source))
        self.assertEqual(result['components'], source['components'])
        self.assertEqual(result['parent'], source['parent'])
        for key, value in [('outer', 'missing'), ('outer', 'BP_Test_C:Mesh'), ('flags', -1)]:
            broken = copy.deepcopy(source)
            broken['components']['objects'][1][key] = value
            with self.assertRaises(ValueError):
                compact(broken)
        broken = copy.deepcopy(source)
        broken['components']['generatedTemplates'].append('unknown')
        with self.assertRaises(ValueError):
            compact(broken)
        broken = copy.deepcopy(source)
        broken['root'] = {}
        with self.assertRaises(ValueError):
            compact(broken)

    def test_preserves_native_fields_and_function_signature(self):
        source = fixture()
        output = native(compact(source))
        self.assertEqual(parse_nodes(source['graphs'][0]['text']), parse_nodes(output['graphs'][0]['text']))
        for key in ('properties', 'variables', 'defaults', 'root'):
            self.assertEqual(source[key], output[key])
        self.assertEqual(compare(compact(source), compact(output)), [])

    def test_node_and_link_order_is_canonical(self):
        source = compact(fixture())
        reordered = copy.deepcopy(source)
        reordered['graphs'][0]['body']['nodes'].reverse()
        self.assertEqual(compare(source, reordered), [])
        reordered['defaults']['Amount'] = '9.000000'
        self.assertEqual(compare(source, reordered)[0]['path'], '/defaults/Amount')

    def test_unknown_envelope_field_is_rejected(self):
        source = fixture()
        source['unhandledState'] = 'must not disappear'
        with self.assertRaisesRegex(ValueError, 'Unknown'):
            compact(source)

    def test_nested_objects_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Nested native objects'):
            parse_nodes('Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="N"\n'
                        'Begin Object Class=/Script/CoreUObject.Object Name="Inner"\nEnd Object\nEnd Object')

    def test_bad_connections_are_rejected(self):
        for replacement, message in [('Missing ' + '2' * 32, 'Unresolved'), ('', 'Non-reciprocal')]:
            source = fixture()
            text = source['graphs'][0]['text']
            if replacement:
                text = text.replace('Sink ' + '2' * 32, replacement)
            else:
                text = text.replace('LinkedTo=(Entry ' + '1' * 32 + ',),', '')
            source['graphs'][0]['text'] = text
            with self.assertRaisesRegex(ValueError, message):
                compact(source)
        source = fixture()
        source['graphs'][0]['text'] = source['graphs'][0]['text'].replace('Direction="EGPD_Output",', '')
        with self.assertRaisesRegex(ValueError, 'output-to-input'):
            compact(source)

    def test_duplicate_pin_identity_and_unknown_defaults_are_rejected(self):
        source = compact(fixture())
        nodes = source['graphs'][0]['body']['nodes']
        nodes[1]['pins']['in:Amount']['id'] = nodes[0]['pins']['out:Result']['id']
        with self.assertRaisesRegex(ValueError, 'Duplicate pin GUID'):
            native(source)
        source = compact(fixture())
        source['graphs'][0]['body']['nodes'][0]['defaults']['in:Missing'] = '3'
        with self.assertRaisesRegex(ValueError, 'unknown pin'):
            native(source)

    def test_factory_inserts_call_without_manual_guids_or_pin_pool_work(self):
        insertion = '\n'.join([
            'Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="Math"',
            'FunctionReference=(MemberName="Divide_DoubleDouble")',
            pin('5' * 32, 'A'), pin('6' * 32, 'B'), pin('7' * 32, 'ReturnValue', True), 'End Object'])
        catalog = {'entries': [{'identity': 'math.divide', 'nativeInsertion': insertion}]}
        original = compact(fixture())
        edited = add_call(original, catalog, 'math.divide', 'Division', 'Calculate')
        self.assertEqual(len(original['graphs'][0]['body']['nodes']), 2)
        new = next(n for n in edited['graphs'][0]['body']['nodes'] if n['id'] == 'Division')
        self.assertEqual(set(new['pins']), {'in:left', 'in:right', 'out:result'})
        self.assertEqual(new['pins']['in:left']['name'], 'A')
        self.assertNotEqual(new['pins']['in:left']['id'], '5' * 32)
        edited['graphs'][0]['body']['links'].append({'from': 'Division.out:result', 'to': 'Sink.in:Amount'})
        self.assertEqual(compare(edited, compact(native(edited))), [])

    def test_dependencies_are_derived_from_pin_defaults(self):
        source = compact(fixture())
        source['graphs'][0]['body']['nodes'][0]['pins']['out:Result']['DefaultObject'] = '/Game/Input/Action.Action'
        self.assertEqual(native(source)['dependencies'], ['/Game/Input/Action.Action'])

    def test_schema2_view_uses_native_names_and_disabled_state(self):
        source = compact(fixture())
        source['graphs'][0]['body']['nodes'][0]['properties']['EnabledState'] = 'Disabled'
        svg, _, nodes, links = render(source)
        self.assertEqual((nodes, links), (1, 0))
        self.assertIn('Set Amount', svg)
        self.assertIn('disabled node (hidden)', svg)
        self.assertEqual(render(source, view='full')[2:], (2, 1))

    def test_tuple_parser_rejects_unbalanced_input(self):
        for value in ('A=(1,2', 'A=)(1,2)', 'A="unterminated'):
            with self.assertRaisesRegex(ValueError, 'Unbalanced'):
                split_fields(value)


if __name__ == '__main__':
    unittest.main()
