"""Compare Harpoon native import inventory and cooked colors with a GLB audit.

Python 3.10+ / NumPy. Read-only; no source conversion or Unreal invocation.
This checks rigid node transforms and untextured PBR factors, not shader appearance.
"""
import argparse
import json
from pathlib import Path
import numpy as np


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def verify(audit, inventory, semantic):
    assert audit['counts']['textures'] == 0 and audit['counts']['animations'] == 0, 'This check covers untextured rigid models only'
    source = {n['name']: n for n in audit['nodes'] if n['active']}
    components = {n['name']: n for n in inventory['components']}
    assert len(components) == len(inventory['components']), 'Duplicate imported names'
    root = inventory['roles']['root']
    change = np.eye(4)
    change[:3, :3] = [[1, 0, 0], [0, 0, 1], [0, 1, 0]]
    max_error = 0.0
    for name, node in source.items():
        original = np.array(node['localMatrixColumnMajor']).reshape(4, 4, order='F')
        if name not in components:
            assert node['mesh'] is None and np.allclose(original, np.eye(4)), 'Lost nontrivial GLB node: ' + name
            assert name == source[root]['parent'], 'Lost model node: ' + name
            continue
        actual = components[name]
        if name != root:
            assert actual['parent'] == node['parent'], 'Reparented: ' + name
        expected = change @ original @ change
        expected[:3, 3] *= 100
        pitch, yaw, roll = np.deg2rad(actual['rotation'])
        cp, sp, cy, sy, cr, sr = np.cos(pitch), np.sin(pitch), np.cos(yaw), np.sin(yaw), np.cos(roll), np.sin(roll)
        rotate = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]]) @ np.array([[cp, 0, -sp], [0, 1, 0], [sp, 0, cp]]) @ np.array([[1, 0, 0], [0, cr, sr], [0, -sr, cr]])
        imported = np.eye(4)
        imported[:3, :3] = rotate @ np.diag(actual['scale'])
        imported[:3, 3] = actual['location']
        error = float(np.max(np.abs(expected - imported)))
        max_error = max(max_error, error)
        assert error < 0.002, f'Transform differs: {name}: {error}'
        assert bool(actual['mesh']) == (node['mesh'] is not None), 'Mesh presence differs: ' + name
    assert set(components) <= set(source), 'Unexpected imported nodes'
    materials = {m['name']: m for m in audit['materials']}
    checked = set()
    for evidence in semantic['assetEvidence']:
        if '/Materials/' not in evidence['virtualPath']:
            continue
        exports = read(evidence['jsonPath'])
        material = next(e for e in exports if e['Type'] == 'MaterialInstanceConstant')
        name = material['Name']
        src = materials[name]
        pbr = src.get('pbrMetallicRoughness', {})
        vectors = {p['ParameterInfo']['Name']: p['ParameterValue'] for p in material['Properties'].get('VectorParameterValues', [])}
        scalars = {p['ParameterInfo']['Name']: p['ParameterValue'] for p in material['Properties'].get('ScalarParameterValues', [])}
        for key, expected, channels in [('BaseColorFactor', pbr.get('baseColorFactor', [1]*4), 'RGBA'), ('EmissiveFactor', src.get('emissiveFactor', [0]*3), 'RGB')]:
            fallback = [1]*4 if key == 'BaseColorFactor' else [0]*3
            actual = [vectors[key][c] for c in channels] if key in vectors else fallback
            assert np.allclose(actual, expected, atol=1e-6), name + '/' + key
        for key, field in [('MetallicFactor', 'metallicFactor'), ('RoughnessFactor', 'roughnessFactor')]:
            assert abs(scalars.get(key, 1.0) - pbr.get(field, 1.0)) < 1e-6, name + '/' + key
        checked.add(name)
    assert checked == set(materials), 'Incomplete cooked material evidence'
    return dict(status='passed', components=len(components), materials=len(checked), maxMatrixError=max_error,
                limitations='Rigid local transforms and untextured factors; glTF-default fallbacks assume reviewed native parent. Not gameplay/visual validation.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-audit', required=True)
    parser.add_argument('--inventory', required=True)
    parser.add_argument('--semantic', required=True)
    args = parser.parse_args()
    print(json.dumps(verify(read(args.source_audit), read(args.inventory), read(args.semantic))))
