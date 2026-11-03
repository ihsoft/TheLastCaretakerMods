"""Bounded hierarchy preservation, failure and preview navigation contracts."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

from asset_text_codec import compact, native, compare
from graph_hierarchy import select_graph, validate_hierarchy, inspect_graphs
from render_preview_index import generate, graph_navigation
from render_graph import render, review_document


def fixture():
    template = {'PinType.PinCategory': '"int"'}
    def node(identity, kind, direction, flag=None):
        return dict(id=identity, kind=kind, properties={flag: 'True'} if flag else {},
                    pins={direction + ':Value': dict(id=('1' if direction == 'in' else '2')*32, type='int')})
    def graph(identity, name, nodes, owner=None):
        g = dict(name=name, kind='collapsed' if owner else 'function',
                 properties={'GraphGuid': identity*32}, format='compact', body=dict(nodes=nodes, links=[]))
        if owner:
            g['owner'] = owner
        return g
    top = node('Operation', 'Composite', 'in')
    top['pins']['out:Value'] = dict(id='3'*32, type='int')
    entry = node('Entry', 'Tunnel', 'out', 'bCanHaveOutputs')
    exit_node = node('Exit', 'Tunnel', 'in', 'bCanHaveInputs')
    child_top = copy.deepcopy(top)
    root = graph('A', 'Root', [top])
    child = graph('B', 'Details', [entry, exit_node, child_top], dict(graph=root['properties']['GraphGuid'], node='Operation', entry='Entry', exit='Exit'))
    # Pin GUIDs are unique within each graph, while object names are scoped.
    child_top['pins']['in:Value']['id'] = '4'*32
    child_top['pins']['out:Value']['id'] = '5'*32
    inner = graph('C', 'Details', [copy.deepcopy(entry), copy.deepcopy(exit_node)], dict(graph=child['properties']['GraphGuid'], node='Operation', entry='Entry', exit='Exit'))
    inner['body']['links'] = [{'from': 'Entry.out:Value', 'to': 'Exit.in:Value'}]
    return dict(schema=2, format='compact-asset', asset='/Game/Nested', parent='/Script/CoreUObject.Object',
                properties={}, variables=[], defaults={}, dependencies=[],
                classes={k: '/Script/BlueprintGraph.K2Node_'+k for k in ('Composite', 'Tunnel')},
                pinTemplates={'int': template}, graphs=[root, child, inner])


class NestedGraphsTests(unittest.TestCase):
    def test_roundtrip_and_scoped_selection(self):
        source = fixture()
        self.assertEqual(compare(source, compact(native(source))), [])
        self.assertEqual(select_graph(source, 'C'*32)['name'], 'Details')
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            select_graph(source, 'Details')

    def test_read_view_has_boundary_context_and_only_used_definitions(self):
        source = fixture()
        source['pinTemplates']['unused'] = {'PinType.PinCategory': '"float"'}
        detail = inspect_graphs(source, 'C'*32)
        self.assertEqual(detail['view'], 'graph-detail')
        self.assertEqual(len(detail['ancestors']), 2)
        self.assertEqual(set(detail['pinTemplates']), {'int'})
        self.assertEqual(len(inspect_graphs(source)['graphs']), 3)
        self.assertEqual(inspect_graphs(source, 'A'*32)['children'][0]['id'], 'B'*32)

    def test_removing_last_collapsed_graph_restores_flat_shape(self):
        source = fixture()
        source['graphs'] = source['graphs'][:1]
        source['graphs'][0]['body']['nodes'] = []
        result = compact(native(source))
        self.assertNotIn('owner', result['graphs'][0])
        self.assertNotIn('id', result['graphs'][0])
        self.assertEqual(compare(source, result), [])

    def test_invalid_ownership_and_boundaries_rejected_before_native(self):
        changes = [
            lambda d: d['graphs'][1]['owner'].update(graph='F'*32),
            lambda d: d['graphs'][1]['owner'].update(graph='C'*32),
            lambda d: d['graphs'][1]['owner'].update(node='Missing'),
            lambda d: d['graphs'][1]['owner'].update(exit='Entry'),
            lambda d: d['graphs'][1]['owner'].update(entry='Missing'),
            lambda d: d['graphs'][0]['body']['nodes'][0]['pins'].pop('in:Value'),
            lambda d: d['graphs'][1]['body']['nodes'][0]['properties'].update(bCanHaveInputs='True'),
            lambda d: d['graphs'][1]['body']['nodes'][0]['properties'].update(OutputSourceNode='something'),
            lambda d: d['graphs'][2]['body']['links'].append({'from': 'Operation.out:Value', 'to': 'Exit.in:Value'}),
            lambda d: d['graphs'][2]['properties'].update(GraphGuid='A'*32),
            lambda d: d['graphs'][0]['properties'].update(SubGraphs='()'),
            lambda d: d['graphs'].append(dict(copy.deepcopy(d['graphs'][1]), properties={'GraphGuid': 'D'*32})),
            lambda d: d['graphs'].__delitem__(slice(1, None)),
        ]
        for change in changes:
            with self.subTest(change=change):
                source = fixture()
                change(source)
                with self.assertRaises(ValueError):
                    native(source)

        source = fixture()
        source['pinTemplates']['float'] = {'PinType.PinCategory': '"real"', 'PinType.PinSubCategory': '"double"'}
        source['graphs'][1]['body']['nodes'][0]['pins']['out:Value']['type'] = 'float'
        with self.assertRaisesRegex(ValueError, 'boundaries differ'):
            native(source)

    def test_descendant_edit_and_rename_keep_snapshot_navigation(self):
        old = fixture()
        new = copy.deepcopy(old)
        new['graphs'][2]['name'] = 'Renamed'
        new['graphs'][2]['body']['nodes'][0]['properties']['NodeComment'] = '"Edited"'
        pages = {g['properties']['GraphGuid']: str(i)+'.html' for i, g in enumerate(new['graphs'])}
        nav = graph_navigation(new, old, 'A'*32, pages)
        self.assertTrue(nav['children']['Operation']['changed'])
        svg = render(new, 'A'*32, graph_context=nav)[0]
        self.assertIn('Open graph', svg)
        self.assertIn('href="1.html"', svg)
        self.assertIn('Nested implementation changed', svg)
        reviewed = review_document(new, old, {'changed': ['Entry']}, 'C'*32)
        self.assertIn('Renamed', render(reviewed, 'C'*32)[1])
        before_pages = {key: path.replace('.html', '-before.html') for key, path in pages.items()}
        before_nav = graph_navigation(old, None, 'C'*32, before_pages)
        self.assertTrue(all('-before.html' in b['href'] for b in before_nav['breadcrumbs'][1:]))

    def test_full_preview_one_call_and_before_links(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            old, new = fixture(), fixture()
            new['graphs'][2]['name'] = 'Renamed'
            new['graphs'][2]['body']['nodes'][0]['properties']['NodeComment'] = '"Edited"'
            for name, doc in (('before', old), ('after', new)):
                (root/(name+'.json')).write_text(json.dumps(doc), encoding='utf-8')
            manifest = root/'manifest.json'
            manifest.write_text(json.dumps(dict(assets=[dict(source='after.json', before='before.json', status='changed', reviews={'C'*32: {'changed': ['Entry']}})])), encoding='utf-8')
            generate(manifest, root/'preview')
            index = (root/'preview/index.html').read_text(encoding='utf-8')
            self.assertIn('<details open>', index)
            self.assertNotIn('Renamed — Added', index)
            viewer = root/'preview/blueprint-000'
            current = (viewer/'graph-002.html').read_text(encoding='utf-8')
            before = (viewer/'graph-002-before.html').read_text(encoding='utf-8')
            self.assertIn('graph-001.html#node-Operation', current)
            self.assertIn('graph-001-before.html#node-Operation', before)
            self.assertIn('class="graph-breadcrumbs"', current)

    def test_nested_operation_has_two_frames_and_three_separate_controls(self):
        source = fixture()
        pages = {g['properties']['GraphGuid']: str(i)+'.html' for i, g in enumerate(source['graphs'])}
        nav = graph_navigation(source, None, 'A'*32, pages)
        for view in ('simplified', 'full'):
            for direction in ('TB', 'LR'):
                svg = render(source, 'A'*32, view, direction, graph_context=nav)[0]
                tree = ET.fromstring(svg)
                operation = next(e for e in tree.iter() if e.get('data-node-id') == 'Operation')
                controls = [e for e in operation if e.get('class') in ('node-info', 'copy-node-id', 'open-child-graph')]
                self.assertEqual(len(controls), 3)
                self.assertEqual(len({e.get('transform') for e in controls}), 3)
                enter = next(e for e in controls if e.get('class') == 'open-child-graph')
                self.assertEqual(enter.get('href'), '1.html')
                self.assertEqual(enter.get('tabindex'), '0')
                self.assertIn('Details', enter.get('aria-label'))
                frames = [e for e in operation if e.tag.endswith('}rect')]
                self.assertEqual(len(frames), 2)
                self.assertEqual(float(frames[0].get('height')) - float(frames[1].get('height')), 8)
                self.assertEqual(float(frames[1].get('x')) - float(frames[0].get('x')), 4)
        leaf = render(source, 'C'*32, graph_context=graph_navigation(source, None, 'C'*32, pages))[0]
        self.assertNotIn('class="nested-graph-frame"', leaf)
        self.assertNotIn('class="open-child-graph"', leaf)

    def test_added_nested_asset_preview(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root/'source.json').write_text(json.dumps(fixture()), encoding='utf-8')
            manifest = root/'manifest.json'
            manifest.write_text(json.dumps(dict(assets=[dict(source='source.json', status='added')])), encoding='utf-8')
            generate(manifest, root/'preview')
            page = (root/'preview/blueprint-000/graph-001.html').read_text(encoding='utf-8')
            self.assertIn('graph-002.html', page)


if __name__ == '__main__':
    unittest.main()
