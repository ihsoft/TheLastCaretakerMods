"""Function navigation uses explicit BPT identities, snapshot pages and folded steps."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

from preview_graph_links import build_catalog, build_callers, resolve_calls
from render_graph import render, caller_navigation
from render_preview_index import generate


def function(name='Reset', identity='A' * 32):
    return dict(name=name, kind='function', properties={'GraphGuid': identity},
                format='compact', body=dict(nodes=[dict(id='Entry', kind='FunctionEntry',
                    pins={'out:then': {'type': 'exec'}})], links=[]))


def call(identity, reference):
    return dict(id=identity, kind='call', properties={'FunctionReference': reference},
                pins={'in:exec': {'type': 'exec'}, 'out:then': {'type': 'exec'}})


def fixture():
    return dict(schema=2, asset='/Game/Caller',
        pinTemplates={'exec': {'PinType.PinCategory': '"exec"'}},
        graphs=[dict(name='Main', kind='event', format='compact', body=dict(nodes=[], links=[])), function()])


def entry(document, before=None, **item):
    return dict(asset=document['asset'], status='changed' if before else 'unchanged', item=item,
                docs=dict(source=document, **({'before': before} if before else {})))


class GraphLinkTests(unittest.TestCase):
    def test_callers_group_sites_by_graph_and_keep_full_hierarchy(self):
        from test_nested_graphs import fixture as nested_fixture
        source = nested_fixture()
        source['graphs'].append(function(identity='D'*32))
        ref = '(MemberName="Reset",bSelfContext=True)'
        source['graphs'][2]['body']['nodes'] += [call('one', ref), call('two', ref)]
        source['graphs'][0]['body']['nodes'].append(call('root', ref))
        entries = [entry(source)]
        callers = build_callers(entries, build_catalog(entries))['source']
        target = callers[(source['asset'], 'D'*32)]
        self.assertEqual(len(target), 2)
        nested = next(c for c in target if len(c['nodes']) == 2)
        self.assertEqual(nested['name'], 'Nested / Root / Details / Details')
        self.assertNotIn('#node-', nested['href'])
        self.assertEqual(nested['nodes'], ['one', 'two'])
        owner = callers[(source['asset'], 'B'*32)]
        self.assertEqual(len(owner), 1)
        self.assertEqual(owner[0]['href'], '../blueprint-000/graph-000.html#node-Operation')

    def test_caller_control_single_multiple_empty_and_escaped_labels(self):
        self.assertEqual(caller_navigation([]), '')
        caller = dict(name='BP / <Graph>', asset='/Game/BP', href='graph.html#node-call', nodes=['call'])
        direct = caller_navigation([caller])
        self.assertEqual(direct, '')
        menu = caller_navigation([caller, dict(caller, name='BP / Other', nodes=['one', 'two'])])
        self.assertIn('id="caller-menu"', menu)
        self.assertEqual(menu.count('<li>'), 2)
        self.assertIn('2 calls', menu)
        self.assertIn('<aside id="caller-menu" hidden', menu)
        self.assertNotIn('<summary', menu)

    def test_reverse_control_belongs_only_to_entry_node_in_every_view(self):
        from test_nested_graphs import fixture as nested_fixture
        from render_preview_index import graph_navigation
        for doc, key in [(fixture(), 'A'*32), (nested_fixture(), 'C'*32)]:
            pages = {graph.get('properties', {}).get('GraphGuid', graph['name']): str(i)+'.html' for i, graph in enumerate(doc['graphs'])}
            context = graph_navigation(doc, None, key, pages)
            caller = dict(name='BP / <Graph>', asset='/Game/BP', href='graph.html#node-call', nodes=['call'])
            for count in (0, 1, 2):
                context['callers'] = [caller] * count
                for view in ('simplified', 'full'):
                    for direction in ('TB', 'LR'):
                        svg = render(doc, key, view, direction, graph_context=context)[0]
                        tree = ET.fromstring(svg)
                        controls = tree.findall('.//*[@class="open-caller-graph"]')
                        self.assertEqual(len(controls), int(count > 0))
                        if not count:
                            continue
                        entry_node = tree.find('.//{*}g[@data-node-id="Entry"]')
                        self.assertIn(controls[0], list(entry_node))
                        buttons = [e for e in entry_node if e.get('class') in ('open-caller-graph', 'node-info', 'copy-node-id')]
                        self.assertEqual(len({b.get('transform') for b in buttons}), 3)
                        self.assertIn('scale(-1 1)', svg)
                        if count == 1:
                            self.assertEqual(controls[0].get('href'), caller['href'])
                        else:
                            self.assertEqual(controls[0].get('aria-controls'), 'caller-menu')
                            self.assertEqual(controls[0].get('role'), 'button')

    def test_reverse_links_keep_before_callers_for_unchanged_callee(self):
        old = fixture()
        old['graphs'][0]['body']['nodes'] = [call('oldCall', '(MemberParent="/Game/Other.Other_C",MemberName="Reset")')]
        new = copy.deepcopy(old)
        new['graphs'][0]['body']['nodes'] = []
        other = dict(fixture(), asset='/Game/Other')
        entries = [entry(new, old), entry(other)]
        catalog = build_catalog(entries)
        callers = build_callers(entries, catalog)
        key = ('/Game/Other', 'A'*32)
        self.assertNotIn(key, callers['source'])
        self.assertEqual(callers['before'][key][0]['href'], '../blueprint-000/graph-000-before.html#node-oldCall')
        self.assertEqual(resolve_calls(old, 'Main', catalog['before'])['oldCall']['href'], '../blueprint-001/graph-001-before.html')
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for name, doc in [('old', old), ('new', new), ('other', other)]:
                (root/(name+'.json')).write_text(json.dumps(doc), encoding='utf-8')
            (root/'manifest.json').write_text(json.dumps({'assets': [
                dict(source='new.json', before='old.json', status='changed', reviews={'Main': {'removed': ['oldCall']}}),
                'other.json']}), encoding='utf-8')
            generate(root/'manifest.json', root/'out')
            before = (root/'out/blueprint-001/graph-001-before.html').read_text(encoding='utf-8')
            current = (root/'out/blueprint-001/graph-001.html').read_text(encoding='utf-8')
            self.assertIn('class="open-caller-graph" href="../blueprint-000/graph-000-before.html#node-oldCall"', before)
            self.assertNotIn('class="open-caller-graph"', current)
            self.assertNotIn('open-caller-graph', before.split('<nav>', 1)[1].split('</nav>', 1)[0])

    def test_resolve_owner_and_guid_without_cross_asset_name_guesses(self):
        source = fixture()
        other = dict(fixture(), asset='/Game/Other')
        local = '(MemberName="Reset",MemberGuid=' + 'A'*32 + ',bSelfContext=True)'
        foreign = '(MemberParent="/Script/Engine.BlueprintGeneratedClass\'/Game/Other.Other_C\'",MemberName="Reset")'
        source['graphs'][0]['body']['nodes'] = [
            call('local', local), call('foreign', foreign),
            call('unknown', '(MemberName="Reset")'),
            call('stale', local.replace('A'*32, 'B'*32)),
            call('native', '(MemberParent="/Script/CoreUObject.Class\'/Script/Engine.Actor\'",MemberName="Reset",bSelfContext=True)'),
            dict(call('entry', local), kind='FunctionEntry'),
            call('typed', '(MemberName="Reset")')]
        source['pinTemplates']['other'] = {'PinType.PinSubCategoryObject': '"/Game/Other.Other_C"'}
        source['graphs'][0]['body']['nodes'][-1]['pins']['in:target'] = {'type': 'other'}
        catalog = build_catalog([entry(source), entry(other)])
        calls = resolve_calls(source, 'Main', catalog['source'])
        self.assertEqual(set(calls), {'local', 'foreign', 'typed'})
        self.assertEqual(calls['local']['href'], '../blueprint-000/graph-001.html')
        self.assertEqual(calls['foreign']['href'], '../blueprint-001/graph-001.html')
        other['graphs'].append(copy.deepcopy(other['graphs'][1]))
        other['graphs'][-1]['properties']['GraphGuid'] = 'B' * 32
        ambiguous = build_catalog([entry(source), entry(other)])
        self.assertNotIn('foreign', resolve_calls(source, 'Main', ambiguous['source']))

    def test_external_before_target_does_not_jump_to_current_renamed_graph(self):
        source = fixture()
        source['graphs'][0]['body']['nodes'] = [call('call',
            '(MemberParent="/Game/Other.Other_C",MemberName="Reset",MemberGuid=' + 'A'*32 + ')')]
        old = dict(fixture(), asset='/Game/Other')
        new = copy.deepcopy(old)
        new['graphs'][1]['name'] = 'Renamed'
        catalog = build_catalog([entry(source), entry(new, old)])
        current = resolve_calls(source, 'Main', catalog['source'])['call']
        before = resolve_calls(source, 'Main', catalog['before'])['call']
        self.assertEqual((current['name'], current['href']), ('Renamed', '../blueprint-001/graph-001.html'))
        self.assertEqual((before['name'], before['href']), ('Reset', '../blueprint-001/graph-001-before.html'))

    def test_catalog_excludes_hidden_stubs_and_snapshot_absent_targets(self):
        doc = fixture()
        stub = entry(dict(fixture(), asset='/Game/Stub'), placeholder=True,
                     context={'selected': False})
        added = entry(doc)
        added['status'] = 'added'
        removed = dict(asset='/Game/Removed', status='removed', item={},
                       docs={'before': dict(fixture(), asset='/Game/Removed')})
        catalog = build_catalog([added, stub, removed])
        self.assertEqual(set(catalog['source']), {'/Game/Caller'})
        self.assertEqual(set(catalog['before']), {'/Game/Removed'})

    def test_folded_calls_keep_each_step_link_and_full_view_controls(self):
        source = fixture()
        source['graphs'].append(function('Clear', 'B'*32))
        body = source['graphs'][0]['body']
        body['nodes'] = [call('one', '(MemberName="Reset",bSelfContext=True)'),
                         call('two', '(MemberName="Clear",bSelfContext=True)')]
        body['links'] = [{'from': 'one.out:then', 'to': 'two.in:exec'}]
        original = copy.deepcopy(source)
        calls = resolve_calls(source, 'Main', build_catalog([entry(source)])['source'])
        for direction in ('TB', 'LR'):
            for view in ('full', 'simplified'):
                tree = ET.fromstring(render(source, 'Main', view, direction, graph_context={'calls': calls})[0])
                links = tree.findall('.//{*}a[@class="graph-reference"]') if view == 'simplified' else tree.findall('.//{*}a[@class="open-child-graph"]')
                self.assertEqual({a.get('href') for a in links}, {c['href'] for c in calls.values()})
                self.assertEqual(len(tree.findall('.//{*}g[@class="graph-node"]')), 1 if view == 'simplified' else 2)
                if view == 'simplified':
                    self.assertEqual([''.join(a.itertext()).split('FunctionReference')[0] for a in links], ['1. Reset', '2. Clear'])
                    self.assertFalse(tree.findall('.//{*}a[@class="open-child-graph"]'))
                    self.assertFalse(tree.findall('.//{*}rect[@class="nested-graph-frame"]'))
        self.assertEqual(source, original)

    def test_one_linked_step_does_not_make_a_folded_chain_a_graph(self):
        for status in ('unchanged', 'added', 'changed', 'removed'):
            for position in range(3):
                with self.subTest(status=status, position=position):
                    source = fixture()
                    nodes = [call(str(i), '(MemberName="NativeAction",bSelfContext=True)') for i in range(3)]
                    nodes[position]['properties']['FunctionReference'] = '(MemberName="Reset",bSelfContext=True)'
                    source['graphs'][0]['body'] = dict(nodes=nodes, links=[
                        {'from': str(i)+'.out:then', 'to': str(i+1)+'.in:exec'} for i in range(2)])
                    empty = copy.deepcopy(source)
                    empty['graphs'][0]['body'] = dict(nodes=[], links=[])
                    before = empty if status == 'added' else source
                    after = empty if status == 'removed' else source
                    marks = None if status == 'unchanged' else {status: ['0', '1', '2']}
                    catalog = build_catalog([entry(after, before)])
                    context = dict(calls=resolve_calls(after, 'Main', catalog['source']),
                        beforeCalls=resolve_calls(before, 'Main', catalog['before']))
                    expected = '../blueprint-000/graph-001' + ('-before' if status == 'removed' else '') + '.html'
                    for direction in ('TB', 'LR'):
                        for view in ('simplified', 'full'):
                            tree = ET.fromstring(render(after, 'Main', view, direction,
                                review=marks, before=before, graph_context=context)[0])
                            buttons = tree.findall('.//{*}a[@class="open-child-graph"]')
                            frames = tree.findall('.//{*}rect[@class="nested-graph-frame"]')
                            if view == 'simplified':
                                self.assertEqual(len(tree.findall('.//{*}g[@class="graph-node"]')), 1)
                                self.assertEqual((buttons, frames), ([], []))
                                links = tree.findall('.//{*}a[@class="graph-reference"]')
                                self.assertEqual([a.get('href') for a in links], [expected])
                            else:
                                self.assertEqual([a.get('href') for a in buttons], [expected])
                                self.assertEqual(len(frames), 1)

    def test_batch_links_current_before_removed_and_final_views(self):
        old = fixture()
        old['graphs'][0]['body']['nodes'] = [call('gone', '(MemberName="Reset",bSelfContext=True)')]
        new = copy.deepcopy(old)
        new['graphs'][0]['body']['nodes'] = [call('new',
            '(MemberParent="/Game/Other.Other_C",MemberName="Reset",MemberGuid=' + 'A'*32 + ')')]
        other_old = dict(fixture(), asset='/Game/Other')
        other_new = copy.deepcopy(other_old)
        other_new['graphs'][1]['name'] = 'Renamed'
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for name, doc in [('old', old), ('new', new), ('other-old', other_old), ('other-new', other_new)]:
                (root/(name+'.json')).write_text(json.dumps(doc), encoding='utf-8')
            manifest = {'assets': [dict(source='new.json', before='old.json', status='changed',
                reviews={'Main': {'added': ['new'], 'removed': ['gone']}}),
                dict(source='other-new.json', before='other-old.json', status='changed')]}
            (root/'manifest.json').write_text(json.dumps(manifest), encoding='utf-8')
            generate(root/'manifest.json', root/'out')
            base = root/'out/blueprint-000'
            current = (base/'graph-000.html').read_text(encoding='utf-8')
            before = (base/'graph-000-before.html').read_text(encoding='utf-8')
            self.assertIn('href="../blueprint-001/graph-001.html"', current)
            self.assertIn('href="../blueprint-000/graph-001-before.html"', current)
            self.assertIn('href="../blueprint-000/graph-001-before.html"', before)
            self.assertNotIn('href="../blueprint-001/graph-001.html"', before)
            final = current.split('<section id="final-simplified-TB"', 1)[1].split('</section>', 1)[0]
            self.assertIn('href="../blueprint-001/graph-001.html"', final)
            self.assertNotIn('graph-001-before.html', final)
            for page in (base/'graph-000.svg', base/'graph-000-before.svg'):
                tree = ET.fromstring(page.read_text(encoding='utf-8'))
                for link in tree.findall('.//{*}a[@class="open-child-graph"]'):
                    self.assertTrue((base/link.get('href')).is_file())


if __name__ == '__main__':
    unittest.main()
