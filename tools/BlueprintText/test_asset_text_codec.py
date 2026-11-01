"""Offline schema-2 preservation and rejection checks; synthetic native text only."""
import copy
import json
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch

from asset_text_codec import add_call, batch_sources, compact, compare, native, parse_nodes, register_widget, split_fields
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


def sequence_fixture():
    document = fixture()
    document['graphs'] = [{'name': 'EventGraph', 'kind': 'event', 'properties': {}, 'nodeCount': 1,
                          'text': '\n'.join([
        'Begin Object Class=/Script/BlueprintGraph.K2Node_ExecutionSequence Name="Sequence"',
        f'CustomProperties Pin (PinId={"1" * 32},PinName="execute",PinType.PinCategory="exec",)',
        f'CustomProperties Pin (PinId={"2" * 32},PinName="then_0",Direction="EGPD_Output",PinType.PinCategory="exec",)',
        f'CustomProperties Pin (PinId={"3" * 32},PinName="then_1",Direction="EGPD_Output",PinType.PinCategory="exec",)',
        'End Object'])}]
    return document


class AssetTextTests(unittest.TestCase):
    def test_pin_template_addition_and_removal_leave_other_references_unchanged(self):
        original = compact(fixture())
        edited = copy.deepcopy(original)
        selected = edited['graphs'][0]['body']['nodes'][0]['pins']['out:Result']
        metadata = copy.deepcopy(edited['pinTemplates'][selected['type']])
        # Sorts before every original signature, exposing positional pool IDs.
        metadata['AddedMetadata'] = '(Nested="new,value")'
        edited['pinTemplates']['temporary'] = metadata
        selected['type'] = 'temporary'
        changed = compact(native(edited))
        self.assertEqual(len(changed['pinTemplates']), 2)
        self.assertTrue(compare(original, changed))
        self.assertEqual(changed['graphs'][0]['body']['nodes'][1], original['graphs'][0]['body']['nodes'][1])
        new_key = changed['graphs'][0]['body']['nodes'][0]['pins']['out:Result']['type']
        self.assertEqual(changed['pinTemplates'][new_key], metadata)
        # Remove the last user of the other template; the survivor keeps its key.
        removed = copy.deepcopy(changed)
        removed['graphs'][0]['body']['nodes'].pop()
        removed['graphs'][0]['body']['links'] = []
        reduced = compact(native(removed))
        self.assertEqual(reduced['pinTemplates'], {new_key: metadata})
        self.assertEqual(reduced['graphs'][0]['body']['nodes'][0], changed['graphs'][0]['body']['nodes'][0])

    def test_pin_template_keys_depend_on_metadata_only(self):
        original = compact(fixture())
        edited = copy.deepcopy(original)
        edited['graphs'][0]['body']['links'] = []
        edited['graphs'][0]['name'] = 'RenamedGraph'
        edited['graphs'][0]['body']['nodes'].pop()
        node = edited['graphs'][0]['body']['nodes'][0]
        node['id'] = 'RenamedNode'
        pin = node['pins']['out:Result']
        pin.update(id='9' * 32, name='OtherName', DefaultObject='None', DefaultTextValue='NSLOCTEXT("N","K","Text")')
        node['defaults']['out:Result'] = '2'
        edited['pinTemplates'] = {key: dict(reversed(list(value.items())))
                                  for key, value in edited['pinTemplates'].items()}
        changed = compact(native(edited))
        self.assertEqual(changed['pinTemplates'], original['pinTemplates'])
        self.assertEqual(next(iter(changed['graphs'][0]['body']['nodes'][0]['pins'].values()))['type'], pin['type'])

    def test_legacy_pin_keys_expand_and_compare_without_migration(self):
        original = compact(fixture())
        legacy = copy.deepcopy(original)
        metadata = next(iter(original['pinTemplates'].values()))
        legacy['pinTemplates'] = {'p0': metadata, 'p1': metadata}
        for i, node in enumerate(legacy['graphs'][0]['body']['nodes']):
            for pin in node['pins'].values():
                pin['type'] = f'p{i}'
        self.assertEqual(native(legacy), native(original))
        self.assertEqual(compare(legacy, original), [])
        self.assertEqual(compact(native(legacy)), original)

    def test_conflicting_content_keys_fail_instead_of_overwriting_metadata(self):
        source = fixture()
        source['graphs'][0]['text'] = source['graphs'][0]['text'].replace(
            'PinName="Amount",', 'PinName="Amount",AdditionalMetadata=True,')
        with patch('asset_text_codec.hashlib.sha256') as digest:
            digest.return_value.hexdigest.return_value = 'collision'
            with self.assertRaisesRegex(ValueError, 'Conflicting pin template content keys'):
                compact(source)

    def test_factory_merges_new_metadata_into_legacy_pool(self):
        original = compact(fixture())
        legacy = copy.deepcopy(original)
        legacy['pinTemplates'] = {'p1': next(iter(original['pinTemplates'].values()))}
        for node in legacy['graphs'][0]['body']['nodes']:
            for existing in node['pins'].values():
                existing['type'] = 'p1'
        insertion = '\n'.join([
            'Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="Math"',
            'FunctionReference=(MemberName="Divide_DoubleDouble")',
            pin('5' * 32, 'A'), pin('6' * 32, 'B'), pin('7' * 32, 'ReturnValue', True),
            'End Object']).replace('PinName="A",', 'PinName="A",bAdvancedView=True,')
        catalog = {'entries': [{'identity': 'math.divide', 'nativeInsertion': insertion}]}
        edited = add_call(legacy, catalog, 'math.divide', 'Division', 'Calculate')
        self.assertEqual(len(edited['pinTemplates']), 2)
        nodes = {node['id']: node for node in edited['graphs'][0]['body']['nodes']}
        for node in original['graphs'][0]['body']['nodes']:
            self.assertEqual(nodes[node['id']], node)
        new = nodes['Division']['pins']
        self.assertEqual(edited['pinTemplates'][new['in:left']['type']]['bAdvancedView'], 'True')
        self.assertEqual(new['in:right']['type'], nodes['Sink']['pins']['in:Amount']['type'])
        self.assertEqual(compare(edited, compact(native(edited))), [])

    def test_editor_stub_role_survives_both_directions_and_rejects_unknown_role(self):
        document = fixture()
        document['editorRole'] = 'stub'
        reopened = native(compact(document))
        self.assertEqual('stub', reopened['editorRole'])
        self.assertFalse(compare(compact(document), compact(reopened)))
        for role in ('runtime', False, None):
            document['editorRole'] = role
            with self.assertRaisesRegex(ValueError, 'editorRole'):
                compact(document)

    def test_object_name_independent_of_package_is_preserved(self):
        source = fixture()
        source['asset'] = '/Game/RelocatedPackage'
        source['objectName'] = 'BP_Test'
        restored = native(compact(source))
        self.assertEqual(restored['asset'], source['asset'])
        self.assertEqual(restored['objectName'], 'BP_Test')
        self.assertEqual(compare(compact(source), compact(restored)), [])
        self.assertNotIn('objectName', native(compact(fixture())))

    def test_widget_tree_slots_and_properties_round_trip(self):
        source = fixture()
        del source['root']
        source['parent'] = '/Script/UMG.UserWidget'
        source['widget'] = {'tree': 'BP_Test:WidgetTree', 'objects': [
            {'id': 'BP_Test:WidgetTree', 'name': 'WidgetTree', 'class': '/Script/UMG.WidgetTree',
             'outer': '$blueprint', 'flags': 8, 'properties': {'RootWidget': 'BP_Test:WidgetTree.Canvas'}},
            {'id': 'BP_Test:WidgetTree.Canvas', 'name': 'Canvas', 'class': '/Script/UMG.CanvasPanel',
             'outer': 'BP_Test:WidgetTree', 'flags': 8, 'properties': {'Slots': '(BP_Test:WidgetTree.Canvas.Slot)'}},
            {'id': 'BP_Test:WidgetTree.Canvas.Slot', 'name': 'Slot', 'class': '/Script/UMG.CanvasPanelSlot',
             'outer': 'BP_Test:WidgetTree.Canvas', 'flags': 8,
             'properties': {'LayoutData': '(Offsets=(Left=0,Top=15,Right=0,Bottom=0))'}}]}
        self.assertEqual(native(compact(source))['widget'], source['widget'])
        original = copy.deepcopy(source)
        registered = register_widget(source, 'Canvas')
        self.assertEqual(source, original)
        self.assertEqual(register_widget(registered, 'Canvas'), registered)
        self.assertRegex(registered['properties']['WidgetVariableNameToGuidMap'], r'^\(\("Canvas", [A-F0-9]{32}\)\)$')
        with self.assertRaises(ValueError):
            register_widget(source, 'Slot')
        for field, value in [('tree', 'missing'), ('unknown', [])]:
            broken = copy.deepcopy(source)
            broken['widget'][field] = value
            with self.assertRaises(ValueError):
                compact(broken)
        broken = copy.deepcopy(source)
        broken['widget']['objects'][2]['properties']['Content'] = 'MissingWidget'
        with self.assertRaisesRegex(ValueError, 'Unresolved widget hierarchy'):
            native(compact(broken))
        broken['widget']['objects'][2]['properties']['Content'] = 'BP_Test:WidgetTree.Canvas'
        with self.assertRaisesRegex(ValueError, 'Cyclic widget hierarchy'):
            compact(broken)
        broken = copy.deepcopy(source)
        broken['widget']['objects'][2]['properties']['Parent'] = 'BP_Test:WidgetTree'
        with self.assertRaisesRegex(ValueError, 'parent mismatch'):
            compact(broken)
        broken = copy.deepcopy(source)
        broken['widget']['objects'][1]['properties']['Slots'] = '(BP_Test:WidgetTree.Canvas.Slot,BP_Test:WidgetTree.Canvas.Slot)'
        with self.assertRaisesRegex(ValueError, 'Duplicate or empty widget slot'):
            compact(broken)
        broken = copy.deepcopy(source)
        broken['components'] = {}
        with self.assertRaises(ValueError):
            compact(broken)

    def test_batch_manifest_resolves_sources_and_rejects_duplicate_assets(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = compact(fixture())
            (root/'one.json').write_text(json.dumps(source))
            self.assertEqual(batch_sources({'assets': ['one.json']}, root), [source])
            for manifest in ({'assets': []}, {'assets': ['one.json', 'one.json']},
                             {'assets': ['one.json'], 'ignored': True}, {'assets': [42]}):
                with self.assertRaises(ValueError):
                    batch_sources(manifest, root)
            child = copy.deepcopy(source)
            child['asset'] = '/Game/BP_Child'
            child['parent'] = '/Game/BP_Test.BP_Test_C'
            (root/'child.json').write_text(json.dumps(child))
            for sources in (['one.json', 'child.json'], ['child.json', 'one.json']):
                with self.assertRaisesRegex(ValueError, 'parent classes must already exist'):
                    batch_sources({'assets': sources}, root)

    def test_interface_graph_ownership_and_preservation(self):
        source = fixture()
        source['graphs'][0]['kind'] = 'interface'
        source['interfaces'] = [{'class': '/Script/Test.Calculator', 'graphs': ['Calculate']}]
        self.assertEqual(native(compact(source))['interfaces'], source['interfaces'])
        self.assertEqual(native(compact(source))['graphs'][0]['kind'], 'interface')
        for interfaces in ([], source['interfaces'] * 2,
                           [{'class': '/Script/Test.Calculator', 'graphs': ['Missing']}],
                           [{'class': '/Script/Test.Calculator', 'graphs': ['Calculate', 'Calculate']}]):
            broken = copy.deepcopy(source)
            broken['interfaces'] = interfaces
            with self.assertRaises(ValueError):
                compact(broken)

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

    def test_sequence_pin_order_survives_conversion_and_is_compared(self):
        source = compact(sequence_fixture())
        reordered = copy.deepcopy(source)
        node = reordered['graphs'][0]['body']['nodes'][0]
        order = ['in:exec', 'out:then1', 'out:then0']
        node['pins'] = {alias: node['pins'][alias] for alias in order}
        # Ordinary mapping equality misses this change; native emission preserves it.
        self.assertEqual(source, reordered)
        self.assertNotEqual(native(source), native(reordered))
        self.assertEqual(list(compact(native(reordered))['graphs'][0]['body']['nodes'][0]['pins']), order)
        self.assertEqual(compare(source, reordered), [{
            'path': '/graphs/0/body/nodes/0/pins',
            'expected': ['in:exec', 'out:then0', 'out:then1'], 'actual': order}])

    def test_mapping_order_outside_node_pins_is_not_significant(self):
        source = compact(sequence_fixture())
        source['defaults']['Other'] = '0'
        source['graphs'][0]['body']['nodes'][0]['properties'].update(NodePosX='0', NodePosY='10')
        def reverse_mappings(value, path=()):
            if isinstance(value, dict):
                entries = list(value.items())
                if path != ('graphs', 0, 'body', 'nodes', 0, 'pins'):
                    entries.reverse()
                return {key: reverse_mappings(item, path + (key,)) for key, item in entries}
            if isinstance(value, list):
                return [reverse_mappings(item, path + (i,)) for i, item in enumerate(value)]
            return value
        self.assertEqual(compare(source, reverse_mappings(source)), [])

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
