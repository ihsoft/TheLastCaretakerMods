"""Locate literal asset references in editable BPT fields, not registry dependencies."""
import re
import call_arguments
from asset_text_codec import fields as native_fields

ASSET_PATH = re.compile(r'/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)+(?:\.[A-Za-z0-9_]+)?')


def references(value, location=()):
    if isinstance(value, dict):
        for key, child in value.items():
            yield from references(child, (*location, key))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            label = child.get('name', child.get('VarName', str(index))) if isinstance(child, dict) else str(index)
            yield from references(child, (*location, label))
    elif isinstance(value, str):
        for target in dict.fromkeys(ASSET_PATH.findall(value)):
            if not target.startswith('/Script/'):
                yield ' / '.join(location), target


def node_references(node, templates):
    fields = {k: v for k, v in node.items() if not k.startswith('_') and k not in ('id', 'kind', 'comment')}
    # User-defined pin declarations duplicate signature types, but may also
    # contain meaningful defaults. Keep those fields distinct for presentation.
    if 'custom' in fields:
        custom = []
        for text in fields['custom']:
            prefix, separator, payload = text.partition(' (') if isinstance(text, str) else ('', '', '')
            if prefix == 'CustomProperties UserDefinedPin' and separator and payload.endswith(')'):
                try:
                    text = native_fields(payload[:-1])
                except ValueError:
                    pass  # Unrecognized declarations retain their literal references.
            custom.append(text)
        fields['custom'] = custom
    fields['pins'] = {name: dict(templates.get(pin.get('type'), {}), **pin)
                      for name, pin in node.get('pins', {}).items()}
    return references(fields)


def is_type_reference(field):
    """Signature/owner metadata, distinct from a chosen asset or class value."""
    parts = field.split(' / ')
    return (len(parts) >= 3 and parts[0] == 'pins' and parts[2].startswith('PinType.')) or (
        len(parts) == 3 and parts[0] == 'custom' and parts[2] == 'PinType') or (
        parts[0] == 'properties' and len(parts) == 2 and parts[1] in (
            'VariableReference', 'FunctionReference', 'EventReference', 'DelegateReference'))


def is_implicit_target(node, templates, field):
    parts = field.split(' / ')
    if len(parts) != 3 or parts[0] != 'pins' or parts[2] != 'DefaultObject':
        return False
    pin = node.get('pins', {}).get(parts[1], {})
    return (parts[1] in ('in:target', 'in:self') and pin.get('name', 'self') == 'self' and
            templates.get(pin.get('type'), {}).get('bHidden') == 'True' and
            'FunctionReference' in node.get('properties', {}))


def with_type_reference_details(node, templates):
    """View-only metadata keeps resolved pin types accessible in node Details."""
    refs = [dict(field=field, target=target) for field, target in node_references(node, templates)
            if is_type_reference(field) or is_implicit_target(node, templates, field)]
    result = dict(node, _typeReferences=refs) if refs else node
    if 'arguments' in node:
        details = {}
        for alias, pin in node.get('pins', {}).items():
            if call_arguments.governed(node, alias, templates):
                defaults = node.get('declarationDefaults', {})
                details[alias] = dict(mode='override' if alias in node['arguments'] else 'declaration-default' if alias in defaults else 'required',
                    declaration=node.get('properties', {}).get('FunctionReference', ''))
                if alias in defaults:
                    details[alias]['declarationDefault'] = defaults[alias]
        result = dict(result, _argumentDetails=details)
    return result


def property_references(document):
    fields = {k: v for k, v in document.items() if k in (
        'parent', 'defaults', 'variables', 'components', 'root', 'interfaces', 'widget', 'properties', 'nativeDefaults')}
    return references(fields)


def package(target):
    return target.split('.', 1)[0]


def label(target, context):
    state = context.get(package(target), {})
    suffix = ' [STUB]' if state.get('stub') else ' [not in project]' if state.get('missing') else ''
    return target.rsplit('/', 1)[-1] + suffix
