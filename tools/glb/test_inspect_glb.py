"""Small deterministic GLB reader regressions; no game data or Blender."""
import json
from pathlib import Path
import struct
import tempfile
import unittest
from inspect_glb import inspect


def fixture():
    binary = struct.pack('<9f3H', 0,0,0, 1,0,0, 0,1,0, 0,1,2)
    return dict(asset={'version':'2.0'}, buffers=[{'byteLength':len(binary)}],
                bufferViews=[{'buffer':0, 'byteLength':36}, {'buffer':0, 'byteOffset':36, 'byteLength':6}],
                accessors=[{'bufferView':0,'componentType':5126,'type':'VEC3','count':3},
                           {'bufferView':1,'componentType':5123,'type':'SCALAR','count':3}],
                meshes=[{'primitives':[{'attributes':{'POSITION':0},'indices':1}]}],
                nodes=[{'name':'root','translation':[2,3,4],'children':[1]}, {'name':'mesh','mesh':0}],
                scenes=[{'nodes':[0]}], scene=0), binary


class Tests(unittest.TestCase):
    def run_case(self, change=None, invalid=False):
        doc, binary = fixture()
        if change:
            change(doc)
        encoded = json.dumps(doc).encode()
        encoded += b' ' * (-len(encoded) % 4)
        binary += b'\0' * (-len(binary) % 4)
        data = struct.pack('<4sII', b'glTF', 2, 28+len(encoded)+len(binary))
        data += struct.pack('<II', len(encoded), 0x4E4F534A)+encoded
        data += struct.pack('<II', len(binary), 0x004E4942)+binary
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/'fixture.glb'
            path.write_bytes(data)
            if invalid:
                with self.assertRaises((ValueError, IndexError)):
                    inspect(path)
                return
            return inspect(path)

    def test_geometry_and_parent_transform(self):
        report = self.run_case()
        self.assertEqual(report['counts']['instantiatedTriangles'], 1)
        self.assertEqual(report['worldBoundsMeters'], {'min':[2,3,4], 'max':[3,4,4]})

    def test_matrix_column_order(self):
        def change(d):
            d['nodes'][0].pop('translation')
            d['nodes'][0]['matrix'] = [1,0,0,0,0,1,0,0,0,0,1,0,2,3,4,1]
        self.assertEqual(self.run_case(change)['worldBoundsMeters']['min'], [2,3,4])

    def test_duplicate_name(self):
        self.run_case(lambda d:d['nodes'][1].update(name='root'), True)

    def test_cycle(self):
        self.run_case(lambda d:d['nodes'][1].update(children=[0]), True)

    def test_accessor_overrun(self):
        self.run_case(lambda d:d['accessors'][0].update(count=100), True)

    def test_external_buffer(self):
        self.run_case(lambda d:d['buffers'][0].update(uri='remote.bin'), True)

    def test_required_extension(self):
        self.run_case(lambda d:d.update(extensionsRequired=['unknown']), True)

    def test_sparse(self):
        self.run_case(lambda d:d['accessors'][0].update(sparse={}), True)

    def test_bad_index_type(self):
        self.run_case(lambda d:d['accessors'][1].update(componentType=5126), True)

    def test_skin(self):
        self.run_case(lambda d:d.update(skins=[{}]), True)


if __name__ == '__main__':
    unittest.main()
