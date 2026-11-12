"""Source member identities shared by navigation and inspection; no engine queries."""
from asset_text_codec import fields, unquote


CALL_KINDS = {'call', 'CallFunction', 'CallParentFunction', 'K2Node_CallFunction', 'K2Node_CallParentFunction'}


def node_kind(document, node):
    return document.get('classes', {}).get(node.get('kind'), node.get('kind', '')).rsplit('.', 1)[-1]


def reference_fields(raw):
    if not isinstance(raw, str) or not raw.startswith('(') or not raw.endswith(')'):
        return {}
    return {k: unquote(v) for k, v in fields(raw[1:-1]).items()}


def object_path(raw):
    raw = unquote(raw)
    if "'" in raw:
        raw = raw.split("'", 2)[1]
    return raw if raw.startswith('/') else None


def object_package(raw):
    path = object_path(raw)
    return path.split('.', 1)[0] if path else None


def member_owner(raw):
    path = object_path(raw)
    # Native class names are identities, not just their /Script package.
    return path if path and path.startswith('/Script/') else object_package(raw)


def member_identity(document, node, field='FunctionReference'):
    ref = reference_fields(node.get('properties', {}).get(field, ''))
    owner = None
    if 'MemberParent' in ref:
        owner = member_owner(ref['MemberParent'])
    elif ref.get('bSelfContext') == 'True' or ref.get('MemberScope'):
        owner = document['asset']
    else:
        target = node.get('pins', {}).get('in:target', node.get('pins', {}).get('in:self', {}))
        metadata = document.get('pinTemplates', {}).get(target.get('type'), {})
        owner = member_owner(metadata.get('PinType.PinSubCategoryObject', ''))
    name = ref.get('MemberName', '')
    if not ref and field == 'FunctionReference' and '::' in node.get('operation', ''):
        raw_owner, name = node['operation'].rsplit('::', 1)
        owner = member_owner(raw_owner)
    return dict(owner=owner, name=name, guid=ref.get('MemberGuid'), scope=ref.get('MemberScope'))


def matching_symbols(identity, candidates):
    guid = identity.get('guid')
    return [candidate for candidate in candidates
            if (candidate['id'] == guid if guid and guid != '0' * 32 else candidate['name'] == identity['name'])]


def function_matches(identity, catalog):
    return matching_symbols(identity, catalog.get(identity['owner'], []))


def graph_path(document, graph):
    from graph_hierarchy import graph_key
    registry = {graph_key(g): g for g in document['graphs']}
    path = [graph['name']]
    while 'owner' in graph:
        graph = registry[graph['owner']['graph']]
        path.append(graph['name'])
    return list(reversed(path))


def enclosing_graph(document, graph):
    from graph_hierarchy import graph_key
    registry = {graph_key(g): g for g in document['graphs']}
    while 'owner' in graph:
        graph = registry[graph['owner']['graph']]
    return graph


def input_alias(node, name):
    return next((alias for alias, pin in node.get('pins', {}).items()
                 if alias.startswith('in:') and pin.get('name', alias[3:]) == name), None)


def incoming(graph, node, alias):
    return [link['from'] for link in graph['body']['links'] if link['to'] == node['id'] + '.' + str(alias)]


def input_literal(graph, node, name):
    alias = input_alias(node, name)
    if alias is None or incoming(graph, node, alias):
        return None
    return node.get('arguments', {}).get(alias, node.get('defaults', {}).get(alias))


def target_owner(document, graph, node, name, default_self=False):
    """Only explicit self, concrete pin types or literal class identity; no flow inference."""
    alias = input_alias(node, name)
    if alias is None:
        return None
    sources = incoming(graph, node, alias)
    pin = node['pins'][alias]
    if len(sources) == 1:
        source = sources[0]
        matches = [(n, p) for n in graph['body']['nodes'] for a, p in n['pins'].items()
                   if n['id'] + '.' + a == source]
        if len(matches) != 1:
            return None
        producer, pin = matches[0]
        if node_kind(document, producer) in {'K2Node_Self', 'self'}:
            return document['asset']
    elif sources:
        return None
    elif default_self and pin.get('DefaultObject') in (None, 'None', '"None"') and not node.get('defaults', {}).get(alias):
        return document['asset']
    metadata = document.get('pinTemplates', {}).get(pin.get('type'), {})
    if unquote(metadata.get('PinType.PinSubCategory', '')) == 'self':
        return document['asset']
    owner = member_owner(metadata.get('PinType.PinSubCategoryObject', ''))
    # Object/Actor pins alone cannot identify a named callback's declaring class.
    return None if owner in {'/Script/CoreUObject.Object', '/Script/Engine.Actor'} else owner
