"""Synthetic widget preview contract tests; no game assets or Unreal required."""
import base64
import copy
import json
from pathlib import Path
import tempfile
import unittest

from render_widget import load_resources, parse, render, resource_path, widget_tree


def fixture():
    asset = '/Game/Test/HUD'
    def ref(key, cls='CanvasPanel'):
        return f"/Script/UMG.{cls}'{asset}.{key}'"
    def obj(key, cls, props):
        return dict(id=key, name=key.rsplit('.', 1)[-1], **{'class': '/Script/UMG.' + cls}, properties=props)
    return dict(schema=2, asset=asset, widget=dict(tree='HUD:WidgetTree', objects=[
        obj('HUD:WidgetTree', 'WidgetTree', {'RootWidget': ref('HUD:WidgetTree.Root')}),
        obj('HUD:WidgetTree.Root', 'CanvasPanel', {'Slots': '(' + ref('HUD:WidgetTree.Root.Slot') + ',)'}),
        obj('HUD:WidgetTree.Root.Slot', 'CanvasPanelSlot', {'Content': ref('HUD:WidgetTree.Icon')}),
        obj('HUD:WidgetTree.Icon', 'Image', {
            'Visibility': 'Collapsed', 'Brush': "(ResourceObject=/Script/Engine.Texture2D'/Game/Test/Picture.Picture',ImageSize=(X=32,Y=32))"}),
    ]))


class WidgetPreviewTests(unittest.TestCase):
    def test_parse_structs_localized_text_and_unknown_literals(self):
        self.assertEqual(parse('(A=(X=1,Y=2),B="hello, world")'), {'A': {'X': 1, 'Y': 2}, 'B': 'hello, world'})
        self.assertEqual(parse('NSLOCTEXT("Namespace","Key","Readable text")'), 'Readable text')
        self.assertEqual(parse('NaN'), 'NaN')
        self.assertEqual(resource_path("Texture2D'/Game/Test/Picture.Picture'"), '/Game/Test/Picture.Picture')

    def test_tree_order_identity_hidden_child_and_no_input_mutation(self):
        doc = fixture(); before = copy.deepcopy(doc)
        root, widgets = widget_tree(doc)
        self.assertEqual(root, 'HUD:WidgetTree.Root')
        self.assertEqual(len(widgets), 2)
        self.assertEqual(widgets[1]['parent'], root)
        self.assertEqual(widgets[1]['props']['Visibility'], 'Collapsed')
        self.assertEqual(doc, before)

    def test_duplicate_cycle_and_external_reference_are_rejected(self):
        doc = fixture(); doc['widget']['objects'].append(copy.deepcopy(doc['widget']['objects'][0]))
        with self.assertRaisesRegex(ValueError, 'Duplicate'): widget_tree(doc)
        doc = fixture(); slot = doc['widget']['objects'][2]['properties']
        slot['Content'] = "CanvasPanel'/Game/Test/HUD.HUD:WidgetTree.Root'"
        with self.assertRaisesRegex(ValueError, 'Cycle'): widget_tree(doc)
        slot['Content'] = "CanvasPanel'/Game/Other/HUD.HUD:WidgetTree.Root'"
        with self.assertRaisesRegex(ValueError, 'Unresolved'): widget_tree(doc)

    def test_bundle_validation_and_standalone_render(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary); source = folder / 'HUD.json'
            doc = fixture(); doc['widget']['objects'][-1]['name'] = '</script><script>unsafe</script>'
            source.write_text(json.dumps(doc), encoding='utf-8')
            bundle = folder / 'HUD.resources'; bundle.mkdir()
            # Synthetic 1x1 PNG, deliberately unrelated to any mod/source image directory.
            png = base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aX1sAAAAASUVORK5CYII=')
            (bundle / 'pixel.png').write_bytes(png)
            manifest = {'schema': 1, 'asset': doc['asset'], 'textures': {'/Game/Test/Picture.Picture':
                        {'file': 'pixel.png', 'width': 1, 'height': 1}}, 'unsupported': []}
            path = bundle / 'manifest.json'
            def save(): path.write_text(json.dumps(manifest), encoding='utf-8')
            save(); result = render(source, folder / 'preview.html')
            self.assertEqual((result['widgets'], result['textures']), (2, 1))
            html = (folder / 'preview.html').read_text(encoding='utf-8')
            self.assertIn('data:image/png;base64,', html)
            self.assertNotIn('</script><script>unsafe', html)
            self.assertEqual(len(html.split('<script>')), 2)
            manifest['asset'] = '/Game/Other'; save()
            with self.assertRaisesRegex(ValueError, 'does not match'): load_resources(path, doc['asset'])
            manifest['asset'] = doc['asset']
            item = manifest['textures']['/Game/Test/Picture.Picture']
            item['file'] = '../outside.png'; save()
            with self.assertRaisesRegex(ValueError, 'escapes'): load_resources(path, doc['asset'])
            item['file'] = 'pixel.png'; item['width'] = 2; save()
            with self.assertRaisesRegex(ValueError, 'dimensions'): load_resources(path, doc['asset'])
            item['width'] = 1; save(); (bundle / 'pixel.png').unlink()
            with self.assertRaises(OSError): load_resources(path, doc['asset'])

    def test_missing_images_reported_and_never_guessed(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary); source = folder / 'HUD.json'; doc = fixture()
            source.write_text(json.dumps(doc), encoding='utf-8')
            manifest = folder / 'manifest.json'
            manifest.write_text(json.dumps({'schema': 1, 'asset': doc['asset'], 'textures': {},
                                           'unsupported': [{'resource': '/Game/Test/Picture.Picture', 'reason': 'No source'}]}), encoding='utf-8')
            result = render(source, folder / 'preview.html', manifest)
            self.assertEqual(result['textures'], 0)
            self.assertTrue(any('no preview image' in warning for warning in result['warnings']))
            self.assertTrue(any('No source' in warning for warning in result['warnings']))


if __name__ == '__main__':
    unittest.main()
