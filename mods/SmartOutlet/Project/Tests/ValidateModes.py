"""Compiled port-mode transitions and tag snapshots; not native network/save proof."""
import json
import os
from pathlib import Path
import unreal
root=Path(os.environ['SMARTOUTLET_TEST_OUTPUT'])
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
system=unreal.get_default_object(unreal.SystemLibrary)
created=[]
controller_class=unreal.load_class(None,'/Game/Mods/SmartOutlet/BP_OutletController.BP_OutletController_C')
rule_class=unreal.load_class(None,'/Game/Mods/SmartOutlet/Settings/BP_OutletRule.BP_OutletRule_C')
original_material=unreal.load_asset('/Engine/EngineMaterials/DefaultMaterial')
rule=unreal.new_object(rule_class)
profile_class=unreal.load_class(None,'/Game/Mods/SmartOutlet/Settings/BP_StorageProfile.BP_StorageProfile_C')
input_rule=unreal.new_object(rule_class)
profile=unreal.new_object(profile_class)
system.call_method('SetBoolPropertyByName',(input_rule,'IsInput',True))
system.call_method('SetObjectPropertyByName',(profile,'InputRule',input_rule))
system.call_method('SetObjectPropertyByName',(profile,'OutputRule',rule))
# Explicit scenario inputs keep the state-machine checks independent of defaults.
for value, is_input in [(rule,False),(input_rule,True)]:
    system.call_method('SetDoublePropertyByName',(value,'LowFraction',0.2))
    system.call_method('SetDoublePropertyByName',(value,'HighFraction',0.8))
    system.call_method('SetBoolPropertyByName',(value,'IsInput',is_input))
    system.call_method('SetBytePropertyByName',(value,'Direction',1 if is_input else 2))
    system.call_method('SetIntPropertyByName',(value,'GlowMaterialSlot',0))
def prop(o,k,v,kind='Object'):
    system.call_method('Set'+kind+'PropertyByName',(o,k,v))
def storage(tags=()):
    a=actors.spawn_actor_from_class(unreal.VoyageModuleActor,unreal.Vector());created.append(a)
    a.set_editor_property('tags',list(tags));return a
def fixture(owner,socket='PortA',normal=False,resource=unreal.ModuleResourceType.ELECTRICITY,profiles=None):
    c=actors.spawn_actor_from_class(controller_class,unreal.Vector());created.append(c)
    module=unreal.new_object(unreal.VoyageModuleComponent,outer=owner)
    module.set_editor_property('editor_validation_capacity',100.0)
    module.set_editor_property('editor_validation_amount',50.0)
    port=unreal.new_object(unreal.VoyageModuleSocketViewComponent,outer=owner)
    data=port.get_editor_property('port');data.set_editor_property('bandwidth',unreal.ModuleSocketBandwidth(input=42,output=73,item_bandwidth_in_kg=9));data.set_editor_property('resource_type',resource);port.set_editor_property('port',data)
    mesh=unreal.new_object(unreal.StaticMeshComponent,outer=owner);mesh.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));mesh.set_material(0,original_material);prop(port,'mesh',mesh)
    for k,v in [('Storage',owner),('Profile',profiles or profile),('Module',module),('Port',port),('OriginalMaterial',original_material)]:prop(c,k,v)
    prop(c,'SocketName',socket,'String');prop(c,'Initialized',True,'Bool')
    if not normal and not any(str(t).startswith('SmartOutlet.Mode.v1.'+socket+'.') for t in owner.get_editor_property('tags')):
        owner.set_editor_property('tags',list(owner.get_editor_property('tags'))+['SmartOutlet.Mode.v1.'+socket+'.output'])
    c.call_method('RestoreMode');c.call_method('ApplyMode')
    return c,module,port,mesh
def state(f,mode,amount=None,expected=None):
    c,m,p,mesh=f
    if amount is not None:m.set_editor_property('editor_validation_amount',float(amount));c.call_method('Refresh')
    assert c.get_editor_property('Mode')==mode
    b=p.get_editor_property('port').get_editor_property('bandwidth')
    assert b.get_editor_property('item_bandwidth_in_kg')==9
    if expected is not None:
        assert c.get_editor_property('Enabled')==expected
        assert b.get_editor_property('input')==(42 if mode==unreal.ModuleSocketType.ST_INPUT and expected else 0)
        assert b.get_editor_property('output')==(73 if mode==unreal.ModuleSocketType.ST_OUTPUT and expected else 0)
    return b
I=unreal.ModuleSocketType.ST_INPUT;O=unreal.ModuleSocketType.ST_OUTPUT;N=unreal.ModuleSocketType.ST_ANY
try:
    owner=storage(['Unrelated']);a=fixture(owner);b=fixture(owner,'PortB',True,unreal.ModuleResourceType.FRESH_WATER)
    state(a,O,50,True);assert a[0].call_method('GetModeHint')=='Set normal (now: smart output 80/20)'
    state(b,N);assert b[2].get_editor_property('bAutoInitialize');assert b[3].get_material(0)==original_material
    # Output -> normal restores finite limits, type, automatic initialization and rim.
    a[0].call_method('CycleMode');limits=state(a,N)
    assert (limits.input,limits.output)==(42,73)
    assert a[2].get_editor_property('port').get_editor_property('type')==N
    assert a[2].get_editor_property('bAutoInitialize')
    assert a[3].get_material(0)==original_material
    normal_tags=list(owner.get_editor_property('tags'))
    # Normal is passive even when stock control changes limits afterwards.
    data=a[2].get_editor_property('port');data.bandwidth=unreal.ModuleSocketBandwidth(input=55,output=66,item_bandwidth_in_kg=9);a[2].set_editor_property('port',data)
    a[0].call_method('Refresh');assert a[2].get_editor_property('port').bandwidth.input==55
    a[0].call_method('CycleMode');assert a[0].get_editor_property('Mode')==I
    a[0].call_method('CycleMode');assert a[0].get_editor_property('Mode')==O
    a[0].call_method('CycleMode');assert a[2].get_editor_property('port').bandwidth.input==55 and a[2].get_editor_property('port').bandwidth.output==66
    # Independently activate the previously unconfigured fresh-water port.
    b[0].call_method('CycleMode');state(b,I,20,True);state(b,I,50,True);state(b,I,80,False);state(b,I,50,False)
    assert b[0].call_method('GetModeHint')=='Set smart output 80/20 (now: smart input 20/80)'
    snapshots=list(owner.get_editor_property('tags'))
    assert 'Unrelated' in [str(t) for t in snapshots]
    assert rule.get_editor_property('Direction')==O and not rule.get_editor_property('IsInput'), 'Shared INI rule mutated'
    # Restore explicit normal despite output default, and input+closed latch despite normal default.
    restored_owner=storage(snapshots)
    restored_a=fixture(restored_owner);restored_b=fixture(restored_owner,'PortB',True,unreal.ModuleResourceType.FRESH_WATER)
    state(restored_a,N);state(restored_b,I,50,False)
    restored_b[0].call_method('CycleMode');state(restored_b,O,50,True);state(restored_b,O,20,False);state(restored_b,O,50,False);state(restored_b,O,80,True)
    assert restored_b[0].call_method('GetModeHint')=='Set normal (now: smart output 80/20)'
    restored_b[0].call_method('CycleMode');state(restored_b,N);assert restored_b[0].call_method('GetModeHint')=='Set smart input 20/80 (now: normal)'
    assert restored_b[3].get_material(0)==original_material
    # Hint uses this rule's values, including fractional percentages.
    prop(rule,'LowFraction',0.125,'Double');prop(rule,'HighFraction',0.875,'Double')
    prop(input_rule,'LowFraction',0.15,'Double');prop(input_rule,'HighFraction',0.9,'Double')
    assert restored_b[0].call_method('GetModeHint')=='Set smart input 15/90 (now: normal)'
    restored_b[0].call_method('CycleMode')
    assert restored_b[0].call_method('GetModeHint')=='Set smart output 87.5/12.5 (now: smart input 15/90)'
    restored_b[0].call_method('CycleMode')
    assert restored_b[0].call_method('GetModeHint')=='Set normal (now: smart output 87.5/12.5)'
    restored_b[0].call_method('CycleMode')
    # Cleanup cannot undo stock changes made while normal.
    before=restored_b[2].get_editor_property('port');before.bandwidth=unreal.ModuleSocketBandwidth(input=15,output=25,item_bandwidth_in_kg=9);restored_b[2].set_editor_property('port',before)
    restored_b[0].call_method('Cleanup');assert restored_b[2].get_editor_property('port').bandwidth.input==15
    # Optional modes use the same resolver for labels and actual V transitions.
    only_input=unreal.new_object(profile_class);prop(only_input,'InputRule',input_rule)
    only_output=unreal.new_object(profile_class);prop(only_output,'OutputRule',rule)
    empty=unreal.new_object(profile_class)
    for available,expected,label in [(only_input,I,'smart input 15/90'),(only_output,O,'smart output 87.5/12.5')]:
        f=fixture(storage(),normal=True,profiles=available)
        assert f[0].call_method('GetModeHint')=='Set '+label+' (now: normal)'
        assert f[0].call_method('GetNextMode')==expected
        f[0].call_method('CycleMode');state(f,expected)
        assert f[0].call_method('GetModeHint')=='Set normal (now: '+label+')'
        f[0].call_method('CycleMode');state(f,N)
    f=fixture(storage(['SmartOutlet.Mode.v1.PortA.input']),normal=True,profiles=only_output)
    state(f,N);assert 'SmartOutlet.Mode.v1.PortA.normal' in [str(t) for t in f[0].get_editor_property('Storage').get_editor_property('tags')]
    f=fixture(storage(),normal=True,profiles=empty);assert f[0].call_method('GetNextMode')==N
    # Switching to a disabled visual slot restores the previous rim and clears its MID.
    prop(input_rule,'GlowMaterialSlot',-1,'Int')
    f=fixture(storage());assert f[3].get_material(0)!=original_material
    f[0].call_method('CycleMode');state(f,N)
    f[0].call_method('CycleMode');state(f,I);assert f[3].get_material(0)==original_material
    assert not f[0].get_editor_property('Indicator')
    f[0].call_method('CycleMode');state(f,O);assert f[3].get_material(0)!=original_material
    # Distinct valid slots: smart input -> smart output must restore input's slot.
    prop(input_rule,'GlowMaterialSlot',1,'Int')
    f=fixture(storage(),normal=True)
    two_slots=unreal.new_object(unreal.StaticMesh)
    two_slots.set_editor_property('static_materials',[
        unreal.StaticMaterial(material_interface=original_material,material_slot_name='First'),
        unreal.StaticMaterial(material_interface=original_material,material_slot_name='Second')])
    f[3].set_static_mesh(two_slots)
    assert f[3].get_num_materials()==2, 'Fixture must expose two real mesh slots'
    f[3].set_material(0,original_material);f[3].set_material(1,original_material)
    f[0].call_method('CycleMode');state(f,I)
    assert f[3].get_material(0)==original_material and f[3].get_material(1)!=original_material
    f[0].call_method('CycleMode');state(f,O)
    assert f[3].get_material(0)!=original_material and f[3].get_material(1)==original_material
    f[0].call_method('CycleMode');state(f,N)
    assert f[3].get_material(0)==original_material and f[3].get_material(1)==original_material
    (root/'modes-validation.json').write_text(json.dumps({'status':'passed','checks':['input/output/normal cycle','finite limits and original material restored','normal is passive','recapture stock settings on reacquisition','per-port independence across resources','shared rule immutable','mode and latch tag snapshots','explicit normal overrides configured smart mode','unrelated tags preserved','next/current hints with independent integer and fractional thresholds','optional mode cycling','removed saved mode falls back to normal','disabled and distinct valid material slots'],'boundary':'Compiled Blueprint with editor mirrors; native network and game save pipeline require runtime test'},indent=2)+'\n')
finally:
    for a in reversed(created):actors.destroy_actor(a)
