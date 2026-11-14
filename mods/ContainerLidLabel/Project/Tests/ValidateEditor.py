"""Compiled shared type-library BPs; synthetic fixtures do not prove shipping timing."""
import json
import os
import traceback
from pathlib import Path
import unreal

# Refuse to create fixtures or write INI files in the maintained project.
# The runner supplies a fresh project below the mod's Tmp owner directory.
run_root = Path(os.environ['CONTAINER_LID_LABEL_TEST_RUN']).resolve()
project_root = Path(unreal.Paths.project_content_dir()).resolve().parent
assert any(p.name == 'ContainerLidLabel' and p.parent.name == 'Tmp' for p in run_root.parents), 'Use Test-ContainerLidLabel.ps1'
assert project_root == run_root / 'Project', 'Tests require the disposable project copy'
assert Path(__file__).resolve().parents[1] == project_root, 'Run the copied test script'
report = run_root / 'editor-validation.json'
editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
manager_class = unreal.load_class(None, '/Game/Mods/ContainerLidLabel/ModActor.ModActor_C')
probe_class = unreal.load_class(None, '/Game/Mods/ContainerLidLabel/BP_ContainerLidDisplay.BP_ContainerLidDisplay_C')
medium = unreal.load_class(None, '/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox.BP_Inventory_PlayerBox_C')
large = unreal.load_class(None, '/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large.BP_Inventory_PlayerBox_Large_C')
small = unreal.load_class(None, '/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Small.BP_Inventory_PlayerBox_Small_C')
types_class = unreal.load_class(None, '/Game/Mods/ContainerLidLabel/BFL_ContainerTypes.BFL_ContainerTypes_C')
assert all((manager_class, probe_class, medium, large, small, types_class))
# Reflection entry for testing static functions only; runtime callers use BFL calls.
types = unreal.get_default_object(types_class)
manager = editor.spawn_actor_from_class(manager_class, unreal.Vector())
targets, extra_managers = [], []
settings_path = Path(unreal.Paths.project_content_dir()).resolve() / 'Paks/LogicMods/ContainerLidLabel.ini'
settings_before = settings_path.read_bytes() if settings_path.exists() else None
settings_path.parent.mkdir(parents=True, exist_ok=True)
settings_path.write_text('LargeContainerMaxWeightKg=321\nMediumContainerMaxWeightKg=123\nSmallContainerMaxWeightKg=57\n', encoding='utf-8')


def prop(obj, name):
    return obj.get_editor_property(name)


def inventory(target):
    return target.get_component_by_class(unreal.VoyageInventoryWeightLimitedComponent)


def component(target, name):
    return next(c for c in target.get_components_by_class(unreal.SceneComponent) if c.get_name() == name)


def helpers():
    # Test observation only. The runtime manager has no registry or helper scan.
    return [actor for actor in editor.get_all_level_actors() if actor.get_class() == probe_class]


results = {'status': 'failed', 'boundary': 'Compiled BPs and synthetic editor actors; no shipping event ordering, bootstrap overlap, persistence or frame-time claim'}
try:
    descendants = []
    for name, parent in [('Medium', medium), ('Large', large), ('Small', small)]:
        blueprint = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(
            '/Game/ContainerLidLabelTests/Child' + name, parent)
        assert blueprint and unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        descendants.append(blueprint.generated_class())
    assert prop(manager, 'MediumClass') == medium
    # Editor Large is independent; shipping Large inherits Medium. Existing
    # fixtures use the Medium family to exercise the one shipping base-class query.
    # Two instances of each initial type exercise independent ownership without
    # turning lifecycle acceptance into an unmeasured population stress run.
    initial_count = 4
    restored_count = 6
    total_count = initial_count + restored_count
    for index in range(initial_count):
        kind = [small, medium][index % 2]
        targets.append(editor.spawn_actor_from_class(kind, unreal.Vector(index * 100, 0, 0)))
    unknown = []
    for kind in descendants:
        target = editor.spawn_actor_from_class(kind, unreal.Vector())
        targets.append(target)
        inventory(target).set_max_weight_limit(41)
        unknown.append(target)
    # Real Small destroys the stock right component after parent initialization.
    # Mirror that absence before the manager discovers existing/new Small actors.
    for target in targets:
        if unreal.MathLibrary.class_is_child_of(target.get_class(), small):
            component(target, 'LeftIconPlane').create_dynamic_material_instance(0)
            component(target, 'RightIconPlane').destroy_component(target)
    texture_a = unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
    texture_b = unreal.load_asset('/Engine/EngineResources/DefaultTexture')
    assert texture_a and texture_b and texture_a != texture_b
    icon_material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_IconFixture', '/Game/ContainerLidLabelTests', unreal.Material, unreal.MaterialFactoryNew())
    sample = unreal.MaterialEditingLibrary.create_material_expression(icon_material, unreal.MaterialExpressionTextureSampleParameter2D)
    sample.set_editor_property('parameter_name', 'IconTexture')
    sample.set_editor_property('texture', texture_a)
    unreal.MaterialEditingLibrary.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(icon_material)
    palette = unreal.new_object(unreal.VoyageIconPaletteData)
    palette.set_editor_property('IconPalette', [texture_a, texture_a, texture_b])
    inventory(targets[0]).set_editor_property('IconPaletteData', palette)
    source_left = component(targets[0], 'LeftIconPlane')
    source_left.set_material(0, icon_material)
    material_left = source_left.create_dynamic_material_instance(0)
    assert material_left
    manager.call_method('Start')
    actual_capacities = tuple(prop(manager, name) for name in ('LargeCapacity', 'MediumCapacity', 'SmallCapacity'))
    assert actual_capacities == (321, 123, 57), actual_capacities
    assert len(helpers()) == initial_count, f'Existing helpers: {len(helpers())}'
    assert not manager.is_actor_tick_enabled()
    subsystem = prop(manager, 'PersistentSubsystem')
    assert unreal.SystemLibrary.is_valid(subsystem)
    # Exact type policy: both initial discovery and future Restored events
    # leave unknown descendants' capacity and presentation untouched.
    for kind in descendants:
        target = editor.spawn_actor_from_class(kind, unreal.Vector())
        targets.append(target)
        inventory(target).set_max_weight_limit(43)
        subsystem.test_restored(target)
        unknown.append(target)
    assert len(helpers()) == initial_count
    for index, target in enumerate(unknown):
        assert prop(inventory(target), 'MaxWeightLimit') == (41 if index < 3 else 43)
        assert not inventory(target).test_has_presentation_bindings()
        assert not subsystem.test_target_has_end_play_bindings(target)
        for predicate in ('IsLarge', 'IsMedium', 'IsSmall'):
            assert not types.call_method(predicate, (target, manager))
    # Every subsequently restored actor is new, never a second Restored event
    # for the same instance. Model both native paths: Restored-only and the
    # public spawn pair Restored -> Spawned. The latter still creates one helper.
    settings_path.write_text('LargeContainerMaxWeightKg=999\nMediumContainerMaxWeightKg=999\nSmallContainerMaxWeightKg=77\n', encoding='utf-8')
    for index in range(restored_count):
        is_small = index % 3 == 0
        is_large = index % 3 == 1
        kind = [small, large, medium][index % 3]
        target = editor.spawn_actor_from_class(kind, unreal.Vector((initial_count + index) * 100, 0, 0))
        targets.append(target)
        if is_small:
            component(target, 'LeftIconPlane').create_dynamic_material_instance(0)
            component(target, 'RightIconPlane').destroy_component(target)
        before = len(helpers())
        subsystem.test_restored(target)
        assert len(helpers()) == before + 1, 'Restored deferred initialization'
        assert prop(inventory(target), 'MaxWeightLimit') == (57 if is_small else 321 if is_large else 123)
        if index % 2 == 0:
            subsystem.test_spawned(target)
            assert len(helpers()) == before + 1, 'Spawned was also handled'
    assert len(helpers()) == total_count
    by_target = {}
    for helper in helpers():
        target = prop(helper, 'TargetActor')
        assert target not in by_target, 'Multiple helpers for one creation'
        by_target[target] = helper
        assert not helper.get_actor_enable_collision() and not helper.is_actor_tick_enabled()
        assert helper.get_owner() is None, 'Helper still owned by manager'
        root = prop(helper, 'root_component')
        assert root.get_attach_parent() == component(target, 'PlayerBox_Lid')
        is_large = target.get_class() == large
        is_small = target.get_class() == small
        assert types.call_method('IsLarge', (target, manager)) == is_large
        assert types.call_method('IsMedium', (target, manager)) == (target.get_class() == medium)
        assert types.call_method('IsSmall', (target, manager)) == is_small
        assert prop(inventory(target), 'MaxWeightLimit') == (57 if is_small else 321 if is_large else 123)
        assert manager.call_method('GetConfiguredWeight', (target,)) == (57 if is_small else 321 if is_large else 123)
        label = prop(helper, 'Label')
        assert label and str(prop(label, 'TestText')) == str(prop(inventory(target), 'Name'))
        # Begin helper play so its actual K2_DestroyActor invokes ReceiveEndPlay
        # and Cleanup in this editor world, which has no normal play lifecycle.
        subsystem.test_begin_play(helper)
    assert len(by_target) == total_count
    unsupported = editor.spawn_actor_from_class(unreal.Actor, unreal.Vector())
    targets.append(unsupported)
    subsystem.test_restored(unsupported)
    subsystem.test_restored(None)
    assert len(helpers()) == total_count
    for value in (None, unsupported):
        assert not types.call_method('IsLarge', (value, manager))
        assert not types.call_method('IsMedium', (value, manager))
        assert not types.call_method('IsSmall', (value, manager))
    target = targets[0]
    helper = by_target[target]
    inv = inventory(target)
    assert inv.test_has_presentation_bindings() and subsystem.test_target_has_end_play_bindings(target)
    inv.test_rename(unreal.Text('Руда №1'))
    assert str(prop(prop(helper, 'Label'), 'TestText')) == 'Руда №1'
    left, right = prop(helper, 'LidLeftIcon'), prop(helper, 'LidRightIcon')
    for left_index, right_index in [(1, 0), (0, 2), (1, 2), (0, 0)]:
        inv.test_icons(left_index, right_index)
        subsystem.test_flush_icon_loads()
        assert left.is_visible() == (left_index > 0) and right.is_visible() == (right_index > 0)
        assert left.get_material(0) == material_left
        assert right.get_material(0) == prop(helper, 'SmallRightMaterial')
        assert right.get_material(0) != material_left
        if right_index:
            assert right.get_material(0).get_texture_parameter_value('IconTexture') == [texture_a, texture_a, texture_b][right_index]
    own_right = right.get_material(0)
    inv.test_icons(1, 2)
    inv.test_icons(1, 1)
    subsystem.test_flush_icon_loads()
    assert right.get_material(0) == own_right and right.is_visible()
    assert own_right.get_texture_parameter_value('IconTexture') == texture_a
    inv.test_icons(1, 2)
    inv.test_icons(1, 0)
    subsystem.test_flush_icon_loads()
    assert not right.is_visible(), 'Late completion resurrected cleared icon'
    inv.test_icons(1, 99)
    subsystem.test_flush_icon_loads()
    assert not right.is_visible(), 'Out-of-range icon did not hide'
    inv.test_icons(1, 2)
    subsystem.test_flush_icon_loads()
    assert right.is_visible() and own_right.get_texture_parameter_value('IconTexture') == texture_b
    label_component = prop(helper, 'LidLabelWidget')
    before = label_component.get_world_location()
    component(target, 'PlayerBox_Lid').set_relative_rotation(unreal.Rotator(35, 0, 0), False, True)
    assert (label_component.get_world_location() - before).length() > 1
    subsystem.test_target_end_play(target)
    assert not unreal.SystemLibrary.is_valid(helper)
    assert not inv.test_has_presentation_bindings()
    assert not subsystem.test_target_has_end_play_bindings(target)
    assert len(helpers()) == total_count - 1
    # Stop only unsubscribes. Helpers have their own target lifetime.
    manager.call_method('Stop')
    assert len(helpers()) == total_count - 1
    after_stop = editor.spawn_actor_from_class(medium, unreal.Vector())
    targets.append(after_stop)
    subsystem.test_restored(after_stop)
    subsystem.test_spawned(after_stop)
    assert len(helpers()) == total_count - 1 and prop(inventory(after_stop), 'MaxWeightLimit') == 0
    # Capacity works without creating a lid display; display initialization never changes it.
    manager.call_method('ApplyCapacity', (after_stop,))
    assert prop(inventory(after_stop), 'MaxWeightLimit') == 123 and len(helpers()) == total_count - 1
    inventory(after_stop).set_max_weight_limit(17)
    visual_only = editor.spawn_actor_from_class(probe_class, unreal.Vector())
    assert visual_only.call_method('Initialize', (after_stop,))
    subsystem.test_begin_play(visual_only)
    assert prop(inventory(after_stop), 'MaxWeightLimit') == 17
    assert prop(visual_only, 'root_component').get_attach_parent() == component(after_stop, 'PlayerBox_Lid')
    for target in targets:
        subsystem.test_target_end_play(target)
        editor.destroy_actor(target)
    targets.clear()
    assert not helpers()
    assert prop(manager, 'LargeCapacity') == 321 and prop(manager, 'MediumCapacity') == 123 and prop(manager, 'SmallCapacity') == 57
    second = editor.spawn_actor_from_class(manager_class, unreal.Vector())
    extra_managers.append(second)
    second.call_method('Start')
    assert prop(second, 'LargeCapacity') == 999 and prop(second, 'MediumCapacity') == 999 and prop(second, 'SmallCapacity') == 77
    second.call_method('Stop')
    for contents in ['LargeContainerMaxWeightKg=1e99\nMediumContainerMaxWeightKg=0\nSmallContainerMaxWeightKg=0\n', 'LargeContainerMaxWeightKg=200\nMediumContainerMaxWeightKg=100\n', None]:
        if contents is None:
            settings_path.unlink()
        else:
            settings_path.write_text(contents, encoding='utf-8')
        check = editor.spawn_actor_from_class(manager_class, unreal.Vector())
        extra_managers.append(check)
        check.call_method('Start')
        assert prop(check, 'LargeCapacity') == 200 and prop(check, 'MediumCapacity') == 100 and prop(check, 'SmallCapacity') == 50
        check.call_method('Stop')
    results.update(status='passed', actors=total_count, synchronousInitialPass=initial_count,
                   restoredNewActors=restored_count, restoredThenSpawnedSingleHelper='passed',
                   restoredOnlyNewActor='passed', unknownDescendantsUntouched=6,
                   collisionDisabled='passed', nameDelegate='passed',
                   iconDelegateAndSharedMaterials='passed', attachment='passed',
                   immediateCapacity='passed', logicModsIni='passed',
                   instanceSnapshotAndFallbacks='passed', targetEndPlayDestroysHelper='passed',
                   helperEndPlayUnbindsInventoryAndTarget='passed', stopOnlyUnsubscribes='passed',
                   exclusiveSizePredicates='passed', capacityBySize='passed',
                   smallRightWithoutStockComponent='passed', asyncIconChangeAndClear='passed', nullAndUnrelatedPredicates='passed',
                   actorBasedConfiguredWeight='passed', capacityIndependentOfDisplay='passed',
                   displayDoesNotChangeCapacity='passed', tickAlwaysDisabled=True)
except Exception as error:
    results['error'] = repr(error)
    results['traceback'] = traceback.format_exc()
    raise
finally:
    for item in extra_managers + [manager]:
        if unreal.SystemLibrary.is_valid(item):
            item.call_method('Stop')
            editor.destroy_actor(item)
    for helper in helpers():
        helper.call_method('Cleanup')
        editor.destroy_actor(helper)
    for target in targets:
        if unreal.SystemLibrary.is_valid(target):
            editor.destroy_actor(target)
    if settings_before is None:
        settings_path.unlink(missing_ok=True)
    else:
        settings_path.write_bytes(settings_before)
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(results, indent=2), encoding='utf-8')
    unreal.SystemLibrary.quit_editor()
assert results['status'] == 'passed'
unreal.log('ContainerLidLabel shared type-library compiled Blueprint tests PASSED')
