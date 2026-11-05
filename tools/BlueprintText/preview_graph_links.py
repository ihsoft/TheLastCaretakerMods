"""Resolve function navigation only against BPT graphs present in the preview."""
import json
from urllib.parse import quote
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


def snapshot_graphs(entry, snapshot, reviewing):
    current = entry['docs'].get('source')
    old = entry['docs'].get('before')
    if entry['status'] == 'unchanged' and reviewing:
        # Dedicated Before pages keep reverse navigation in the old snapshot,
        # even when the callee itself did not change.
        old = current
    current_pages, old_pages = graph_pages(current, old)
    if snapshot == 'before' and entry['status'] == 'unchanged' and not reviewing:
        return current, current_pages
    return (current, current_pages) if snapshot == 'source' else (old, old_pages)


def build_catalog(entries):
    result = {'source': {}, 'before': {}}
    reviewing = any(e['status'] != 'unchanged' for e in entries)
    for index, entry in enumerate(entries):
        if not displayed(entry):
            continue
        for snapshot in result:
            doc, pages = snapshot_graphs(entry, snapshot, reviewing)
            if doc is None:
                continue
            result[snapshot][entry['asset']] = [dict(id=graph_key(g), name=g['name'],
                href=f'../blueprint-{index:03d}/' + pages[graph_key(g)])
                for g in doc.get('graphs', []) if g.get('kind') == 'function']
    return result


def build_callers(entries, catalog):
    """Reverse represented calls/ownership, grouped by calling graph per snapshot."""
    result = {'source': {}, 'before': {}}
    reviewing = any(e['status'] != 'unchanged' for e in entries)
    for snapshot, targets in result.items():
        for index, entry in enumerate(entries):
            if not displayed(entry):
                continue
            doc, pages = snapshot_graphs(entry, snapshot, reviewing)
            if doc is None:
                continue
            registry = {graph_key(g): g for g in doc.get('graphs', [])}

            def add(target_asset, target_graph, caller, node):
                key = (target_asset, target_graph)
                caller_key = (entry['asset'], graph_key(caller))
                group = targets.setdefault(key, {})
                if caller_key not in group:
                    path, cursor = [], caller
                    while True:
                        path.append(cursor['name'])
                        if 'owner' not in cursor:
                            break
                        cursor = registry[cursor['owner']['graph']]
                    group[caller_key] = dict(asset=entry['asset'], graph=graph_key(caller),
                        name=' / '.join([entry['asset'].rsplit('/', 1)[-1], *reversed(path)]),
                        href=f'../blueprint-{index:03d}/' + pages[graph_key(caller)], nodes=[])
                if node not in group[caller_key]['nodes']:
                    group[caller_key]['nodes'].append(node)

            for key, graph in registry.items():
                if 'owner' in graph:
                    owner = graph['owner']
                    add(entry['asset'], key, registry[owner['graph']], owner['node'])
                for node, target in resolve_calls(doc, key, catalog[snapshot]).items():
                    add(target['target'].split('::', 1)[0], target['id'], graph, node)
        for key, group in targets.items():
            callers = sorted(group.values(), key=lambda c: (c['name'], c['asset'], c['graph']))
            for caller in callers:
                if len(caller['nodes']) == 1:
                    caller['href'] += '#node-' + quote(caller['nodes'][0], safe='')
            targets[key] = callers
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
