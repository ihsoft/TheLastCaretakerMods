"""Test-only selection of operations from the isolated native nested-graph fixture."""
import json
from pathlib import Path
import sys


def plan(root):
    manifest = json.loads((root/'workspace.json').read_text(encoding='utf-8-sig'))
    result = []
    for item in manifest['assets']:
        doc = json.loads((root/item['source']).read_text(encoding='utf-8-sig'))
        for graph in doc['graphs']:
            if graph['name'] == 'RunProbe':
                child = next(g for g in doc['graphs'] if g.get('owner', {}).get('graph') == graph['properties']['GraphGuid'])
                nodes = {n['id']: n for n in child['body']['nodes']}
                selected = {next(n['id'] for n in nodes.values() if n['kind'] == 'setVariable')}
                while True:
                    before = set(selected)
                    for edge in child['body']['links']:
                        origin, alias = edge['from'].split('.', 1)
                        if edge['to'].split('.', 1)[0] in selected:
                            pin = nodes[origin]['pins'][alias]
                            if doc['pinTemplates'][pin['type']]['PinType.PinCategory'] != '"exec"':
                                selected.add(origin)
                    if before == selected:
                        break
                result.append(dict(asset=doc['asset'], graph=child['properties']['GraphGuid'], nodes=sorted(selected), name='AppendTraceDigit'))
                arithmetic = next(g for g in doc['graphs'] if g.get('owner', {}).get('graph') == child['properties']['GraphGuid'])
                result.append(dict(asset=doc['asset'], graph=arithmetic['properties']['GraphGuid'],
                                   nodes=[n['id'] for n in arithmetic['body']['nodes'] if n['kind'] == 'call'], name='AddOffset'))
            if doc['asset'].endswith('Actor') and graph['name'] in ('EventGraph', 'UserConstructionScript'):
                composite = next(n for n in graph['body']['nodes'] if n['kind'] == 'Composite')
                result.append(dict(asset=doc['asset'], graph=graph['properties']['GraphGuid'],
                                   nodes=[composite['id']], name='Wrap' + graph['name']))
    if len(result) != 4:
        raise ValueError('Fixture shape changed; review test selections')
    return result


if __name__ == '__main__':
    print(json.dumps(plan(Path(sys.argv[1]))))
