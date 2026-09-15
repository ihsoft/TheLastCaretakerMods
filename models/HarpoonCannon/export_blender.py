"""Run with Blender --background --factory-startup --python this_file -- --output file.blend.

Requires Blender 2.80+; uses bpy/stdlib only. No OBJ add-on, pip or game tools.
Reads the active pinned OBJ and model palette, creates editable geometry and
articulation parents, then reopens the saved file for independent roundtrip QA.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Vector

# Early 2.80 does not populate __file__ for --python (runpy does).
SCRIPT_FILE=globals().get('__file__')
if SCRIPT_FILE is None and '--python' in sys.argv:
    SCRIPT_FILE=sys.argv[sys.argv.index('--python')+1]
if SCRIPT_FILE is None:
    raise RuntimeError('Cannot locate converter source; invoke with --python or runpy.run_path')
SOURCE_DIR=Path(SCRIPT_FILE).resolve().parent
CM_TO_METERS=0.01
ROLE_NAMES={'base':'HC_Base','yaw':'HC_Yaw','pitch':'HC_Pitch'}


def require(condition,message):
    if not condition:
        raise RuntimeError(message)


def load_inputs():
    descriptor_path=SOURCE_DIR/'runtime-model.json'
    descriptor=json.loads(descriptor_path.read_text(encoding='utf-8'))
    require(descriptor['schemaVersion']==1 and descriptor['units']=='centimeters'
            and descriptor['uniformScale']==1
            and descriptor['axes']=={'forward':'+X','right':'+Y','up':'+Z'},
            'Unsupported source coordinate contract')
    for source in descriptor['sources'].values():
        path=(SOURCE_DIR/source['path']).resolve()
        require(path.parent==SOURCE_DIR,'Source must be a descriptor sibling')
        require(hashlib.sha256(path.read_bytes()).hexdigest().upper()==source['sha256'],
                'Source hash drift: '+str(path))
    palette=json.loads((SOURCE_DIR/'material-palette.json').read_text(encoding='utf-8'))
    require(palette['schemaVersion']==1 and palette['sourceMtl']==descriptor['sources']['mtl'],
            'Palette is not bound to the active MTL')
    objects={}
    global_vertices=[]
    current=None
    for line in (SOURCE_DIR/descriptor['sources']['obj']['path']).read_text(encoding='utf-8').splitlines():
        fields=line.split()
        if not fields:
            continue
        if fields[0]=='o':
            require(fields[1] not in objects,'Duplicate OBJ object')
            current={'vertices':[],'faces':[],'indices':{},'material':None}
            objects[fields[1]]=current
        elif fields[0]=='v':
            global_vertices.append(tuple(float(v) for v in fields[1:4]))
        elif fields[0]=='usemtl':
            require(current is not None,'Material outside OBJ object')
            require(current['material'] in (None,fields[1]),'Multiple materials per object unsupported')
            current['material']=fields[1]
        elif fields[0]=='f':
            require(current is not None,'Face outside OBJ object')
            face=[]
            for field in fields[1:]:
                index=int(field.split('/')[0])
                index=index-1 if index>0 else len(global_vertices)+index
                require(0<=index<len(global_vertices),'Invalid OBJ vertex index')
                if index not in current['indices']:
                    current['indices'][index]=len(current['vertices'])
                    current['vertices'].append(global_vertices[index])
                face.append(current['indices'][index])
            require(len(face)==3,'Active source must be triangulated')
            current['faces'].append(face)
    selected=[name for mesh in descriptor['meshes'] for name in mesh['objectNames']]
    selected += [a['sourceObjectName'] for a in descriptor['ammo']['instances']]
    require(len(selected)==len(set(selected)) and set(selected)==set(objects),'Incomplete/duplicate model partition')
    require(set(o['material'] for o in objects.values())==set(s['name'] for s in palette['slots']),
            'Material palette does not exactly cover the model')
    return descriptor,palette,objects


def collection(name,parent):
    result=bpy.data.collections.new(name)
    parent.children.link(result)
    return result


def empty(name,parent,location_cm,group):
    obj=bpy.data.objects.new(name,None)
    group.objects.link(obj)
    obj.empty_display_type='ARROWS'
    obj.empty_display_size=0.2
    obj.parent=parent
    obj.matrix_parent_inverse=Matrix.Identity(4)
    obj.location=Vector(location_cm)*CM_TO_METERS
    return obj


def make_material(slot):
    material=bpy.data.materials.new(slot['name'])
    material.use_nodes=True
    rgba=tuple(slot['baseColorLinear'])+(1.0,)
    material.diffuse_color=rgba[:len(material.diffuse_color)]
    if hasattr(material,'use_backface_culling'):
        material.use_backface_culling=True
    shader=material.node_tree.nodes.get('Principled BSDF')
    require(shader is not None,'Principled shader unavailable')
    shader.inputs['Base Color'].default_value=rgba
    shader.inputs['Roughness'].default_value=slot['roughness']
    shader.inputs['Metallic'].default_value=slot['metallic']
    # Separate emission works even in older Principled implementations which
    # lack the built-in Emission socket; parameters remain explicit/editable.
    emission=material.node_tree.nodes.new('ShaderNodeEmission')
    emission.name='PaletteEmission'
    emission.inputs['Color'].default_value=tuple(slot['emissiveColorLinear'])+(1.0,)
    emission.inputs['Strength'].default_value=1.0
    add=material.node_tree.nodes.new('ShaderNodeAddShader')
    material.node_tree.links.new(shader.outputs[0],add.inputs[0])
    material.node_tree.links.new(emission.outputs[0],add.inputs[1])
    material.node_tree.links.new(add.outputs[0],material.node_tree.nodes['Material Output'].inputs['Surface'])
    material['source_slot']=slot['name']
    return material


def make_mesh(name,source,origin_cm):
    origin=Vector(origin_cm)
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata([tuple((Vector(v)-origin)*CM_TO_METERS) for v in source['vertices']],[],source['faces'])
    mesh.update()
    require(not mesh.validate(verbose=False),'Blender corrected invalid source mesh: '+name)
    for polygon in mesh.polygons:
        polygon.use_smooth=False
    return mesh


def mesh_object(name,mesh,parent,location_cm,group,source):
    obj=bpy.data.objects.new(name,mesh)
    group.objects.link(obj)
    obj.parent=parent
    obj.matrix_parent_inverse=Matrix.Identity(4)
    obj.location=Vector(location_cm)*CM_TO_METERS
    obj['source_object']=name
    obj['source_material']=source['material']
    return obj


def build_scene(descriptor,palette,objects):
    # Executed only in a separate factory-startup/background process, never an
    # interactive user's scene. Remove startup objects and orphan collections.
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    for group in list(bpy.data.collections):
        bpy.data.collections.remove(group)
    scene=bpy.context.scene
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1.0
    if hasattr(scene.unit_settings,'length_unit'):
        scene.unit_settings.length_unit='CENTIMETERS'
    root_collection=collection('HarpoonCannon',scene.collection)
    groups={role:collection(role.upper(),root_collection) for role in ('base','yaw','pitch','ammo')}
    controls=collection('Controls',root_collection)
    root=empty('HarpoonCannon',None,(0,0,0),controls)
    root['source_obj_sha256']=descriptor['sources']['obj']['sha256']
    root['coordinate_contract']='Blender meters = source centimeters / 100; axes unchanged'
    nodes={}
    for node in descriptor['hierarchy']:
        role=node['role']
        parent=nodes[node['parentRole']] if node['parentRole'] else root
        nodes[role]=empty(ROLE_NAMES[role],parent,node['locationRelativeToParentCm'],controls)
        nodes[role]['role']=role
    nodes['yaw']['editing_hint']='Rotate local Z to traverse; pitch and ammo follow'
    nodes['pitch']['editing_hint']='Rotate local Y to elevate; negative Blender Y raises muzzle (+X)'
    nodes['yaw'].lock_rotation=(True,True,False)
    nodes['pitch'].lock_rotation=(True,False,True)
    materials={slot['name']:make_material(slot) for slot in palette['slots']}
    for part in descriptor['meshes']:
        for name in part['objectNames']:
            source=objects[name]
            mesh=make_mesh(name,source,part['meshOriginInSourceCm'])
            mesh.materials.append(materials[source['material']])
            mesh_object(name,mesh,nodes[part['role']],(0,0,0),groups[part['role']],source)
    prototype=descriptor['ammo']['prototype']
    source=objects[prototype['objectNames'][0]]
    ammo_mesh=make_mesh('Ammo_SharedGeometry',source,prototype['meshOriginInSourceCm'])
    ammo_mesh.materials.append(materials[source['material']])
    for instance in descriptor['ammo']['instances']:
        name=instance['sourceObjectName']
        obj=mesh_object(name,ammo_mesh,nodes[instance['parentRole']],
                        instance['locationRelativeToParentCm'],groups['ammo'],objects[name])
        obj['instance_name']=instance['name']
    if 'sight' in descriptor:
        sight=descriptor['sight']
        obj=empty('HC_Sight',nodes[sight['parentRole']],sight['locationRelativeToParentCm'],controls)
        obj['optical_forward']='local +X; marker only, not game camera logic'
    notes=bpy.data.texts.new('MODEL_README')
    notes.write('HarpoonCannon editable model\n\n'
                '83 separate named parts; 6 ammo objects share one editable mesh.\n'
                'Units: meters internally, centimeters in the UI. Source axes preserved.\n'
                'HC_Yaw: rotate Z. HC_Pitch: rotate Y; negative Y raises the muzzle.\n'
                'No animation, constraints, drivers or executable Text scripts.\n'
                'PreviewStudio collection holds only camera/lights, not game geometry.\n'
                'Materials use the accepted linear game palette. Blender lighting differs.\n'
                'OBJ mesh is triangulated and flat shaded, deliberately no smoothing/remeshing.\n'
                'Editing this .blend does not alter the repository OBJ or installed mod.\n'
                'Source OBJ SHA-256: '+descriptor['sources']['obj']['sha256']+'\n')
    setup_preview(scene)
    bpy.context.view_layer.update()
    for obj in scene.objects:
        obj.select_set(False)
    nodes['yaw'].select_set(True)
    bpy.context.view_layer.objects.active=nodes['yaw']


def setup_preview(scene):
    studio=collection('PreviewStudio',scene.collection)
    camera_data=bpy.data.cameras.new('PreviewCamera')
    camera=bpy.data.objects.new('PreviewCamera',camera_data)
    studio.objects.link(camera)
    camera.location=(4.7,-5.7,3.25)
    target=Vector((0.23,-0.08,0.82))
    camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
    camera_data.type='ORTHO'
    camera_data.ortho_scale=4.8
    camera_data.clip_start=0.01
    camera_data.clip_end=100
    scene.camera=camera
    for name,position,energy,size in (
            ('Key',(1.0,-4.0,6.0),1100,4.0),
            ('Fill',(-3.0,1.0,3.0),800,3.0),
            ('Rim',(2.0,4.0,5.0),1400,3.0)):
        lights=getattr(bpy.data,'lights',None)
        if lights is None:
            lights=bpy.data.lamps
        data=lights.new(name,'AREA')
        data.energy=energy
        data.size=size
        lamp=bpy.data.objects.new(name,data)
        studio.objects.link(lamp)
        lamp.location=position
        lamp.rotation_euler=(target-lamp.location).to_track_quat('-Z','Y').to_euler()
    world=bpy.data.worlds.new('PreviewWorld')
    world.use_nodes=True
    world.node_tree.nodes['Background'].inputs['Color'].default_value=(0.09,0.12,0.15,1)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value=0.5
    scene.world=world
    scene.render.engine='CYCLES'
    scene.cycles.device='CPU'
    scene.cycles.samples=24
    scene.render.resolution_x=1000
    scene.render.resolution_y=760
    scene.render.resolution_percentage=100
    try:
        scene.view_settings.view_transform='Standard'
    except TypeError:
        scene.view_settings.view_transform='Default'  # Early 2.80 OCIO label.
    scene.view_settings.exposure=0
    scene.view_settings.gamma=1
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                space=area.spaces.active
                space.shading.type='SOLID'
                space.shading.color_type='MATERIAL'
                space.clip_start=0.01
                space.clip_end=100
                space.region_3d.view_location=target
                space.region_3d.view_distance=5.7
                space.region_3d.view_rotation=camera.rotation_euler.to_quaternion()


def validate(descriptor,palette,objects):
    model_objects=[o for o in bpy.data.objects if 'source_object' in o]
    require(len(model_objects)==len(objects),'Model object count changed')
    max_error=0.0
    triangles=0
    for obj in model_objects:
        source=objects[obj['source_object']]
        require(obj.type=='MESH' and len(obj.data.vertices)==len(source['vertices']),'Vertex count drift')
        require([list(p.vertices) for p in obj.data.polygons]==source['faces'],'Face topology/winding drift')
        require(len(obj.data.materials)==1 and obj.data.materials[0].name==source['material'],'Material assignment drift')
        require(not obj.modifiers and not obj.constraints,'Unexpected geometry/rig modifier')
        for vertex,expected in zip(obj.data.vertices,source['vertices']):
            error=(obj.matrix_world @ vertex.co / CM_TO_METERS-Vector(expected)).length
            max_error=max(max_error,error)
        triangles+=len(obj.data.polygons)
    require(max_error<0.001,'Neutral world geometry differs by >=0.001 cm')
    for slot in palette['slots']:
        material=bpy.data.materials[slot['name']]
        shader=material.node_tree.nodes.get('Principled BSDF')
        for a,b in zip(shader.inputs['Base Color'].default_value[:3],slot['baseColorLinear']):
            require(abs(a-b)<1e-6,'Material color drift')
        for field in ('Roughness','Metallic'):
            require(abs(shader.inputs[field].default_value-slot[field.lower()])<1e-6,'Material scalar drift')
        emission=material.node_tree.nodes['PaletteEmission'].inputs['Color']
        require(all(abs(a-b)<1e-6 for a,b in zip(emission.default_value[:3],slot['emissiveColorLinear'])),
                'Emissive color drift')
    ammo=[bpy.data.objects[a['sourceObjectName']] for a in descriptor['ammo']['instances']]
    require(len(set(obj.data.name for obj in ammo))==1,'Ammo prototype was duplicated')
    for node in descriptor['hierarchy']:
        obj=bpy.data.objects[ROLE_NAMES[node['role']]]
        expected_parent=ROLE_NAMES[node['parentRole']] if node['parentRole'] else 'HarpoonCannon'
        require(obj.parent.name==expected_parent,'Rig hierarchy drift')
        require((obj.location/CM_TO_METERS-Vector(node['locationRelativeToParentCm'])).length<0.001,
                'Rig pivot drift')
    if 'sight' in descriptor:
        sight=bpy.data.objects['HC_Sight']
        require(sight.parent.name==ROLE_NAMES[descriptor['sight']['parentRole']], 'Sight parent drift')
        require((sight.location/CM_TO_METERS-Vector(descriptor['sight']['locationRelativeToParentCm'])).length<0.001,
                'Sight position drift')
    return {'modelObjects':len(model_objects),'triangles':triangles,'materials':len(palette['slots']),
            'sharedAmmoInstances':len(ammo),'maxVertexErrorCm':max_error,
            'sourceObjSha256':descriptor['sources']['obj']['sha256']}


def validate_motion(descriptor,objects):
    yaw=bpy.data.objects['HC_Yaw']
    pitch=bpy.data.objects['HC_Pitch']
    old_yaw=yaw.rotation_euler.copy()
    old_pitch=pitch.rotation_euler.copy()
    try:
        yaw.rotation_euler.z=math.radians(37)
        pitch.rotation_euler.y=math.radians(-8)
        bpy.context.view_layer.update()
        worlds={}
        for node in descriptor['hierarchy']:
            role=node['role']
            parent=worlds[node['parentRole']] if node['parentRole'] else Matrix.Identity(4)
            rotation=(Matrix.Rotation(math.radians(37),4,'Z') if role=='yaw' else
                      Matrix.Rotation(math.radians(-8),4,'Y') if role=='pitch' else Matrix.Identity(4))
            worlds[role]=parent @ Matrix.Translation(Vector(node['locationRelativeToParentCm'])*CM_TO_METERS) @ rotation
        origins={mesh['role']:Vector(mesh['meshOriginInSourceCm']) for mesh in descriptor['meshes']}
        roles={name:mesh['role'] for mesh in descriptor['meshes'] for name in mesh['objectNames']}
        roles.update({a['sourceObjectName']:a['parentRole'] for a in descriptor['ammo']['instances']})
        worst=0.0
        for name,source in objects.items():
            obj=bpy.data.objects[name]
            role=roles[name]
            for vertex,source_vertex in zip(obj.data.vertices,source['vertices']):
                expected=worlds[role] @ ((Vector(source_vertex)-origins[role])*CM_TO_METERS)
                worst=max(worst,((obj.matrix_world @ vertex.co)-expected).length/CM_TO_METERS)
        require(worst<0.001,'Articulation differs from descriptor hierarchy')
        return {'yawDegrees':37,'blenderPitchYDegrees':-8,'maxVertexErrorCm':worst,
                'baseFixedAndAmmoFollowsYaw':True}
    finally:
        yaw.rotation_euler=old_yaw
        pitch.rotation_euler=old_pitch
        bpy.context.view_layer.update()


def main():
    require(bpy.app.background,'Run only in a new background Blender process, not the interactive editor')
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--render',action='store_true')
    parser.add_argument('--verify-only',action='store_true',help='Reopen and audit an existing file without saving it')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    output=args.output.resolve()
    require(output.suffix.lower()=='.blend','Output must be .blend')
    require(output.exists() if args.verify_only else not output.exists(),
            'Verify requires an existing file; export refuses to overwrite one')
    require(SOURCE_DIR not in output.parents,'Keep generated Blender files outside source directory')
    descriptor,palette,objects=load_inputs()
    output.parent.mkdir(parents=True,exist_ok=True)
    before=None
    if not args.verify_only:
        build_scene(descriptor,palette,objects)
        before=validate(descriptor,palette,objects)
        preferences=getattr(bpy.context,'preferences',None)
        if preferences is None:
            preferences=bpy.context.user_preferences
        preferences.filepaths.save_version=0
        bpy.ops.wm.save_as_mainfile(filepath=str(output),check_existing=False)
    bpy.ops.wm.open_mainfile(filepath=str(output))
    bpy.context.view_layer.update()
    after=validate(descriptor,palette,objects)
    motion=validate_motion(descriptor,objects)
    validate(descriptor,palette,objects)
    report={'status':'passed','blenderVersion':bpy.app.version_string,
            'output':str(output),'beforeSave':before,'reopened':after,'motion':motion,
            'blendSha256':hashlib.sha256(output.read_bytes()).hexdigest().upper()}
    if args.render:
        bpy.context.scene.render.image_settings.file_format='PNG'
        bpy.context.scene.render.filepath=str(output.with_suffix('.png'))
        bpy.ops.render.render(write_still=True)
        report['preview']=str(output.with_suffix('.png'))
    output.with_suffix('.audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('HARPOON_BLEND_RESULT '+json.dumps(report))


if __name__=='__main__':
    try:
        main()
    except Exception:
        import traceback
        traceback.print_exc()
        raise
