"""Select whole triangles on one side of a mesh-local glTF plane; no clipping."""
import argparse
import copy
import json
from pathlib import Path
import struct

import numpy as np

from inspect_glb import inspect, read_accessor, read_glb, require


def write_glb(path, doc, binary):
    encoded = json.dumps(doc, separators=(',', ':')).encode('utf-8')
    encoded += b' ' * (-len(encoded) % 4)
    binary += b'\0' * (-len(binary) % 4)
    with path.open('xb') as stream:
        stream.write(struct.pack('<4sII', b'glTF', 2, 28 + len(encoded) + len(binary)))
        stream.write(struct.pack('<II', len(encoded), 0x4E4F534A) + encoded)
        stream.write(struct.pack('<II', len(binary), 0x004E4942) + binary)


def select(source, output, axis, side, plane=0.0, material_slot=None):
    source, output = Path(source).resolve(), Path(output).resolve()
    require(not output.exists(), 'Use a fresh output path')
    require(output.suffix.lower() == '.glb', 'Output must be a GLB file')
    require(axis in ('x', 'y', 'z') and side in ('positive', 'negative'), 'Invalid axis/side')
    require(np.isfinite(plane), 'Plane coordinate must be finite')
    require(material_slot is None or material_slot.strip(), 'Material slot must not be empty')
    inspect(source)
    original, data = read_glb(source)
    require(len(original['meshes']) == 1, 'Select from a single mesh; isolate assemblies before selection')
    require(not original.get('animations'), 'Animated input needs a dedicated authoring operation')
    # Optional extensions may carry vertex/index references. Do not silently invalidate them.
    require(not original.get('extensionsUsed'), 'Extended GLB needs a dedicated authoring operation')
    doc = copy.deepcopy(original)
    binary = bytearray(data[:doc['buffers'][0]['byteLength']])

    def append_accessor(values, template):
        binary.extend(b'\0' * (-len(binary) % 4))
        raw = values.tobytes()
        view = len(doc['bufferViews'])
        doc['bufferViews'].append({'buffer': 0, 'byteOffset': len(binary), 'byteLength': len(raw)})
        binary.extend(raw)
        accessor = copy.deepcopy(template)
        accessor.update(bufferView=view, byteOffset=0, count=len(values))
        for field, reducer in [('min', np.min), ('max', np.max)]:
            if field in accessor:
                accessor[field] = reducer(values, axis=0).tolist()
        index = len(doc['accessors'])
        doc['accessors'].append(accessor)
        return index

    primitives, expected = [], []
    total = 0
    for primitive in original['meshes'][0]['primitives']:
        attrs = {name: read_accessor(original, data, index) for name, index in primitive['attributes'].items()}
        indices = (read_accessor(original, data, primitive['indices']).reshape(-1)
                   if 'indices' in primitive else np.arange(len(attrs['POSITION'])))
        triangles = indices.reshape(-1, 3)
        distance = attrs['POSITION'][triangles, 'xyz'.index(axis)].astype(float) - plane
        low, high = distance.min(axis=1), distance.max(axis=1)
        require(not np.any((low < 0) & (high > 0)), 'Triangle crosses the selection plane; clipping is unsupported')
        # Boundary vertices follow their face; a wholly coplanar face belongs to positive.
        keep = low >= 0 if side == 'positive' else (high <= 0) & (low < 0)
        selected = triangles[keep]
        total += len(triangles)
        if not len(selected):
            continue
        vertices, remapped = np.unique(selected, return_inverse=True)
        result = copy.deepcopy(primitive)
        result['attributes'] = {
            name: append_accessor(values[vertices], original['accessors'][primitive['attributes'][name]])
            for name, values in attrs.items()}
        result['indices'] = append_accessor(remapped.astype('<u4').reshape(-1, 1),
                                           {'componentType': 5125, 'type': 'SCALAR'})
        primitives.append(result)
        expected.append({name: values[selected] for name, values in attrs.items()})
    require(primitives, 'Selection is empty')
    doc['meshes'][0]['primitives'] = primitives
    if material_slot is not None:
        materials = {p.get('material') for p in primitives}
        require(len(materials) == 1, 'Material slot rename requires exactly one selected slot')
        index = materials.pop()
        if index is None:
            index = len(doc.setdefault('materials', []))
            doc['materials'].append({})
            for primitive in primitives:
                primitive['material'] = index
        doc['materials'][index]['name'] = material_slot
    doc['buffers'][0]['byteLength'] = len(binary)
    output.parent.mkdir(parents=True, exist_ok=True)
    write_glb(output, doc, binary)
    # Validate the produced file and compare actual corner attributes to this input selection.
    inspect(output)
    saved, saved_data = read_glb(output)
    for primitive, wanted in zip(saved['meshes'][0]['primitives'], expected):
        indices = read_accessor(saved, saved_data, primitive['indices']).reshape(-1, 3)
        for name, values in wanted.items():
            actual = read_accessor(saved, saved_data, primitive['attributes'][name])[indices]
            require(actual.dtype == values.dtype and actual.tobytes() == values.tobytes(),
                    'Selection changed corner attributes: ' + name)
    return {'status': 'passed', 'source': str(source), 'output': str(output),
            'coordinateSpace': 'mesh-local glTF metres', 'axis': axis, 'plane': plane, 'side': side,
            'sourceTriangles': total, 'selectedTriangles': sum(len(e['POSITION']) for e in expected)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--axis', choices=['x', 'y', 'z'], required=True)
    parser.add_argument('--side', choices=['positive', 'negative'], required=True)
    parser.add_argument('--plane', type=float, default=0.0)
    parser.add_argument('--material-slot')
    args = parser.parse_args()
    print(json.dumps(select(args.source, args.output, args.axis, args.side, args.plane, args.material_slot)))
