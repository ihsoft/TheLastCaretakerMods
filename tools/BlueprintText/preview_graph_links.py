"""Resolve function navigation only against BPT graphs present in the preview."""
import json
from asset_text_codec import fields
from graph_hierarchy import graph_key, select_graph


def displayed(entry):
    context = entry['item'].get('context')
    return bool(entry['docs']) and (not context or not entry['item'].get('placeholder') or context['selected'])


def graph_pages(current, before):
    current_ids = [graph_key(g) for g in (current or {}).get('graphs', [])]
    old_ids = [graph_key(g) for g in (before or {}).get('graphs', [])]
    names = current_ids + [key for key in old_ids if key not in current_ids]
    return ({key: f'graph-{i:03d}.html' for i, key in enumerate(names) if key in current_ids},
            {key: f'graph-{i:03d}-before.html' for i, key in enumerate(names) if key in old_ids})


def build_catalog(entries):
    result = {'source': {}, 'before': {}}
    for index, entry in enumerate(entries):
        if not displayed(entry):
            continue
        docs = entry['docs']
        current, old = docs.get('source'), docs.get('before')
        current_pages, old_pages = graph_pages(current, old)
        for snapshot in result:
            doc, pages = (current, current_pages) if snapshot == 'source' else (old, old_pages)
            # An unchanged dependency has the same implementation in both snapshots.
            if snapshot == 'before' and entry['status'] == 'unchanged':
                doc, pages = current, current_pages
            if doc is None:
                continue
            result[snapshot][entry['asset']] = [dict(id=graph_key(g), name=g['name'],
                href=f'../blueprint-{index:03d}/' + pages[graph_key(g)])
                for g in doc.get('graphs', []) if g.get('kind') == 'function']
    return result


def object_package(raw):
    if raw.startswith('"'):
        raw = json.loads(raw)
    if "'" in raw:
        raw = raw.split("'", 2)[1]
    return raw.split('.', 1)[0] if raw.startswith('/') else None


def resolve_calls(document, selection, catalog):
    result = {}
    for node in select_graph(document, selection)['body']['nodes']:
        kind = document.get('classes', {}).get(node.get('kind'), node.get('kind', '')).rsplit('.', 1)[-1]
        if kind not in ('call', 'CallFunction', 'CallParentFunction', 'K2Node_CallFunction', 'K2Node_CallParentFunction'):
            continue
        raw = node.get('properties', {}).get('FunctionReference', '')
        reference = fields(raw[1:-1]) if raw.startswith('(') and raw.endswith(')') else {}
        name = reference.get('MemberName', '')
        name = json.loads(name) if name.startswith('"') else name
        identity = reference.get('MemberGuid')
        owner = None
        if 'MemberParent' in reference:
            owner = object_package(reference['MemberParent'])
        elif reference.get('bSelfContext') == 'True':
            owner = document['asset']
        else:
            target = node.get('pins', {}).get('in:target', node.get('pins', {}).get('in:self', {}))
            metadata = document.get('pinTemplates', {}).get(target.get('type'), {})
            owner = object_package(metadata.get('PinType.PinSubCategoryObject', ''))
        if not reference and '::' in node.get('operation', ''):
            raw_owner, name = node['operation'].rsplit('::', 1)
            owner = object_package(raw_owner)
        matches = [g for g in catalog.get(owner, [])
                   if (g['id'] == identity if identity and identity != '0' * 32 else g['name'] == name)]
        if len(matches) == 1:
            target = matches[0]
            result[node['id']] = dict(target, field='FunctionReference', kind='graph',
                target=owner + '::' + target['name'])
    return result
