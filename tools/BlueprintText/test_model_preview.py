"""Rigid model transforms, reflection shading and project-source isolation."""
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
import blueprint_workspace as bw
from render_model import Model, render_model


def triangle_glb(path, *, scale=(1, 1, 1), normals=True, parent=None):
    # Slanted outward face: the expected reflected normal can be derived without
    # the renderer's matrix code. Indices keep the original CCW winding.
    points = [(0, 0, 0), (1, 0, 1), (0, 1, 0)]
    blob = struct.pack('<9f', *(x for p in points for x in p))
    accessors = [dict(bufferView=0, componentType=5126, count=3, type='VEC3')]
    views = [dict(buffer=0, byteOffset=0, byteLength=len(blob))]
    attributes = dict(POSITION=0)
    if normals:
        v = np.array([-1., 0., 1.]) / np.sqrt(2)
        views.append(dict(buffer=0, byteOffset=len(blob), byteLength=36))
        blob += struct.pack('<9f', *(list(v) * 3))
        accessors.append(dict(bufferView=1, componentType=5126, count=3, type='VEC3'))
        attributes['NORMAL'] = 1
    nodes = [dict(mesh=0, scale=scale)]
    if parent:
        nodes.append(dict(children=[0], **parent))
    doc = dict(asset=dict(version='2.0'), buffers=[dict(byteLength=len(blob))],
               bufferViews=views, accessors=accessors, nodes=nodes,
               meshes=[dict(primitives=[dict(attributes=attributes)])],
               scenes=[dict(nodes=[len(nodes)-1])], scene=0)
    encoded = json.dumps(doc).encode()
    encoded += b' ' * (-len(encoded) % 4)
    path.write_bytes(struct.pack('<4sII', b'glTF', 2, 28+len(encoded)+len(blob)) +
                     struct.pack('<I4s', len(encoded), b'JSON') + encoded +
                     struct.pack('<I4s', len(blob), b'BIN\0') + blob)


class ModelTests(unittest.TestCase):
    def setUp(self):
        bw.OWNER.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=bw.OWNER)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.glb = self.root / 'model.glb'

    def test_reflection_and_nonuniform_scale_preserve_outward_shading(self):
        for normals in (True, False):
            for scale in ((-2, 3, 4), (-2, -3, 4), (2, 3, 4)):
                with self.subTest(normals=normals, scale=scale):
                    triangle_glb(self.glb, scale=scale, normals=normals)
                    v = Model(self.glb).geometry()
                    expected = np.array([-1/scale[0], 0, 1/scale[2]])
                    expected /= np.linalg.norm(expected)
                    np.testing.assert_allclose(v[:, 3:], np.tile(expected, (3, 1)), atol=1e-6)
                    geometric = np.cross(v[1, :3]-v[0, :3], v[2, :3]-v[0, :3])
                    self.assertGreater(np.dot(geometric, expected), 0)

    def test_parent_reflection_and_translation_preserve_positions(self):
        triangle_glb(self.glb, parent=dict(scale=[-1, 1, 1], translation=[5, 6, 7]))
        v = Model(self.glb).geometry()
        np.testing.assert_allclose(v[:, :3], [[5, 6, 7], [5, 7, 7], [4, 6, 8]])

    def test_self_contained_english_viewer_preserves_input(self):
        triangle_glb(self.glb)
        original = self.glb.read_bytes()
        output = self.root / 'index.html'
        report = render_model(self.glb, output, '/Game/Model')
        self.assertEqual(report['meshInstances'], 1)
        self.assertEqual(self.glb.read_bytes(), original)
        page = output.read_text(encoding='utf-8')
        self.assertIn('lang="en"', page)
        self.assertIn('Saved Unreal project model', page)
        self.assertNotIn('__GEOMETRY__', page)
        self.assertNotIn('fetch(', page)

    def workspace(self):
        project = self.root / 'Project/Test.uproject'
        bw.write(project, {})
        package = project.parent / 'Content/Model.uasset'
        package.parent.mkdir()
        package.write_bytes(b'saved project asset')
        ws = bw.Workspace(self.root / 'workspace', self.root / 'Engine')
        ws.project = project
        return ws, package

    def test_saved_project_export_needs_no_graph_source_and_preserves_inputs(self):
        ws, package = self.workspace()
        pending = ws.root / 'blueprints/pending.json'
        bw.write(pending, dict(unsaved='proposal must not be applied'))
        before = pending.read_bytes()
        def export(args, name):
            out = ws.run / 'model-preview'
            self.assertEqual(bw.read(out / 'request.json')['asset'], '/Game/Model')
            triangle_glb(out / 'model.glb')
            bw.write(out / 'export.json', dict(status='exported', asset='/Game/Model', meshInstances=1))
        with patch.object(ws, 'prepare'), patch.object(ws, 'command', side_effect=export), patch.object(bw, 'closed'):
            result = ws.model_preview(['/Game/Model'])
        self.assertEqual(result['source'], 'saved-project')
        self.assertTrue(result['previewUrl'].endswith('index.html'))
        self.assertEqual(package.read_bytes(), b'saved project asset')
        self.assertEqual(pending.read_bytes(), before)
        self.assertFalse(ws.manifest.exists())

    def test_invalid_selection_rejected_before_preparation(self):
        ws, _ = self.workspace()
        with patch.object(ws, 'prepare') as prepare:
            for selection in ([], ['/Game/A', '/Game/B'], ['/Game/../Model'], ['/Game/Missing']):
                with self.subTest(selection=selection), self.assertRaises(ValueError):
                    ws.model_preview(selection)
            prepare.assert_not_called()

    def test_empty_export_is_not_a_successful_preview(self):
        ws, _ = self.workspace()
        def export(args, name):
            out = ws.run / 'model-preview'
            bw.write(out / 'export.json', dict(status='exported', asset='/Game/Model', meshInstances=1))
            (out / 'model.glb').write_bytes(b'')
        with patch.object(ws, 'prepare'), patch.object(ws, 'command', side_effect=export), patch.object(bw, 'closed'):
            with self.assertRaisesRegex(ValueError, 'GLB header'):
                ws.model_preview(['/Game/Model'])

    def test_export_failure_exposes_reason_without_publishing_success(self):
        ws, package = self.workspace()
        def export(args, name):
            bw.write(ws.run / 'model-preview/export-error.json', dict(error='Unsupported visible model component: Cloth'))
            raise RuntimeError('model-export failed')
        with patch.object(ws, 'prepare'), patch.object(ws, 'command', side_effect=export), patch.object(bw, 'closed'):
            with self.assertRaisesRegex(ValueError, 'Unsupported visible model component: Cloth'):
                ws.model_preview(['/Game/Model'])
        self.assertFalse((ws.run / 'model-preview/index.html').exists())
        self.assertEqual(package.read_bytes(), b'saved project asset')


if __name__ == '__main__':
    unittest.main()
