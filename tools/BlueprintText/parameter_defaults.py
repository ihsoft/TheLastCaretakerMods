"""Function declaration defaults, including the distinction between empty and absent."""
import json
import call_arguments

METADATA = 'BlueprintText.OptionalParameters'


def metadata(properties, names=None):
    from asset_text_codec import fields, split_fields, quote, unquote
    raw = properties.get('MetaData', '()')
    meta = fields(raw[1:-1])
    pairs = split_fields(meta.get('MetaDataMap', '()')[1:-1])
    kept, found = [], None
    for pair in pairs:
        parts = split_fields(pair[1:-1])
        if len(parts) == 2 and parts[0].strip('"') == METADATA:
            found = json.loads(unquote(parts[1]))
        else:
            kept.append(pair)
    if names is not None:
        kept.append('(' + quote(METADATA) + ',' + quote(json.dumps(names)) + ')')
    if kept:
        meta['MetaDataMap'] = '(' + ','.join(kept) + ')'
    else:
        meta.pop('MetaDataMap', None)
    if meta:
        properties['MetaData'] = '(' + ','.join(k+'='+v for k,v in meta.items()) + ')'
    else:
        properties.pop('MetaData', None)
    return found


def supported(alias, pin):
    return alias.startswith('out:') and call_arguments.eligible('in:' + alias[4:], pin)


def user_pins(custom, values, managed_names):
    from asset_text_codec import fields, quote, unquote
    result = []
    for line in custom:
        prefix = 'CustomProperties UserDefinedPin ('
        if line.startswith(prefix) and line.endswith(')'):
            pin = fields(line[len(prefix):-1])
            name = unquote(pin['PinName'])
            if name in managed_names:
                pin.pop('PinDefaultValue', None)
                if values is not None and name in values:
                    pin['PinDefaultValue'] = quote(values[name])
                line = prefix + ','.join(k+'='+v for k,v in pin.items()) + ')'
        result.append(line)
    return result


def decode(node):
    from asset_text_codec import unquote, pin_alias
    if node['class'] != '/Script/BlueprintGraph.K2Node_FunctionEntry':
        return None
    # Do not normalize unrelated native metadata on legacy declarations.
    if METADATA not in node['properties'].get('MetaData', ''):
        return None
    names = metadata(node['properties'])
    if not isinstance(names, list) or any(not isinstance(n,str) for n in names) or len(set(names)) != len(names):
        raise ValueError('Invalid optional parameter declaration')
    values, managed = {}, set()
    for pin in node['pins']:
        alias = pin_alias(node,pin)
        if supported(alias,pin):
            name = unquote(pin['PinName']); managed.add(name)
            if name in names:
                values[alias] = unquote(pin.get('DefaultValue', '""'))
            pin.pop('DefaultValue', None)
    if set(names) - managed:
        raise ValueError('Unsupported optional parameter declaration')
    node['custom'] = user_pins(node['custom'], None, managed)
    return values


def validate(node, templates):
    if 'parameterDefaults' not in node:
        return
    values = node['parameterDefaults']
    if node['kind'] != 'FunctionEntry' or not isinstance(values,dict):
        raise ValueError('parameterDefaults belongs to FunctionEntry')
    for alias,value in values.items():
        if alias not in node['pins'] or not supported(alias,templates[node['pins'][alias]['type']]) or not isinstance(value,str):
            raise ValueError('Unsupported parameter default: ' + alias)
    for alias in node.get('defaults',{}):
        if supported(alias,templates[node['pins'][alias]['type']]):
            raise ValueError('Use parameterDefaults for a governed function declaration')


def enable(node, templates):
    """Preserve nonempty legacy declaration values; empty defaults need intent."""
    if node['kind'] != 'FunctionEntry':
        raise ValueError('Select a FunctionEntry declaration')
    node.setdefault('parameterDefaults', {})
    names = set()
    for alias,pin in node['pins'].items():
        if supported(alias,templates[pin['type']]):
            names.add(pin.get('name',alias[4:]))
            value = node.get('defaults', {}).pop(alias,None)
            if value:
                node['parameterDefaults'].setdefault(alias,value)
    if not node.get('defaults'):
        node.pop('defaults',None)
    node['custom'] = user_pins(node.get('custom',[]),None,names)
    validate(node,templates)
