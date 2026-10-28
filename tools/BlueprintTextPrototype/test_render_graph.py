"""Offline regression checks; no Unreal, catalogue, or external packages."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

from render_graph import render, node_lines, pin_type_label, fold_data_chains, fold_chains, unconnected_outputs, edge_output_type

ROOT = Path(__file__).resolve().parent


class GraphRenderingTests(unittest.TestCase):
    def setUp(self):
        self.example = json.loads((ROOT / 'Examples/division.json').read_text())

    def test_native_call_label_shows_return_type_and_keeps_identity_in_tooltip(self):
        node = {'id': 'K2Node_CallFunction_1', 'kind': 'call',
                'properties': {'FunctionReference': '(MemberName="GetParentActor",bSelfContext=True)'},
                'pins': {'out:result': {'type': 'actor', 'name': 'ReturnValue'},
                         'out:then': {'type': 'exec'}}}
        templates = {'actor': {'PinType.PinCategory': '"object"',
                              'PinType.PinSubCategoryObject': '"/Script/CoreUObject.Class\'/Script/Engine.Actor\'"'},
                     'exec': {'PinType.PinCategory': '"exec"'}}
        self.assertEqual(node_lines(node, set(), pin_templates=templates),
                         ['Get Parent Actor', 'Returns: Actor reference'])
        document = {'schema': 2, 'nodes': [node], 'links': [], 'pinTemplates': templates}
        original = copy.deepcopy(document)
        svg = ET.fromstring(render(document)[0])
        texts = [n.text for n in svg.findall('.//{*}g[@class="graph-node"]/{*}text')]
        self.assertNotIn(node['id'], texts)
        self.assertIn(node['id'], svg.find('.//{*}g[@class="graph-node"]/{*}title').text)
        self.assertEqual(document, original)

    def test_signature_labels_preserve_named_outputs_containers_and_unknown_types(self):
        templates = {'bool': {'PinType.PinCategory': '"bool"'}, 'exec': {'PinType.PinCategory': '"exec"'}}
        node = {'id': 'ReadSettings', 'kind': 'call', 'operation': 'ReadSettings',
                'pins': {'out:Success': {'type': 'bool'}, 'out:then': {'type': 'exec'}}}
        self.assertIn('Output Success: bool', node_lines(node, set(), pin_templates=templates))
        del node['pins']['out:Success']
        self.assertIn('No data outputs', node_lines(node, set(), pin_templates=templates))
        node['pins']['out:result'] = {'type': 'missing'}
        self.assertIn('Returns: unknown', node_lines(node, set(), pin_templates=templates))
        del node['pins']
        self.assertFalse(any(line.startswith('Returns:') for line in node_lines(node, set())))
        self.assertEqual(pin_type_label({'PinType.PinCategory': '"real"', 'PinType.PinSubCategory': '"double"',
                                        'PinType.ContainerType': 'Array'}), 'array<double>')
        self.assertEqual(pin_type_label({'PinType.PinCategory': '"name"', 'PinType.ContainerType': 'Map',
                                        'PinType.PinValueType': '(TerminalCategory="bool")'}), 'map<name, bool>')

    def test_four_views_are_deterministic_and_do_not_modify_source(self):
        original = copy.deepcopy(self.example)
        for view in ('simplified', 'full'):
            for direction in ('TB', 'LR'):
                first = render(self.example, view=view, direction=direction)
                self.assertEqual(first, render(self.example, view=view, direction=direction))
                ET.fromstring(first[0])
                self.assertEqual(first[2:], (4, 3))
        self.assertEqual(self.example, original)

    def test_hidden_nodes_do_not_create_bypass_links(self):
        document = {'schema': 1, 'nodes': [
            {'id': 'start', 'kind': 'event'},
            {'id': 'off', 'kind': 'sequence', 'enabled': False},
            {'id': 'self', 'kind': 'self'},
            {'id': 'self.result', 'kind': 'getVariable', 'variable': 'Result'},
        ], 'links': [
            {'from': 'start.then', 'to': 'off.exec'},
            {'from': 'off.then', 'to': 'self.result.exec'},
            {'from': 'self.result', 'to': 'self.result.target'},
        ]}
        simplified = render(document)
        self.assertEqual(simplified[2:], (2, 0))
        self.assertIn('Self (hidden)', simplified[0])
        self.assertIn('disabled node (hidden)', simplified[0])
        self.assertEqual(render(document, view='full')[2:], (4, 3))
        document['links'].append({'from': 'off.then', 'to': 'missing.exec'})
        with self.assertRaisesRegex(ValueError, 'Unresolved'):
            render(document)

    def test_cycles_and_empty_simplified_graph(self):
        document = {'schema': 1, 'nodes': [{'id': 'loop', 'kind': 'sequence'}],
                    'links': [{'from': 'loop.then', 'to': 'loop.exec'}]}
        for direction in ('TB', 'LR'):
            result = render(document, direction=direction)
            ET.fromstring(result[0])
            self.assertEqual(result[2:], (1, 1))
        document['nodes'][0]['enabled'] = False
        self.assertEqual(render(document)[2:], (0, 0))

    def test_labels_are_escaped(self):
        self.example['graphs'][0]['body']['nodes'][0]['comment'] = '<script>alert("x")</script>'
        svg = render(self.example)[0]
        self.assertNotIn('<script>', svg)
        ET.fromstring(svg)

    def test_execution_colours_follow_pin_types_not_return_values(self):
        templates = {'exec': {'PinType.PinCategory': '"exec"'}, 'bool': {'PinType.PinCategory': '"bool"'}}
        nodes = [
            {'id': 'entry', 'kind': 'event', 'pins': {'out:Start': {'type': 'exec'}}},
            {'id': 'void', 'kind': 'call', 'pins': {'in:Go': {'type': 'exec'}}},
            {'id': 'returning', 'kind': 'call', 'pins': {'in:Go': {'type': 'exec'}, 'out:result': {'type': 'bool'}}},
            {'id': 'pure', 'kind': 'call', 'pins': {'out:then': {'type': 'bool'}}},
            {'id': 'value', 'kind': 'getVariable', 'pins': {'in:flag': {'type': 'bool'}}}]
        document = {'schema': 2, 'nodes': nodes, 'pinTemplates': templates, 'links': [
            {'from': 'entry.Start', 'to': 'void.Go'}, {'from': 'pure.then', 'to': 'value.flag'}]}
        svg = ET.fromstring(render(document)[0])
        fills = {n.attrib['data-node-id']: n.find('{*}rect').attrib['fill']
                 for n in svg.findall('.//{*}g[@class="graph-node"]')}
        self.assertEqual(fills, {'entry': '#dce9fc', 'void': '#f7e9d7', 'returning': '#f7e9d7',
                                 'pure': '#e8edf4', 'value': '#e8edf4'})
        strokes = {e.attrib['data-source']: e.find('{*}path').attrib['stroke']
                   for e in svg.findall('.//{*}g[@class="graph-edge"]')}
        self.assertEqual(strokes, {'entry': '#b36b13', 'pure': '#317b9b'})

    def test_action_labels_describe_native_node_roles_without_ids(self):
        cases = [
            ('CreateDelegate', {'SelectedFunctionName': '"HandleControllerChanged"'},
             ('Create callback', 'HandleControllerChanged', 'does not call it')),
            ('cast', {'TargetType': '"/Script/CoreUObject.Class\'/Script/Engine.Pawn\'"'},
             ('Cast to Pawn', 'Check type')),
            ('AddDelegate', {'DelegateReference': '(MemberName="ReceiveControllerChangedDelegate")'},
             ('Bind event handler', 'ReceiveControllerChangedDelegate')),
            ('RemoveDelegate', {'DelegateReference': '(MemberName="Changed")'},
             ('Unbind event handler', 'Changed')),
            ('MacroInstance', {'MacroGraphReference': '(MacroGraph="/Engine/Macros.Macros:ForEachLoop")'},
             ('Macro: For Each Loop',)),
            ('FunctionEntry', {'FunctionReference': '(MemberName="ReloadSettings")'},
             ('Function entry', 'ReloadSettings')),
        ]
        for kind, properties, expected in cases:
            with self.subTest(kind=kind):
                lines = ' '.join(node_lines({'id': 'technical_id', 'kind': kind, 'properties': properties}, set()))
                self.assertNotIn('technical_id', lines)
                for text in expected:
                    self.assertIn(text, lines)

    def test_output_defaults_are_not_presented_as_computed_values(self):
        node = {'id': 'sum', 'kind': 'call', 'operation': 'Add',
                'defaults': {'in:left': '0.0', 'in:right': '1.0', 'out:result': '0.0'}}
        original = copy.deepcopy(node)
        lines = node_lines(node, {'left'})
        self.assertIn('Input: right = "1.0"', lines)
        self.assertFalse(any('0.0' in line for line in lines))
        self.assertEqual(node, original)

    def test_unconnected_outputs_use_original_connections_and_do_not_invent_links(self):
        document = {'schema': 2, 'pinTemplates': {'exec': {'PinType.PinCategory': '"exec"'},
                                                'bool': {'PinType.PinCategory': '"bool"'}},
                    'nodes': [
                        {'id': 'seq', 'kind': 'sequence', 'pins': {'out:then0': {'type': 'exec'}, 'out:then1': {'type': 'exec'}}},
                        {'id': 'disabled', 'kind': 'call', 'enabled': False, 'pins': {'in:exec': {'type': 'exec'}}},
                        {'id': 'data', 'kind': 'call', 'pins': {'out:result': {'type': 'bool'}}}],
                    'links': [{'from': 'seq.out:then0', 'to': 'disabled.in:exec'}]}
        for view in ('simplified', 'full'):
            text, _, _, links = render(document, view=view)
            svg = ET.fromstring(text)
            seq = svg.find('.//{*}g[@data-node-id="seq"]')
            labels = [n.text for n in seq.findall('{*}text')]
            self.assertIn('○ then1 — unconnected', labels)
            self.assertNotIn('○ then0 — unconnected', labels)
            self.assertIn('○ result — unconnected', text)
            self.assertEqual(links, 0 if view == 'simplified' else 1)

    def test_input_parameters_precede_output_types(self):
        node = {'id': 'calculate', 'kind': 'call', 'operation': 'Calculate',
                'defaults': {'in:left': '3', 'in:right': '2', 'out:result': '0'},
                'pins': {'out:result': {'type': 'double', 'name': 'ReturnValue'}}}
        templates = {'double': {'PinType.PinCategory': '"real"', 'PinType.PinSubCategory': '"double"'}}
        lines = node_lines(node, set(), ['target ← Self (hidden)'], templates)
        output_index = lines.index('Returns: double')
        for label in ('Input: left = "3"', 'Input: right = "2"', 'Input: target ← Self (hidden)'):
            self.assertLess(lines.index(label), output_index)

    def test_input_prefix_applies_to_all_node_kinds_and_data_edge_labels(self):
        for kind in ('call', 'event', 'setVariable', 'getVariable', 'branch', 'sequence',
                     'cast', 'CreateDelegate', 'AddDelegate', 'FunctionEntry', 'FunctionResult', 'MacroInstance'):
            with self.subTest(kind=kind):
                lines = node_lines({'id': 'n', 'kind': kind, 'defaults': {'in:value': 'true'}}, set())
                self.assertIn('Input: value = "true"', lines)
        document = {'schema': 1, 'nodes': [{'id': 'a', 'kind': 'getVariable'}, {'id': 'b', 'kind': 'branch'}],
                    'links': [{'from': 'a.result', 'to': 'b.Condition'}]}
        svg = ET.fromstring(render(document)[0])
        edge = svg.find('.//{*}g[@class="graph-edge"]')
        self.assertIsNone(edge.find('{*}text'))
        self.assertIn('Input: Condition', edge.find('{*}title').text)

    def test_nonbranch_labels_are_hover_only_with_full_path_hit_targets(self):
        for view in ('simplified', 'full'):
            for direction in ('TB', 'LR'):
                svg = ET.fromstring(render(self.example, view=view, direction=direction)[0])
                for edge in svg.findall('.//{*}g[@class="graph-edge"]'):
                    line = edge.find('{*}path[@class="edge-line"]')
                    hit = edge.find('{*}path[@class="edge-hit"]')
                    self.assertEqual(hit.attrib['d'], line.attrib['d'])
                    self.assertEqual(hit.attrib['stroke-width'], '14')
                    self.assertEqual(hit.attrib['vector-effect'], 'non-scaling-stroke')
                    self.assertEqual(hit.attrib['pointer-events'], 'stroke')
                    self.assertNotIn('stroke-dasharray', hit.attrib)
                    self.assertNotIn('marker-end', hit.attrib)
                    if edge.attrib['data-source'] == 'divide':
                        self.assertIsNone(edge.find('{*}text'))
                        tooltip = edge.find('{*}title').text
                        self.assertIn('Output: result', tooltip)
                        self.assertIn('Input: value', tooltip)
                        self.assertIn('From: divide', tooltip)
                        self.assertIn('To: set.result', tooltip)
                    else:
                        self.assertIsNone(edge.find('{*}text'))
                        self.assertIn(' → ', edge.find('{*}title').text)

    def test_edge_output_type_comes_from_producing_pin_and_chain_step(self):
        templates = {'double': {'PinType.PinCategory': '"real"', 'PinType.PinSubCategory': '"double"'},
                     'bool': {'PinType.PinCategory': '"bool"'}}
        first = {'id': 'first', 'kind': 'call', 'pins': {'out:result': {'type': 'double'}}}
        last = {'id': 'last', 'kind': 'call', 'pins': {'out:result': {'type': 'bool'}}}
        group = dict(last, _foldedNodes=[first, last])
        self.assertEqual(edge_output_type(group, 'Step 1: result', templates), 'double')
        self.assertEqual(edge_output_type(group, 'Step 2: result', templates), 'bool')
        self.assertEqual(edge_output_type(group, 'result', templates), 'bool')
        self.assertIsNone(edge_output_type(group, 'missing', templates))
        self.assertIsNone(edge_output_type(group, 'Step 9: result', templates))
        document = {'schema': 2, 'nodes': [first, {'id': 'sink', 'kind': 'branch'}],
                    'pinTemplates': templates, 'links': [{'from': 'first.out:result', 'to': 'sink.in:value'}]}
        for view in ('simplified', 'full'):
            edge = ET.fromstring(render(document, view=view)[0]).find('.//{*}g[@class="graph-edge"]')
            self.assertIn('Output: result (double)', edge.find('{*}title').text)
        document['pinTemplates'] = {}
        edge = ET.fromstring(render(document)[0]).find('.//{*}g[@class="graph-edge"]')
        self.assertEqual(edge.find('{*}title').text.splitlines()[0], 'Output: result')

    def test_terminal_execution_output_is_quiet_but_data_and_branch_outputs_remain(self):
        templates = {'exec': {'PinType.PinCategory': '"exec"'}, 'bool': {'PinType.PinCategory': '"bool"'}}
        node = {'id': 'call', 'kind': 'call', 'pins': {
            'out:then': {'type': 'exec'}, 'out:result': {'type': 'bool'}}}
        self.assertEqual(unconnected_outputs(node, set(), templates), ['result'])
        node['pins']['out:else'] = {'type': 'exec'}
        self.assertEqual(unconnected_outputs(node, {'then'}, templates), ['result', 'else'])
        self.assertEqual(unconnected_outputs({'kind': 'sequence', 'outputs': 1}, set(), {}), [])
        self.assertEqual(unconnected_outputs({'kind': 'sequence', 'outputs': 2}, {'then0'}, {}), ['then1'])

    def test_branch_choices_are_labelled_at_source_even_on_long_routes(self):
        document = {'schema': 1, 'nodes': [{'id': 'branch', 'kind': 'branch'},
                    {'id': 'near', 'kind': 'sequence'}, {'id': 'far', 'kind': 'sequence'}],
                    'links': [{'from': 'branch.then', 'to': 'near.exec'},
                              {'from': 'near.then', 'to': 'far.exec'},
                              {'from': 'branch.else', 'to': 'far.exec'}]}
        for direction in ('TB', 'LR'):
            svg = ET.fromstring(render(document, direction=direction)[0])
            rect = svg.find('.//{*}g[@data-node-id="branch"]/{*}rect')
            x, y, w, h = (float(rect.attrib[k]) for k in ('x', 'y', 'width', 'height'))
            labels = svg.findall('.//{*}g[@data-source="branch"]/{*}text')
            self.assertEqual({n.text for n in labels}, {'then', 'else'})
            for label in labels:
                self.assertNotIn('transform', label.attrib)
                lx, ly = float(label.attrib['x']), float(label.attrib['y'])
                if direction == 'TB':
                    self.assertTrue(x < lx < x+w)
                    self.assertEqual(ly, y+h+13)
                else:
                    self.assertEqual(lx, x+w+10)
                    self.assertTrue(y < ly < y+h)

    def test_branch_then_port_stays_first_when_destinations_are_reversed(self):
        document = {'schema': 1, 'nodes': [{'id': 'branch', 'kind': 'branch'},
                    {'id': 'a_else', 'kind': 'sequence'}, {'id': 'z_then', 'kind': 'sequence'}],
                    'links': [{'from': 'branch.then', 'to': 'z_then.exec'},
                              {'from': 'branch.else', 'to': 'a_else.exec'}]}
        for view in ('simplified', 'full'):
            for direction in ('TB', 'LR'):
                svg = ET.fromstring(render(document, view=view, direction=direction)[0])
                labels = {n.text: n for n in svg.findall('.//{*}g[@data-source="branch"]/{*}text')}
                axis = 'x' if direction == 'TB' else 'y'
                self.assertLess(float(labels['then'].attrib[axis]), float(labels['else'].attrib[axis]))
                # Only source port placement changes, not the destination identity.
                then_edge = svg.find('.//{*}g[@data-target="z_then"]')
                self.assertEqual(then_edge.find('{*}text').text, 'then')

    def test_unary_chain_folding_preserves_originals_and_full_view(self):
        templates = {'bool': {'PinType.PinCategory': '"bool"'}}
        nodes = [{'id': 'read', 'kind': 'getVariable', 'variable': 'ResetActionWasActive'},
                 {'id': 'not', 'kind': 'call',
                  'properties': {'FunctionReference': '(MemberName="Not_PreBool")', 'bDefaultsToPureFunc': 'True'},
                  'pins': {'in:A': {'type': 'bool'}, 'out:result': {'type': 'bool', 'name': 'ReturnValue'}}},
                 {'id': 'sink', 'kind': 'branch'}]
        edges = [('read', 'value', 'not', 'A', False), ('not', 'result', 'sink', 'Condition', False)]
        original = copy.deepcopy(nodes)
        folded, links, count = fold_data_chains(nodes, edges, edges, templates)
        self.assertEqual((len(folded), len(links), count), (2, 1, 1))
        self.assertEqual(folded[0]['_expression'], 'NOT (ResetActionWasActive)')
        self.assertEqual(folded[0]['_foldedNodes'], original[:2])
        self.assertEqual(links, edges[1:])
        self.assertEqual(nodes, original)
        document = {'schema': 2, 'nodes': nodes, 'pinTemplates': templates,
                    'links': [{'from': 'read.value', 'to': 'not.A'}, {'from': 'not.result', 'to': 'sink.Condition'}]}
        self.assertEqual(render(document)[2:], (2, 1))
        self.assertEqual(render(document, view='full')[2:], (3, 2))
        self.assertIn('NOT', node_lines(nodes[1], set()))
        # Source fan-out, extra target dependencies and execution stay explicit.
        for extra in [('read', 'value', 'sink', 'Other', False),
                      ('hidden', 'value', 'not', 'target', False)]:
            self.assertEqual(fold_data_chains(nodes, edges, edges + [extra], templates)[2], 0)
        changed = [('read', 'value', 'not', 'A', True), edges[1]]
        self.assertEqual(fold_data_chains(nodes, changed, changed, templates)[2], 0)
        branched = edges + [('not', 'result', 'sink', 'Other', False)]
        self.assertEqual(len(fold_data_chains(nodes, branched, branched, templates)[1]), 2)
        nodes[1]['pins']['in:Other'] = {'type': 'bool'}
        self.assertNotIn('_expression', fold_data_chains(nodes, edges, edges, templates)[0][0])

    def test_pure_calls_fold_with_inputs_and_keep_all_boundary_connections(self):
        templates = {'data': {'PinType.PinCategory': '"bool"'}}
        def call(id, name):
            return {'id': id, 'kind': 'call', 'properties': {
                'FunctionReference': f'(MemberName="{name}")', 'bDefaultsToPureFunc': 'True'},
                'pins': {'in:value': {'type': 'data'}, 'out:result': {'type': 'data'}}}
        nodes = [{'id': 'input', 'kind': 'event'}, call('read', 'GetBoundActionValue'),
                 call('convert', 'Conv_InputActionValueToBool'), {'id': 'sink', 'kind': 'branch'}]
        nodes[1]['defaults'] = {'in:constant': 'true'}
        nodes[1]['pins']['in:constant'] = {'type': 'data'}
        edges = [('input', 'value', 'read', 'value', False),
                 ('read', 'result', 'convert', 'value', False),
                 ('convert', 'result', 'sink', 'Condition', False)]
        result, links, count = fold_data_chains(nodes, edges, edges, templates)
        self.assertEqual(count, 1)
        group = next(n for n in result if '_foldedNodes' in n)
        self.assertEqual([n['id'] for n in group['_foldedNodes']], ['read', 'convert'])
        self.assertIn('constant = "true"', ' '.join(node_lines(group, set())))
        self.assertEqual(links, [('input', 'value', 'convert', 'Step 1: value', False), edges[2]])
        self.assertEqual(group['_boundaryLinks'], [edges[0], edges[2]])
        # A closed data cycle has no eligible start; no link or node is lost.
        cycle = [('read', 'result', 'convert', 'value', False), ('convert', 'result', 'read', 'value', False)]
        self.assertEqual(fold_data_chains(nodes[1:3], cycle, cycle, templates)[2], 0)

    def test_action_chain_preserves_order_data_edges_and_all_ids(self):
        templates = {'exec': {'PinType.PinCategory': '"exec"'}, 'data': {'PinType.PinCategory': '"bool"'}}
        def action(id):
            return {'id': id, 'kind': 'call', 'operation': id,
                    'pins': {'in:exec': {'type': 'exec'}, 'out:then': {'type': 'exec'},
                             'in:value': {'type': 'data'}, 'out:result': {'type': 'data'}},
                    'defaults': {'in:value': 'false'}}
        nodes = [{'id': 'event', 'kind': 'event'}, action('first'), action('second'),
                 {'id': 'consumer', 'kind': 'branch'}, {'id': 'value', 'kind': 'getVariable'}]
        nodes[1]['pins']['out:Unused'] = {'type': 'data'}
        edges = [('event', 'then', 'first', 'exec', True), ('first', 'then', 'second', 'exec', True),
                 ('second', 'then', 'consumer', 'exec', True), ('value', 'value', 'first', 'value', False),
                 ('first', 'result', 'second', 'value', False), ('first', 'result', 'consumer', 'Condition', False)]
        original = copy.deepcopy(nodes)
        result, links, count = fold_chains(nodes, edges, edges, templates, mode='action')
        self.assertEqual(count, 1)
        group = next(n for n in result if '_foldedNodes' in n)
        self.assertEqual([n['id'] for n in group['_foldedNodes']], ['first', 'second'])
        self.assertIn('Action chain', ' '.join(node_lines(group, set())))
        self.assertIn('value ← Step 1: result', ' '.join(node_lines(group, set())))
        self.assertEqual(links, [edges[0][:2] + ('second', 'Step 1: exec', True), edges[2],
                               ('value', 'value', 'second', 'Step 1: value', False),
                               ('second', 'Step 1: result', 'consumer', 'Condition', False)])
        self.assertEqual(nodes, original)
        document = {'schema': 2, 'nodes': nodes, 'pinTemplates': templates, 'links': [
            {'from': a+'.out:'+p, 'to': b+'.in:'+q} for a,p,b,q,_ in edges]}
        svg = ET.fromstring(render(document)[0])
        composite = svg.find('.//{*}g[@data-node-id="second"]')
        self.assertEqual(json.loads(composite.attrib['data-node-ids']), ['first', 'second'])
        self.assertEqual(composite.find('{*}rect').attrib['fill'], '#f7e9d7')
        labels = [n.text for n in composite.findall('{*}text')]
        self.assertIn('○ result — unconnected', labels)
        self.assertNotIn('○ then — unconnected', labels)
        self.assertLess(labels.index('○ Unused — unconnected'), labels.index('2. second'))
        self.assertGreater(labels.index('○ result — unconnected'), labels.index('2. second'))
        self.assertFalse(any('○ Step ' in label for label in labels))
        self.assertIn('Input: value ← Step 1: result', labels)
        self.assertEqual(render(document, view='full')[2:], (5, 6))
        # Merges, multiple execution outputs (even unwired), and cycles are boundaries.
        merge = edges + [('event', 'then', 'second', 'exec', True)]
        self.assertEqual(fold_chains(nodes, merge, merge, templates, mode='action')[2], 0)
        nodes[1]['pins']['out:Failed'] = {'type': 'exec'}
        self.assertEqual(fold_chains(nodes, edges, edges, templates, mode='action')[2], 0)
        del nodes[1]['pins']['out:Failed']
        cycle = [('first', 'then', 'second', 'exec', True), ('second', 'then', 'first', 'exec', True)]
        self.assertEqual(fold_chains(nodes[1:3], cycle, cycle, templates, mode='action')[2], 0)

    def test_selection_metadata_preserves_exact_ids(self):
        special_id = 'node."<&'
        document = {'schema': 1, 'nodes': [
            {'id': special_id, 'kind': 'event'}, {'id': 'next', 'kind': 'sequence'}],
            'links': [{'from': special_id + '.then', 'to': 'next.exec'}]}
        svg = ET.fromstring(render(document)[0])
        nodes = svg.findall('.//{*}g[@class="graph-node"]')
        edges = svg.findall('.//{*}g[@class="graph-edge"]')
        self.assertEqual({n.attrib['data-node-id'] for n in nodes}, {special_id, 'next'})
        self.assertEqual([(e.attrib['data-source'], e.attrib['data-target']) for e in edges],
                         [(special_id, 'next')])

    def test_cli_outputs_all_variants_and_preserves_input(self):
        owner = ROOT.parents[1] / 'Tmp/BlueprintTextPrototype/render-tests'
        owner.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='case-', dir=owner) as directory:
            self.assertEqual(Path(directory).resolve().parent, owner.resolve())
            source, output = Path(directory) / 'input.json', Path(directory) / 'graph.svg'
            source.write_text(json.dumps(self.example))
            before = source.read_bytes()
            result = subprocess.run([sys.executable, '-B', str(ROOT / 'render_graph.py'),
                                     str(source), '--output', str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual((report['nodes'], report['links']), (4, 3))
            page = output.with_suffix('.html').read_text(encoding='utf-8')
            self.assertIn('<html lang="en">', page)
            self.assertNotRegex(page, r'[\u0400-\u04ff]')
            for view in ('simplified', 'full'):
                for direction in ('TB', 'LR'):
                    self.assertIn(f'id="{view}-{direction}"', page)
            self.assertNotIn('<script src=', page)
            ET.parse(output)
            ET.parse(output.with_name('graph.full.svg'))
            self.assertEqual(source.read_bytes(), before)
            collision_source = Path(directory) / 'collision.html'
            collision_source.write_bytes(before)
            rejected = subprocess.run([sys.executable, '-B', str(ROOT / 'render_graph.py'),
                                       str(collision_source), '--output', str(collision_source.with_suffix('.svg'))],
                                      capture_output=True, text=True)
            self.assertEqual(rejected.returncode, 2)
            self.assertIn('must not overwrite', rejected.stderr)
            self.assertEqual(collision_source.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
