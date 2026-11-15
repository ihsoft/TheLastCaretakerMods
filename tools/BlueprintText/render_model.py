"""Render the native export's rigid triangle geometry as a self-contained preview.

This is an internal consumer of model_export.py, not a general glTF importer.
"""
import base64
import html
import json
import struct
from pathlib import Path

import numpy as np


class Model:
    def __init__(self, path):
        raw = Path(path).read_bytes()
        if len(raw) < 28 or struct.unpack_from('<4sII', raw) != (b'glTF', 2, len(raw)):
            raise ValueError('Invalid model GLB header')
        size, kind = struct.unpack_from('<II', raw, 12)
        if kind != 0x4E4F534A:
            raise ValueError('Missing GLB JSON chunk')
        self.doc = json.loads(raw[20:20 + size])
        if not self.doc.get('meshes'):
            raise ValueError('Model export contains no geometry')
        binary_size, kind = struct.unpack_from('<II', raw, 20 + size)
        self.data = raw[28 + size:]
        if kind != 0x004E4942 or binary_size != len(self.data):
            raise ValueError('Invalid model GLB binary chunk')
        if self.doc.get('extensionsRequired') or len(self.doc.get('buffers', [])) != 1:
            raise ValueError('Unsupported model GLB extensions or buffers')
        self.parts = []
        self.instances = 0

    def accessor(self, index):
        a = self.doc['accessors'][index]
        if 'sparse' in a:
            raise ValueError('Sparse model accessors are unsupported')
        v = self.doc['bufferViews'][a['bufferView']]
        dtype = np.dtype({5120:'i1', 5121:'u1', 5122:'<i2', 5123:'<u2', 5125:'<u4', 5126:'<f4'}[a['componentType']])
        width = {'SCALAR':1, 'VEC2':2, 'VEC3':3, 'VEC4':4}[a['type']]
        result = np.ndarray((a['count'], width), dtype=dtype, buffer=self.data,
            offset=v.get('byteOffset', 0) + a.get('byteOffset', 0),
            strides=(v.get('byteStride', width * dtype.itemsize), dtype.itemsize)).copy()
        if a.get('normalized'):
            result = np.maximum(result.astype(float) / np.iinfo(dtype).max, -1)
        return result

    def visit(self, index, parent, ancestors=frozenset()):
        if index in ancestors:
            raise ValueError('Cyclic model scene')
        n = self.doc['nodes'][index]
        if 'skin' in n or 'weights' in n:
            raise ValueError('Skinned or morphed models are unsupported')
        m = parent @ transform(n)
        if 'mesh' in n:
            self.instances += 1
            for primitive in self.doc['meshes'][n['mesh']]['primitives']:
                if primitive.get('mode', 4) != 4 or primitive.get('targets'):
                    raise ValueError('Only rigid triangle primitives are supported')
                pos = self.accessor(primitive['attributes']['POSITION'])
                indices = self.accessor(primitive['indices']).ravel() if 'indices' in primitive else np.arange(len(pos))
                if len(indices) == 0 or len(indices) % 3:
                    raise ValueError('Model primitive has no complete triangles')
                # Reflection reverses winding when transforms are baked. Restore
                # front faces so two-sided shading does not invert outward normals.
                if np.linalg.det(m[:3, :3]) < 0:
                    indices = indices.reshape(-1, 3)[:, [0, 2, 1]].reshape(-1)
                pos = (pos @ m[:3, :3].T + m[:3, 3])[indices]
                if 'NORMAL' in primitive['attributes']:
                    normals = (self.accessor(primitive['attributes']['NORMAL']) @ np.linalg.inv(m[:3, :3]))[indices]
                else:
                    tri = pos.reshape(-1, 3, 3)
                    normals = np.repeat(np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]), 3, axis=0)
                normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
                self.parts.append(np.column_stack((pos, normals)))
        for child in n.get('children', []):
            self.visit(child, m, ancestors | {index})

    def geometry(self):
        for n in self.doc['scenes'][self.doc.get('scene', 0)].get('nodes', []):
            self.visit(n, np.eye(4))
        if not self.parts:
            raise ValueError('Model scene contains no visible geometry')
        vertices = np.concatenate(self.parts)
        if not np.isfinite(vertices).all():
            raise ValueError('Model contains non-finite geometry')
        return vertices


def transform(node):
    if 'matrix' in node:
        return np.array(node['matrix']).reshape(4, 4, order='F')
    x, y, z, w = node.get('rotation', [0, 0, 0, 1])
    m = np.eye(4)
    m[:3, :3] = [[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
        [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
        [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]
    m[:3, :3] = m[:3, :3] @ np.diag(node.get('scale', [1, 1, 1]))
    m[:3, 3] = node.get('translation', [0, 0, 0])
    return m


def render_model(glb, output, asset):
    model = Model(glb)
    vertices = model.geometry()
    lo, hi = vertices[:, :3].min(axis=0), vertices[:, :3].max(axis=0)
    span = float(np.max(hi - lo))
    if span <= 0:
        raise ValueError('Model has zero extent')
    vertices[:, :3] = (vertices[:, :3] - (lo + hi) / 2) / span * 2
    payload = base64.b64encode(vertices.astype('<f4').tobytes()).decode('ascii')
    page = Path(__file__).with_name('model_viewer.html').read_text(encoding='utf-8')
    page = page.replace('__TITLE__', html.escape(asset.rsplit('/', 1)[-1])).replace('__GEOMETRY__', payload)
    output = Path(output)
    output.write_text(page, encoding='utf-8')
    return dict(meshInstances=model.instances, triangles=len(vertices)//3,
                boundsMeters=dict(min=lo.tolist(), max=hi.tolist()), htmlBytes=output.stat().st_size)
