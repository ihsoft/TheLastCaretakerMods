"""Edits for the isolated argument acceptance runner, never a mod authoring tool."""
import json
import sys
from pathlib import Path

root, mode = Path(sys.argv[1]), sys.argv[2]
manifest = json.loads((root / 'workspace.json').read_text(encoding='utf-8-sig'))
for item in manifest['assets']:
    path = root / item['source']
    doc = json.loads(path.read_text(encoding='utf-8-sig'))
    if doc['asset'].endswith('Caller') and mode == 'move':
        node = next(n for g in doc['graphs'] if g['name'] == 'Probe' for n in g['body']['nodes'] if n['id'] == 'Inherited')
        node['properties']['NodePosX'] = '123'
    elif doc['asset'].endswith('Caller') and mode == 'missing':
        node=next(n for g in doc['graphs'] if g['name']=='Probe' for n in g['body']['nodes'] if n['id']=='Required')
        node['arguments'].pop('in:Value')
    elif doc['asset'].endswith('Callee') and mode == 'default':
        entry = next(n for g in doc['graphs'] if g['name'] == 'EchoInt' for n in g['body']['nodes'] if n['kind'] == 'FunctionEntry')
        entry['parameterDefaults']['out:Value'] = '9'
    else:
        continue
    path.write_text(json.dumps(doc, indent=2) + '\n', encoding='utf-8')
