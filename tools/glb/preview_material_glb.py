"""Blender 5+ material-swatch render from a GLB without rewriting the source.
Usage: --input source.glb --output fresh.png. Imported materials are shown on labeled UV spheres.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import math

import bpy
from mathutils import Vector


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
assert bpy.app.background and not args.output.exists()
assert args.output.suffix.lower() == '.png'
if bpy.app.version < (5, 0, 0):
    raise RuntimeError('Blender 5.0 or newer is required')
before = hashlib.sha256(args.input.read_bytes()).hexdigest()
bpy.ops.wm.read_factory_settings(use_empty=True)
assert 'FINISHED' in bpy.ops.import_scene.gltf(filepath=str(args.input.resolve()))
source_meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
materials = []
for obj in source_meshes:
    for material in obj.data.materials:
        if material is not None and material not in materials:
            materials.append(material)
if not materials:
    raise RuntimeError('Imported GLB contains no materials')
for obj in source_meshes:
    bpy.data.objects.remove(obj, do_unlink=True)

spacing = 3.0
center_x = (len(materials) - 1) * spacing * 0.5
swatches = []
for index, material in enumerate(materials):
    x = index * spacing
    bpy.ops.mesh.primitive_uv_sphere_add(segments=96, ring_count=64, location=(x, 0, 1.45), radius=1.15)
    sphere = bpy.context.object
    sphere.name = f'MaterialPreview_{index:03d}_{material.name}'
    sphere.data.materials.append(material)
    for polygon in sphere.data.polygons:
        polygon.use_smooth = True
    swatches.append(sphere)

    bpy.ops.object.text_add(location=(x, -1.3, 0.2), rotation=(math.pi / 2, 0, 0))
    label = bpy.context.object
    label.data.body = material.name
    label.data.align_x = 'CENTER'
    label.data.align_y = 'CENTER'
    label.data.size = 0.28
    label.data.extrude = 0.002
    label_material = bpy.data.materials.get('PreviewLabelMaterial')
    if label_material is None:
        label_material = bpy.data.materials.new('PreviewLabelMaterial')
        label_material.use_nodes = True
        label_shader = label_material.node_tree.nodes.get('Principled BSDF')
        label_shader.inputs['Base Color'].default_value = (0.9, 0.92, 0.96, 1)
        label_shader.inputs['Emission Color'].default_value = (0.9, 0.92, 0.96, 1)
        label_shader.inputs['Emission Strength'].default_value = 0.4
        label_shader.inputs['Roughness'].default_value = 1.0
    label.data.materials.append(label_material)

floor_material = bpy.data.materials.new('PreviewFloorMaterial')
floor_material.use_nodes = True
floor_shader = floor_material.node_tree.nodes.get('Principled BSDF')
floor_shader.inputs['Base Color'].default_value = (0.055, 0.065, 0.08, 1)
floor_shader.inputs['Roughness'].default_value = 0.9
bpy.ops.mesh.primitive_plane_add(size=max(12, len(materials) * spacing + 5), location=(center_x, 0, 0))
floor = bpy.context.object
floor.name = 'PreviewFloor'
floor.data.materials.append(floor_material)

scene = bpy.context.scene
camera_data = bpy.data.cameras.new('MaterialPreviewCamera')
camera = bpy.data.objects.new('MaterialPreviewCamera', camera_data)
scene.collection.objects.link(camera)
target = Vector((center_x, 0, 1.15))
camera.location = Vector((center_x, -8.5, 3.4))
camera.rotation_euler = (target - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera_data.type = 'ORTHO'
camera_data.ortho_scale = max(4.2, len(materials) * 2.75)
scene.camera = camera

lights = [
    ('Key', (center_x - 3.5, -4.0, 7.0), 950, 4.0),
    ('Fill', (center_x + 4.0, -2.0, 4.0), 650, 5.0),
    ('Rim', (center_x, 3.0, 6.0), 1100, 3.5),
]
for name, location, energy, size in lights:
    data = bpy.data.lights.new(name, 'AREA')
    data.energy = energy
    data.size = size
    light = bpy.data.objects.new(name, data)
    scene.collection.objects.link(light)
    light.location = location
    light.rotation_euler = (target - light.location).to_track_quat('-Z', 'Y').to_euler()

scene.world = bpy.data.worlds.new('MaterialPreviewWorld')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.025, 0.032, 0.045, 1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.25
scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = max(900, len(materials) * 520)
scene.render.resolution_y = 700
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.film_transparent = False
args.output.parent.mkdir(parents=True, exist_ok=True)
scene.render.filepath = str(args.output.resolve())
bpy.ops.render.render(write_still=True)
assert hashlib.sha256(args.input.read_bytes()).hexdigest() == before
print('MATERIAL_PREVIEW_OK ' + json.dumps({
    'sourceSha256': before,
    'materials': [material.name for material in materials],
    'swatches': len(swatches),
    'output': str(args.output.resolve()),
    'sourceUnchanged': True,
}))
