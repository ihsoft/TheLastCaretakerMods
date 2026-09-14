"""Build an isolated v5 geometry candidate; never replace the accepted runtime model."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

import numpy as np

import build_model as baseline
from prepare_handoff import validate_runtime

STEM='harpoon_cannon_railgun_c_blockout_v5'
CONTACT_OVERLAP_CM=0.1
RIM_REDUCTION=2.5


def limits(obj):
    return np.min(obj.vertices,axis=0),np.max(obj.vertices,axis=0)


def refine(scene):
    by_name={o.name:o for o in scene.objects}
    rail_lo,rail_hi=limits(by_name['HC_RG_PITCH_RailRight'])
    rail_outer=rail_hi[1]
    rail_top=rail_hi[2]
    for obj in scene.objects:
        if 'CoilWindowLeft_' in obj.name or 'CoilWindowRight_' in obj.name:
            lo,hi=limits(obj)
            side=1 if lo[1]>0 else -1
            center=(lo[1]+hi[1])/2
            target=side*(rail_outer+(hi[1]-lo[1])/2-CONTACT_OVERLAP_CM)
            for p in obj.vertices:
                p[1]+=target-center
        elif 'CoilModuleTop_' in obj.name:
            lo,hi=limits(obj)
            for p in obj.vertices:
                p[1]*=(rail_outer-CONTACT_OVERLAP_CM)/hi[1]
                p[2]+=rail_top-CONTACT_OVERLAP_CM-lo[2]
        elif 'CoilStatus_' in obj.name:
            lo,hi=limits(obj)
            for p in obj.vertices:
                # Status plaque remains attached to the now-narrower top cover.
                p[1]+=(-rail_outer+2*CONTACT_OVERLAP_CM)-hi[1]
                p[2]-=2
        elif obj.name=='HC_RG_PITCH_MuzzleShroud':
            for p in obj.vertices:
                yz=p[1:]-[0,111]
                # Inner octagon is unchanged; map each corresponding outer corner
                # toward it by exactly 1/2.5, including the chamfer thickness.
                if max(abs(yz))<=5+1e-6:
                    continue
                inner=np.sign(yz)*np.where(abs(yz)==max(abs(yz)),5,4.5)
                p[1:]=[0,111]+inner+(yz-inner)/RIM_REDUCTION
        elif obj.name.startswith('HC_RG_PITCH_MuzzleGuard'):
            for p in obj.vertices:
                yz=p[1:]-[0,111]
                p[1:]=[0,111]+np.sign(yz)*(5+(abs(yz)-5)/RIM_REDUCTION)
    return scene


def verify(original,candidate):
    before={o.name:o for o in original.objects}
    after={o.name:o for o in candidate.objects}
    assert before.keys()==after.keys()
    changed=[]
    for name,obj in after.items():
        old=before[name]
        assert old.faces==obj.faces and old.material==obj.material
        assert np.array_equal(np.array(old.vertices)[:,0],np.array(obj.vertices)[:,0])
        if not np.array_equal(old.vertices,obj.vertices):
            changed.append(name)
            assert any(token in name for token in ('CoilWindow','CoilModuleTop_',
                                                   'CoilStatus_','Muzzle'))
    for i in range(1,6):
        top_lo,top_hi=limits(after[f'HC_RG_PITCH_CoilModuleTop_{i:02}'])
        assert np.allclose(top_lo[1:],[-10.9,117.9])
        assert np.allclose(top_hi[1:],[10.9,125.9])
        for side,expected in [('Left',[-13.9,-10.9]),('Right',[10.9,13.9])]:
            lo,hi=limits(after[f'HC_RG_PITCH_CoilWindow{side}_{i:02}'])
            assert np.allclose([lo[1],hi[1]],expected)
    old=np.array(before['HC_RG_PITCH_MuzzleShroud'].vertices)
    new=np.array(after['HC_RG_PITCH_MuzzleShroud'].vertices)
    inner=np.max(abs(old[:,1:]-[0,111]),axis=1)<=5+1e-6
    assert np.array_equal(old[inner],new[inner])
    assert np.array_equal(old[:,0],new[:,0])
    for old_p,new_p in zip(old[~inner],new[~inner]):
        yz=old_p[1:]-[0,111]
        bore=np.sign(yz)*np.where(abs(yz)==max(abs(yz)),5,4.5)
        assert np.allclose(new_p[1:]-[0,111]-bore,(yz-bore)/RIM_REDUCTION)
    report=baseline.audit(candidate)
    report.update(changedObjects=changed,unchangedObjects=len(before)-len(changed),
                  muzzleOuterYZCm=[20.4,20.4],muzzleNoseYZCm=[18,18],
                  rimReduction=RIM_REDUCTION,magnetContactOverlapCm=CONTACT_OVERLAP_CM,
                  topMagnetWidthCm=21.8,
                  preserved='Barrel rails/floor, bore, all source X coordinates, pivots, sight, ammo and role selections')
    descriptor=json.loads((baseline.SOURCE_DIR/'runtime-model.json').read_text())
    validate_runtime(descriptor,candidate)
    report['runtimeNeutralContract']='passed; candidate does not replace runtime descriptor'
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=baseline.ROOT/'geometry-v5')
    parser.add_argument('--render',action='store_true')
    parser.add_argument('--adopt',action='store_true',
                        help='After source review, add v5 OBJ/MTL and select them in runtime-model.json')
    args=parser.parse_args()
    out=args.output.resolve()
    if out==baseline.SOURCE_DIR or baseline.SOURCE_DIR in out.parents:
        parser.error('Candidate must remain outside accepted source directory')
    registry=json.loads((baseline.SOURCE_DIR/'baseline.json').read_text())
    for name,expected in registry['sha256'].items():
        assert hashlib.sha256((baseline.SOURCE_DIR/name).read_bytes()).hexdigest().upper()==expected
    original=baseline.read_obj(baseline.SOURCE_DIR/(baseline.STEM+'.obj'))
    candidate=refine(copy.deepcopy(original))
    report=verify(original,candidate)
    out.mkdir(parents=True,exist_ok=True)
    baseline.ROOT=out
    baseline.STEM=STEM
    obj=baseline.export(candidate)
    report['exportReadback']=verify(original,baseline.read_obj(obj))
    report['sha256']=hashlib.sha256(obj.read_bytes()).hexdigest().upper()
    if args.adopt:
        descriptor_path=baseline.SOURCE_DIR/'runtime-model.json'
        descriptor=json.loads(descriptor_path.read_text())
        validate_runtime(descriptor,candidate)
        for extension in ('obj','mtl'):
            source=out/(STEM+'.'+extension)
            destination=baseline.SOURCE_DIR/source.name
            data=source.read_bytes()
            if destination.exists() and destination.read_bytes()!=data:
                raise ValueError('Refusing to replace different reviewed model bytes: '+str(destination))
            destination.write_bytes(data)
            descriptor['sources'][extension]={'path':source.name,
                'sha256':hashlib.sha256(data).hexdigest().upper()}
        descriptor_path.write_bytes((json.dumps(descriptor,indent=2)+'\n').encode('utf-8'))
    if args.render:
        import render_model as renderer
        renderer.ROOT=out
        renderer.STEM=STEM
        renderer._render_all()
        renderer._render_details()
    (out/'geometry-audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'result':'passed','obj':str(obj),'sha256':report['sha256'],
                      'changedObjects':len(report['changedObjects']),
                      'unchangedObjects':report['unchangedObjects'],
                      'runtimeContract':'unchanged','installed':False}))


if __name__=='__main__': main()
