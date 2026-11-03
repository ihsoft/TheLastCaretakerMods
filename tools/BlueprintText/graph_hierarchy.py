"""Explicit, asset-local collapsed graph ownership; no engine lookups."""
import re


def graph_key(graph):
    # GraphGuid already owns identity; do not duplicate it in a second field.
    return graph.get('properties', {}).get('GraphGuid', graph['name'])


def select_graph(document, selection=None):
    graphs = document.get('graphs', [])
    matches = [g for g in graphs if graph_key(g) == selection] if selection else graphs
    if selection and not matches:
        matches = [g for g in graphs if g['name'] == selection]
    if len(matches) != 1:
        raise ValueError('Select exactly one graph by ID (or an unambiguous name)')
    return matches[0]


def validate_hierarchy(document):
    graphs = document['graphs']
    nested = any('owner' in g or g['kind'] == 'collapsed' for g in graphs)
    by_id, scopes, owners = {}, set(), set()
    for g in graphs:
        if set(g['properties']) & {'Nodes', 'SubGraphs', 'Schema'}:
            raise ValueError('Graph structure must use its body and owner records')
        key = graph_key(g)
        if key in by_id:
            raise ValueError('Duplicate graph identity')
        by_id[key] = g
        if nested and not re.fullmatch('[A-F0-9]{32}', g['properties'].get('GraphGuid', '')):
            raise ValueError('Nested graph hierarchy requires existing GraphGuid identities')
        owner = g.get('owner')
        if (g['kind'] == 'collapsed') != (owner is not None):
            raise ValueError('Collapsed graph requires an owner')
        if owner is not None:
            if not isinstance(owner, dict) or set(owner) != {'graph', 'node', 'entry', 'exit'} or any(not isinstance(v, str) or not v for v in owner.values()):
                raise ValueError('Invalid collapsed graph owner record')
            identity = (owner['graph'], owner['node'])
            if identity in owners:
                raise ValueError('Composite owns multiple graphs')
            owners.add(identity)
            scope = (*identity, g['name'])
        else:
            if g['kind'] not in ('event', 'function', 'interface'):
                raise ValueError('Unsupported root graph kind')
            scope = (g['name'],)
        if scope in scopes:
            raise ValueError('Duplicate graph name in its owner scope')
        scopes.add(scope)
    for g in graphs:
        seen, cursor = set(), g
        while 'owner' in cursor:
            key = graph_key(cursor)
            if key in seen:
                raise ValueError('Cyclic collapsed graph ownership')
            seen.add(key)
            parent = cursor['owner']['graph']
            if parent not in by_id:
                raise ValueError('Missing parent graph')
            cursor = by_id[parent]
    classes = document.get('classes', {})
    templates = document.get('pinTemplates', {})

    def cls(node):
        return classes.get(node['kind'])

    def pins(node):
        result = {}
        for alias, p in node['pins'].items():
            name = p.get('name', alias.split(':', 1)[1])
            direction = alias.split(':', 1)[0]
            key = (name, direction)
            if key in result:
                raise ValueError('Ambiguous collapsed graph boundary pin')
            metadata = templates[p['type']]
            result[key] = {k: v for k, v in metadata.items() if k.startswith('PinType.')}
        return result

    for g in graphs:
        nodes = {n['id']: n for n in g['body']['nodes']}
        for n in nodes.values():
            if cls(n) in ('/Script/BlueprintGraph.K2Node_Composite', '/Script/BlueprintGraph.K2Node_Tunnel'):
                if set(n['properties']) & {'BoundGraph', 'InputSinkNode', 'OutputSourceNode'}:
                    raise ValueError('Boundary ownership must use the graph owner record')
            if cls(n) == '/Script/BlueprintGraph.K2Node_Composite' and (graph_key(g), n['id']) not in owners:
                raise ValueError('Composite has no owned graph')
            if cls(n) == '/Script/BlueprintGraph.K2Node_Tunnel':
                owner = g.get('owner', {})
                if n['id'] not in (owner.get('entry'), owner.get('exit')):
                    raise ValueError('Tunnel is not an owned entry or exit')
        if 'owner' not in g:
            continue
        owner = g['owner']
        parent_nodes = {n['id']: n for n in by_id[owner['graph']]['body']['nodes']}
        composite = parent_nodes.get(owner['node'])
        if not composite or cls(composite) != '/Script/BlueprintGraph.K2Node_Composite':
            raise ValueError('Collapsed graph owner is not a Composite node')
        if owner['entry'] == owner['exit']:
            raise ValueError('Entry and exit must be distinct tunnels')
        boundary = {}
        for field, direction, enabled, disabled in (
                ('entry', 'out', 'bCanHaveOutputs', 'bCanHaveInputs'),
                ('exit', 'in', 'bCanHaveInputs', 'bCanHaveOutputs')):
            tunnel = nodes.get(owner[field])
            if not tunnel or cls(tunnel) != '/Script/BlueprintGraph.K2Node_Tunnel':
                raise ValueError('Missing entry or exit tunnel')
            if tunnel['properties'].get(enabled, 'False') != 'True' or tunnel['properties'].get(disabled, 'False') != 'False':
                raise ValueError('Invalid tunnel direction flags')
            for (name, actual), ptype in pins(tunnel).items():
                if actual != direction:
                    raise ValueError('Invalid tunnel pin direction')
                mirror = (name, 'in' if actual == 'out' else 'out')
                if mirror in boundary:
                    raise ValueError('Duplicate tunnel boundary')
                boundary[mirror] = ptype
        if boundary != pins(composite):
            raise ValueError('Composite and tunnel pin boundaries differ')
    return by_id


def inspect_graphs(document, selection=None):
    """Return a read-only working view, never an incomplete Apply document."""
    registry = validate_hierarchy(document)
    def row(g):
        return dict(id=graph_key(g), name=g['name'], kind=g['kind'],
                    nodes=len(g['body']['nodes']), links=len(g['body']['links']),
                    **({'owner': g['owner']} if 'owner' in g else {}))
    result = dict(asset=document['asset'], view='graph-summary', graphs=[row(g) for g in document['graphs']])
    if selection is None:
        return result
    graph = select_graph(document, selection)
    nodes = list(graph['body']['nodes'])
    ancestors, cursor = [], graph
    while 'owner' in cursor:
        parent = registry[cursor['owner']['graph']]
        operation = next(n for n in parent['body']['nodes'] if n['id'] == cursor['owner']['node'])
        ancestors.append(dict(graph=row(parent), operation=operation))
        nodes.append(operation)
        cursor = parent
    return dict(asset=document['asset'], view='graph-detail', graph=graph, ancestors=list(reversed(ancestors)),
                children=[row(g) for g in document['graphs'] if g.get('owner', {}).get('graph') == graph_key(graph)],
                classes={n['kind']: document['classes'][n['kind']] for n in nodes},
                pinTemplates={p['type']: document['pinTemplates'][p['type']] for n in nodes for p in n['pins'].values()})


def main():
    import argparse
    import json
    from pathlib import Path
    parser = argparse.ArgumentParser(description='Read graph summaries or one graph and its boundary context from BPT. No Unreal launch; output is a read-only view.')
    parser.add_argument('input', type=Path)
    parser.add_argument('--graph', help='Stable GraphGuid or unambiguous display name')
    args = parser.parse_args()
    try:
        document = json.loads(args.input.read_text(encoding='utf-8-sig'))
        if document.get('schema') != 2 or document.get('format') != 'compact-asset':
            raise ValueError('Expected schema-2 compact-asset')
        print(json.dumps(inspect_graphs(document, args.graph), ensure_ascii=False, indent=2))
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(2, f'Cannot inspect BPT graphs: {error}\n')


if __name__ == '__main__':
    main()
