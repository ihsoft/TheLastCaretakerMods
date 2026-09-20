"""Thin Blender 5+ glTF exporter wrapper; use a disposable background process.

Blender --background --factory-startup --python-exit-code 1 --python export_blender_glb.py
  -- --input model.blend --output model.glb
Uses Blender's built-in glTF exporter, not a custom format converter.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import bpy
from mathutils.kdtree import KDTree


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def read_glb(path):
    data = path.read_bytes()
    require(struct.unpack_from('<4sII', data) == (b'glTF', 2, len(data)),
            'Invalid GLB header/length')
    size, kind = struct.unpack_from('<II', data, 12)
    require(kind == 0x4E4F534A, 'Missing GLB JSON chunk')
    doc = json.loads(data[20:20 + size].decode('utf-8'))
    require(all('uri' not in b for b in doc.get('buffers', [])), 'External buffer')
    require(all('uri' not in i for i in doc.get('images', [])), 'External image')
    return doc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--root', default='HarpoonCannon')
    parser.add_argument('--verify-only', action='store_true', help='Check existing GLB without exporting')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    source, output = args.input.resolve(), args.output.resolve()
    require(bpy.app.background, 'Use a separate background Blender process')
    require(bpy.app.version >= (5, 0, 0), 'Blender 5.0 or newer is required')
    try:
        bpy.ops.import_scene.gltf.get_rna_type()
        bpy.ops.export_scene.gltf.get_rna_type()
    except (AttributeError, RuntimeError) as error:
        raise RuntimeError('Blender built-in glTF import/export operators are unavailable') from error
    require(source.is_file() and source.suffix.lower() == '.blend', 'Input must be a .blend')
    require(output.suffix.lower() == '.glb', 'Output must be .glb')
    require(output.is_file() if args.verify_only else not output.exists(),
            'Verification needs an existing GLB; export needs a fresh path')
    audit_path = output.with_suffix('.audit.json')
    require(args.verify_only or not audit_path.exists(), 'Audit path already exists')
    source_hash = sha(source)
    bpy.ops.wm.open_mainfile(filepath=str(source))
    root = bpy.data.objects.get(args.root)
    require(root is not None, 'Model root not found: ' + args.root)
    chosen = set()

    def visit(obj):
        chosen.add(obj)
        for child in obj.children:
            visit(child)

    visit(root)
    require(all(o.type in ('MESH', 'EMPTY') for o in chosen), 'Unexpected object under model root')
    expected_parents = {o.name: o.parent.name if o.parent in chosen else None for o in chosen}
    expected_triangles = {}
    expected_points = {}
    materials = {}
    for obj in bpy.context.scene.objects:
        obj.select_set(obj in chosen)
    for obj in chosen:
        if obj.type != 'MESH':
            continue
        obj.data.calc_loop_triangles()
        expected_triangles[obj.name] = len(obj.data.loop_triangles)
        expected_points[obj.name] = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
        for material in obj.data.materials:
            shader = material.node_tree.nodes.get('Principled BSDF')
            require(shader is not None, 'Requires Principled material: ' + material.name)
            emission = material.node_tree.nodes.get('PaletteEmission')
            materials[material.name] = {
                'baseColorFactor': list(shader.inputs['Base Color'].default_value),
                'metallicFactor': shader.inputs['Metallic'].default_value,
                'roughnessFactor': shader.inputs['Roughness'].default_value,
                'emissiveFactor': [v * emission.inputs['Strength'].default_value
                                   for v in emission.inputs['Color'].default_value[:3]] if emission else [0, 0, 0],
            }
    properties = bpy.ops.export_scene.gltf.get_rna_type().properties
    options = dict(filepath=str(output), export_format='GLB', export_yup=True,
                   export_normals=True, export_animations=False, export_cameras=False,
                   export_apply=False, export_extras=True)
    options['use_selection' if 'use_selection' in properties else 'export_selected'] = True
    options['export_materials'] = ('EXPORT' if properties['export_materials'].type == 'ENUM' else True)
    if 'export_lights' in properties:
        options['export_lights'] = False
    output.parent.mkdir(parents=True, exist_ok=True)
    if not args.verify_only:
        require('FINISHED' in bpy.ops.export_scene.gltf(**options), 'GLB export failed')
    doc = read_glb(output)
    nodes = doc['nodes']
    by_name = {n['name']: i for i, n in enumerate(nodes)}
    require(set(by_name) == set(expected_parents), 'Export changed model node coverage')
    parents = {child: i for i, node in enumerate(nodes) for child in node.get('children', [])}
    for name, expected in expected_parents.items():
        parent = parents.get(by_name[name])
        require((nodes[parent]['name'] if parent is not None else None) == expected,
                'Parent mismatch: ' + name)
    for name, expected in expected_triangles.items():
        mesh = doc['meshes'][nodes[by_name[name]]['mesh']]
        require(all(p.get('mode', 4) == 4 for p in mesh['primitives']), 'Non-triangle primitive')
        actual = sum(doc['accessors'][p['indices']]['count'] // 3 for p in mesh['primitives'])
        require(actual == expected, 'Triangle count mismatch: ' + name)
    exported_materials = {m['name']: m for m in doc['materials']}
    require(set(exported_materials) == set(materials), 'Material coverage mismatch')
    for name, expected in materials.items():
        material = exported_materials[name]
        pbr = material.get('pbrMetallicRoughness', {})
        actual = dict(baseColorFactor=pbr.get('baseColorFactor', [1, 1, 1, 1]),
                      metallicFactor=pbr.get('metallicFactor', 1),
                      roughnessFactor=pbr.get('roughnessFactor', 1),
                      emissiveFactor=material.get('emissiveFactor', [0, 0, 0]))
        for key, wanted in expected.items():
            got = actual[key]
            pairs = zip(got, wanted) if isinstance(wanted, list) else [(got, wanted)]
            require(all(abs(a - b) < 1e-6 for a, b in pairs), 'Material mismatch: ' + name + '/' + key)
    # Standard importer checks the binary positions and transformed assembly too,
    # including the exporter's Z-up -> glTF Y-up -> Blender Z-up conversion.
    # Sharp-normal vertex splits are legal, so compare points in both directions.
    bpy.ops.wm.read_factory_settings(use_empty=True)
    require('FINISHED' in bpy.ops.import_scene.gltf(filepath=str(output)), 'GLB re-import failed')
    imported = {o.name: o for o in bpy.context.scene.objects if o.type == 'MESH'}
    require(set(imported) == set(expected_points), 'Re-import mesh coverage mismatch')
    max_error = 0.0
    for name, before in expected_points.items():
        obj = imported[name]
        after = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
        obj.data.calc_loop_triangles()
        require(len(obj.data.loop_triangles) == expected_triangles[name], 'Re-import triangle mismatch: ' + name)
        for left, right in ((before, after), (after, before)):
            tree = KDTree(len(right))
            for i, point in enumerate(right):
                tree.insert(point, i)
            tree.balance()
            max_error = max(max_error, max(tree.find(point)[2] for point in left))
    require(max_error < 1e-5, 'Re-import world geometry differs by >0.001 cm')
    require(sha(source) == source_hash, 'Source blend changed')
    audit = dict(source=str(source), sourceSha256=source_hash, output=str(output),
                 outputSha256=sha(output), blender=bpy.app.version_string,
                 exporter='Blender built-in glTF 2.0', nodes=len(nodes),
                 meshObjects=len(expected_triangles), triangles=sum(expected_triangles.values()),
                 materials=len(materials), hierarchyVerified=True, materialValuesVerified=True,
                 selfContained=True, sourceUnchanged=True, units='meters', upAxis='+Y',
                 roundtripMaxWorldErrorMeters=max_error)
    if not args.verify_only:
        audit_path.write_text(json.dumps(audit, indent=2) + '\n', encoding='utf-8')
    print('GLB_EXPORT_OK ' + json.dumps(audit))


if __name__ == '__main__':
    main()
