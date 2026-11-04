"""Function navigation uses explicit BPT identities, snapshot pages and folded steps."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

from preview_graph_links import build_catalog, resolve_calls
from render_graph import render
from render_preview_index import generate


def function(name='Reset', identity='A' * 32):
    return dict(name=name, kind='function', properties={'GraphGuid': identity},
                format='compact', body=dict(nodes=[], links=[]))


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
