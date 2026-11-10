"""Optional exact declarations behind a selected graph, without runtime inference."""
import copy

from graph_hierarchy import graph_key
from graph_symbols import (CALL_KINDS, node_kind, member_identity, reference_fields,
                           enclosing_graph, input_literal, input_alias, incoming)


def local_declarations(document, graph):
    parent = enclosing_graph(document, graph)
    result = []
    for entry in parent['body']['nodes']:
        if node_kind(document, entry) not in {'K2Node_FunctionEntry', 'FunctionEntry'}:
            continue
        for field, raw in entry.get('properties', {}).items():
            if field.startswith('LocalVariables('):
                declaration = reference_fields(raw)
                result.append(dict(name=declaration.get('VarName'), id=declaration.get('VarGuid'),
                    graph=graph_key(parent), scope=parent['name'], node=entry['id'], field=field,
                    source=raw, declaration=declaration))
    return result


def local_context(document, graph, nodes):
    declarations = local_declarations(document, graph)
    result = []
    seen = set()
    for node in nodes:
        identity = member_identity(document, node, 'VariableReference')
        if not identity['scope']:
            continue
        key = (identity['scope'], identity['name'], identity['guid'])
        if key in seen:
            continue
        seen.add(key)
        guid = identity['guid']
        matches = [d for d in declarations if d['scope'] == identity['scope'] and
                   (d['id'] == guid if guid and guid != '0' * 32 else d['name'] == identity['name'])]
        row = dict(name=identity['name'], scope=identity['scope'])
        if len(matches) == 1 and identity['owner'] == document['asset']:
            row.update(status='resolved', **matches[0])
        else:
            row.update(status='unresolved', reason='No unique declaration in the enclosing function')
        result.append(row)
    return result


def component_detail(document, selection):
    native = document.get('nativeDefaults', {}).get('objects', [])
    objects = document.get('components', {}).get('objects', []) + native
    matches = [obj for obj in objects if obj['id'] == selection]
    if not matches:
        matches = [obj for obj in objects if obj['name'] == selection]
    if len(matches) != 1:
        raise ValueError('Select exactly one component object by ID or unambiguous name')
    root = matches[0]
    owned = {root['id']}
    while True:
        expanded = owned | {obj['id'] for obj in objects if obj.get('outer') in owned}
        if expanded == owned:
            break
        owned = expanded
    section = document.get('components', {})
    return copy.deepcopy(dict(asset=document['asset'], view='component-detail', readOnly=True,
        object=root, ownedObjects=[obj for obj in objects if obj['id'] in owned and obj != root],
        roles=(['native-default-subobject'] if root in native else
               [key for key in ('templates', 'generatedTemplates') if root['id'] in section.get(key, [])])))


def component_context(document, graph, nodes):
    section = document.get('components', {})
    templates = set(section.get('templates', []) + section.get('generatedTemplates', []))
    result = []
    for node in nodes:
        identity = member_identity(document, node)
        if node_kind(document, node) not in CALL_KINDS or identity['name'] != 'AddComponent':
            continue
        if identity['owner'] not in {document['asset'], '/Script/Engine.Actor'}:
            continue
        name = input_literal(graph, node, 'TemplateName')
        row = dict(node=node['id'], name=name)
        context_alias = input_alias(node, 'ComponentTemplateContext')
        explicit_context = (context_alias is not None and (
            incoming(graph, node, context_alias) or node['pins'][context_alias].get('DefaultObject') not in (None, 'None', '"None"') or
            node.get('defaults', {}).get(context_alias) not in (None, '', 'None')))
        matches = [obj for obj in section.get('objects', []) if obj['name'] == name and obj['id'] in templates]
        if name is not None and not explicit_context and len(matches) == 1:
            row.update(status='source-template', detail=component_detail(document, matches[0]['id']))
        else:
            row.update(status='unresolved', reason='Dynamic/explicit template context or no unique local template')
        result.append(row)
    return result


def inspect_context(document, graph, nodes, context):
    if context not in ('locals', 'components', 'all'):
        raise ValueError('Expected context locals, components or all')
    result = {}
    if context in ('locals', 'all'):
        result['locals'] = local_context(document, graph, nodes)
    if context in ('components', 'all'):
        result['components'] = component_context(document, graph, nodes)
    return copy.deepcopy(result)
