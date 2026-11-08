"""Offline end-to-end batch preview checks using synthetic Blueprints."""
import copy
import json
from pathlib import Path
import re
import tempfile
import unittest

from render_preview_index import generate, plan
from test_render_widget import fixture as widget_fixture


def fixture(asset='/Game/Test'):
    return dict(schema=1, asset=asset, graphs=[dict(name='Main', format='compact', body=dict(
        nodes=[dict(id='n', kind='call', operation='SetLimit', defaults={'in:Limit': '200'})], links=[]))])


class PreviewIndexTests(unittest.TestCase):
    def test_pin_order_only_change_has_before_page_and_node_mark(self):
        from asset_text_codec import compact
        from test_asset_text_codec import sequence_fixture
        old = compact(sequence_fixture())
        new = copy.deepcopy(old)
        node = new['graphs'][0]['body']['nodes'][0]
        node['pins'] = {key: node['pins'][key] for key in ('in:exec', 'out:then1', 'out:then0')}
        self.save('old.json', old); self.save('new.json', new)
        manifest = self.save('pins.json', dict(assets=[dict(
            source='new.json', before='old.json', status='changed')]))
        generate(manifest, self.root / 'pins')
        graph = self.root / 'pins/blueprint-000/graph-000.html'
        page = graph.read_text(encoding='utf-8')
        self.assertIn('id="show-changes"', page)
        self.assertIn('data-review-status="changed"', page)
        self.assertTrue(graph.with_name('graph-000-before.html').is_file())
        marks = json.loads(graph.with_suffix('.review.json').read_text())
        self.assertEqual(marks, dict(added=[], changed=['Sequence'], removed=[]))

    def test_changed_graphs_automatically_show_node_edits_without_reviews(self):
        old = fixture()
        old['graphs'][0]['name'] = 'CreateMooringPlates'
        old['graphs'].append(dict(name='CreateMooringPlate', format='compact', body=dict(
            nodes=[dict(id='entry', kind='call', operation='Entry'),
                   dict(id='setMesh', kind='call', operation='SetStaticMesh'),
                   dict(id='attach', kind='call', operation='Attach')],
            links=[{'from': 'entry.then', 'to': 'setMesh.exec'},
                   {'from': 'setMesh.then', 'to': 'attach.exec'}])))
        new = copy.deepcopy(old)
        new['graphs'][0]['body']['nodes'][0]['defaults'] = {'in:Scale': '1,-1,1'}
        new['graphs'][1]['body']['nodes'].pop(1)
        new['graphs'][1]['body']['links'] = [{'from': 'entry.then', 'to': 'attach.exec'}]
        self.save('old.json', old); self.save('new.json', new)
        for annotations in ({}, {'reviews': {'CreateMooringPlates': {'changed': []}}}):
            with self.subTest(annotations=annotations):
                suffix = 'partial' if annotations else 'automatic'
                manifest = self.save(suffix+'.json', dict(assets=[dict(
                    source='new.json', before='old.json', status='changed', **annotations)]))
                generate(manifest, self.root / suffix)
                for index in (0, 1):
                    page = (self.root / suffix / f'blueprint-000/graph-{index:03d}.html').read_text(encoding='utf-8')
                    self.assertIn('id="show-changes"', page)
                    self.assertIn('data-review-status="changed"', page)
                removed_page = (self.root / suffix / 'blueprint-000/graph-001.html').read_text(encoding='utf-8')
                self.assertIn('data-review-status="removed"', removed_page)
                marks = json.loads((self.root / suffix / 'blueprint-000/graph-001.review.json').read_text())
                self.assertEqual(marks, dict(added=[], changed=['attach', 'entry'], removed=['setMesh']))

    def test_root_node_search_has_scoped_current_and_before_links_and_safe_labels(self):
        old, new = fixture('/Game/A'), fixture('/Game/A')
        old['graphs'][0]['body']['nodes'].append(dict(id='gone node',kind='call',operation='Old'))
        new['graphs'][0]['body']['nodes'][0]['operation']='</script><script>alert(1)</script>'
        other=fixture('/Game/B')
        for name,doc in [('old',old),('new',new),('other',other)]:self.save(name+'.json',doc)
        manifest=self.save('search.json',dict(assets=[dict(source='new.json',before='old.json',status='changed'), 'other.json']))
        generate(manifest,self.root/'search')
        page=(self.root/'search/index.html').read_text(encoding='utf-8')
        payload=re.search(r'<script id="node-search-data" type="application/json">(.*?)</script>',page,re.S)[1]
        records=json.loads(payload)
        current=[n for n in records if n['snapshot']=='Current']
        self.assertEqual([(n['id'],n['path']) for n in current],[('n','A / Main'),('n','B / Main')])
        self.assertEqual(current[0]['href'],'blueprint-000/graph-000.html#node-n')
        removed=next(n for n in records if n['id']=='gone node')
        self.assertEqual(removed['snapshot'],'Before')
        self.assertEqual(removed['href'],'blueprint-000/graph-000-before.html#node-gone%20node')
        self.assertIn('\\u003c',payload)
        self.assertNotIn('</script>',payload)
        self.assertIn('id="node-search-query"',page)
        self.assertNotIn('id="node-search-query"',(self.root/'search/blueprint-000/graph-000.html').read_text(encoding='utf-8'))

    def test_node_search_names_nested_graphs_and_retains_each_scoped_node_id(self):
        from test_nested_graphs import fixture as nested_fixture
        source=nested_fixture()
        self.save('source.json',source)
        generate(self.save('search.json',dict(assets=['source.json'])),self.root/'nested-search')
        page=(self.root/'nested-search/index.html').read_text(encoding='utf-8')
        records=json.loads(re.search(r'<script id="node-search-data" type="application/json">(.*?)</script>',page,re.S)[1])
        entries=[n for n in records if n['id']=='Entry']
        self.assertEqual([n['path'] for n in entries],['Nested / Root / Details','Nested / Root / Details / Details'])
        self.assertEqual([n['href'] for n in entries],['blueprint-000/graph-001.html#node-Entry','blueprint-000/graph-002.html#node-Entry'])
        self.assertEqual(next(n for n in records if n['id']=='Operation')['title'],'Details')
        self.assertTrue(all(n['snapshot']=='Current' for n in records))

    def test_review_and_final_arrows_have_unique_local_markers(self):
        source = fixture()
        source['graphs'][0]['body']['nodes'].append(dict(id='sink', kind='call', operation='Consume'))
        source['graphs'][0]['body']['links'] = [
            {'from': 'n.then', 'to': 'sink.exec'}, {'from': 'n.result', 'to': 'sink.value'}]
        self.save('source.json', source)
        manifest = self.save('markers.json', dict(assets=[dict(source='source.json', before='source.json',
            status='changed', reviews={'Main': {'changed': ['n']}})]))
        generate(manifest, self.root / 'markers')
        page = (self.root / 'markers/blueprint-000/graph-000.html').read_text(encoding='utf-8')
        sections = re.findall(r'<section id="([^"]+)"[^>]*>(.*?)</section>', page, re.S)
        self.assertEqual(len(sections), 12)
        all_markers = re.findall(r'<marker id="([^"]+)"', page)
        self.assertEqual(len(all_markers), len(set(all_markers)), 'SVG IDs must be unique across hidden variants')
        for name, section in sections:
            markers = set(re.findall(r'<marker id="([^"]+)"', section))
            references = re.findall(r'marker-end="url\(#([^)]+)\)"', section)
            self.assertTrue(references)
            self.assertTrue(set(references) <= markers, name)
            if 'full' in name:
                edges = re.findall(r'<path class="edge-line"[^>]*marker-end="url\(#([^)]+)\)"', section)
                self.assertEqual(len(edges), 2)
                self.assertEqual({ref.removeprefix('final-').removeprefix('fullTB').removeprefix('fullLR')
                                  for ref in edges}, {'exec', 'data'})

    def test_review_has_independent_final_only_graph(self):
        old = fixture()
        old['graphs'][0]['body']['nodes'].append(dict(id='deleted', kind='call', operation='OldOperation'))
        self.save('old.json', old)
        self.save('new.json', fixture())
        manifest = self.save('review.json', dict(assets=[dict(source='new.json', before='old.json',
            status='changed', reviews={'Main': {'changed': ['n'], 'removed': ['deleted']}})]))
        output = self.root / 'clean-review'
        generate(manifest, output)
        page = (output / 'blueprint-000/graph-000.html').read_text(encoding='utf-8')
        self.assertIn('id="show-changes"', page)
        self.assertRegex(page, r'>Show unconnected</label>\s*<label[^>]*><input id="show-changes"')
        self.assertNotIn('Click node: highlight.', page)
        sections = dict(re.findall(r'<section id="([^"]+)"[^>]*>(.*?)</section>', page, re.S))
        self.assertEqual(len(sections), 12)
        for view in ('simplified', 'full', 'expressions'):
            for direction in ('TB', 'LR'):
                review = sections[f'{view}-{direction}']
                final = sections[f'final-{view}-{direction}']
                self.assertIn('data-node-id="deleted"', review)
                self.assertNotIn('data-node-id="deleted"', final)
                self.assertNotIn('data-review-status="changed"', final)
                self.assertNotIn('REVIEW ·', final)
                self.assertIn('data-node-id="n"', final)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def save(self, name, value):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value), encoding='utf-8')
        return path

    def test_informational_stub_is_visible_without_fake_graph_and_escapes_text(self):
        context = dict(asset='/Game/Stub', assetClass='UObject', role='stub', selected=False,
                       reason='Unsupported <script> graph', dependencies=['/Script/CoreUObject'], referencedBy=['/Game/Item'])
        manifest = self.save('context.json', dict(assets=[dict(context=context)]))
        result = generate(manifest, self.root / 'context-preview')
        page = Path(result['index']).read_text(encoding='utf-8')
        self.assertEqual(0, result['graphs'])
        self.assertTrue(result['items'][0]['placeholder'])
        self.assertIn('STUB', page)
        self.assertIn('Unsupported &lt;script&gt; graph', page)
        self.assertNotIn('/Game/Item', page)
        self.assertEqual(0, result['editableAssets'])
        self.assertEqual(1, result['listedAssets'])
        self.assertNotIn('<section id="asset-', page)
        context['reason'] = ''
        self.save('context.json', dict(assets=[dict(context=context)]))
        with self.assertRaisesRegex(ValueError, 'explicit reason'):
            plan(manifest)

    def test_source_editor_role_marks_stub_without_manifest_hint(self):
        source = fixture()
        source['editorRole'] = 'stub'
        self.save('stub.json', source)
        manifest = self.save('manifest.json', dict(assets=['stub.json']))
        self.assertTrue(plan(manifest)[1][0]['item']['placeholder'])

    def test_editable_graphs_and_property_references_replace_flat_dependencies(self):
        source = fixture()
        source['parent'] = '/Game/Stub.Stub_C'
        source['defaults'] = {'Mesh': '/Game/Mesh.Mesh'}
        source['graphs'][0]['body']['nodes'][0]['pins'] = {
            'in:Asset': {'DefaultObject': '/Game/Stub.Stub'}}
        stub = fixture('/Game/Stub')
        self.save('editable.json', source)
        self.save('stub.json', stub)
        def context(asset, role, **kwargs):
            return dict(asset=asset, assetClass='Blueprint', role=role, selected=False,
                        reason='', dependencies=[], referencedBy=[], **kwargs)
        parent = context('/Game/Stub', 'stub')
        data = context('/Game/Item', 'unclassified')
        data.update(assetClass='DataAsset', reason='No BPT editor', dependencies=['/Game/Noise'] * 19)
        manifest = self.save('manifest.json', dict(assets=[dict(source='editable.json'),
            dict(source='stub.json', context=parent), dict(context=data)]))
        original = (self.root / 'editable.json').read_bytes()
        result = generate(manifest, self.root / 'out')
        page = Path(result['index']).read_text(encoding='utf-8')
        self.assertEqual((1, 2, 1), (result['editableAssets'], result['listedAssets'], result['graphs']))
        self.assertNotIn('/Game/Noise', page)
        self.assertNotIn('Referenced by', page)
        self.assertIn('parent</code>', page)
        self.assertIn('defaults / Mesh', page)
        self.assertIn('Stub_C [STUB]', page)
        self.assertFalse((self.root / 'out/blueprint-001').exists())
        self.assertGreater(page.index('Other assets (2)'), page.index('graph-000.html'))
        svg = (self.root / 'out/blueprint-000/graph-000.svg').read_text(encoding='utf-8')
        self.assertIn('in:Asset', svg)
        self.assertIn('[STUB]', svg)
        self.assertIn('href="../index.html#asset-001"', svg)
        self.assertEqual(original, (self.root / 'editable.json').read_bytes())

    def test_all_statuses_placeholder_style_and_node_review(self):
        before = fixture()
        after = copy.deepcopy(before)
        after['graphs'][0]['body']['nodes'][0]['defaults']['in:Limit'] = '250'
        for name, value in [('old.json', before), ('new.json', after),
                            ('added.json', fixture('/Game/Added')), ('removed.json', fixture('/Game/Removed')),
                            ('plain.json', fixture('/Game/Plain'))]:
            self.save(name, value)
        manifest = self.save('manifest.json', dict(title='<unsafe>', demo=True, assets=[
            dict(source='new.json', before='old.json', status='changed', reviews={'Main': {'changed': ['n']}}),
            dict(source='added.json', status='added'),
            dict(before='removed.json', status='removed', placeholder=True), 'plain.json']))
        originals = {p: p.read_bytes() for p in self.root.glob('*.json')}
        output = self.root / 'preview'
        result = generate(manifest, output)
        self.assertEqual(result['mode'], 'review')
        self.assertEqual(result['counts'], dict(unchanged=1, added=1, changed=1, removed=1))
        page = (output / 'index.html').read_text(encoding='utf-8')
        self.assertIn('&lt;unsafe&gt;', page)
        self.assertIn('DEMO — fictional changes', page)
        self.assertIn('<nav aria-label="Blueprint review status">', page)
        self.assertIn('<span class="badge unchanged">Unchanged</span>', page)
        self.assertIn('blueprint removed placeholder', page)
        for graph_status in ('added', 'changed', 'removed'):
            self.assertIn(f'class="badge {graph_status} graph-status">{graph_status.title()}</span>', page)
        self.assertNotIn('badge unchanged graph-status', page)
        self.assertIn('font-style:italic;color:#a3332b', page)
        for href in re.findall(r'href="([^"]+)"', page):
            self.assertTrue((output / href).is_file(), href)
        changed = (output / 'blueprint-000/graph-000.svg').read_text(encoding='utf-8')
        self.assertIn('Previously: Input: Limit = &quot;200&quot;', changed)
        self.assertIn('data-review-status="changed"', changed)
        for number, status in ((1, 'added'), (2, 'removed')):
            svg = (output / f'blueprint-{number:03d}/graph-000.svg').read_text(encoding='utf-8')
            self.assertIn(f'data-review-status="{status}"', svg)
            viewer = (output / f'blueprint-{number:03d}/graph-000.html').read_text(encoding='utf-8')
            self.assertNotIn('id="show-changes"', viewer)
            self.assertNotIn('id="final-', viewer)
        self.assertEqual(originals, {p:p.read_bytes() for p in originals})
        with self.assertRaisesRegex(ValueError, 'new or empty'):
            generate(manifest, output)

    def test_browse_mode_omits_review_ui_but_keeps_placeholder_warning(self):
        source = fixture()
        source['graphs'][0]['body']['nodes'].append(dict(id='sink', kind='call', operation='Consume'))
        source['graphs'][0]['body']['links'] = [{'from':'n.result', 'to':'sink.value'}]
        self.save('plain.json', source)
        self.save('stub.json', fixture('/Game/Stub'))
        manifest = self.save('manifest.json', {'assets': ['plain.json', {'source':'stub.json', 'placeholder':True, 'status':'unchanged'}]})
        result = generate(manifest, self.root / 'out')
        self.assertEqual(result['mode'], 'browse')
        page = (self.root / 'out/index.html').read_text(encoding='utf-8')
        self.assertNotIn('<nav', page)
        self.assertNotIn('<span class="badge', page)
        self.assertNotIn('Review badges', page)
        self.assertNotIn('automatic or complete diff', page)
        self.assertIn('Editor dependency placeholder', page)
        self.assertIn('blueprint unchanged placeholder', page)
        self.assertIn('2 nodes · 1 links', page)
        self.assertIn('1 nodes · 0 links', page)
        viewer = (self.root / 'out/blueprint-000/graph-000.html').read_text(encoding='utf-8')
        self.assertNotIn('id="show-changes"', viewer)
        self.assertNotIn('id="final-', viewer)

    def test_empty_review_marks_do_not_offer_changes_switch(self):
        self.save('source.json', fixture())
        manifest = self.save('empty-review.json', {'assets': [dict(source='source.json',
            before='source.json', status='changed', reviews={'Main': {'changed': [], 'added': [], 'removed': []}})]})
        generate(manifest, self.root / 'empty-review')
        page = (self.root / 'empty-review/blueprint-000/graph-000.html').read_text(encoding='utf-8')
        self.assertNotIn('id="show-changes"', page)
        self.assertNotIn('REVIEW ·', page)

    def test_changed_asset_includes_deleted_and_added_graphs(self):
        old = fixture()
        new = fixture()
        new['graphs'][0]['name'] = 'NewGraph'
        self.save('old.json', old); self.save('new.json', new)
        manifest = self.save('manifest.json', {'assets': [{'source':'new.json','before':'old.json','status':'changed'}]})
        result = generate(manifest, self.root / 'out')
        self.assertEqual(result['graphs'], 2)
        page = (self.root / 'out/index.html').read_text(encoding='utf-8')
        self.assertIn('NewGraph</a> <span class="badge added graph-status">Added</span>', page)
        self.assertIn('Main</a> <span class="badge removed graph-status">Removed</span>', page)
        self.assertIn('title="Before full graph">1 nodes · 0 links', page)
        self.assertIn('title="Current full graph">1 nodes · 0 links', page)
        for number in (0, 1):
            viewer = (self.root / f'out/blueprint-000/graph-{number:03d}.html').read_text(encoding='utf-8')
            self.assertNotIn('id="show-changes"', viewer)
            self.assertNotIn('id="final-', viewer)

    def test_before_link_is_graph_scoped_not_asset_scoped(self):
        old = fixture()
        untouched = copy.deepcopy(old['graphs'][0])
        untouched['name'] = 'UserConstructionScript'
        old['graphs'].append(untouched)
        old['pinTemplates'] = {'old': {'PinType.PinCategory': '"int"'}}
        untouched['body']['nodes'][0]['pins'] = {'in:Limit': {'type': 'old'}}
        new = copy.deepcopy(old)
        new['graphs'][0]['body']['nodes'][0]['defaults']['in:Limit'] = '300'
        new['pinTemplates']['new'] = new['pinTemplates'].pop('old')
        new['graphs'][1]['body']['nodes'][0]['pins']['in:Limit']['type'] = 'new'
        self.save('old.json', old)
        self.save('new.json', new)
        manifest = self.save('manifest.json', {'assets': [dict(source='new.json', before='old.json',
            status='changed', reviews={'Main': {'changed': ['n']}})]})
        generate(manifest, self.root / 'out')
        page = (self.root / 'out/index.html').read_text(encoding='utf-8')
        self.assertIn('href="blueprint-000/graph-000-before.html"', page)
        self.assertIn('Main</a> <span class="badge changed graph-status">Changed</span>', page)
        self.assertNotIn('UserConstructionScript</a> <span class="badge', page)
        self.assertNotIn('href="blueprint-000/graph-001-before.html"', page)
        viewer = (self.root / 'out/blueprint-000/graph-001.html').read_text(encoding='utf-8')
        self.assertNotIn('id="show-changes"', viewer)

    def test_invalid_identity_and_review_fail_before_creating_output(self):
        self.save('source.json', fixture()); self.save('other.json', fixture('/Game/Other'))
        for assets in ([{'before':'source.json','status':'changed'}],
                       ['source.json','source.json'],
                       [{'source':'source.json','before':'other.json','status':'changed'}],
                       [{'source':'source.json','before':'source.json','status':'changed','reviews':{'Main':{'changed':['typo']}}}]):
            manifest = self.save('manifest.json', {'assets':assets})
            with self.assertRaises(ValueError):
                generate(manifest, self.root / 'out')
            self.assertFalse((self.root / 'out').exists())

    def test_widget_batch_and_empty_tree_have_defined_outcomes(self):
        widget = widget_fixture()
        self.save('widget.json', widget)
        self.save('widget.resources/manifest.json', dict(schema=1, asset=widget['asset'], textures={}, unsupported=[]))
        empty = copy.deepcopy(widget)
        empty['asset'] = '/Game/Empty'
        empty['widget']['objects'][0]['properties']['RootWidget'] = 'None'
        self.save('empty.json', empty)
        manifest = self.save('manifest.json', {'assets':['widget.json', 'empty.json']})
        generate(manifest, self.root / 'out')
        self.assertTrue((self.root / 'out/blueprint-000/source.widget.html').is_file())
        self.assertIn('no local widget layout', (self.root / 'out/index.html').read_text())


if __name__ == '__main__':
    unittest.main()
