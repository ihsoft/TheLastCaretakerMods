"""Render one material from an internal GLB to a fixed 768x768 WebP sphere preview."""
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
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
if not bpy.app.background or args.output.exists() or args.output.suffix.lower() != '.webp':
    raise RuntimeError('Expected Blender background mode and a fresh .webp output')
if bpy.app.version < (5, 0, 0):
    raise RuntimeError('Blender 5.0 or newer is required')

before = hashlib.sha256(args.input.read_bytes()).hexdigest().upper()
bpy.ops.wm.read_factory_settings(use_empty=True)
if 'FINISHED' not in bpy.ops.import_scene.gltf(filepath=str(args.input.resolve())):
    raise RuntimeError('Internal material GLB import failed')
materials = []
for obj in list(bpy.context.scene.objects):
    if obj.type != 'MESH':
        continue
    for material in obj.data.materials:
        if material is not None and material not in materials:
            materials.append(material)
    bpy.data.objects.remove(obj, do_unlink=True)
if len(materials) != 1:
    raise RuntimeError(f'Expected exactly one material, found {len(materials)}')

bpy.ops.mesh.primitive_uv_sphere_add(segments=128, ring_count=96, location=(0, 0, 1.25), radius=1.05)
sphere = bpy.context.object
sphere.name = 'MaterialPackPreviewSphere'
sphere.data.materials.append(materials[0])
for polygon in sphere.data.polygons:
    polygon.use_smooth = True

floor_material = bpy.data.materials.new('MaterialPackPreviewFloor')
floor_material.use_nodes = True
floor_shader = floor_material.node_tree.nodes.get('Principled BSDF')
floor_shader.inputs['Base Color'].default_value = (0.055, 0.065, 0.08, 1)
floor_shader.inputs['Roughness'].default_value = 0.9
bpy.ops.mesh.primitive_plane_add(size=12, location=(0, 0, 0))
bpy.context.object.data.materials.append(floor_material)

scene = bpy.context.scene
target = Vector((0, 0, 1.15))
camera_data = bpy.data.cameras.new('MaterialPackPreviewCamera')
camera = bpy.data.objects.new('MaterialPackPreviewCamera', camera_data)
scene.collection.objects.link(camera)
camera.location = Vector((0, -6.6, 2.9))
camera.rotation_euler = (target - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera_data.type = 'ORTHO'
camera_data.ortho_scale = 3.25
scene.camera = camera

for name, location, energy, size in (
    ('Key', (-3.5, -4.0, 6.5), 950, 4.0),
    ('Fill', (4.0, -2.0, 3.8), 620, 5.0),
    ('Rim', (0, 3.2, 5.8), 1050, 3.5),
):
    data = bpy.data.lights.new(name, 'AREA')
    data.energy = energy
    data.size = size
    light = bpy.data.objects.new(name, data)
    scene.collection.objects.link(light)
    light.location = location
    light.rotation_euler = (target - light.location).to_track_quat('-Z', 'Y').to_euler()

scene.world = bpy.data.worlds.new('MaterialPackPreviewWorld')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.025, 0.032, 0.045, 1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.25
scene.render.engine = 'BLENDER_EEVEE'
scene.view_settings.look = 'AgX - Medium High Contrast'
scene.view_settings.exposure = 0
scene.render.resolution_x = 768
scene.render.resolution_y = 768
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'WEBP'
scene.render.image_settings.color_mode = 'RGB'
scene.render.image_settings.quality = 95
scene.render.film_transparent = False
args.output.parent.mkdir(parents=True, exist_ok=True)
scene.render.filepath = str(args.output.resolve())
bpy.ops.render.render(write_still=True)
after = hashlib.sha256(args.input.read_bytes()).hexdigest().upper()
if before != after:
    raise RuntimeError('Internal material GLB changed during preview rendering')
print('MATERIAL_PACK_PREVIEW_OK ' + json.dumps({
    'sourceSha256': before,
    'material': materials[0].name,
    'output': str(args.output.resolve()),
    'width': 768,
    'height': 768,
    'blenderVersion': '.'.join(map(str, bpy.app.version)),
    'sourceUnchanged': True,
}))
