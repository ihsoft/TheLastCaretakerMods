"""Import through Unreal Interchange, preserve a rigid hierarchy in an Actor BP."""
import json
import math
import os
from pathlib import Path
import re
import sys
import unreal

sys.dont_write_bytecode = True


def matrix(component,world=False):
    transform = unreal.Transform(location=component.get_world_location(),rotation=component.get_world_rotation(),
        scale=component.get_world_scale()) if world else unreal.Transform(
        location=component.get_editor_property('relative_location'),
        rotation=component.get_editor_property('relative_rotation'),
        scale=component.get_editor_property('relative_scale3d'))
    origin = transform.transform_location(unreal.Vector())
    axes = [transform.transform_location(v) - origin for v in
            [unreal.Vector(1,0,0),unreal.Vector(0,1,0),unreal.Vector(0,0,1)]]
    return [[axes[0].x,axes[1].x,axes[2].x,origin.x],
            [axes[0].y,axes[1].y,axes[2].y,origin.y],
            [axes[0].z,axes[1].z,axes[2].z,origin.z],[0,0,0,1]]


def same_matrix(left,right,label):
    if any(not math.isclose(a,b,rel_tol=1e-5,abs_tol=0.002) for x,y in zip(left,right) for a,b in zip(x,y)):
        raise ValueError(f'Transform/pivot differs from GLB: {label}: {left} != {right}')


def tag_name(component):
    return next((str(x)[len('GLB.Name:'):] for x in component.component_tags if str(x).startswith('GLB.Name:')),None)


def verify(request, blueprint):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cls = unreal.EditorAssetLibrary.load_blueprint_class(blueprint)
    if not cls:
        raise ValueError('Imported Blueprint could not be loaded')
    actor = subsystem.spawn_actor_from_class(cls,unreal.Vector())
    try:
        components = {tag_name(c):c for c in actor.get_components_by_class(unreal.SceneComponent) if tag_name(c) is not None}
        if set(components) != {n['name'] for n in request['nodes']}:
            raise ValueError('Saved Blueprint node set differs from the selected GLB scene')
        records=[]
        source_meshes={}
        for node in request['nodes']:
            component=components[node['name']]
            parent=component.get_attach_parent()
            actual_parent=tag_name(parent) if parent else None
            if actual_parent != node['parent']:
                raise ValueError(f'Changed hierarchy: {node["name"]}')
            same_matrix(matrix(component),node['localMatrix'],node['name'])
            same_matrix(matrix(component,world=True),node['worldMatrix'],node['name']+' world')
            if component.mobility != unreal.ComponentMobility.MOVABLE:
                raise ValueError(f'Component cannot move: {node["name"]}')
            mesh=None
            materials=[]
            if node['mesh'] is not None:
                if not isinstance(component,unreal.StaticMeshComponent) or not component.static_mesh:
                    raise ValueError(f'Missing imported mesh: {node["name"]}')
                mesh=component.static_mesh.get_path_name()
                if not mesh.startswith(request['destination']+'/'):
                    raise ValueError('Mesh escaped the selected destination')
                if node['mesh'] in source_meshes and source_meshes[node['mesh']] != mesh:
                    raise ValueError('A shared source mesh was split into unrelated assets')
                source_meshes[node['mesh']]=mesh
                for i in range(component.get_num_materials()):
                    material=component.get_material(i)
                    if material is None:
                        raise ValueError(f'Missing material: {node["name"]}/{i}')
                    materials.append(material.get_path_name())
            records.append({'sourceIndex':node['index'],'name':node['name'],'component':component.get_name(),
                            'parent':actual_parent,'localMatrix':matrix(component),'worldMatrix':matrix(component,world=True),
                            'mesh':mesh,'materials':materials})
        return records
    finally:
        subsystem.destroy_actor(actor)


def do_import(request):
    destination=request['destination']
    if unreal.EditorAssetLibrary.does_directory_exist(destination):
        raise ValueError('Destination already exists')
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    parameters=unreal.ImportAssetParameters()
    parameters.is_automated=True
    parameters.replace_existing=False
    parameters.import_level=unreal.EditorLevelUtils.get_levels(world)[0]
    manager=unreal.InterchangeManager.get_interchange_manager_scripted()
    if not manager.import_scene(destination,manager.create_source_data(request['source']),parameters):
        raise ValueError('Unreal Interchange scene import failed')
    localize_materials(destination)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    names={n['name'] for n in request['nodes']}
    selected={}
    for actor in actors:
        label=actor.get_actor_label()
        if label in names:
            if label in selected:
                raise ValueError(f'Ambiguous imported node: {label}')
            selected[label]=actor
        elif isinstance(actor,unreal.StaticMeshActor):
            raise ValueError(f'Unmapped imported mesh: {label}')
    if set(selected)!=names:
        raise ValueError(f'Imported hierarchy lost named nodes: {names-set(selected)}')
    # Before creating a Blueprint, verify that Interchange preserved source frames.
    for node in request['nodes']:
        actor=selected[node['name']]
        parent=actor.get_attach_parent_actor()
        actual_parent=parent.get_actor_label() if parent and parent.get_actor_label() in names else None
        if actual_parent!=node['parent']:
            raise ValueError(f'Interchange changed parent: {node["name"]}')
        same_matrix(matrix(actor.root_component),node['localMatrix'],node['name'])
        same_matrix(matrix(actor.root_component,world=True),node['worldMatrix'],node['name']+' world')

    factory=unreal.BlueprintFactory()
    factory.set_editor_property('parent_class',unreal.Actor)
    bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset('BP_Model',destination,unreal.Blueprint,factory)
    subsystem=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library=unreal.SubobjectDataBlueprintFunctionLibrary
    handles=subsystem.k2_gather_subobject_data_for_blueprint(bp)
    root=handles[1]
    root_object=library.get_object_for_blueprint(library.get_data(root),bp)
    root_object.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
    pending={n['name']:n for n in request['nodes']}
    created={}
    while pending:
        ready=[n for n in pending.values() if n['parent'] is None or n['parent'] in created]
        if not ready:
            raise ValueError('Cannot construct the selected hierarchy')
        for node in ready:
            source_component=selected[node['name']].root_component
            parent=created.get(node['parent'],root)
            options=unreal.AddNewSubobjectParams(parent_handle=parent,
                new_class=unreal.StaticMeshComponent if node['mesh'] is not None else unreal.SceneComponent,
                blueprint_context=bp,skip_mark_blueprint_modified=True)
            handle,error=subsystem.add_new_subobject(options)
            if not library.is_handle_valid(handle):
                raise ValueError(f'Cannot create {node["name"]}: {error}')
            variable=f'N{node["index"]:04d}_'+re.sub(r'[^A-Za-z0-9_]','_',node['name'])
            if not subsystem.rename_subobject(handle,unreal.Text(variable)):
                raise ValueError(f'Cannot name component {variable}')
            if not subsystem.attach_subobject(parent,handle):
                raise ValueError(f'Cannot attach component {variable}')
            component=library.get_object_for_blueprint(library.get_data(handle),bp)
            component.set_editor_property('component_tags',[unreal.Name('GLB.Name:'+node['name'])])
            component.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
            for field in ['relative_location','relative_rotation','relative_scale3d']:
                component.set_editor_property(field,source_component.get_editor_property(field))
            if node['mesh'] is not None:
                component.set_static_mesh(source_component.static_mesh)
                component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                for i in range(source_component.get_num_materials()):
                    component.set_material(i,source_component.get_material(i))
            created[node['name']]=handle
            del pending[node['name']]
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not unreal.EditorAssetLibrary.save_directory(destination,False,True):
        raise ValueError('Could not save imported assets')
    return bp.get_path_name()


def dependency_options():
    return unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,
        include_soft_package_references=True,include_searchable_names=False,
        include_hard_management_references=False,include_soft_management_references=False)


def localize_materials(destination):
    # GLTF's stock parents are editor-plugin content, unavailable in the game.
    # Copy the used dependency closure with native reference remapping, then
    # reparent the imported instances. Never consolidate/delete engine originals.
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    materials=[unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(destination,True,False)
               if isinstance(unreal.load_asset(p),unreal.MaterialInstanceConstant)]
    pending=[m.parent.get_path_name().split('.')[0] for m in materials if m.parent]
    copies={}
    while pending:
        package=pending.pop()
        if package.startswith(('/Engine/','/Script/',destination+'/')) or package in copies:
            continue
        if not package.startswith('/InterchangeAssets/'):
            raise ValueError(f'Unsupported external material dependency: {package}')
        copies[package]=destination+'/MaterialSupport/'+package.lstrip('/')
        pending.extend(str(p) for p in registry.get_dependencies(package,dependency_options()))
    if copies:
        if not unreal.GlbImportLibrary.copy_material_dependencies(copies):
            raise ValueError('Could not copy GLTF material dependencies into the project')
        registry.scan_paths_synchronous([destination+'/MaterialSupport'],True)
        for material in materials:
            if material.parent:
                package=material.parent.get_path_name().split('.')[0]
                if package in copies:
                    parent=unreal.load_asset(copies[package])
                    if not parent:
                        raise ValueError(f'Missing copied material parent: {package}')
                    unreal.MaterialEditingLibrary.set_material_instance_parent(material,parent)


def inventory(request):
    destination=request['destination']
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([destination],True)
    options=dependency_options()
    records=[]
    for path in sorted(unreal.EditorAssetLibrary.list_assets(destination,True,False)):
        asset=unreal.load_asset(path)
        kind=asset.get_class().get_name()
        record={'asset':path,'type':kind,'editorOnly':kind=='InterchangeSceneImportAsset'}
        if not record['editorOnly']:
            dependencies=sorted(str(p) for p in registry.get_dependencies(path.split('.')[0],options))
            escaped=[p for p in dependencies if not p.startswith((destination+'/', '/Engine/', '/Script/'))]
            if escaped:
                raise ValueError(f'Imported resource has dependencies outside its destination: {path}: {escaped}')
            record['dependencies']=dependencies
        if isinstance(asset,unreal.MaterialInterface):
            record['textures']=sorted(t.get_path_name() for t in unreal.MaterialEditingLibrary.get_material_used_textures(asset))
        records.append(record)
    return records


request=json.loads(Path(os.environ['UNREAL_GLB_REQUEST']).read_text(encoding='utf-8'))
phase=os.environ['UNREAL_GLB_PHASE']
output=Path(request['output'])
try:
    blueprint=do_import(request) if phase=='import' else request['destination']+'/BP_Model.BP_Model'
    if phase=='reopen':
        unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    nodes=verify(request,blueprint)
    assets=inventory(request)
    report={'status':'passed','blueprint':blueprint,'nodes':nodes,'assets':assets}
    if phase=='reopen':
        previous=json.loads((output/'import.json').read_text(encoding='utf-8'))
        if report!=previous:
            raise ValueError('Fresh-process model readback differs from the imported state')
    (output/(phase+'.json')).write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
except Exception as error:
    (output/(phase+'.json')).write_text(json.dumps({'status':'failed','error':str(error)},indent=2)+'\n',encoding='utf-8')
    raise
