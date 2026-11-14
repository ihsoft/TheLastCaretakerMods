"""Check import-specific request decisions and coordinate conversion.

A wrong unit/basis conversion can agree with the importer's own readback
expectations. A tiny independent example checks that boundary. Destination and
scene rejection cases cover requests that could otherwise overwrite or lose data.
Saved-asset validation belongs to the import operation itself.
"""
import json
from pathlib import Path
import struct
import tempfile
import unittest

from glb_request import prepare

REPO=Path(__file__).resolve().parents[2]


def fixture(path, mutate=None):
    binary=struct.pack('<9f',0,0,0,1,0,0,0,1,0)
    doc={'asset':{'version':'2.0'},'buffers':[{'byteLength':len(binary)}],
         'bufferViews':[{'buffer':0,'byteLength':len(binary)}],
         'accessors':[{'bufferView':0,'componentType':5126,'type':'VEC3','count':3}],
         'meshes':[{'primitives':[{'attributes':{'POSITION':0}}]}],
         'nodes':[{'name':'root','translation':[1,2,3],'children':[1]},
                  {'name':'mesh','mesh':0}],
         'scenes':[{'nodes':[0]}],'scene':0}
    if mutate:
        mutate(doc)
    encoded=json.dumps(doc).encode()
    encoded+=b' '*(-len(encoded)%4)
    binary+=b'\0'*(-len(binary)%4)
    path.write_bytes(struct.pack('<4sII',b'glTF',2,28+len(encoded)+len(binary))
                     +struct.pack('<II',len(encoded),0x4E4F534A)+encoded
                     +struct.pack('<II',len(binary),0x004E4942)+binary)


class ImportTests(unittest.TestCase):
    def setUp(self):
        parent=REPO/'Tmp/tests/UnrealGlbImport'
        parent.mkdir(parents=True,exist_ok=True)
        self.root=Path(tempfile.mkdtemp(prefix='test-',dir=parent))
        self.project=self.root/'Project/Fixture.uproject'
        self.project.parent.mkdir()
        self.project.write_text('{"FileVersion":3}')
        self.source=self.root/'assembly.glb'
        fixture(self.source)

    def prepared(self):
        return prepare(self.project,self.source,'/Game/Assembly',self.root/'output')

    def test_units_and_axes(self):
        data=self.prepared()
        nodes={n['name']:n for n in data['nodes']}
        self.assertEqual([row[3] for row in nodes['root']['localMatrix'][:3]],[100,300,200])

    def test_reject_unsafe_destinations_before_mutation(self):
        for destination in ['/Engine/Assembly','/Game/../Other','/Game']:
            with self.subTest(destination=destination),self.assertRaises(ValueError):
                prepare(self.project,self.source,destination,self.root/'output')
        (self.project.parent/'Content/Assembly').mkdir(parents=True)
        with self.assertRaisesRegex(ValueError,'Destination already exists'):
            self.prepared()
        self.assertFalse((self.root/'output').exists())

    def test_reject_lossy_or_ambiguous_scene(self):
        cases=[lambda d:d.update(animations=[{}]),
               lambda d:d['nodes'][1].pop('name'),
               lambda d:d['nodes'][1].update(name='ROOT'),
               lambda d:d['nodes'][1].update(matrix=[1,0,0,0,0.5,1,0,0,0,0,1,0,0,0,0,1]),
               lambda d:(d['nodes'][0].update(scale=[2,1,1]),
                         d['nodes'][1].update(rotation=[0,0,0.3826834324,0.9238795325]))]
        for change in cases:
            fixture(self.source,change)
            with self.assertRaises(ValueError):
                self.prepared()
            self.assertFalse((self.root/'output').exists())

if __name__=='__main__':
    unittest.main()
