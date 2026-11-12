"""Edits for isolated library acceptance; not a mod authoring helper."""
import json
import sys
from pathlib import Path
from asset_text_codec import add_call

root, mode = Path(sys.argv[1]), sys.argv[2]
manifest = json.loads((root / 'workspace.json').read_text(encoding='utf-8-sig'))
for item in manifest['assets']:
    path = root / item['source']
    doc = json.loads(path.read_text(encoding='utf-8-sig'))
    library = doc['parent'] == '/Script/Engine.BlueprintFunctionLibrary'
    if mode == 'move' and not library:
        catalog = json.loads(Path(sys.argv[3]).read_text(encoding='utf-8-sig'))
        graph = next(g for g in doc['graphs'] if g['name'] == 'Probe')
        links = graph['body']['links']
        graph['body'] = dict(nodes=[n for n in graph['body']['nodes'] if n['id'] not in ('IsLarge', 'IsMedium', 'Optional')], links=[])
        for name in ('IsLarge', 'IsMedium', 'Optional'):
            doc = add_call(doc, catalog, '/Game/BPTTypeLibrary.BPTTypeLibrary_C::' + name, name, 'Probe')
        next(g for g in doc['graphs'] if g['name'] == 'Probe')['body']['links'] = links
    for graph in doc['graphs']:
        large = library and graph['name'] == 'IsLarge'
        if mode == 'rename' and large:
            graph['name'] = 'IsLargeFamily'
        for node in graph['body']['nodes']:
            if mode == 'move':
                node['properties']['NodePosX'] = str(int(node['properties'].get('NodePosX', '0')) + 20)
            if mode != 'rename':
                continue
            if library and graph['name'] == 'Optional' and node['kind'] == 'FunctionEntry':
                node['parameterDefaults']['out:Value'] = '9'
            ref = node['properties'].get('FunctionReference', '')
            calls_large = 'MemberName="IsLarge"' in ref or 'MemberName=IsLarge,' in ref
            if calls_large:
                node['properties']['FunctionReference'] = ref.replace('MemberName="IsLarge"', 'MemberName="IsLargeFamily"').replace('MemberName=IsLarge,', 'MemberName=IsLargeFamily,')
            if large or calls_large:
                for old in ('in:Actor', 'out:Actor'):
                    if old not in node['pins']:
                        continue
                    new = old.replace('Actor', 'Candidate')
                    node['pins'] = {new if k == old else k:v for k,v in node['pins'].items()}
                    for field in ('defaults', 'arguments', 'declarationDefaults', 'parameterDefaults'):
                        if old in node.get(field, {}):
                            node[field][new] = node[field].pop(old)
                    node['custom'] = [text.replace('PinName="Actor"', 'PinName="Candidate"') for text in node.get('custom', [])]
                    if not node['custom']:
                        node.pop('custom')
                    for edge in graph['body']['links']:
                        for direction in ('from', 'to'):
                            if edge[direction] == node['id'] + '.' + old:
                                edge[direction] = node['id'] + '.' + new
    path.write_text(json.dumps(doc, indent=2) + '\n', encoding='utf-8')
