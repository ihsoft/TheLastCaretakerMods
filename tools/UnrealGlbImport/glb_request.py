"""Validate a rigid GLB scene and prepare current-input hierarchy expectations."""
import argparse
import json
from pathlib import Path
import re
import struct
import sys

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'glb'))
from inspect_glb import inspect
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'UnrealResources'))
from request import below_tmp


def prepare(project, source, destination, output):
    project, output = below_tmp(project), below_tmp(output)
    source = Path(source).resolve()
    if not project.is_file() or project.suffix.lower() != '.uproject':
        raise ValueError('Expected a prepared working .uproject below repository Tmp')
    if not re.fullmatch(r'/Game/(?:[A-Za-z_][A-Za-z0-9_]*/)*[A-Za-z_][A-Za-z0-9_]*', destination):
        raise ValueError('Destination must be a new /Game/ content folder')
    if (project.parent / 'Content' / destination.removeprefix('/Game/')).exists():
        raise ValueError('Destination already exists; import into a fresh folder and review before replacement')
    if output.exists() or output.is_relative_to(project.parent) or project.is_relative_to(output):
        raise ValueError('Output must be fresh and separate from the working project')
    if source.suffix.lower() != '.glb':
        raise ValueError('Expected a self-contained GLB 2 file')
    audit = inspect(source)
    raw = source.read_bytes()
    source_json = json.loads(raw[20:20 + struct.unpack_from('<I',raw,12)[0]])
    if source_json.get('cameras') or 'KHR_lights_punctual' in source_json.get('extensionsUsed',[]):
        raise ValueError('Camera/light scenes require a separate authoring operation')
    if audit['counts']['animations']:
        raise ValueError('This tool imports rigid component assemblies, not animation clips')
    # Match the GLTF translator coordinate convention: metres -> centimetres,
    # glTF Y-up right-handed -> Unreal Z-up left-handed, X unchanged.
    basis = np.array([[100,0,0,0],[0,0,100,0],[0,100,0,0],[0,0,0,1]], dtype=float)
    nodes = []
    names = set()
    for index, node in enumerate(audit['nodes']):
        if not node['active']:
            continue
        explicit_name=source_json['nodes'][index].get('name')
        if not isinstance(explicit_name,str) or not explicit_name.strip() or explicit_name.casefold() in names:
            raise ValueError('Active nodes require explicit names unique without regard to case')
        names.add(explicit_name.casefold())
        local = np.array(node['localMatrixColumnMajor']).reshape(4,4,order='F')
        world = np.array(node['worldMatrixColumnMajor']).reshape(4,4,order='F')
        for transform in [local,world]:
            axes = transform[:3,:3]
            norms = np.linalg.norm(axes, axis=0)
            if np.any(norms < 1e-8) or not np.allclose((axes/norms).T @ (axes/norms), np.eye(3), atol=1e-5):
                raise ValueError(f'Node {node["name"]}: zero scale or accumulated shear cannot be preserved by SceneComponent TRS')
        nodes.append({'index': index, 'name':node['name'], 'parent':node['parent'], 'mesh':node['mesh'],
                      'localMatrix':(basis @ local @ np.linalg.inv(basis)).tolist(),
                      'worldMatrix':(basis @ world @ np.linalg.inv(basis)).tolist()})
    if not nodes or not any(n['mesh'] is not None for n in nodes):
        raise ValueError('The selected GLB scene must contain a mesh')
    return {'project':str(project),'source':str(source),'destination':destination,'output':str(output),
            'nodes':nodes,'sourceCounts':audit['counts']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['project','source','destination','output']:
        parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    request = prepare(args.project,args.source,args.destination,args.output)
    output = Path(request['output'])
    output.mkdir(parents=True)
    (output/'request.json').write_text(json.dumps(request,indent=2)+'\n',encoding='utf-8')
