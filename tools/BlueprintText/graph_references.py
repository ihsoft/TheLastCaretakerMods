"""Find represented member uses in explicitly supplied BPT sources; read-only."""
import copy
import json
from pathlib import Path

from asset_text_codec import unquote
from graph_hierarchy import graph_key, validate_hierarchy
from graph_inspection import checked_graph
from graph_context import local_declarations
from graph_symbols import (CALL_KINDS, node_kind, member_identity, matching_symbols,
                           function_matches, graph_path, input_literal, target_owner)


TIMERS = {'K2_SetTimer', 'K2_SetTimerForNextTick'}
GET_KINDS = {'K2Node_VariableGet', 'getVariable'}
SET_KINDS = {'K2Node_VariableSet', 'setVariable'}


def function_catalog(documents):
    return {d['asset']: [dict(id=graph_key(g), name=g['name']) for g in d['graphs']
                         if g['kind'] in ('function', 'interface')]
            for d in documents}


def function_resolution(identity, catalog):
    matches = function_matches(identity, catalog)
    if len(matches) == 1:
        return dict(status='resolved', graph=matches[0]['id'], name=matches[0]['name'])
    if identity['owner'] is None:
        reason = 'Target owner is not represented'
    elif identity['owner'] not in catalog:
        reason = 'Owner source was not supplied (or is native)'
    else:
        reason = 'No unique function declaration; inherited/runtime dispatch is not inferred'
    return dict(status='unresolved', reason=reason)


def variable_resolution(identity, document, graph, by_asset):
    owner = by_asset.get(identity['owner'])
    if owner is None:
        return dict(status='unresolved', reason='Owner source was not supplied (or is native)')
    if identity['scope']:
        if owner is not document:
            return dict(status='unresolved', reason='Local variable owner differs from source')
        candidates = [d for d in local_declarations(document, graph) if d['scope'] == identity['scope']]
    else:
        candidates = [dict(name=unquote(v.get('VarName', '')), id=v.get('VarGuid')) for v in owner.get('variables', [])]
    matches = matching_symbols(identity, candidates)
    if len(matches) != 1:
        return dict(status='unresolved', reason='No unique variable declaration in its owner/scope')
    result = dict(status='resolved', name=matches[0]['name'])
    if identity['scope']:
        result['graph'] = matches[0]['graph']
    return result


def uses_in_graph(document, graph):
    """Yield category, identity and source evidence; never create execution links."""
    for node in graph['body']['nodes']:
        kind = node_kind(document, node)
        if kind in CALL_KINDS:
            identity = member_identity(document, node)
            yield node, 'function', 'direct-call', identity, dict(field='FunctionReference')
            if identity['owner'] == '/Script/Engine.KismetSystemLibrary' and identity['name'] in TIMERS:
                name = input_literal(graph, node, 'FunctionName')
                owner = target_owner(document, graph, node, 'Object')
                yield node, 'function', 'timer', dict(owner=owner, name=name, guid=None, scope=None), dict(
                    field='FunctionName', api=identity['name'])
        elif kind in GET_KINDS | SET_KINDS:
            yield node, 'variable', ('read' if kind in GET_KINDS else 'write'), member_identity(
                document, node, 'VariableReference'), dict(field='VariableReference')
        elif kind in {'K2Node_CreateDelegate', 'CreateDelegate'}:
            props = node.get('properties', {})
            identity = dict(owner=target_owner(document, graph, node, 'self', default_self=True),
                name=unquote(props.get('SelectedFunctionName', '')), guid=props.get('SelectedFunctionGuid'), scope=None)
            bindings = []
            for link in graph['body']['links']:
                if not any(link['from'] == node['id'] + '.' + a for a in node['pins'] if a.startswith('out:')):
                    continue
                for consumer in graph['body']['nodes']:
                    if not any(link['to'] == consumer['id'] + '.' + a for a in consumer['pins'] if a.startswith('in:')):
                        continue
                    consumer_kind = node_kind(document, consumer)
                    action = {'K2Node_AddDelegate': 'bind', 'AddDelegate': 'bind',
                              'K2Node_RemoveDelegate': 'unbind', 'RemoveDelegate': 'unbind',
                              'K2Node_AssignDelegate': 'assign', 'AssignDelegate': 'assign'}.get(consumer_kind)
                    if action:
                        bindings.append(dict(node=consumer['id'], action=action,
                            delegate=member_identity(document, consumer, 'DelegateReference')))
            yield node, 'function', 'delegate-reference', identity, dict(field='SelectedFunctionName', bindings=bindings)


def find_references(documents, *, function=None, variable=None, owner=None, scope=None):
    if (function is None) == (variable is None):
        raise ValueError('Select exactly one function or variable name')
    query, category = (function, 'function') if function is not None else (variable, 'variable')
    if not query:
        raise ValueError('Expected a nonempty exact member name')
    if scope is not None and category != 'variable':
        raise ValueError('--scope applies only to local variables')
    by_asset = {}
    for document in documents:
        if document.get('schema') != 2 or document.get('format') != 'compact-asset':
            raise ValueError('Expected schema-2 compact-asset')
        if document['asset'] in by_asset:
            raise ValueError('Supply one source per asset: ' + document['asset'])
        by_asset[document['asset']] = document
        validate_hierarchy(document)
        for graph in document['graphs']:
            checked_graph(document, graph)
    catalog = function_catalog(documents)
    results, unresolved = [], []
    for document in documents:
        for graph in document['graphs']:
            for node, item_category, kind, identity, evidence in uses_in_graph(document, graph):
                if item_category != category:
                    continue
                if owner is not None and identity['owner'] not in (owner, None):
                    continue
                if scope is not None and identity['scope'] != scope:
                    continue
                resolution = (function_resolution(identity, catalog) if category == 'function' else
                              variable_resolution(identity, document, graph, by_asset))
                matched = identity['name'] == query or (resolution['status'] == 'resolved' and resolution['name'] == query)
                # A dynamic name cannot be counted as a query match. Keep it in
                # a separate uncertainty list so an empty result is not proof of no use.
                if not matched and identity['name']:
                    continue
                row = dict(asset=document['asset'], graph=graph_key(graph), path=graph_path(document, graph),
                           node=node['id'], kind=kind, target=identity, resolution=resolution, **evidence)
                if 'EnabledState' in node.get('properties', {}):
                    row['enabledState'] = node['properties']['EnabledState']
                if matched and (owner is None or identity['owner'] == owner):
                    results.append(row)
                else:
                    row['reason'] = 'Name or owner cannot be matched statically'
                    unresolved.append(row)
    return copy.deepcopy(dict(view='member-uses', readOnly=True,
        query=dict(kind=category, name=query, owner=owner, scope=scope), sources=list(by_asset),
        uses=results, unresolvedCandidates=unresolved,
        coverage='Explicit call/get/set, CreateDelegate and named Kismet timers in supplied sources only; '
                 'no runtime dispatch, delegate propagation, arbitrary string calls or external asset discovery'))


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', nargs='+', type=Path, help='Explicit schema-2 source files; no recursive discovery')
    query = parser.add_mutually_exclusive_group(required=True)
    query.add_argument('--function', help='Exact function name')
    query.add_argument('--variable', help='Exact variable name')
    parser.add_argument('--owner', help='Exact owning asset package or /Script/Module.Class')
    parser.add_argument('--scope', help='Exact enclosing function name for a local variable')
    args = parser.parse_args()
    try:
        documents = [json.loads(path.read_text(encoding='utf-8-sig')) for path in args.input]
        result = find_references(documents, function=args.function, variable=args.variable, owner=args.owner, scope=args.scope)
        result['sourceFiles'] = [dict(asset=d['asset'], path=str(p.resolve())) for p, d in zip(args.input, documents)]
        print(json.dumps(result, ensure_ascii=False, indent=2))
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(2, f'Cannot inspect BPT references: {error}\n')


if __name__ == '__main__':
    main()
