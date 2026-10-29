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
        self.assertIn('font-style:italic;color:#a3332b', page)
        for href in re.findall(r'href="([^"]+)"', page):
            self.assertTrue((output / href).is_file(), href)
        changed = (output / 'blueprint-000/graph-000.svg').read_text(encoding='utf-8')
        self.assertIn('Previously: Input: Limit = &quot;200&quot;', changed)
        self.assertIn('data-review-status="changed"', changed)
        for number, status in ((1, 'added'), (2, 'removed')):
            svg = (output / f'blueprint-{number:03d}/graph-000.svg').read_text(encoding='utf-8')
            self.assertIn(f'data-review-status="{status}"', svg)
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

    def test_changed_asset_includes_deleted_and_added_graphs(self):
        old = fixture()
        new = fixture()
        new['graphs'][0]['name'] = 'NewGraph'
        self.save('old.json', old); self.save('new.json', new)
        manifest = self.save('manifest.json', {'assets': [{'source':'new.json','before':'old.json','status':'changed'}]})
        result = generate(manifest, self.root / 'out')
        self.assertEqual(result['graphs'], 2)
        page = (self.root / 'out/index.html').read_text(encoding='utf-8')
        self.assertIn('NewGraph — Added', page)
        self.assertIn('Main — Removed (Before)', page)
        self.assertIn('title="Before full graph">1 nodes · 0 links', page)
        self.assertIn('title="Current full graph">1 nodes · 0 links', page)

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
