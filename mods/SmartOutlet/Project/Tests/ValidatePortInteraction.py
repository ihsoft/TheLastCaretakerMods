"""Runtime mode action: compiled graph checks against editor-only native mirrors."""
import json
import os
from pathlib import Path
import unreal

root = Path(os.environ['SMARTOUTLET_TEST_OUTPUT'])
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
system = unreal.get_default_object(unreal.SystemLibrary)
spawned = []

def spawn(path):
    cls = unreal.load_class(None, path)
    obj = actors.spawn_actor_from_class(cls, unreal.Vector())
    assert obj, path
    spawned.append(obj)
    return obj

def assign(obj, name, value):
    system.call_method('SetObjectPropertyByName', (obj, name, value))

try:
    controller = spawn('/Game/Mods/SmartOutlet/BP_OutletController.BP_OutletController_C')
    manager = spawn('/Game/Mods/SmartOutlet/ModActor.ModActor_C')
    # Exercise the compiled configuration operation, independently of Slate.
    # The mirror captures the class as SetActions is called, checking ordering.
    hint = unreal.new_object(unreal.VoyageButtonInfoContainerWidget, outer=manager)
    assign(manager, 'ModeHint', hint)
    card_class = unreal.load_class(None,
        '/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical.BP_ButtonInfoDetailed_Vertical_C')
    assert card_class
    manager.call_method('ConfigureModeHint', ('Normal',))
    assert hint.get_editor_property('ButtonInfoWidgetClass') == card_class
    assert hint.get_editor_property('EditorConfiguredClass') == card_class, 'SetActions ran before card configuration'
    assert hint.get_editor_property('EditorHintText') == 'Normal'
    # Unreal arrays returned by reflection wrap the live property storage.
    manager.get_editor_property('Controllers').append(controller)
    # Controllers are NotPlaceable: use the game spawn API, not editor placement.
    gameplay = unreal.get_default_object(unreal.GameplayStatics)
    player = gameplay.call_method('BeginDeferredActorSpawnFromClass', (
        manager, unreal.VoyagePlayerController.static_class(), unreal.Transform(),
        unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None,
        unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert player
    gameplay.call_method('FinishSpawningActor', (
        player, unreal.Transform(), unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    spawned.append(player)
    detector = unreal.new_object(unreal.InteractiveDetectorPointerComponent, outer=manager)
    port = unreal.new_object(unreal.VoyageModuleSocketViewComponent, outer=controller)
    box = unreal.new_object(unreal.BoxComponent, outer=controller)
    mesh = unreal.new_object(unreal.StaticMeshComponent, outer=controller)
    unrelated = unreal.new_object(unreal.SceneComponent, outer=controller)
    assign(port, 'SocketCollisionBox', box)
    assign(port, 'mesh', mesh)
    assign(controller, 'Port', port)
    assign(manager, 'LocalPlayer', player)
    assign(manager, 'HoverDetector', detector)
    assert list(manager.get_editor_property('Controllers')) == [controller]

    def hover(component):
        assign(detector, 'EditorHoverReference', component)
        manager.call_method('RefreshSelectedPort')
        return manager.get_editor_property('SelectedPort')

    assert hover(None) is None, 'Null hover selected a port'
    for component in (port, box, mesh):
        assert hover(component) == controller, 'Exact port component was not matched'
    assert hover(unrelated) is None, 'Unrelated component retained the old selection'
    assign(controller, 'Port', None)
    assert hover(box) is None, 'Uninitialized port matched'
    assign(controller, 'Port', port)
    rule = unreal.new_object(unreal.load_class(None,
        '/Game/Mods/SmartOutlet/Settings/BP_OutletRule.BP_OutletRule_C'))
    module = unreal.new_object(unreal.VoyageModuleComponent, outer=controller)
    module.set_editor_property('editor_validation_amount', 3000.0)
    for key, value in [('Rule', rule), ('Module', module), ('Storage', manager)]:
        assign(controller, key, value)
    system.call_method('SetStringPropertyByName', (controller, 'SocketName', port.get_name()))
    system.call_method('SetBoolPropertyByName', (controller, 'Initialized', True))
    profile = unreal.new_object(unreal.load_class(None, '/Game/Mods/SmartOutlet/Settings/BP_StorageProfile.BP_StorageProfile_C'))
    input_rule = unreal.new_object(rule.get_class())
    system.call_method('SetObjectPropertyByName', (profile, 'InputRule', input_rule))
    system.call_method('SetObjectPropertyByName', (profile, 'OutputRule', rule))
    system.call_method('SetObjectPropertyByName', (controller, 'Profile', profile))
    system.call_method('SetBoolPropertyByName', (input_rule, 'IsInput', True))
    manager.get_editor_property('tags').append('SmartOutlet.Mode.v1.'+port.get_name()+'.output')
    controller.call_method('RestoreMode')
    controller.call_method('ApplyMode')
    assert controller.get_editor_property('Mode') == unreal.ModuleSocketType.ST_OUTPUT, 'Saved output selection not restored'
    hover(box)
    manager.call_method('HandleModeInput')
    assert controller.get_editor_property('Mode') == unreal.ModuleSocketType.ST_ANY
    manager.call_method('HandleModeInput')
    assert controller.get_editor_property('Mode') == unreal.ModuleSocketType.ST_ANY, 'Holding repeated the action'
    manager.call_method('ReleaseModeInput')
    hover(unrelated)
    manager.call_method('HandleModeInput')
    assert controller.get_editor_property('Mode') == unreal.ModuleSocketType.ST_ANY, 'Off-port input changed a mode'
    manager.call_method('ReleaseModeInput')
    hover(port)
    manager.call_method('HandleModeInput')
    assert controller.get_editor_property('Mode') == unreal.ModuleSocketType.ST_INPUT
    manager.call_method('ReleaseModeInput')
    system.call_method('SetBoolPropertyByName', (player, 'EditorHudActive', False))
    manager.call_method('HandleModeInput')
    assert controller.get_editor_property('Mode') == unreal.ModuleSocketType.ST_INPUT, 'Menu input changed a mode'
    manager.call_method('ReleaseModeInput')
    system.call_method('SetBoolPropertyByName', (player, 'EditorHudActive', True))
    # Execute runtime attachment against real UMG panel objects. Native focus
    # and HUD visibility are supplied by the editor-only controller fixture.
    widget_class = unreal.load_class(None,
        '/Game/Mods/SmartOutlet/WBP_PortModeAction.WBP_PortModeAction_C')
    widget = unreal.new_object(widget_class, outer=manager)
    host = unreal.new_object(unreal.HorizontalBox, outer=widget)
    assign(widget, 'HintHost', host)
    assign(manager, 'InteractionWidget', widget)
    host.add_child(hint)
    hud = unreal.new_object(unreal.VoyageInGameCharacterWidget, outer=player)
    row = unreal.new_object(unreal.HorizontalBox, outer=hud)
    stock_card = unreal.new_object(unreal.TextBlock, outer=hud)
    row.add_child(stock_card)
    assign(hud, 'CentralInputActionContainer', row)
    assign(player, 'EditorFocusWidget', hud)
    manager.call_method('AttachModeHintToHud')
    assert host.get_parent() == row and row.get_children_count() == 2
    manager.call_method('AttachModeHintToHud')
    assert row.get_children_count() == 2, 'Repeated refresh duplicated the hint'
    row.clear_children()  # Native HUD rebuilds the action row this way.
    row.add_child(stock_card)
    manager.call_method('AttachModeHintToHud')
    assert host.get_parent() == row and stock_card.get_parent() == row
    assert row.get_children_count() == 2
    replacement_hud = unreal.new_object(unreal.VoyageInGameCharacterWidget, outer=player)
    replacement_row = unreal.new_object(unreal.HorizontalBox, outer=replacement_hud)
    assign(replacement_hud, 'CentralInputActionContainer', replacement_row)
    assign(player, 'EditorFocusWidget', replacement_hud)
    manager.call_method('AttachModeHintToHud')
    assert host.get_parent() == replacement_row
    assert row.get_children_count() == 1 and stock_card.get_parent() == row
    assign(player, 'EditorFocusWidget', None)
    manager.call_method('AttachModeHintToHud')
    assert host.get_parent() is None, 'Unavailable HUD retained an old attachment'
    assign(player, 'EditorFocusWidget', hud)
    assign(hud, 'CentralInputActionContainer', None)
    manager.call_method('AttachModeHintToHud')
    assert host.get_parent() is None, 'Missing action row was not handled'
    assign(hud, 'CentralInputActionContainer', row)
    assert hover(port) == controller
    assert player.get_editor_property('EditorHudActive')
    assert manager.get_editor_property('InteractionWidget') == widget
    manager.call_method('RefreshPortInteraction')
    assert host.get_parent() == row, (
        f'UI refresh did not attach: selected={manager.get_editor_property("SelectedPort")}, '
        f'parent={host.get_parent()}, visible={host.is_visible()}')
    # No Slate tree is built in this commandlet. IsVisible() requires a cached
    # SWidget; inspect the visibility policy written by the graph instead.
    assert host.get_visibility() == unreal.SlateVisibility.HIT_TEST_INVISIBLE
    assert hint.get_editor_property('EditorHintText') == 'Set smart output 80/20 (now: smart input 20/80)'
    before_updates = hint.get_editor_property('EditorHintUpdates')
    manager.call_method('RefreshPortInteraction')
    assert hint.get_editor_property('EditorHintUpdates') == before_updates, 'Hint rebuilt every UI tick'
    manager.call_method('HandleModeInput')
    assert hint.get_editor_property('EditorHintText') == 'Set normal (now: smart output 80/20)', 'V did not update mode hint immediately'
    manager.call_method('ReleaseModeInput')
    system.call_method('SetBoolPropertyByName', (player, 'EditorHudActive', False))
    manager.call_method('RefreshPortInteraction')
    assert host.get_visibility() == unreal.SlateVisibility.COLLAPSED, 'Hint visible over a menu'
    system.call_method('SetBoolPropertyByName', (player, 'EditorHudActive', True))
    hover(unrelated)
    manager.call_method('RefreshPortInteraction')
    assert host.get_visibility() == unreal.SlateVisibility.COLLAPSED, 'Hint visible away from configured port'
    manager.call_method('StopPortInteraction')
    assert host.get_parent() is None and stock_card.get_parent() == row
    assert row.get_children_count() == 1, 'Cleanup changed stock cards'
    (root / 'port-interaction-validation.json').write_text(json.dumps({
        'status': 'passed',
        'checks': ['card class assigned before SetActions', 'class readback',
                   'current mode text', 'no redundant hint recreation', 'null hover', 'port/box/mesh identity', 'stale selection cleared',
                   'uninitialized port ignored', 'one mode change per press',
                   'off-port input rejected', 'second press accepted',
                   'menu input rejected', 'attach to native row', 'no duplicate attachment',
                   'reattach after native row clear', 'HUD replacement', 'unavailable HUD/row detach',
                   'menu and off-port hide', 'cleanup preserves stock cards'],
        'boundary': 'Editor mirrors only; real hover selection, input dispatch, stock key widget and menu visibility need game validation'
    }, indent=2) + '\n', encoding='utf-8')
finally:
    for actor in reversed(spawned):
        actors.destroy_actor(actor)
