"""Exchange-time call arguments and legacy node decoding; saved calls are native."""
CALL_CLASS = '/Script/BlueprintText.BlueprintTextCallFunction'
CALL_CLASSES = (CALL_CLASS, '/Script/BlueprintTextPrototype.BlueprintTextCallFunction')
LEGACY_CLASS = '/Script/BlueprintGraph.K2Node_CallFunction'
STATE = 'BPTArgumentOverrides'
DECLARATION_STATE = 'BPTDeclarationDefaults'


def eligible(alias, metadata):
    return (alias.startswith('in:') and alias != 'in:self'
            and metadata.get('PinType.PinCategory', '').strip('"') in ('int', 'int64', 'real', 'bool', 'string')
            and metadata.get('PinType.ContainerType', 'None') == 'None'
            and metadata.get('PinType.bIsReference', 'False') != 'True'
            and metadata.get('bDefaultValueIsIgnored', 'False') != 'True'
            and metadata.get('bOrphanedPin', 'False') != 'True'
            and not metadata.get('ParentPin') and not metadata.get('SubPins'))


def governed(node, alias, templates):
    return 'arguments' in node and eligible(alias, templates[node['pins'][alias]['type']])


def validate(node, templates):
    if 'arguments' not in node:
        if 'declarationDefaults' in node:
            raise ValueError('declarationDefaults requires arguments')
        return
    args = node['arguments']
    if node['kind'] != 'call' or not isinstance(args, dict):
        raise ValueError('arguments requires an ordinary function call and an object of overrides')
    for alias, value in args.items():
        if alias not in node['pins'] or not governed(node, alias, templates) or not isinstance(value, str):
            raise ValueError('Unsupported argument override: ' + alias)
    if not isinstance(node.get('declarationDefaults', {}),dict):
        raise ValueError('declarationDefaults must be a snapshot object')
    for alias,value in node.get('declarationDefaults', {}).items():
        if alias not in node['pins'] or not governed(node,alias,templates) or not isinstance(value,str):
            raise ValueError('Invalid declaration snapshot: ' + alias)
    for alias in node.get('defaults', {}):
        if governed(node, alias, templates):
            raise ValueError('Use arguments, not defaults, for optional input: ' + alias)


def enable(node, templates, *, inherit=False):
    """Explicit migration preserves literals; new template calls inherit by default."""
    if node['kind'] != 'call':
        raise ValueError('Only ordinary calls support arguments')
    node.setdefault('arguments', {})
    for alias in list(node.get('defaults', {})):
        if governed(node, alias, templates):
            value = node['defaults'].pop(alias)
            if not inherit:
                node['arguments'][alias] = value
    if not node.get('defaults'):
        node.pop('defaults', None)
    validate(node, templates)


def explicit_values(node):
    return {**node.get('defaults', {}), **node.get('arguments', {})}


def preview_documents(documents):
    """Resolve proposed selected declarations for review, without writing sources.

    Unavailable/native declarations retain the last engine-exported snapshot.
    This is a view, never engine validation or a runtime dispatch prediction.
    """
    import copy
    from asset_text_codec import quote
    from graph_references import function_catalog
    from graph_symbols import member_identity, function_matches
    result = copy.deepcopy(documents)
    catalog = function_catalog(result)
    by_asset = {d['asset']: d for d in result}
    for doc in result:
        for graph in doc['graphs']:
            for node in graph['body']['nodes']:
                if 'arguments' not in node:
                    continue
                identity = member_identity(doc, node)
                matches = function_matches(identity, catalog)
                if len(matches) != 1:
                    continue
                callee = next(g for g in by_asset[identity['owner']]['graphs'] if g['name'] == matches[0]['name'])
                entries = [n for n in callee['body']['nodes'] if n['kind'] == 'FunctionEntry']
                if len(entries) != 1:
                    continue
                entry = entries[0]
                for alias, pin in node['pins'].items():
                    if not governed(node, alias, doc['pinTemplates']):
                        continue
                    name = pin.get('name', alias[3:])
                    inputs = [a for a,p in entry['pins'].items() if a.startswith('out:') and p.get('name', a[4:]) == name]
                    if len(inputs) != 1:
                        continue
                    metadata = dict(doc['pinTemplates'][pin['type']])
                    if 'parameterDefaults' in entry:
                        value = entry['parameterDefaults'].get(inputs[0])
                    else:
                        # Legacy empty values have no reliable presence marker.
                        value = entry.get('defaults', {}).get(inputs[0]) or None
                    node.setdefault('declarationDefaults', {}).pop(alias, None)
                    if value is None:
                        continue
                    node['declarationDefaults'][alias] = value
                    metadata['AutogeneratedDefaultValue'] = quote(value)
                    key = '__argument_preview_' + pin['id']
                    doc['pinTemplates'][key] = metadata
                    pin['type'] = key
    return result
