"""Run in Unreal via the shared queue. Tests actual compiled Blueprint logic.

The Voyage provider supplies test-only resource values. This does not validate
native game transfer, DML discovery, attachment ordering or persistence.
"""
import json
from pathlib import Path
import unreal

report_path = Path(__file__).resolve().parents[3] / 'Tmp/SmartOutlet/editor-validation.json'
controller_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/BP_OutletController.BP_OutletController_C')
assert controller_class, 'Controller class missing'
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
defaults = unreal.get_default_object(controller_class)
module = unreal.new_object(unreal.VoyageModuleComponent)
port = unreal.new_object(unreal.VoyageModuleSocketViewComponent)
bandwidth = unreal.ModuleSocketBandwidth(input=321.0, output=987.0, item_bandwidth_in_kg=11.0)
port_data = port.get_editor_property('port')
port_data.set_editor_property('bandwidth', bandwidth)
port.set_editor_property('port', port_data)
# Seed the struct default for this disposable process only; no asset is saved.
# CDO editing propagates to instances, so do not reset it while testing them.
defaults.set_editor_property('OriginalBandwidth', bandwidth)
controller = actors.spawn_actor_from_class(controller_class, unreal.Vector())
system = unreal.get_default_object(unreal.SystemLibrary)
rule_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/Settings/BP_OutletRule.BP_OutletRule_C')
assert rule_class, 'Rule class missing'
rule = unreal.new_object(rule_class, outer=controller)
system.call_method('SetObjectPropertyByName', (controller, 'Rule', rule))
system.call_method('SetObjectPropertyByName', (controller, 'Module', module))
system.call_method('SetObjectPropertyByName', (controller, 'Port', port))
system.call_method('SetBoolPropertyByName', (controller, 'Initialized', True))
glow = unreal.load_asset('/Game/Mods/SmartOutlet/M_PortGlow')
assert glow, 'Glow material missing'
assert unreal.MaterialEditingLibrary.get_material_default_scalar_parameter_value(glow, 'GlowStrength') == 2.0
original_material = unreal.load_asset('/Engine/EngineMaterials/DefaultMaterial')
assert original_material
mesh = unreal.new_object(unreal.StaticMeshComponent, outer=controller)
mesh.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
mesh.set_material(0, original_material)
system.call_method('SetObjectPropertyByName', (port, 'mesh', mesh))
system.call_method('SetObjectPropertyByName', (controller, 'OriginalMaterial', original_material))
module.set_editor_property('editor_validation_capacity', 6000.0)
results = []
refreshes = 0
previous_enabled = None
for amount, enabled in [(0, False), (4799, False), (4800, True), (3000, True), (1201, True), (1200, False), (3000, False), (6000, True), (0, False)]:
    module.set_editor_property('editor_validation_amount', float(amount))
    controller.call_method('Refresh')
    current = port.get_editor_property('port')
    limits = current.get_editor_property('bandwidth')
    actual = bool(controller.get_editor_property('enabled'))
    unreal.log('Test step: ' + str((amount, actual, controller.get_editor_property('HasApplied'), current)))
    assert actual == enabled, (amount, actual, enabled)
    assert limits.get_editor_property('input') == 0.0, 'Input must always be blocked'
    assert limits.get_editor_property('output') == (987.0 if enabled else 0.0), (amount, limits)
    assert limits.get_editor_property('item_bandwidth_in_kg') == 11.0, 'Untouched bandwidth field changed'
    assert current.get_editor_property('resource_type') == port_data.get_editor_property('resource_type'), 'Resource type changed'
    assert current.get_editor_property('type') == unreal.ModuleSocketType.ST_OUTPUT, 'Port is not output-only'
    indicator = controller.get_editor_property('Indicator')
    assert indicator and mesh.get_material(0) == indicator, 'Indicator not assigned to rim slot'
    assert indicator.get_editor_property('parent') == glow, 'Indicator uses a different material'
    tint = indicator.call_method('K2_GetVectorParameterValue', ('ColorTint',))
    expected_color = (0.02, 1.0, 0.02) if enabled else (1.0, 0.02, 0.02)
    assert all(abs(a-b) < 0.0001 for a,b in zip((tint.r,tint.g,tint.b), expected_color)), 'Wrong glow color'
    if previous_enabled is None or previous_enabled != enabled:
        refreshes += 1
    previous_enabled = enabled
    assert module.get_editor_property('editor_validation_network_refreshes') == refreshes, 'Changed limits did not notify the network exactly once'
    controller.call_method('ReconcileConnection')
    assert module.get_editor_property('editor_validation_network_refreshes') == refreshes, 'Unchanged reconciliation caused a notification loop'
    results.append({'stored': amount, 'enabled': actual, 'input': 0, 'output': limits.get_editor_property('output')})

# A stock reconnection can replace a limit without changing our desired state.
changed = port.get_editor_property('port')
changed_limits = changed.get_editor_property('bandwidth')
changed_limits.set_editor_property('output', 123.0)
changed.set_editor_property('bandwidth', changed_limits)
port.set_editor_property('port', changed)
controller.call_method('ReconcileConnection')
assert port.get_editor_property('port').get_editor_property('bandwidth').get_editor_property('output') == 0.0
assert module.get_editor_property('editor_validation_network_refreshes') == refreshes + 1, 'Repaired limit did not notify the network'

# A second independent controller must not inherit the first storage's state.
other = actors.spawn_actor_from_class(controller_class, unreal.Vector())
assert not other.get_editor_property('enabled'), 'State leaked between storage controllers'
assert not other.get_editor_property('initialized'), 'Binding leaked between storage controllers'
actors.destroy_actor(other)
controller.call_method('Cleanup')
assert mesh.get_material(0) == original_material, 'Cleanup did not restore the original rim material'
actors.destroy_actor(controller)
with open(report_path, 'w', encoding='utf-8') as f:
    json.dump({'status': 'passed', 'cases': results, 'networkRefreshes': refreshes + 1, 'glowStrength': 2.0, 'indicatorColorsAndRestore': 'passed', 'boundary': 'Compiled Blueprint against editor mirrors, not native game flow or in-game brightness'}, f, indent=2)
unreal.log('SmartOutlet editor validation PASSED')
