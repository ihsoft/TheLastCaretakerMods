"""Compiled profile lookup/discovery and independent resource ports; editor mirrors only."""
import json
from pathlib import Path
import unreal
root=Path(__file__).resolve().parents[3]
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
system=unreal.get_default_object(unreal.SystemLibrary)
reader=unreal.get_default_object(unreal.VoyageEditorBlueprintFunctionLibrary)
parser_class=unreal.load_class(None,'/Game/Mods/SmartOutlet/Settings/BP_SectionedIniFileParser.BP_SectionedIniFileParser_C')
profile_class=unreal.load_class(None,'/Game/Mods/SmartOutlet/Settings/BP_StorageProfile.BP_StorageProfile_C')
manager_class=unreal.load_class(None,'/Game/Mods/SmartOutlet/ModActor.ModActor_C')
parser=unreal.new_object(parser_class)
fixture=root/'Tmp/SmartOutlet/profile-fixture.ini'
created=[]
def block(key,mode,enable,disable,slot=-1):
    return f'[{key}:{mode}]\nEnablePortAtPercent={enable}\nDisablePortAtPercent={disable}\nGlowMaterialSlot={slot}\n'
def profile(text,key='Test.Class_C'):
    fixture.write_text(text,encoding='utf-8');parser.call_method('Load',(str(fixture),))
    p=unreal.new_object(profile_class);p.call_method('ReadSettings',(parser,key));return p
def spawn(cls):
    a=actors.spawn_actor_from_class(cls,unreal.Vector());created.append(a);return a
try:
    before=reader.call_method('GetTestReadCount')
    parser.call_method('Load',(str(root/'mods/SmartOutlet/Assets/SmartOutlet.ini'),))
    for key, mode, low, high in [
        ('BP_Module_Battery.BP_Module_Battery_C','Output',.2,.8),
        ('BP_Module_fresh_Water_Tank.BP_Module_fresh_Water_Tank_C','Input',.2,1.0),
        ('BP_Module_Salt_Water_Tank.BP_Module_salt_Water_Tank_C','Input',.2,1.0)]:
        p=unreal.new_object(profile_class);p.call_method('ReadSettings',(parser,key))
        assert p.get_editor_property('HasModes')
        selected=p.get_editor_property(mode+'Rule')
        assert selected and bool(selected.get_editor_property('IsInput'))==(mode=='Input')
        assert p.get_editor_property(('Input' if mode=='Output' else 'Output')+'Rule') is None
        assert abs(selected.get_editor_property('LowFraction')-low)<1e-8
        assert abs(selected.get_editor_property('HighFraction')-high)<1e-8
    assert reader.call_method('GetTestReadCount')==before+1
    for modes in [[],['Input'],['Output'],['Input','Output']]:
        p=profile(''.join(block('Test.Class_C',m,20 if m=='Input' else 80,80 if m=='Input' else 20) for m in modes))
        assert bool(p.get_editor_property('HasModes'))==bool(modes)
        for m in ['Input','Output']:assert bool(p.get_editor_property(m+'Rule'))==(m in modes)
    for mode in ['Input','Output']:
        for enable,disable,slot in [(50,50,0),(80 if mode=='Input' else 20,20 if mode=='Input' else 80,0),(-1,80,0),(20,101,0),(20,80,0.5),(20,80,-2)]:
            p=profile(block('Test.Class_C',mode,enable,disable,slot));assert not p.get_editor_property('HasModes'),(mode,enable,disable,slot)
    for bad in ['[Test.Class_C:Input]\n','[Test.Class_C:input]\nEnablePortAtPercent=20\nDisablePortAtPercent=80\nGlowMaterialSlot=0\n',block('Test.Class_C','Input','bad',80)]:
        assert not profile(bad).get_editor_property('HasModes')
    p=profile(block('Test.Class_C','Input',12.5,66,-1)+block('Test.Class_C','Output',95,30,0))
    assert abs(p.get_editor_property('InputRule').get_editor_property('HighFraction')-.66)<1e-8
    assert abs(p.get_editor_property('OutputRule').get_editor_property('LowFraction')-.3)<1e-8
    # Manager uses actual class identity and the same snapshot for every instance.
    settings=Path(unreal.Paths.project_content_dir())/'Paks/LogicMods/SmartOutlet.ini'
    assert not settings.exists(),'Would replace existing editor settings'
    settings.parent.mkdir(parents=True,exist_ok=True)
    settings.write_text('UnlockSaltWaterTank=false\n'+block('Voyage.VoyageModuleActor','Input',20,70)+block('Voyage.VoyageModuleActor','Output',90,30),encoding='utf-8')
    manager=spawn(manager_class);count=reader.call_method('GetTestReadCount');manager.call_method('LoadSettings')
    target=spawn(unreal.VoyageModuleActor);unrelated=spawn(unreal.Actor)
    module=reader.call_method('AddTestComponent',(target,unreal.VoyageModuleComponent.static_class()))
    ports=[reader.call_method('AddTestComponent',(target,unreal.VoyageModuleSocketViewComponent.static_class())) for _ in range(2)]
    module.set_editor_property('editor_validation_capacity',2000.0)
    for port in ports:
        data=port.get_editor_property('port');data.bandwidth=unreal.ModuleSocketBandwidth(input=15,output=25,item_bandwidth_in_kg=3);data.resource_type=unreal.ModuleResourceType.FRESH_WATER;port.set_editor_property('port',data)
    manager.call_method('ObserveStorage',(unrelated,));assert len(manager.get_editor_property('Controllers'))==0
    manager.call_method('ObserveStorage',(target,));pair=list(manager.get_editor_property('Controllers'));assert len(pair)==2
    for c in pair:
        c.call_method('Initialize');assert c.get_editor_property('Mode')==unreal.ModuleSocketType.ST_ANY
        assert c.get_editor_property('Port').get_editor_property('port').bandwidth.output==25
    assert pair[0].get_editor_property('Profile')==pair[1].get_editor_property('Profile')
    second=spawn(unreal.VoyageModuleActor)
    assert manager.call_method('GetStorageProfile',(second,))==pair[0].get_editor_property('Profile')
    manager.call_method('ObserveStorage',(target,));assert len(manager.get_editor_property('Controllers'))==2
    assert reader.call_method('GetTestReadCount')==count+1
    pair[0].call_method('CycleMode');pair[1].call_method('CycleMode');pair[1].call_method('CycleMode')
    cases=[]
    for amount,incoming,outgoing in [(0,True,False),(1000,True,False),(1400,False,False),(1800,False,True),(1000,False,True),(600,False,False),(400,True,False)]:
        module.set_editor_property('editor_validation_amount',float(amount))
        for c in pair:c.call_method('Refresh')
        assert pair[0].get_editor_property('Enabled')==incoming,(amount,'input')
        assert pair[1].get_editor_property('Enabled')==outgoing,(amount,'output')
        assert pair[0].get_editor_property('Port').get_editor_property('port').bandwidth.input==(15 if incoming else 0)
        assert pair[1].get_editor_property('Port').get_editor_property('port').bandwidth.output==(25 if outgoing else 0)
        count_before=module.get_editor_property('editor_validation_network_refreshes')
        for c in pair:c.call_method('ReconcileConnection')
        assert module.get_editor_property('editor_validation_network_refreshes')==count_before
        cases.append([amount,incoming,outgoing])
    manager.call_method('Stop')
    (root/'Tmp/SmartOutlet/rules-validation.json').write_text(json.dumps({'status':'passed','profiles':'input/output/both/neither; invalid/missing/case-sensitive','lookup':'real owner class; shared class cache; one file read; all ports; duplicate discovery','waterCases':cases,'boundary':'Compiled editor mirrors, not native transport or saves'},indent=2))
finally:
    fixture.unlink(missing_ok=True)
    if 'settings' in globals():settings.unlink(missing_ok=True)
    if 'manager' in globals():manager.call_method('Stop')
    for a in reversed(created):
        if unreal.SystemLibrary.is_valid(a):actors.destroy_actor(a)
