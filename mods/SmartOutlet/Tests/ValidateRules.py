"""Compiled Blueprint checks for INI rules and non-electric resource control.

Run through the shared Unreal queue. Native mirrors do not simulate game flow.
"""
import json
from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[3]
report = root / 'Tmp/SmartOutlet/rules-validation.json'
parser_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/Settings/BP_IniFileParser.BP_IniFileParser_C')
rule_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/Settings/BP_OutletRule.BP_OutletRule_C')
controller_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/BP_OutletController.BP_OutletController_C')
manager_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/ModActor.ModActor_C')
assert all((parser_class, rule_class, controller_class, manager_class))
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
system = unreal.get_default_object(unreal.SystemLibrary)
parser = unreal.new_object(parser_class)
reader = unreal.get_default_object(unreal.VoyageEditorBlueprintFunctionLibrary)
before = reader.call_method('GetTestReadCount')
parser.call_method('Load', (str(root/'mods/SmartOutlet/Assets/SmartOutlet.ini'),))
rules = []
for i, expected in [(1, 'BP_Module_Battery'), (2, 'BP_Module_fresh_Water_Tank'), (3, 'BP_Module_salt_Water_Tank')]:
    rule = unreal.new_object(rule_class)
    assert rule.call_method('ReadSettings', (parser, i)), 'Default rule rejected'
    assert expected in rule.get_editor_property('ActorClass')
    assert rule.get_editor_property('SocketName') == ('VoyageModuleSocketView2' if i == 1 else 'VoyageModuleSocketView1')
    assert abs(rule.get_editor_property('HighFraction') - .8) < 1e-8
    assert abs(rule.get_editor_property('LowFraction') - .2) < 1e-8
    assert rule.get_editor_property('IsInput') == (i != 1)
    assert rule.get_editor_property('Direction') == (unreal.ModuleSocketType.ST_INPUT if i != 1 else unreal.ModuleSocketType.ST_OUTPUT)
    rules.append(rule)
assert reader.call_method('GetTestReadCount') == before + 1, 'Rule getters reread INI'
missing = unreal.new_object(rule_class)
assert not missing.call_method('ReadSettings', (parser, 5)), 'Missing rule accepted'

fixture = root/'Tmp/SmartOutlet/rules-invalid.ini'
fixture.write_text('Rule1ActorClass="/Script/Voyage.VoyageModuleActor"\nRule1SocketName="AlternativeSocket"\nRule1EnablePortAtPercent=30\nRule1DisablePortAtPercent=70\nRule1GlowMaterialSlot=-1\n', encoding='utf-8')
parser.call_method('Load', (str(fixture),))
assert not missing.call_method('ReadSettings', (parser, 1)), 'Reversed thresholds accepted'
fixture.write_text(fixture.read_text().replace('=30', '=75').replace('=70', '=25'), encoding='utf-8')
parser.call_method('Load', (str(fixture),))
assert missing.call_method('ReadSettings', (parser, 1)), 'Custom rule rejected'
assert missing.get_editor_property('SocketName') == 'AlternativeSocket'
assert missing.get_editor_property('GlowMaterialSlot') == -1
fixture.write_text(fixture.read_text() + 'Rule1Mode="both"\n', encoding='utf-8')
parser.call_method('Load', (str(fixture),))
assert not missing.call_method('ReadSettings', (parser, 1)), 'Unknown direction accepted'
fixture.write_text(fixture.read_text().replace('"both"', '"output"'), encoding='utf-8')
parser.call_method('Load', (str(fixture),))
assert missing.call_method('ReadSettings', (parser, 1))

# Explicit transition names invert with mode. Missing values use mode defaults;
# a reversed/equal range is rejected without sorting away the requested meaning.
for mode in ('input', 'output'):
    base = ('Rule1ActorClass="/Script/Voyage.VoyageModuleActor"\n'
            'Rule1SocketName="AlternativeSocket"\n'
            f'Rule1Mode="{mode}"\nRule1GlowMaterialSlot=-1\n')
    for enable, disable, valid in [(None, None, True),
                                   (25 if mode == 'input' else 75,
                                    75 if mode == 'input' else 25, True),
                                   (75 if mode == 'input' else 25,
                                    25 if mode == 'input' else 75, False),
                                   (50, 50, False)]:
        text = base
        if enable is not None:
            text += f'Rule1EnablePortAtPercent={enable}\nRule1DisablePortAtPercent={disable}\n'
        fixture.write_text(text, encoding='utf-8')
        parser.call_method('Load', (str(fixture),))
        probe = unreal.new_object(rule_class)
        assert bool(probe.call_method('ReadSettings', (parser, 1))) == valid, (mode, enable, disable)
        if valid:
            assert abs(probe.get_editor_property('HighFraction') - (.8 if enable is None else .75)) < 1e-8
            assert abs(probe.get_editor_property('LowFraction') - (.2 if enable is None else .25)) < 1e-8
fixture.unlink()

# The same controller must use FreshWater even when Electricity is always full.
module = unreal.new_object(unreal.VoyageModuleComponent)
port = unreal.new_object(unreal.VoyageModuleSocketViewComponent)
data = port.get_editor_property('port')
limits = unreal.ModuleSocketBandwidth(input=42.0, output=73.0, item_bandwidth_in_kg=9.0)
data.set_editor_property('resource_type', unreal.ModuleResourceType.FRESH_WATER)
data.set_editor_property('bandwidth', limits)
port.set_editor_property('port', data)
unreal.get_default_object(controller_class).set_editor_property('OriginalBandwidth', limits)
controller = actors.spawn_actor_from_class(controller_class, unreal.Vector())
for key, value in [('Rule', missing), ('Module', module), ('Port', port)]:
    system.call_method('SetObjectPropertyByName', (controller, key, value))
system.call_method('SetBoolPropertyByName', (controller, 'Initialized', True))
module.set_editor_property('editor_validation_amount', 6000.0)
module.set_editor_property('editor_validation_capacity', 6000.0)
module.set_editor_property('editor_validation_capacities', {unreal.ModuleResourceType.FRESH_WATER: 2000.0})
cases = []
for amount, enabled in [(0,False),(1499,False),(1500,True),(1000,True),(501,True),(500,False),(1000,False)]:
    module.set_editor_property('editor_validation_amounts', {unreal.ModuleResourceType.FRESH_WATER:float(amount)})
    controller.call_method('Refresh')
    assert bool(controller.get_editor_property('Enabled')) == enabled, (amount, enabled)
    live = port.get_editor_property('port')
    assert live.get_editor_property('resource_type') == unreal.ModuleResourceType.FRESH_WATER
    bandwidth = live.get_editor_property('bandwidth')
    assert bandwidth.get_editor_property('input') == 0
    assert bandwidth.get_editor_property('output') == (73.0 if enabled else 0.0)
    assert bandwidth.get_editor_property('item_bandwidth_in_kg') == 9.0
    assert not controller.get_editor_property('Indicator'), 'Visual disabled rule created an indicator'
    cases.append({'water':amount,'enabled':enabled})
controller.call_method('Cleanup')
actors.destroy_actor(controller)

# Execute the manager's actual file-load path in the isolated editor project.
# Never write to the installed game's settings here.
settings = Path(unreal.Paths.project_content_dir())/'Paks/LogicMods/SmartOutlet.ini'
assert not settings.exists(), 'Editor test would replace an existing settings file'
settings.parent.mkdir(parents=True, exist_ok=True)
manager = actors.spawn_actor_from_class(manager_class, unreal.Vector())
try:
    settings.write_text((root/'mods/SmartOutlet/Assets/SmartOutlet.ini').read_text(encoding='utf-8'), encoding='utf-8')
    count_before = reader.call_method('GetTestReadCount')
    manager.call_method('LoadRules')
    loaded = manager.get_editor_property('Rules')
    assert len(loaded) == 3, 'Manager did not load all rules'
    assert reader.call_method('GetTestReadCount') == count_before + 1
    # Match a real editor actor class using configuration, without stock asset stubs.
    system.call_method('SetStringPropertyByName', (loaded[0], 'ActorClass', '/Script/Voyage.VoyageModuleActor'))
    target = actors.spawn_actor_from_class(unreal.VoyageModuleActor, unreal.Vector())
    def add_instance_component(component_class):
        # Instance ownership survives editor property edits/reconstruction.
        return reader.call_method('AddTestComponent', (target, component_class.static_class()))
    test_module = add_instance_component(unreal.VoyageModuleComponent)
    test_ports = [add_instance_component(unreal.VoyageModuleSocketViewComponent) for _ in range(2)]
    assert test_module and all(test_ports)
    for test_port in test_ports:
        port_data = test_port.get_editor_property('port')
        port_data.set_editor_property('bandwidth', unreal.ModuleSocketBandwidth(input=15.0, output=25.0, item_bandwidth_in_kg=3.0))
        port_data.set_editor_property('resource_type', unreal.ModuleResourceType.FRESH_WATER)
        test_port.set_editor_property('port', port_data)
    system.call_method('SetStringPropertyByName', (loaded[0], 'SocketName', test_ports[1].get_name()))
    system.call_method('SetIntPropertyByName', (loaded[0], 'GlowMaterialSlot', -1))
    test_module.set_editor_property('editor_validation_amount', 0.0)
    test_module.set_editor_property('editor_validation_capacity', 2000.0)
    unrelated = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector())
    manager.call_method('ObserveStorage', (unrelated,))
    assert len(manager.get_editor_property('Controllers')) == 0
    manager.call_method('ObserveStorage', (target,))
    controlled = manager.get_editor_property('Controllers')
    assert len(controlled) == 1, 'Configured class not observed'
    assert controlled[0].get_editor_property('Rule') == loaded[0], 'Rule not assigned before finishing spawn'
    assert controlled[0].get_owner() == target
    controlled[0].call_method('Initialize')
    assert controlled[0].get_editor_property('Port') == test_ports[1], 'Configured socket selection ignored'
    assert controlled[0].get_editor_property('Initialized')
    assert test_ports[1].get_editor_property('port').get_editor_property('bandwidth').get_editor_property('output') == 0.0
    assert test_ports[0].get_editor_property('port').get_editor_property('bandwidth').get_editor_property('output') == 25.0, 'Other socket changed'
    manager.call_method('ObserveStorage', (target,))
    assert len(manager.get_editor_property('Controllers')) == 1, 'Duplicate controller'
    assert reader.call_method('GetTestReadCount') == count_before + 1, 'Discovery reread the file'

    # Add an input on the other port plus a conflicting later rule on that SAME
    # port. Two controllers must coexist; the duplicate must not take ownership.
    fixture.write_text('\n'.join(
        f'Rule{i}ActorClass="/Script/Voyage.VoyageModuleActor"\n'
        f'Rule{i}SocketName="{test_ports[0].get_name()}"\n'
        f'Rule{i}Mode="{mode}"\nRule{i}EnablePortAtPercent={20 if mode == "input" else 80}\n'
        f'Rule{i}DisablePortAtPercent={80 if mode == "input" else 20}\nRule{i}GlowMaterialSlot=-1'
        for i, mode in [(1, 'input'), (2, 'output')]), encoding='utf-8')
    parser.call_method('Load', (str(fixture),))
    assert loaded[1].call_method('ReadSettings', (parser, 1))
    assert loaded[2].call_method('ReadSettings', (parser, 2))
    fixture.unlink()
    discovery_reads = reader.call_method('GetTestReadCount')
    manager.call_method('ObserveStorage', (target,))
    pair = manager.get_editor_property('Controllers')
    assert len(pair) == 2, 'Multiple ports rejected or duplicate socket registered'
    incoming = pair[1]
    assert incoming.get_editor_property('Rule') == loaded[1], 'First rule did not win'
    # Disabled visuals must remain disabled even when there is a valid socket mesh.
    socket_mesh = unreal.new_object(unreal.StaticMeshComponent, outer=target)
    socket_mesh.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    system.call_method('SetObjectPropertyByName', (test_ports[0], 'mesh', socket_mesh))
    incoming.call_method('Initialize')
    assert incoming.get_editor_property('Port') == test_ports[0]
    assert incoming.get_editor_property('Initialized')
    pair_cases = []
    for amount, input_on, output_on in [(0,True,False),(1000,True,False),
            (1599,True,False),(1600,False,True),(1000,False,True),
            (401,False,True),(400,True,False),(1000,True,False)]:
        test_module.set_editor_property('editor_validation_amount', float(amount))
        for active in pair:
            active.call_method('Refresh')
        assert incoming.get_editor_property('Enabled') == input_on
        assert pair[0].get_editor_property('Enabled') == output_on
        in_data = test_ports[0].get_editor_property('port')
        out_data = test_ports[1].get_editor_property('port')
        assert in_data.get_editor_property('type') == unreal.ModuleSocketType.ST_INPUT
        assert out_data.get_editor_property('type') == unreal.ModuleSocketType.ST_OUTPUT
        assert in_data.get_editor_property('bandwidth').get_editor_property('input') == (15.0 if input_on else 0.0)
        assert in_data.get_editor_property('bandwidth').get_editor_property('output') == 0.0
        assert out_data.get_editor_property('bandwidth').get_editor_property('input') == 0.0
        assert out_data.get_editor_property('bandwidth').get_editor_property('output') == (25.0 if output_on else 0.0)
        refresh_count = test_module.get_editor_property('editor_validation_network_refreshes')
        for active in pair:
            active.call_method('ReconcileConnection')
        assert test_module.get_editor_property('editor_validation_network_refreshes') == refresh_count, 'Port reconciliation loops'
        assert not incoming.get_editor_property('Indicator'), 'Disabled indicator recreated'
        pair_cases.append({'water': amount, 'input': input_on, 'output': output_on})
    manager.call_method('ObserveStorage', (target,))
    assert len(manager.get_editor_property('Controllers')) == 2
    assert reader.call_method('GetTestReadCount') == discovery_reads
    # Removing one controller must preserve the other's limits and state.
    incoming.call_method('Cleanup')
    restored = test_ports[0].get_editor_property('port').get_editor_property('bandwidth')
    assert restored.get_editor_property('input') == 15.0 and restored.get_editor_property('output') == 25.0
    assert test_ports[1].get_editor_property('port').get_editor_property('bandwidth').get_editor_property('output') == 0.0
    controlled[0].call_method('Cleanup')
    assert test_ports[1].get_editor_property('port').get_editor_property('bandwidth').get_editor_property('output') == 25.0
    manager.call_method('Stop')
    for active in pair:
        actors.destroy_actor(active)
    actors.destroy_actor(target)
    actors.destroy_actor(unrelated)
finally:
    actors.destroy_actor(manager)
    settings.unlink(missing_ok=True)
report.write_text(json.dumps({'status':'passed','defaultRules':4,'waterCases':cases,'pairedPorts':pair_cases,'managerSelectionAndSingleRead':'passed','invalidThresholdsAndMode':'rejected','duplicatePortFirstWins':'passed','disabledVisualWithMesh':'passed','boundary':'Compiled Blueprint against editor mirrors; native input flow and salt-water configuration need game testing'},indent=2))
unreal.log('SmartOutlet rule validation PASSED')
