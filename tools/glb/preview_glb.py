"""Blender background GLB import + direct preview, without rewriting the source.
Usage: --input source.glb --output fresh.png. Standard bundled glTF importer.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import bpy
from mathutils import Vector

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--input', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args(sys.argv[sys.argv.index('--')+1:])
assert bpy.app.background and not a.output.exists()
assert a.output.suffix.lower() == '.png'
before = hashlib.sha256(a.input.read_bytes()).hexdigest()
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.addon_enable(module='io_scene_gltf2')
assert 'FINISHED' in bpy.ops.import_scene.gltf(filepath=str(a.input.resolve()))
scene = bpy.context.scene
meshes = [o for o in scene.objects if o.type == 'MESH']
points = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
assert points
lo = Vector(tuple(min(p[i] for p in points) for i in range(3)))
hi = Vector(tuple(max(p[i] for p in points) for i in range(3)))
center = (lo+hi)*.5
size = (hi-lo).length
camera_data = bpy.data.cameras.new('PreviewCamera')
camera = bpy.data.objects.new('PreviewCamera', camera_data)
scene.collection.objects.link(camera)
camera.location = center + Vector((1.25, -1.7, 1.0)).normalized()*size*2
camera.rotation_euler = (center-camera.location).to_track_quat('-Z','Y').to_euler()
camera_data.type = 'ORTHO'
camera_data.ortho_scale = size*1.07
camera_data.clip_start = .001
camera_data.clip_end = size*20
scene.camera = camera
lights = getattr(bpy.data, 'lights', None)
if lights is None:
    lights = bpy.data.lamps
for index, (offset, factor) in enumerate([((1,-1,2),100), ((-1,-.5,.8),60), ((.2,1,1.5),130)]):
    data = lights.new('PreviewLight'+str(index), 'AREA')
    data.energy = factor*size*size
    data.size = size
    light = bpy.data.objects.new(data.name,data)
    scene.collection.objects.link(light)
    light.location = center+Vector(offset)*size
    light.rotation_euler = (center-light.location).to_track_quat('-Z','Y').to_euler()
scene.world = bpy.data.worlds.new('PreviewWorld')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (.15,.18,.23,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = .5
scene.render.engine = 'CYCLES'
scene.cycles.samples = 32
scene.render.resolution_x = 1200
scene.render.resolution_y = 900
scene.render.resolution_percentage = 100
a.output.parent.mkdir(parents=True,exist_ok=True)
scene.render.filepath = str(a.output.resolve())
bpy.ops.render.render(write_still=True)
assert hashlib.sha256(a.input.read_bytes()).hexdigest() == before
print('GLB_PREVIEW_OK '+json.dumps(dict(sourceSha256=before, meshObjects=len(meshes),
      triangles=sum(len(o.data.polygons) for o in meshes), output=str(a.output.resolve()), sourceUnchanged=True)))
