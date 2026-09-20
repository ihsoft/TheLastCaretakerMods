"""Blender 5+ only: --input extracted.json --output fresh.blend [--render].
Original electrical socket geometry, approximate flat PBR slot colors only.
Does not integrate the reference into a mod or implement socket behavior.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import bpy
from mathutils import Vector

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--render', action='store_true')
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
assert bpy.app.background
if bpy.app.version < (5, 0, 0):
    raise RuntimeError('Blender 5.0 or newer is required')
assert not args.output.exists() and args.output.suffix == '.blend'
d = json.loads(args.input.read_text(encoding='utf-8'))
assert d['source'] == '/Game/AssetSets/Sockets/SM_Socket_Electrical_Output'
assert d['materials'] == ['Trims', 'Socket_Hologram', 'Socket_Color']
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.unit_settings.length_unit = 'CENTIMETERS'
root = bpy.data.objects.new('ElectricalSocket_Reference', None)
scene.collection.objects.link(root)
root['source_asset'] = d['source']
root['source_build'] = d['steamBuild']
root['purpose'] = 'Planning reference only; replace with original game socket at final mount point'
root['coordinates'] = 'UE cm -> Blender meters (X,-Y,Z); source pivot retained'
root['socket_data_roll_degrees_unapplied'] = 90
verts = [(p[0] * .01, -p[1] * .01, p[2] * .01) for p in d['vertices']]
faces = [d['indices'][i:i + 3] for i in range(0, len(d['indices']), 3)]
source_indices = list(range(len(verts)))
seen = set()
for face in faces:
    key = tuple(sorted(face))
    if key in seen:
        for j, index in enumerate(face):
            face[j] = len(verts)
            verts.append(verts[index])
            source_indices.append(index)
    seen.add(key)
mesh = bpy.data.meshes.new('SM_Socket_Electrical_Output_LOD0')
mesh.from_pydata(verts, [], faces)
mesh.update()
assert not mesh.validate(), 'Blender corrected invalid extracted geometry'
obj = bpy.data.objects.new('SM_Socket_Electrical_Output', mesh)
scene.collection.objects.link(obj)
obj.parent = root
# Unreal's clockwise indices + handedness reflection yield Blender outward faces.
# Keep every original triangle, including the small authored label surfaces.
normals = [tuple(Vector((d['normals'][i][0], -d['normals'][i][1], d['normals'][i][2])).normalized()) for i in source_indices]
mesh.normals_split_custom_set_from_vertices(normals)
for polygon in mesh.polygons:
    polygon.use_smooth = True
for channel in range(len(d['texcoords'][0])):
    layer = mesh.uv_layers.new(name='UV' + str(channel))
    for loop in mesh.loops:
        u, v = d['texcoords'][source_indices[loop.vertex_index]][channel]
        layer.data[loop.index].uv = (u, 1 - v)
# Approximate diffuse appearance only, intentionally not an Unreal shader recreation.
palette = [((.34, .38, .40), .7, .42), ((.03, .15, .8), .1, .35),
           ((.16, .19, .21), .45, .5)]
for name, (srgb, metal, rough) in zip(d['materials'], palette):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    rgba = tuple(c ** 2.2 for c in srgb) + (1,)
    mat.diffuse_color = rgba[:len(mat.diffuse_color)]
    if hasattr(mat, 'use_backface_culling'):
        mat.use_backface_culling = True
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = rgba
    shader.inputs['Metallic'].default_value = metal
    shader.inputs['Roughness'].default_value = rough
    mesh.materials.append(mat)
for section in d['sections']:
    first = section['firstIndex'] // 3
    for polygon in list(mesh.polygons)[first:first + section['triangles']]:
        polygon.material_index = section['material']
studio = bpy.data.collections.new('PreviewStudio')
scene.collection.children.link(studio)
camera_data = bpy.data.cameras.new('ReferenceCamera')
camera = bpy.data.objects.new('ReferenceCamera', camera_data)
studio.objects.link(camera)
camera.location = (.4, -.62, .33)
camera.rotation_euler = (Vector((0, 0, 0)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera_data.type = 'ORTHO'
camera_data.ortho_scale = .46
camera_data.clip_start = .001
scene.camera = camera
lights = getattr(bpy.data, 'lights', None)
if lights is None:
    lights = bpy.data.lamps
for i, (position, energy, size) in enumerate([((.3, .4, .5), 35, .35), ((-.4, .2, .1), 25, .3), ((0, -.4, .4), 45, .25)]):
    light_data = lights.new('Light' + str(i), 'AREA')
    light_data.energy = energy
    light_data.size = size
    light = bpy.data.objects.new(light_data.name, light_data)
    studio.objects.link(light)
    light.location = position
    light.rotation_euler = (-light.location).to_track_quat('-Z', 'Y').to_euler()
scene.world = bpy.data.worlds.new('ReferenceWorld')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (.25, .3, .36, 1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = .6
scene.render.engine = 'CYCLES'
scene.cycles.samples = 32
scene.render.resolution_x = 900
scene.render.resolution_y = 760
scene.render.resolution_percentage = 100
obj.select_set(True)
bpy.context.view_layer.objects.active = obj
readme = bpy.data.texts.new('REFERENCE_README')
readme.write(root['purpose'] + '\n' + root['coordinates'] + '\n'
             'Raw mesh local orientation; socket-data Roll=90 is metadata only.\n'
             'No textures, procedural dirt, icon, glow or animations recreated.\n'
             'Cooked ordinary render LOD0, not reconstructed Nanite geometry.\n')
args.output.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(args.output.resolve()))
bpy.ops.wm.open_mainfile(filepath=str(args.output.resolve()))
saved = bpy.data.objects['SM_Socket_Electrical_Output']
assert len(saved.data.vertices) == len(verts) and len(saved.data.polygons) == len(faces)
error = max((a.co - Vector(b)).length for a, b in zip(saved.data.vertices, verts))
assert error < 1e-6
report = dict(source=d['source'], triangles=len(faces), vertices=len(verts),
              splitDuplicateFaceVertices=len(verts)-len(d['vertices']),
              maxErrorMeters=error, sourceSha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
              blendSha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
              approximateMaterials=True, sourcePivotPreserved=True)
args.output.with_suffix('.audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('SOCKET_REFERENCE_OK ' + json.dumps(report))
if args.render:
    bpy.context.scene.render.filepath = str(args.output.with_suffix('.png').resolve())
    bpy.ops.render.render(write_still=True)
