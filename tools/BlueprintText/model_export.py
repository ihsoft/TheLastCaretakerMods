"""Internal Unreal Python worker; request/output live beside the run-local copy."""
import json
from pathlib import Path

import unreal


def export_model(out):
    asset = json.loads((out / 'request.json').read_text(encoding='utf-8'))['asset']
    source = unreal.load_asset(asset)
    if not source:
        raise ValueError('Cannot load saved asset: ' + asset)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    spawned = []
    copies = set()
    components = []

    def spawn(cls):
        actor = actors.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        if not actor:
            raise ValueError('Cannot instantiate model: ' + asset)
        spawned.append(actor)
        return actor

    def visible(component):
        return component.is_visible() and not component.get_editor_property('hidden_in_game')

    try:
        if isinstance(source, unreal.StaticMesh):
            actor = spawn(unreal.StaticMeshActor)
            actor.static_mesh_component.set_static_mesh(source)
        elif isinstance(source, unreal.Blueprint):
            cls = unreal.load_class(None, source.get_path_name() + '_C')
            if not cls or not isinstance(unreal.get_default_object(cls), unreal.Actor):
                raise ValueError('ModelPreview supports Actor Blueprints, not this Blueprint type: ' + asset)
            actor = spawn(cls)
        else:
            raise ValueError('ModelPreview supports Static Mesh or Actor Blueprint assets: ' + asset)
        if actor.get_editor_property('hidden'):
            raise ValueError('Selected actor is hidden in game: ' + asset)
        for component in actor.get_components_by_class(unreal.SceneComponent):
            if not visible(component):
                continue
            if isinstance(component, unreal.ChildActorComponent):
                if component.get_editor_property('child_actor_class'):
                    raise ValueError('Child Actor geometry is unsupported: ' + component.get_name())
            elif isinstance(component, unreal.StaticMeshComponent):
                mesh = component.get_editor_property('static_mesh')
                if not mesh:
                    continue
                if isinstance(component, (unreal.InstancedStaticMeshComponent, unreal.SplineMeshComponent)):
                    raise ValueError('Instanced or spline-deformed geometry is unsupported: ' + component.get_name())
                # Unreal and glTF compose nested non-uniform scale differently.
                # Flatten evaluated Unreal world transforms on temporary actors.
                copy = spawn(unreal.StaticMeshActor)
                copy.set_actor_label(component.get_name())
                copy.static_mesh_component.set_static_mesh(mesh)
                copy.set_actor_transform(component.get_world_transform(), False, True)
                copies.add(copy)
                components.append(dict(component=component.get_name(), mesh=mesh.get_path_name(),
                                       transform=str(component.get_world_transform())))
            elif isinstance(component, unreal.MeshComponent) or component.get_class().get_name() in (
                    'NiagaraComponent', 'ParticleSystemComponent', 'BillboardComponent',
                    'TextRenderComponent', 'DecalComponent'):
                raise ValueError('Unsupported visible model component: ' + component.get_name() +
                                 ' (' + component.get_class().get_name() + ')')
        if not copies:
            raise ValueError('No visible Static Mesh components: ' + asset)
        options = unreal.GLTFExportOptions()
        options.set_editor_property('export_source_model', True)
        options.set_editor_property('default_level_of_detail', 0)
        options.set_editor_property('bake_material_inputs', unreal.GLTFMaterialBakeMode.DISABLED)
        options.set_editor_property('texture_image_format', unreal.GLTFTextureImageFormat.NONE)
        options.set_editor_property('use_mesh_quantization', False)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        result = unreal.GLTFExporter.export_to_gltf(world, str(out / 'model.glb'), options, copies)
        if result is None or result is False or (isinstance(result, tuple) and not result[0]):
            raise ValueError('Unreal GLB export failed: ' + str(result))
        (out / 'export.json').write_text(json.dumps(dict(status='exported', asset=asset,
            meshInstances=len(copies), components=components, exportMessages=str(result)), indent=2), encoding='utf-8')
    finally:
        for actor in reversed(spawned):
            actors.destroy_actor(actor)


if __name__ == '__main__':
    out = Path(__file__).parent
    try:
        export_model(out)
    except Exception as exc:
        (out / 'export-error.json').write_text(json.dumps(dict(error=str(exc))), encoding='utf-8')
        raise
