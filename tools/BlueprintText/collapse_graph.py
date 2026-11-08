"""Bounded, offline extraction of explicitly selected BPT nodes. No semantic grouping."""
from collections import Counter
import copy
import re
import uuid

import asset_text_codec as codec
from graph_review import review_marks
from graph_hierarchy import graph_key, select_graph


TYPE_DEFAULTS = dict(PinCategory='""', PinSubCategory='""', PinSubCategoryObject='None',
                     PinSubCategoryMemberReference='()', PinValueType='()', ContainerType='None',
                     bIsReference='False', bIsConst='False', bIsWeakPointer='False',
                     bIsUObjectWrapper='False', bSerializeAsSinglePrecisionFloat='False')
NODE_CLASSES = {'CallFunction', 'VariableGet', 'VariableSet', 'IfThenElse', 'ExecutionSequence',
                'Self', 'DynamicCast', 'Knot', 'MakeStruct', 'BreakStruct', 'Select',
                'EnumEquality', 'CommutativeAssociativeBinaryOperator', 'Composite'}


def uid():
    return uuid.uuid4().hex.upper()


def node_id(endpoint):
    return endpoint.split('.', 1)[0]


def valid_name(value):
    return isinstance(value, str) and re.fullmatch('[A-Za-z_][A-Za-z0-9_]*', value)


def pin_type(document, nodes, endpoint):
    identity, alias = endpoint.split('.', 1)
    metadata = document['pinTemplates'][nodes[identity]['pins'][alias]['type']]
    result = dict(TYPE_DEFAULTS)
    for key, value in metadata.items():
        if key.startswith('PinType.'):
            key = key.removeprefix('PinType.')
            if key not in result:
                raise ValueError('Unsupported boundary pin type field: ' + key)
            result[key] = value
        elif key in ('ParentPin', 'SubPins') or (key in ('bOrphanedPin', 'bNotConnectable') and value == 'True'):
            raise ValueError('Split, orphaned or non-connectable boundary pin: ' + endpoint)
    return result


def boundary_type(document, nodes, endpoint):
    result = pin_type(document, nodes, endpoint)
    if codec.unquote(result['PinCategory']) not in {
            'exec', 'bool', 'byte', 'int', 'int64', 'real', 'float', 'double',
            'name', 'string', 'text', 'object', 'class', 'struct'}:
        raise ValueError('Unsupported boundary pin category at ' + endpoint)
    if result['ContainerType'] not in ('None', 'Array') or result['bIsReference'] != 'False' or result['bIsWeakPointer'] != 'False':
        raise ValueError('Reference, weak pointer, map and set boundaries are not supported: ' + endpoint)
    return result


def expand(document, child_id):
    """Exact structural inverse, also used by tests. Never evaluates or rewrites operations."""
    result = copy.deepcopy(document)
    child = select_graph(result, child_id)
    owner = child['owner']
    parent = select_graph(result, owner['graph'])
    composite = next(n for n in parent['body']['nodes'] if n['id'] == owner['node'])
    borders = {owner['entry'], owner['exit']}
    moved = [n for n in child['body']['nodes'] if n['id'] not in borders]
    existing = {n['id'] for n in parent['body']['nodes'] if n is not composite}
    if existing & {n['id'] for n in moved}:
        raise ValueError('Expansion would collide with existing node identities')
    outer, inner = parent['body']['links'], child['body']['links']
    links = [e for e in outer if all(node_id(v) != composite['id'] for v in e.values())]
    links += [e for e in inner if all(node_id(v) not in borders for v in e.values())]
    for alias in composite['pins']:
        direction, name = alias.split(':', 1)
        if direction == 'in':
            sources = [e['from'] for e in outer if e['to'] == composite['id'] + '.' + alias]
            targets = [e['to'] for e in inner if e['from'] == owner['entry'] + '.out:' + name]
        else:
            sources = [e['from'] for e in inner if e['to'] == owner['exit'] + '.in:' + name]
            targets = [e['to'] for e in outer if e['from'] == composite['id'] + '.' + alias]
        links += [dict(zip(('from', 'to'), (a, b))) for a in sources for b in targets]
    parent['body'] = dict(nodes=[n for n in parent['body']['nodes'] if n is not composite] + moved, links=links)
    for graph in result['graphs']:
        if graph.get('owner', {}).get('graph') == child_id:
            graph['owner']['graph'] = graph_key(parent)
    result['graphs'].remove(child)
    return result


def verify_inverse(before, after, child_id):
    restored = expand(after, child_id)
    # New pool entries are allowed; every original entry and all original node payloads stay intact.
    restored['classes'] = {k: restored['classes'][k] for k in before['classes']}
    restored['pinTemplates'] = {k: restored['pinTemplates'][k] for k in before['pinTemplates']}
    def normalized(document):
        result = copy.deepcopy(document)
        result['graphs'].sort(key=graph_key)
        for graph in result['graphs']:
            graph['body']['nodes'].sort(key=lambda n: n['id'])
            graph['body']['links'].sort(key=lambda e: (e['from'], e['to']))
        return result
    if normalized(before) != normalized(restored):
        raise ValueError('Collapse inverse check failed: original payload or connectivity changed')


def collapse(document, selection, selected, name, boundary_names=None):
    codec.native(document)
    original = document
    document = copy.deepcopy(document)
    parent = select_graph(document, selection)
    nodes = {n['id']: n for n in parent['body']['nodes']}
    if not selected or len(set(selected)) != len(selected) or not set(selected) <= nodes.keys():
        raise ValueError('Select existing, distinct node IDs in the chosen graph')
    selected = set(selected)
    if not valid_name(name):
        raise ValueError('Name must be an ASCII identifier')
    composite_id = 'K2Node_Composite_' + name
    if composite_id in nodes or any(g['name'].casefold() == name.casefold() for g in document['graphs']):
        raise ValueError('Graph or composite name already exists: ' + name)
    boundary_names = {} if boundary_names is None else boundary_names
    if not isinstance(boundary_names, dict) or any(not valid_name(v) for v in boundary_names.values()):
        raise ValueError('BoundaryNames must map crossing source endpoints to ASCII parameter names')
    for identity in selected:
        cls = document['classes'][nodes[identity]['kind']]
        if cls not in {'/Script/BlueprintGraph.K2Node_' + c for c in NODE_CLASSES}:
            raise ValueError('Unsupported selected node class: ' + cls + ' (' + identity + ')')
    links = parent['body']['links']
    incoming = [e for e in links if node_id(e['from']) not in selected and node_id(e['to']) in selected]
    outgoing = [e for e in links if node_id(e['from']) in selected and node_id(e['to']) not in selected]
    internal = [e for e in links if all(node_id(v) in selected for v in e.values())]
    reached = {next(iter(selected))}
    while True:
        previous = set(reached)
        for e in internal:
            if any(node_id(v) in reached for v in e.values()):
                reached.update(node_id(v) for v in e.values())
        if reached == previous:
            break
    if reached != selected:
        raise ValueError('Selected nodes must form one connected operation')
    # Reject a path leaving the selection and re-entering it, including data dependencies.
    reached = {node_id(e['to']) for e in outgoing}
    while True:
        previous = set(reached)
        reached.update(node_id(e['to']) for e in links if node_id(e['from']) in previous)
        if reached & selected:
            raise ValueError('Selection has a dependency path leaving and re-entering it')
        if reached == previous:
            break
    def execution(endpoint):
        return pin_type(document, nodes, endpoint)['PinCategory'] == '"exec"'
    exec_in = [e for e in incoming if execution(e['from'])]
    exec_out = [e for e in outgoing if execution(e['from'])]
    if len({e['to'] for e in exec_in}) > 1 or len({e['to'] for e in exec_out}) > 1:
        raise ValueError('Collapse supports one execution entry and at most one continuation')
    executable = any(execution(n + '.' + p) for n in selected for p in nodes[n]['pins'])
    if executable and not exec_in:
        raise ValueError('Executable selection requires one connected execution entry')
    for edge in incoming + outgoing:
        source_type = boundary_type(document, nodes, edge['from'])
        target_type = boundary_type(document, nodes, edge['to'])
        # Existing object/class edges may point from a derived value to a base parameter.
        # Carry the actual source type through the tunnel, keeping the old conversion
        # at the original target. No class hierarchy discovery or guessed casts.
        ignored = {'PinSubCategoryObject'} if source_type['PinCategory'] in ('"object"', '"class"') else set()
        if any(source_type[k] != target_type[k] for k in source_type.keys() - ignored):
            raise ValueError('Unsupported boundary type conversion: ' + str(edge))
    crossing = {e['from'] for e in incoming + outgoing}
    if boundary_names.keys() - crossing:
        raise ValueError('BoundaryNames contains endpoints outside the selected boundary')
    used_names = set()
    groups = []
    for direction, edges in (('in', incoming), ('out', outgoing)):
        buckets = {}
        for edge in edges:
            key = 'execution' if execution(edge['from']) else edge['from']
            buckets.setdefault(key, []).append(edge)
        for key, edges in buckets.items():
            explicit = {boundary_names[e['from']] for e in edges if e['from'] in boundary_names}
            if len(explicit) > 1:
                raise ValueError('One execution boundary cannot have several names')
            if key == 'execution':
                parameter = 'execute' if direction == 'in' else 'then'
                if explicit and explicit != {parameter}:
                    raise ValueError('Execution boundary names are fixed: execute / then')
            else:
                parameter = next(iter(explicit), '')
                if not parameter:
                    endpoint = edges[0]['from']
                    identity, alias = endpoint.split('.', 1)
                    hint = nodes[identity]['pins'][alias].get('name', alias.split(':', 1)[1])
                    hint = hint if valid_name(hint) and hint.casefold() not in ('returnvalue', 'result', 'output_get', 'execute', 'then') else 'Value'
                    parameter, suffix = hint, 2
                    while parameter.casefold() in used_names:
                        parameter, suffix = hint + str(suffix), suffix + 1
            if parameter.casefold() in used_names:
                raise ValueError('Duplicate boundary parameter name: ' + parameter)
            used_names.add(parameter.casefold())
            groups.append((direction, parameter, boundary_type(document, nodes, edges[0]['from']), edges))
    for kind in ('Composite', 'Tunnel'):
        cls = '/Script/BlueprintGraph.K2Node_' + kind
        if kind in document['classes'] and document['classes'][kind] != cls:
            raise ValueError('Conflicting class alias: ' + kind)
        document['classes'][kind] = cls
    def tunnel(identity, flag):
        while identity in selected:
            identity += '_'
        return dict(id=identity, kind='Tunnel', properties={flag: 'True', 'NodeGuid': uid()}, pins={}, custom=[])
    entry = tunnel('K2Node_Tunnel_Entry', 'bCanHaveOutputs')
    exit_node = tunnel('K2Node_Tunnel_Exit', 'bCanHaveInputs')
    composite = dict(id=composite_id, kind='Composite', properties={'NodeGuid': uid()}, pins={})
    child_links = copy.deepcopy(internal)
    parent_links = [copy.deepcopy(e) for e in links if all(node_id(v) not in selected for v in e.values())]
    def add(edges, a, b):
        edge = dict(zip(('from', 'to'), (a, b)))
        if edge not in edges:
            edges.append(edge)
    for direction, parameter, ptype, edges in sorted(groups, key=lambda g: g[0] == 'in'):
        metadata = {'PinType.' + k: v for k, v in ptype.items()}
        metadata.update(PersistentGuid='0'*32, **{k: 'False' for k in (
            'bHidden', 'bNotConnectable', 'bDefaultValueIsReadOnly', 'bDefaultValueIsIgnored', 'bAdvancedView', 'bOrphanedPin')})
        template = next((k for k, v in document['pinTemplates'].items() if v == metadata), None)
        if template is None:
            template = 'collapse_' + str(len(document['pinTemplates']))
            while template in document['pinTemplates']:
                template += '_'
            document['pinTemplates'][template] = metadata
        alias = 'exec' if parameter == 'execute' else parameter
        def pin():
            return dict(id=uid(), type=template, **({'name': parameter} if alias != parameter else {}))
        composite['pins'][direction + ':' + alias] = pin()
        boundary = entry if direction == 'in' else exit_node
        reverse = 'out' if direction == 'in' else 'in'
        boundary['pins'][reverse + ':' + alias] = pin()
        type_text = ','.join(k + '=' + v for k, v in ptype.items() if v != TYPE_DEFAULTS[k])
        boundary['custom'].append('CustomProperties UserDefinedPin (PinName="' + parameter + '",PinType=(' + type_text
                                  + '),DesiredPinDirection=EGPD_' + ('Output' if reverse == 'out' else 'Input') + ')')
        for edge in edges:
            if direction == 'in':
                add(parent_links, edge['from'], composite_id + '.in:' + alias)
                add(child_links, entry['id'] + '.out:' + alias, edge['to'])
            else:
                add(child_links, edge['from'], exit_node['id'] + '.in:' + alias)
                add(parent_links, composite_id + '.out:' + alias, edge['to'])
    child_id = uid()
    child = dict(name=name, kind='collapsed', properties={'GraphGuid': child_id}, format='compact',
                 owner=dict(graph=graph_key(parent), node=composite_id, entry=entry['id'], exit=exit_node['id']),
                 body=dict(nodes=[entry] + [n for n in parent['body']['nodes'] if n['id'] in selected] + [exit_node], links=child_links))
    before_count = len(nodes)
    parent['body'] = dict(nodes=[n for n in parent['body']['nodes'] if n['id'] not in selected] + [composite], links=parent_links)
    for graph in document['graphs']:
        if graph.get('owner', {}).get('graph') == graph_key(parent) and graph['owner']['node'] in selected:
            graph['owner']['graph'] = child_id
    document['graphs'].insert(document['graphs'].index(parent) + 1, child)
    codec.native(document)
    verify_inverse(original, document, child_id)
    return document, dict(status='collapsed', asset=document['asset'], graph=graph_key(parent),
                          childGraph=child_id, name=name, movedNodes=len(selected), parentNodesBefore=before_count,
                          parentNodesAfter=len(parent['body']['nodes']), boundaryPins=len(groups), inverseVerified=True, applied=False)
