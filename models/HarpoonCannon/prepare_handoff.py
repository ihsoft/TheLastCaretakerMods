"""Describe the immutable v4 OBJ as independently movable integration groups."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from build_model import SOURCE_DIR, ROOT, STEM, read_obj, audit

SIGHT_BREECH_REAR_INSET_CM=6.0
SIGHT_TOP_CLEARANCE_CM=5.0
SIGHT_NEIGHBORHOOD_HALF_SIZE_CM=10.0
SIGHT_FORWARD_CORRIDOR_HALF_WIDTH_CM=5.0


def bounds(objects):
    p=np.array([v for o in objects for v in o.vertices])
    return {'minCm':p.min(axis=0).round(6).tolist(),
            'maxCm':p.max(axis=0).round(6).tolist()}


def sight_in_source(scene):
    """Place the eye above the rear breech, parallel to the barrel's +X axis."""
    objects={o.name:o for o in scene.objects}
    panel=np.array(objects['HC_RG_PITCH_BreechTopPanel'].vertices)
    x=float(panel[:,0].min())+SIGHT_BREECH_REAR_INSET_CM
    if x>=panel[:,0].max():
        raise ValueError('Sight rear inset exceeds breech top panel length')
    rod=np.array(objects['HC_RG_PITCH_ChamberedRod'].vertices)
    y=float((rod[:,1].min()+rod[:,1].max())/2)
    boxes=[(np.min(o.vertices,axis=0),np.max(o.vertices,axis=0)) for o in scene.objects]
    xy=np.array([x,y])
    nearby=[hi[2] for lo,hi in boxes
            if np.all(hi[:2]>=xy-SIGHT_NEIGHBORHOOD_HALF_SIZE_CM)
            and np.all(lo[:2]<=xy+SIGHT_NEIGHBORHOOD_HALF_SIZE_CM)]
    z=max(nearby)+SIGHT_TOP_CLEARANCE_CM
    # Conservative AABB envelope: clear central forward strip, not camera/FOV proof.
    ahead=[hi[2] for lo,hi in boxes if hi[0]>=x
           and hi[1]>=y-SIGHT_FORWARD_CORRIDOR_HALF_WIDTH_CM
           and lo[1]<=y+SIGHT_FORWARD_CORRIDOR_HALF_WIDTH_CM]
    if max(ahead)>z-SIGHT_TOP_CLEARANCE_CM+1e-6:
        raise ValueError('Forward central corridor does not retain sight clearance')
    return np.array([x,y,z])


def runtime_descriptor(groups, ammo, sources, scene):
    """Stable importer contract; all revision-specific selections stay model-owned."""
    origins={g['id']:np.array(g['meshOriginInSourceCm']) for g in groups}
    parents={'base':None,'yaw':'base','pitch':'yaw'}
    return {
        'schemaVersion':1,
        'units':'centimeters','uniformScale':1,
        'axes':{'forward':'+X','right':'+Y','up':'+Z'},
        'sources':{Path(s['path']).suffix[1:]:{
            'path':Path(s['path']).name,'sha256':s['sha256']} for s in sources},
        'hierarchy':[{
            'role':g['id'],'parentRole':parents[g['id']],
            'locationRelativeToParentCm':(origins[g['id']] - (
                origins[parents[g['id']]] if parents[g['id']] else np.zeros(3))).tolist()
        } for g in groups],
        'meshes':[{'role':g['id'],
                   'meshOriginInSourceCm':g['meshOriginInSourceCm'],
                   'objectNames':g['objectNames']} for g in groups],
        'ammo':{
            'prototype':{'role':'ammo', 'objectNames':[ammo[0]['objectName']],
                         'meshOriginInSourceCm':ammo[0]['meshOriginInSourceCm']},
            'instances':[{'name':f'ammo{i+1:02}', 'parentRole':'yaw',
                          'sourceObjectName':a['objectName'],
                          'locationRelativeToParentCm':(
                              np.array(a['meshOriginInSourceCm'])-origins['yaw']).tolist()}
                         for i,a in enumerate(ammo)]},
        'sight':{'parentRole':'pitch','locationRelativeToParentCm':(
            sight_in_source(scene)-origins['pitch']).round(6).tolist()}}


def validate_runtime(descriptor, scene):
    """Check the actual exported transforms, including shared-prototype instances."""
    objects={o.name:o for o in scene.objects}
    locations={}
    for node in descriptor['hierarchy']:
        parent=node['parentRole']
        locations[node['role']]=np.array(node['locationRelativeToParentCm']) + (
            locations[parent] if parent else np.zeros(3))
    selected=[]
    for mesh in descriptor['meshes']:
        for name in mesh['objectNames']:
            vertices=np.array(objects[name].vertices)
            restored=vertices-np.array(mesh['meshOriginInSourceCm'])+locations[mesh['role']]
            if not np.allclose(restored,vertices,atol=1e-6,rtol=0):
                raise ValueError('Runtime neutral transform mismatch: '+name)
            selected.append(name)
    prototype=descriptor['ammo']['prototype']
    obj=objects[prototype['objectNames'][0]]
    local=np.array(obj.vertices)-np.array(prototype['meshOriginInSourceCm'])
    instances=descriptor['ammo']['instances']
    if len(instances)!=6 or len({a['name'] for a in instances})!=6:
        raise ValueError('Exactly six independently named ammo instances required')
    for instance in instances:
        name=instance['sourceObjectName']
        restored=local+np.array(instance['locationRelativeToParentCm'])+locations[instance['parentRole']]
        target=objects[name]
        if (obj.faces!=target.faces or obj.material!=target.material or
                not np.allclose(restored,target.vertices,atol=1e-6,rtol=0)):
            raise ValueError('Runtime ammo instance mismatch: '+name)
        selected.append(name)
    if len(selected)!=len(set(selected)) or set(selected)!=set(objects):
        raise ValueError('Runtime object selections must cover the source exactly once')
    for source in descriptor['sources'].values():
        path=Path(source['path'])
        if path.is_absolute() or '..' in path.parts or ':' in source['path']:
            raise ValueError('Source path must be relative to the descriptor')
    if 'sight' in descriptor:
        sight=descriptor['sight']
        if set(sight)!={'parentRole','locationRelativeToParentCm'} or sight['parentRole']!='pitch':
            raise ValueError('Sight must belong to pitch with no rotation override')
        local=np.array(sight['locationRelativeToParentCm'],dtype=float)
        if (local.shape!=(3,) or not np.all(np.isfinite(local)) or
                not np.allclose(local+locations['pitch'],sight_in_source(scene),atol=1e-6,rtol=0)):
            raise ValueError('Sight must be above the rear breech with 5 cm top clearance')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'integration-v4')
    parser.add_argument('--write-runtime-descriptor',action='store_true',
                        help='Regenerate tracked runtime-model.json; default checks it without writing')
    args=parser.parse_args()
    out=args.output.resolve()
    if out==SOURCE_DIR or SOURCE_DIR in out.parents:
        parser.error('Handoff output must remain outside the accepted source directory')
    existing=json.loads((SOURCE_DIR/'runtime-model.json').read_text(encoding='utf-8'))
    sources=[]
    for source in existing['sources'].values():
        p=(SOURCE_DIR/source['path']).resolve()
        if p.parent!=SOURCE_DIR or Path(source['path']).is_absolute():
            raise RuntimeError('Model source must be a sibling of the descriptor')
        expected=source['sha256']
        actual=hashlib.sha256(p.read_bytes()).hexdigest().upper()
        if actual!=expected: raise RuntimeError('Baseline changed: '+str(p))
        sources.append({'path':str(p),'sha256':actual})
    scene=read_obj(SOURCE_DIR/existing['sources']['obj']['path'])
    checked=audit(scene)
    definitions=[('base','HC_RG_STATIC_','mountRoot',[0,0,0]),
                 ('yaw','HC_RG_YAW_','yawPivot',[0,0,0]),
                 ('pitch','HC_RG_PITCH_','pitchPivot',[-18,0,107])]
    groups=[]
    selected=[]
    for name,prefix,parent,pivot in definitions:
        objects=[o for o in scene.objects if o.name.startswith(prefix)]
        selected.extend(o.name for o in objects)
        groups.append({'id':name,'parentComponent':parent,'meshOriginInSourceCm':pivot,
                       'objectNames':[o.name for o in objects],
                       'objectCount':len(objects),'triangles':sum(len(o.faces) for o in objects),
                       'sourceBounds':bounds(objects)})
    ammo=[]
    reference=None
    for i in range(6):
        name=f'HC_RG_AMMO_Rod_{i+1:02}'
        obj=next(o for o in scene.objects if o.name==name)
        origin=np.array([-106+i*15.5,-86,58])
        local=np.array(obj.vertices)-origin
        if reference is None: reference=(local,obj.faces)
        assert np.allclose(local,reference[0],atol=1e-6,rtol=0) and obj.faces==reference[1]
        selected.append(name)
        ammo.append({'objectName':name,'parentComponent':'yawPivot',
                     'meshOriginInSourceCm':origin.tolist(),
                     'componentLocationRelativeToYawCm':origin.tolist(),
                     'triangles':len(obj.faces),'sourceBounds':bounds([obj])})
    assert len(selected)==len(set(selected))==len(scene.objects)==83
    assert set(selected)=={o.name for o in scene.objects}
    # Source vertices must reconstruct exactly at neutral aim after recentering.
    for g in groups:
        pivot=np.array(g['meshOriginInSourceCm'])
        for o in scene.objects:
            if o.name in g['objectNames']:
                assert np.allclose((np.array(o.vertices)-pivot)+pivot,o.vertices,atol=1e-8,rtol=0)
    handoff={
        'identity':'railgun-c-v4-integration', 'sources':sources,
        'sourceCommit':'bcd3f3cd08bb37899c68faed3ecdd94f0ccc3a75',
        'units':'centimeters', 'uniformScale':1,
        'axes':{'forward':'+X','right':'+Y','up':'+Z'},
        'coordinateContract':'OBJ positions are assembly-local, not component-local. Subtract meshOriginInSourceCm exactly once; restore it through hierarchy offsets.',
        'hierarchy':[
            {'component':'mountRoot','parent':'existing ship-attached module root','locationCm':[0,0,0]},
            {'component':'yawPivot','parent':'mountRoot','locationCm':[0,0,0],'axis':'+Z'},
            {'component':'pitchPivot','parent':'yawPivot','locationCm':[-18,0,107],'axis':'+Y'}],
        'groups':groups,'ammoObjects':ammo,
        'ammoReuse':'All six rods have identical local geometry within 1e-6 cm. Either six separately selected meshes, or one shared mesh with six independent components. Never bake them into yaw geometry.',
        'chamberedRod':'HC_RG_PITCH_ChamberedRod remains in pitch; optional later independent display is not part of this request.',
        'aimContract':{'yawDegrees':[-40,40], 'yawReference':'neutral installation orientation, as requested by integration owner',
                       'pitchDegrees':[-50,10], 'positivePitchMeaning':'muzzle rises; UE Pitch, not unqualified right-hand rotation about +Y',
                       'limitsOwner':'logic task; no limit encoded by OBJ'},
        'placement':'Use existing ship attachment and construction contracts. No world-space stabilization or independent physics implied.',
        'importNotes':[
            'Current source path SourceAssets/harpoon_cannon_blockout.obj and exact-name lists still target the older model. Replace path/lists as an integration change; plain file swap is insufficient.',
            'Current parser accepts v//vn face tokens by taking position index before slash. It ignores vn and vt; actual importer normals/tangents and construction material attributes remain to validate.',
            'Current CreateVisualMesh subtracts its Pivot argument, matching this handoff. Replace old pitch (-5,0,94) with (-18,0,107) in mesh recenter AND component transform.',
            'Current CreateVisualMesh reverses triangle order (0,i+1,i). Keep deliberate engine-conversion handling under logic ownership; do not add a second blind flip. Runtime culling is not source-audit proof.',
            'MTL palette is not automatically imported by current generator, which assigns default material to each slot. Slot names are provided, but gameplay colors/materials require explicit integration.',
            'Power inlet is an authored fixed placeholder, not a hard reference to a stock game asset. No mooring connector, cable or asset dependency should be added for v4.',
            'Preserve existing base collision/Q contract; do not infer collision from render mesh groups.'],
        'materialSlots':sorted({o.material for o in scene.objects}),
        'sourceBounds':bounds(scene.objects),
        'validation':{'hashes':'match accepted baseline','closedObjects':len(checked['objects']),
                      'coverage':'83 of 83 objects exactly once', 'neutralReconstruction':'passed',
                      'rodLocalGeometryReuse':'passed 1e-6 cm', 'triangleCount':sum(len(o.faces) for o in scene.objects)},
        'limitations':[
            'No Unreal import, cook, package, install or runtime test in this handoff.',
            'Full motion/camera/operator clearances not validated, especially pitch -50 degrees.',
            'AlignmentActuator_Left/Right are rigid decorative yaw objects, not rigged telescoping links.',
            'UVs, final materials and smoothing remain pending. Do not claim game-ready from source audit.']}
    runtime=runtime_descriptor(groups,ammo,sources,scene)
    validate_runtime(runtime,scene)
    runtime_path=SOURCE_DIR/'runtime-model.json'
    runtime_bytes=(json.dumps(runtime,indent=2)+'\n').encode('utf-8')
    if args.write_runtime_descriptor:
        runtime_path.write_bytes(runtime_bytes)
    elif not runtime_path.exists() or runtime_path.read_bytes()!=runtime_bytes:
        raise RuntimeError('runtime-model.json missing or stale; review model changes, then use --write-runtime-descriptor')
    out.mkdir(parents=True,exist_ok=True)
    target=out/'handoff.json'
    target.write_text(json.dumps(handoff,indent=2),encoding='utf-8')
    print(json.dumps({'handoff':str(target),'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
                      'runtimeDescriptor':str(runtime_path),
                      'runtimeSha256':hashlib.sha256(runtime_bytes).hexdigest().upper(),
                      'groups':{g['id']:g['objectCount'] for g in groups},'ammo':len(ammo),
                      'triangles':handoff['validation']['triangleCount'],'validation':'passed'}))


if __name__=='__main__': main()
