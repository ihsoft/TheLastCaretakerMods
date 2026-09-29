"""Blender 5+ isolated material renders from a GLB without rewriting the source.
Usage: --input source.glb --output fresh.png. Each material gets its own fixed-lit
tile; tiles are assembled into a contact sheet with at most six columns.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import math
import shutil

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
scene = bpy.context.scene
for obj in source_meshes:
    bpy.data.objects.remove(obj, do_unlink=True)

bpy.ops.mesh.primitive_uv_sphere_add(segments=96, ring_count=64, location=(0, 0, 1.45), radius=1.15)
sphere = bpy.context.object
sphere.name = 'IsolatedMaterialPreviewSphere'
for polygon in sphere.data.polygons:
    polygon.use_smooth = True

label_material = bpy.data.materials.new('PreviewLabelMaterial')
label_material.use_nodes = True
label_shader = label_material.node_tree.nodes.get('Principled BSDF')
label_shader.inputs['Base Color'].default_value = (0.9, 0.92, 0.96, 1)
label_shader.inputs['Emission Color'].default_value = (0.9, 0.92, 0.96, 1)
label_shader.inputs['Emission Strength'].default_value = 0.4
label_shader.inputs['Roughness'].default_value = 1.0
bpy.ops.object.text_add(location=(0, -1.3, 0.08), rotation=(math.pi / 2, 0, 0))
label = bpy.context.object
label.name = 'IsolatedMaterialPreviewLabel'
label.data.align_x = 'CENTER'
label.data.align_y = 'CENTER'
label.data.extrude = 0.002
label.data.materials.append(label_material)

floor_material = bpy.data.materials.new('PreviewFloorMaterial')
floor_material.use_nodes = True
floor_shader = floor_material.node_tree.nodes.get('Principled BSDF')
floor_shader.inputs['Base Color'].default_value = (0.055, 0.065, 0.08, 1)
floor_shader.inputs['Roughness'].default_value = 0.9
bpy.ops.mesh.primitive_plane_add(size=12, location=(0, 0, 0))
floor = bpy.context.object
floor.name = 'IsolatedMaterialPreviewFloor'
floor.data.materials.append(floor_material)

camera_data = bpy.data.cameras.new('MaterialPreviewCamera')
camera = bpy.data.objects.new('MaterialPreviewCamera', camera_data)
scene.collection.objects.link(camera)
target = Vector((0, 0, 1.15))
camera.location = Vector((0, -8.5, 3.4))
camera.rotation_euler = (target - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera_data.type = 'ORTHO'
camera_data.ortho_scale = 3.8
scene.camera = camera

lights = [
    ('Key', (-3.5, -4.0, 7.0), 950, 4.0),
    ('Fill', (4.0, -2.0, 4.0), 650, 5.0),
    ('Rim', (0, 3.0, 6.0), 1100, 3.5),
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
scene.view_settings.look = 'AgX - Medium High Contrast'
tile_width = 512
tile_height = 640
scene.render.resolution_x = tile_width
scene.render.resolution_y = tile_height
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.film_transparent = False
args.output.parent.mkdir(parents=True, exist_ok=True)
tile_directory = args.output.parent / (args.output.stem + '.tiles')
if tile_directory.exists():
    raise RuntimeError(f'Fresh tile directory required: {tile_directory}')
tile_directory.mkdir()
tile_paths = []
for index, material in enumerate(materials):
    sphere.data.materials.clear()
    sphere.data.materials.append(material)
    label.data.body = material.name
    label.data.size = min(0.24, max(0.065, 5.2 / max(1, len(material.name))))
    # Human-facing 1-based position: even labels above, odd labels below.
    label.location.z = 0.42 if (index + 1) % 2 == 0 else 0.08
    tile_path = tile_directory / f'{index:03d}.png'
    scene.render.filepath = str(tile_path.resolve())
    bpy.ops.render.render(write_still=True)
    tile_paths.append(tile_path)

# Assemble already lit, isolated tiles as emission planes. This second render is
# only a contact-sheet compositor and cannot introduce cross-material lighting.
for obj in list(scene.objects):
    bpy.data.objects.remove(obj, do_unlink=True)
columns = min(6, len(materials))
rows = math.ceil(len(materials) / columns)
tile_aspect = tile_height / tile_width
for index, tile_path in enumerate(tile_paths):
    image = bpy.data.images.load(str(tile_path.resolve()), check_existing=False)
    tile_material = bpy.data.materials.new(f'ContactSheetTile_{index:03d}')
    tile_material.use_nodes = True
    nodes = tile_material.node_tree.nodes
    nodes.clear()
    texture = nodes.new('ShaderNodeTexImage')
    texture.image = image
    emission = nodes.new('ShaderNodeEmission')
    output = nodes.new('ShaderNodeOutputMaterial')
    tile_material.node_tree.links.new(texture.outputs['Color'], emission.inputs['Color'])
    tile_material.node_tree.links.new(emission.outputs['Emission'], output.inputs['Surface'])
    column = index % columns
    row = index // columns
    x = column - (columns - 1) / 2
    y = (rows - 1) * tile_aspect / 2 - row * tile_aspect
    bpy.ops.mesh.primitive_plane_add(size=2, location=(x, y, 0))
    plane = bpy.context.object
    plane.name = f'ContactSheetTile_{index:03d}'
    plane.scale = (0.5, tile_aspect / 2, 1)
    plane.data.materials.append(tile_material)

sheet_camera_data = bpy.data.cameras.new('ContactSheetCamera')
sheet_camera = bpy.data.objects.new('ContactSheetCamera', sheet_camera_data)
scene.collection.objects.link(sheet_camera)
sheet_camera.location = (0, 0, 10)
sheet_camera.rotation_euler = (0, 0, 0)
sheet_camera_data.type = 'ORTHO'
sheet_world_width = columns
sheet_world_height = rows * tile_aspect
sheet_fit_axis = 'horizontal' if sheet_world_width >= sheet_world_height else 'vertical'
# Blender AUTO sensor fit interprets ortho_scale along the dominant camera-frame
# axis. Match that axis so wide single-row sheets do not crop to the old height.
sheet_camera_data.ortho_scale = sheet_world_width if sheet_fit_axis == 'horizontal' else sheet_world_height
scene.camera = sheet_camera
scene.world = bpy.data.worlds.new('ContactSheetWorld')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (0, 0, 0, 1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'None'
scene.view_settings.exposure = 0
scene.render.resolution_x = columns * tile_width
scene.render.resolution_y = rows * tile_height
scene.render.image_settings.file_format = 'PNG'
scene.render.filepath = str(args.output.resolve())
bpy.ops.render.render(write_still=True)
shutil.rmtree(tile_directory)
assert hashlib.sha256(args.input.read_bytes()).hexdigest() == before
print('MATERIAL_PREVIEW_OK ' + json.dumps({
    'sourceSha256': before,
    'materials': [material.name for material in materials],
    'swatches': len(materials),
    'isolatedRenders': True,
    'lightingRecipe': 'voyage.material-preview-fixed-lighting/2',
    'columns': columns,
    'rows': rows,
    'tileWidth': tile_width,
    'tileHeight': tile_height,
    'contactSheetFitAxis': sheet_fit_axis,
    'contactSheetOrthoScale': sheet_camera_data.ortho_scale,
    'output': str(args.output.resolve()),
    'sourceUnchanged': True,
}))
