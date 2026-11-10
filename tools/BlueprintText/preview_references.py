"""Locate literal asset references in editable BPT fields, not registry dependencies."""
import re

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
    fields['pins'] = {name: dict(templates.get(pin.get('type'), {}), **pin)
                      for name, pin in node.get('pins', {}).items()}
    return references(fields)


def is_type_reference(field):
    """Signature/owner metadata, distinct from a chosen asset or class value."""
    parts = field.split(' / ')
    return (len(parts) >= 3 and parts[0] == 'pins' and parts[2].startswith('PinType.')) or (
        parts[0] == 'properties' and len(parts) == 2 and parts[1] in (
            'VariableReference', 'FunctionReference', 'EventReference', 'DelegateReference'))


def with_type_reference_details(node, templates):
    """View-only metadata keeps resolved pin types accessible in node Details."""
    refs = [dict(field=field, target=target) for field, target in node_references(node, templates)
            if is_type_reference(field)]
    return dict(node, _typeReferences=refs) if refs else node


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
