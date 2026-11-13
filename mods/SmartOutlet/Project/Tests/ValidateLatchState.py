"""Exercise compiled latch state transitions with actor-tag snapshot fixtures.

Snapshot copying here does NOT test the game's native save/restore of Actor.Tags.
"""
import json
import os
from pathlib import Path
import unreal

root = Path(os.environ['SMARTOUTLET_TEST_OUTPUT'])
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
system = unreal.get_default_object(unreal.SystemLibrary)
controller_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/BP_OutletController.BP_OutletController_C')
rule_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/Settings/BP_OutletRule.BP_OutletRule_C')
parser_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/Settings/BP_SectionedIniFileParser.BP_SectionedIniFileParser_C')
rule_fixture_path = root/'latch-rule.ini'
limits = unreal.ModuleSocketBandwidth(input=42, output=73, item_bandwidth_in_kg=9)
unreal.get_default_object(controller_class).set_editor_property('OriginalBandwidth', limits)
created = []
checks = []
def owner(tags=()):
    value = actors.spawn_actor_from_class(unreal.VoyageModuleActor, unreal.Vector())
    value.set_editor_property('tags', list(tags))
    created.append(value)
    return value
def tags(value):
    return [str(t) for t in value.get_editor_property('tags')]
def fixture(storage, socket='SocketA', is_input=False):
    mode = 'input' if is_input else 'output'
    rule_fixture_path.write_text(
        f'[Test:{mode}]\nEnablePortAtPercent={20 if is_input else 80}\n'
        f'DisablePortAtPercent={80 if is_input else 20}\nGlowMaterialSlot=-1\n')
    parser = unreal.new_object(parser_class)
    parser.call_method('Load', (str(rule_fixture_path),))
    rule = unreal.new_object(rule_class)
    assert rule.call_method('ReadSettings', (parser, 'Test:'+mode, is_input))
    module = unreal.new_object(unreal.VoyageModuleComponent)
    module.set_editor_property('editor_validation_capacity', 100)
    port = unreal.new_object(unreal.VoyageModuleSocketViewComponent)
    data = port.get_editor_property('port')
    data.set_editor_property('resource_type', unreal.ModuleResourceType.ELECTRICITY)
    data.set_editor_property('bandwidth', limits)
    port.set_editor_property('port', data)
    c = actors.spawn_actor_from_class(controller_class, unreal.Vector())
    created.append(c)
    for k, v in [('Storage',storage), ('Rule',rule), ('Module',module), ('Port',port)]:
        system.call_method('SetObjectPropertyByName', (c,k,v))
    system.call_method('SetBoolPropertyByName', (c,'Initialized',True))
    system.call_method('SetStringPropertyByName', (c, 'SocketName', socket))
    system.call_method('SetBytePropertyByName', (c, 'Mode', 1 if is_input else 2))
    c.call_method('RestoreLatch')
    return c, module, port
def refresh(f, amount, expected):
    c, module, port = f
    module.set_editor_property('editor_validation_amount', amount)
    c.call_method('Refresh')
    assert c.get_editor_property('Enabled') == expected, (amount, expected)
    limits_now = port.get_editor_property('port').get_editor_property('bandwidth')
    inp = c.get_editor_property('Rule').get_editor_property('IsInput')
    assert limits_now.get_editor_property('input') == (42 if inp and expected else 0)
    assert limits_now.get_editor_property('output') == (73 if not inp and expected else 0)
try:
    storage = owner(['UnrelatedTag'])
    original = fixture(storage)
    refresh(original, 80, True)
    refresh(original, 50, True)
    open_snapshot = tags(storage)
    assert 'UnrelatedTag' in open_snapshot
    assert len(open_snapshot) == 3
    original[0].call_method('Cleanup')
    assert tags(storage) == open_snapshot, 'Cleanup erased the saved latch'
    restored = fixture(owner(open_snapshot))
    refresh(restored, 50, True)
    refresh(restored, 20, False)
    refresh(restored, 50, False)
    closed_snapshot = tags(restored[0].get_editor_property('Storage'))
    refresh(fixture(owner(closed_snapshot)), 50, False)
    refresh(fixture(owner(open_snapshot)), 50, True)
    checks.append('Output 80->50 stays open; 20->50 stays closed; old snapshot retains its own history')
    refresh(fixture(owner(open_snapshot)), 10, False)
    refresh(fixture(owner(closed_snapshot)), 90, True)
    refresh(fixture(owner()), 50, True)
    refresh(fixture(owner(), is_input=True), 50, True)
    refresh(fixture(owner()), 20, False)
    refresh(fixture(owner(), is_input=True), 80, False)
    # Legacy open-only saves have no known-state marker. Legacy closed and no
    # record are indistinguishable and intentionally take the open fallback.
    refresh(fixture(owner(['SmartOutlet.Latch.v1.SocketA.output'])), 50, True)
    refresh(fixture(owner(['UnrelatedTag'])), 50, True)
    refresh(fixture(owner(['SmartOutlet.LatchKnown.v1.SocketA.output'])), 50, False)
    checks.append('Unknown state opens in both modes; explicit closed state restores; boundaries still close normally')
    water = owner()
    inlet = fixture(water, 'SocketA', True)
    outlet = fixture(water, 'SocketB', False)
    refresh(inlet, 20, True)
    refresh(outlet, 20, False)
    refresh(outlet, 50, False)
    refresh(inlet, 50, True)
    pair_snapshot = tags(water)
    copy = owner(pair_snapshot)
    refresh(fixture(copy, 'SocketA', True), 50, True)
    refresh(fixture(copy, 'SocketB', False), 50, False)
    refresh(inlet, 80, False)
    checks.append('Independent ports and inverted input hysteresis round-trip through snapshots')
    switched_owner = owner(pair_snapshot)
    switched = fixture(switched_owner, 'SocketA', False)
    refresh(switched, 50, True)
    assert not any(t.endswith('.input') for t in tags(switched_owner)), 'Old input-mode markers were not retired'
    assert len(tags(switched_owner)) == 3, 'Unrelated port marker was lost'
    checks.append('New mode uses open fallback, retires opposite-mode markers and preserves other ports')
    repeat_owner = owner(open_snapshot)
    repeat = fixture(repeat_owner)
    refresh(repeat, 50, True)
    before = repeat[1].get_editor_property('editor_validation_network_refreshes')
    for _ in range(4):
        refresh(repeat, 50, True)
    assert tags(repeat_owner) == open_snapshot
    assert repeat[1].get_editor_property('editor_validation_network_refreshes') == before
    checks.append('Repeated application does not duplicate tags or trigger network refreshes')
finally:
    for a in reversed(created):
        actors.destroy_actor(a)
    rule_fixture_path.unlink(missing_ok=True)
(root/'latch-validation.json').write_text(json.dumps({'status':'passed','checks':checks,'nativeSaveRestoreValidated':False}, indent=2)+'\n')
unreal.log('SmartOutlet latch checks passed: ' + str(len(checks)))
