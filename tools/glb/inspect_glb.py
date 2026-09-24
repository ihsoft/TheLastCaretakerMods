"""Read-only GLB 2 mesh/hierarchy audit. Python 3.10+, NumPy; no Blender needed.

Reports source facts, not gameplay compatibility. Unknown required extensions,
external resources, sparse accessors and non-triangle primitives are rejected.
Metadata/extras are data, never instructions. No re-export or geometry rewriting.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np


def require(ok, message):
    if not ok:
        raise ValueError(message)


def local_matrix(node):
    require(not ('matrix' in node and any(k in node for k in ('translation', 'rotation', 'scale'))),
            'Node has matrix and TRS')
    if 'matrix' in node:
        result = np.array(node['matrix'], dtype=float).reshape(4, 4, order='F')
    else:
        x, y, z, w = node.get('rotation', [0, 0, 0, 1])
        require(abs(x*x + y*y + z*z + w*w - 1) < 1e-5, 'Non-unit node quaternion')
        result = np.eye(4)
        result[:3, :3] = np.array([
            [1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
            [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
            [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]) @ np.diag(node.get('scale', [1, 1, 1]))
        result[:3, 3] = node.get('translation', [0, 0, 0])
    require(np.isfinite(result).all(), 'Non-finite node transform')
    require(np.allclose(result[3], [0, 0, 0, 1]), 'Non-affine matrix')
    return result


def inspect(path):
    data = path.read_bytes()
    require(len(data) >= 20 and struct.unpack_from('<4sII', data) == (b'glTF', 2, len(data)), 'Invalid GLB header')
    chunks = []
    cursor = 12
    while cursor < len(data):
        require(cursor + 8 <= len(data), 'Truncated chunk header')
        length, kind = struct.unpack_from('<II', data, cursor)
        cursor += 8
        require(length % 4 == 0 and cursor + length <= len(data), 'Invalid chunk length/alignment')
        chunks.append((kind, data[cursor:cursor+length]))
        cursor += length
    require(chunks and chunks[0][0] == 0x4E4F534A, 'Missing JSON chunk')
    doc = json.loads(chunks[0][1])
    require(doc['asset']['version'] == '2.0', 'Not glTF 2.0')
    require(not doc.get('skins'), 'Skinned geometry requires a dedicated pose validator')
    require(not doc.get('extensionsRequired'), 'Required extension needs a dedicated validator')
    require(len(chunks) == 2 and chunks[1][0] == 0x004E4942, 'Expected one embedded BIN chunk')
    binary = chunks[1][1]
    require(len(doc.get('buffers', [])) == 1 and 'uri' not in doc['buffers'][0], 'External/multiple buffers unsupported')
    require(0 <= len(binary) - doc['buffers'][0]['byteLength'] <= 3, 'BIN size mismatch')
    for view in doc.get('bufferViews', []):
        require(view['buffer'] == 0, 'Non-embedded buffer view')
        require(view.get('byteOffset', 0) >= 0 and view.get('byteOffset', 0) + view['byteLength'] <= doc['buffers'][0]['byteLength'], 'Buffer view exceeds data')
    for image in doc.get('images', []):
        require('uri' not in image and 0 <= image['bufferView'] < len(doc['bufferViews']), 'External/invalid image')

    def image_content(index):
        require(isinstance(index, int) and 0 <= index < len(doc.get('images', [])), 'Invalid material-pipeline image index')
        view = doc['bufferViews'][doc['images'][index]['bufferView']]
        start = view.get('byteOffset', 0)
        return binary[start:start + view['byteLength']]

    pipeline_summary = None
    pipeline = doc.get('extras', {}).get('materialPipeline')
    if pipeline is not None:
        require(pipeline.get('schema') == 'voyage.material-pipeline/1', 'Unsupported material-pipeline schema')
        mode = pipeline.get('requestedMode')
        require(mode in ('PbrApproximation', 'BakeReconstructed'), 'Invalid material-pipeline mode')
        artifacts = pipeline.get('sourceArtifacts', [])
        generated = pipeline.get('generatedImages', [])
        operations = pipeline.get('bakeOperations', [])
        unresolved = pipeline.get('unresolvedLayers', [])
        sources = [item.get('Source') for item in artifacts]
        require(all(isinstance(source, str) and source.startswith('/') for source in sources), 'Invalid source-artifact identity')
        require(len(sources) == len(set(sources)), 'Duplicate source-artifact identity')
        referenced_images = set()
        for artifact in artifacts:
            index = artifact.get('ImageIndex')
            if artifact.get('Disposition') == 'embedded-source':
                content = image_content(index)
                referenced_images.add(index)
                require(hashlib.sha256(content).hexdigest().upper() == artifact.get('Sha256'), 'Source-artifact hash mismatch')
                require(content.startswith(b'\x89PNG\r\n\x1a\n') and len(content) >= 24, 'Source artifact is not PNG')
                width, height = struct.unpack_from('>II', content, 16)
                require((width, height) == (artifact.get('Width'), artifact.get('Height')), 'Source-artifact dimensions mismatch')
            else:
                require(index is None and artifact.get('Error'), 'Non-embedded source artifact lacks error evidence')
            require(all(isinstance(c.get('Parameter'), str) and c.get('Material') for c in artifact.get('Consumers', [])),
                    'Invalid source-artifact consumer')
        for generated_image in generated:
            index = generated_image.get('ImageIndex')
            content = image_content(index)
            referenced_images.add(index)
            require(hashlib.sha256(content).hexdigest().upper() == generated_image.get('Sha256'), 'Generated-image hash mismatch')
        for operation in operations:
            index = operation.get('OutputImageIndex')
            require(hashlib.sha256(image_content(index)).hexdigest().upper() == operation.get('OutputSha256'), 'Bake output hash mismatch')
            require(operation.get('Fidelity') == 'reconstructed' and operation.get('InputTextures'), 'Invalid bake operation')
            require(all(source in sources for source in operation['InputTextures']), 'Bake input lacks source artifact')
        if mode == 'BakeReconstructed':
            require(all(layer.get('texture') in sources for layer in unresolved), 'Unresolved layer lacks source artifact')
        texture_images = {texture['source'] for texture in doc.get('textures', []) if 'source' in texture}
        require(texture_images | referenced_images == set(range(len(doc.get('images', [])))),
                'GLB image lacks PBR or material-pipeline provenance')
        pipeline_summary = dict(schema=pipeline['schema'], requestedMode=mode,
                                bakeOperations=len(operations), generatedImages=len(generated),
                                sourceArtifacts=len(artifacts), unresolvedLayers=len(unresolved))
    types = {5120:'i1', 5121:'u1', 5122:'<i2', 5123:'<u2', 5125:'<u4', 5126:'<f4'}
    widths = {'SCALAR':1, 'VEC2':2, 'VEC3':3, 'VEC4':4}

    def accessor(index):
        a = doc['accessors'][index]
        require('sparse' not in a, 'Sparse accessor requires a different reader')
        require(a['type'] in widths, 'Matrix accessor not supported by geometry audit')
        view = doc['bufferViews'][a['bufferView']]
        dtype, width, count = np.dtype(types[a['componentType']]), widths[a['type']], a['count']
        stride = view.get('byteStride', dtype.itemsize * width)
        offset = a.get('byteOffset', 0)
        require(count > 0 and offset >= 0 and stride >= dtype.itemsize * width, 'Invalid accessor dimensions')
        require(offset + (count-1)*stride + dtype.itemsize*width <= view['byteLength'], 'Accessor exceeds view')
        values = np.ndarray((count, width), dtype=dtype, buffer=binary,
                            offset=view.get('byteOffset', 0)+offset, strides=(stride, dtype.itemsize))
        require(np.isfinite(values).all(), 'Non-finite accessor')
        return values

    geometry = []
    for mesh in doc.get('meshes', []):
        parts = []
        for primitive in mesh['primitives']:
            require(primitive.get('mode', 4) == 4, 'Only triangle primitives supported')
            require(not primitive.get('targets'), 'Morph targets require a dedicated pose validator')
            positions = accessor(primitive['attributes']['POSITION'])
            require(positions.shape[1] == 3, 'POSITION must be VEC3')
            if 'indices' in primitive:
                index_accessor = doc['accessors'][primitive['indices']]
                require(index_accessor['type'] == 'SCALAR' and index_accessor['componentType'] in (5121, 5123, 5125), 'Invalid index component type')
            indices = accessor(primitive['indices']).reshape(-1) if 'indices' in primitive else np.arange(len(positions))
            require(len(indices) % 3 == 0 and indices.min() >= 0 and indices.max() < len(positions), 'Invalid triangle indices')
            for semantic, index in primitive['attributes'].items():
                require(len(accessor(index)) == len(positions), 'Attribute length mismatch: ' + semantic)
            if 'material' in primitive:
                require(0 <= primitive['material'] < len(doc.get('materials', [])), 'Invalid material reference')
            parts.append((positions, len(indices)//3))
        geometry.append(parts)
    nodes = doc.get('nodes', [])
    names = [n.get('name', 'node_' + str(i)) for i, n in enumerate(nodes)]
    require(len(names) == len(set(names)), 'Duplicate node names: ambiguous logic bindings')
    parents = {}
    for i, node in enumerate(nodes):
        for child in node.get('children', []):
            require(0 <= child < len(nodes) and child not in parents and child != i, 'Invalid/multiple node parent')
            parents[child] = i
    local = [local_matrix(n) for n in nodes]
    worlds = {}

    def world(i, visiting=None):
        if i in worlds:
            return worlds[i]
        visiting = set() if visiting is None else visiting
        require(i not in visiting, 'Cyclic scene graph')
        visiting.add(i)
        worlds[i] = (world(parents[i], visiting) if i in parents else np.eye(4)) @ local[i]
        visiting.remove(i)
        return worlds[i]

    roots = doc['scenes'][doc.get('scene', 0)]['nodes']
    active = set()

    def mark(i):
        require(0 <= i < len(nodes), 'Invalid scene root')
        require(i not in active, 'Repeated scene node')
        active.add(i)
        for c in nodes[i].get('children', []):
            mark(c)

    for i in range(len(nodes)):
        world(i)
    for i in roots:
        require(i not in parents, 'Scene root has a parent')
        mark(i)
    reports, bounds, triangles = [], [], 0
    for i, node in enumerate(nodes):
        parent = parents.get(i)
        report = dict(name=names[i], parent=names[parent] if parent is not None else None,
                      active=i in active, mesh=node.get('mesh'),
                      localMatrixColumnMajor=local[i].flatten(order='F').tolist(),
                      worldMatrixColumnMajor=worlds[i].flatten(order='F').tolist())
        if 'mesh' in node:
            parts = geometry[node['mesh']]
            report['triangles'] = sum(count for _, count in parts)
            if i in active:
                triangles += report['triangles']
                for points, _ in parts:
                    transformed = points @ worlds[i][:3, :3].T + worlds[i][:3, 3]
                    bounds.extend([transformed.min(axis=0), transformed.max(axis=0)])
        reports.append(report)
    require(bounds, 'No visible mesh geometry')
    return dict(schemaVersion=1, path=str(path.resolve()), sha256=hashlib.sha256(data).hexdigest().upper(),
                byteLength=len(data), format='GLB 2.0', units='meters', upAxis='+Y',
                counts=dict(nodes=len(nodes), activeNodes=len(active), meshDefinitions=len(geometry),
                            meshNodes=sum('mesh' in n for n in nodes), instantiatedTriangles=triangles,
                            materials=len(doc.get('materials', [])), images=len(doc.get('images', [])),
                            textures=len(doc.get('textures', [])), animations=len(doc.get('animations', [])),
                            skins=len(doc.get('skins', []))),
                worldBoundsMeters=dict(min=np.min(bounds, axis=0).tolist(), max=np.max(bounds, axis=0).tolist()),
                nodes=reports, materials=doc.get('materials', []), selfContained=True,
                materialPipeline=pipeline_summary,
                requiredExtensions=doc.get('extensionsRequired', []),
                limitations='Structural/geometry audit only; not complete Khronos validation or runtime acceptance')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, nargs='?')
    parser.add_argument('--registry', type=Path, help='Resolve and verify an owning model-source.json')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--expect-sha256')
    args = parser.parse_args()
    require(bool(args.input) != bool(args.registry), 'Supply input OR --registry')
    if args.registry:
        registry = json.loads(args.registry.read_text(encoding='utf-8'))
        require(registry['schemaVersion'] == 1, 'Unsupported model registry')
        if 'source' in registry:
            args.input = (args.registry.parent / registry['source']['path']).resolve()
            args.expect_sha256 = registry['source'].get('sha256')
        else:
            candidates = list(args.registry.parent.glob('*.glb'))
            require(len(candidates) == 1, 'Registry directory must contain exactly one GLB')
            args.input = candidates[0].resolve()
    report = inspect(args.input)
    if args.expect_sha256:
        require(report['sha256'] == args.expect_sha256.upper(), 'Source hash mismatch')
    if args.registry:
        if 'source' in registry and 'byteLength' in registry['source']:
            require(report['byteLength'] == registry['source']['byteLength'], 'Registry size mismatch')
        active_names = {n['name'] for n in report['nodes'] if n['active']}
        for binding in registry.get('nodes', {}).values():
            for name in (binding if isinstance(binding, list) else [binding]):
                require(name in active_names, 'Missing bound node: ' + name)
        for reference in registry.get('referenceOnlySubtrees', []):
            require(reference['node'] in active_names and reference['anchor'] in active_names, 'Missing reference/anchor')
    if args.output:
        require(not args.output.exists(), 'Choose a fresh audit path')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k:report[k] for k in ('sha256', 'byteLength', 'counts', 'worldBoundsMeters', 'selfContained', 'materialPipeline')} |
                     {'auditPath': str(args.output.resolve()) if args.output else None}))


if __name__ == '__main__':
    main()
