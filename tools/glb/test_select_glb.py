"""Selection risks: axis/side boundaries and attribute remapping across indexed faces."""
from pathlib import Path
import tempfile
import unittest

import numpy as np

from inspect_glb import read_accessor, read_glb
from select_glb import select, write_glb


def fixture(path):
    # Four triangles: positive, negative, touching plane, coplanar. Unused vertex first.
    positions = np.array([[99, 99, 99], [2, 0, 0], [2, 1, 0], [2, 0, 1],
                          [0, 0, 0], [0, 1, 0], [0, 0, 1],
                          [1, 0, 0], [1, 1, 0], [1, 0, 1]], dtype='<f4')
    color = np.arange(40, dtype='u1').reshape(10, 4)
    # Interleaving and normalized attributes must survive vertex compaction verbatim.
    binary = b''.join(p.tobytes() + c.tobytes() for p, c in zip(positions, color))
    triangles = np.array([[1, 2, 3], [4, 5, 6], [7, 2, 3], [7, 8, 9]], dtype='<u2')
    binary += triangles.tobytes()
    doc = {'asset': {'version': '2.0'}, 'buffers': [{'byteLength': len(binary)}],
           'bufferViews': [{'buffer': 0, 'byteLength': 160, 'byteStride': 16},
                           {'buffer': 0, 'byteOffset': 160, 'byteLength': triangles.nbytes}],
           'accessors': [{'bufferView': 0, 'componentType': 5126, 'type': 'VEC3', 'count': 10},
                         {'bufferView': 0, 'byteOffset': 12, 'componentType': 5121,
                          'normalized': True, 'type': 'VEC4', 'count': 10},
                         {'bufferView': 1, 'componentType': 5123, 'type': 'SCALAR', 'count': 12}],
           'meshes': [{'primitives': [{'attributes': {'POSITION': 0, 'COLOR_0': 1}, 'indices': 2}]}],
           'nodes': [{'name': 'part', 'mesh': 0, 'translation': [10, 20, 30]}],
           'scenes': [{'nodes': [0]}], 'scene': 0}
    write_glb(path, doc, binary)


class SelectionTests(unittest.TestCase):
    def test_local_plane_boundary_and_attribute_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'input.glb'
            fixture(source)
            for side, first_corners in [('positive', [1, 7, 7]), ('negative', [4])]:
                output = root / (side + '.glb')
                select(source, output, 'x', side, 1, 'Surface')
                doc, data = read_glb(output)
                primitive = doc['meshes'][0]['primitives'][0]
                indices = read_accessor(doc, data, primitive['indices']).reshape(-1, 3)
                colors = read_accessor(doc, data, primitive['attributes']['COLOR_0'])[indices]
                self.assertEqual(colors[:, 0, 0].tolist(), [index * 4 for index in first_corners])
                self.assertTrue(doc['accessors'][primitive['attributes']['COLOR_0']]['normalized'])
                self.assertEqual(doc['nodes'][0]['translation'], [10, 20, 30])
                self.assertEqual(doc['materials'][primitive['material']]['name'], 'Surface')
                positions = read_accessor(doc, data, primitive['attributes']['POSITION'])
                self.assertLess(float(positions.max()), 99)  # Unused vertex cannot inflate bounds.

    def test_other_axis_and_rejected_selections_leave_no_output(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source, output = root / 'input.glb', root / 'result.glb'
            fixture(source)
            select(source, output, 'z', 'positive')
            before = output.read_bytes()
            with self.assertRaisesRegex(ValueError, 'fresh'):
                select(source, output, 'x', 'negative')
            self.assertEqual(output.read_bytes(), before)
            for axis, side, plane, message in [('z', 'negative', 0, 'empty'),
                                               ('y', 'positive', .5, 'crosses')]:
                rejected = root / (axis + '.glb')
                with self.assertRaisesRegex(ValueError, message):
                    select(source, rejected, axis, side, plane)
                self.assertFalse(rejected.exists())


if __name__ == '__main__':
    unittest.main()
