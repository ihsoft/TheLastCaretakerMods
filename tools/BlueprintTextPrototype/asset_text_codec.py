"""Schema-2 graph text codec. Keeps unknown native fields; never guesses their meaning.

compact/native convert complete envelopes. add-call inserts an engine-generated
call from the local Unreal catalogue. No Unreal process is launched here.
"""
import argparse
import copy
import json
import re
import uuid
from pathlib import Path


KINDS = {'K2Node_CallFunction': 'call', 'K2Node_Event': 'event',
         'K2Node_VariableGet': 'getVariable', 'K2Node_VariableSet': 'setVariable',
         'K2Node_IfThenElse': 'branch', 'K2Node_ExecutionSequence': 'sequence',
         'K2Node_Self': 'self', 'K2Node_DynamicCast': 'cast'}
PIN_SEPARATE = {'PinId', 'PinName', 'Direction', 'LinkedTo',
                'DefaultValue', 'DefaultObject', 'DefaultTextValue'}
FIELDS = {'schema', 'format', 'asset', 'parent', 'properties', 'variables', 'defaults', 'graphs', 'dependencies', 'root', 'components', 'interfaces', 'widget'}


def check_keys(obj, allowed, context):
    extra = set(obj) - set(allowed)
    if extra:
        raise ValueError(f'Unknown {context} fields: {sorted(extra)}')


def validate_interfaces(document):
    classes, owned = set(), set()
    graphs = {g['name'] for g in document['graphs'] if g['kind'] == 'interface'}
    for interface in document.get('interfaces', []):
        check_keys(interface, {'class', 'graphs'}, 'interface')
        cls = interface['class']
        if not isinstance(cls, str) or not cls or cls in classes:
            raise ValueError('Duplicate or invalid interface class')
        classes.add(cls)
        for name in interface['graphs']:
            if name not in graphs or name in owned:
                raise ValueError('Unresolved or duplicate interface graph')
            owned.add(name)
    if owned != graphs:
        raise ValueError('Interface graph requires an owning interface')


def validate_object_tree(rows):
    objects = {}
    for row in rows:
        check_keys(row, {'id', 'name', 'class', 'outer', 'flags', 'properties'}, 'component object')
        if any(not isinstance(row[k], str) or not row[k] for k in ('id', 'name', 'class', 'outer')):
            raise ValueError('Component object identities must be nonempty strings')
        if row['id'] in objects or row['id'] in ('$blueprint', '$class'):
            raise ValueError('Duplicate or reserved component object identity')
        if type(row['flags']) is not int or row['flags'] < 0:
            raise ValueError('Component flags must be a nonnegative integer')
        if not isinstance(row['properties'], dict) or any(not isinstance(v, str) for v in row['properties'].values()):
            raise ValueError('Component properties must contain native text values')
        objects[row['id']] = row
    resolved, pending = {'$blueprint', '$class'}, set(objects)
    while pending:
        ready = {key for key in pending if objects[key]['outer'] in resolved}
        if not ready:
            raise ValueError('Unresolved or cyclic component object owners')
        resolved.update(ready)
        pending.difference_update(ready)
    return objects


def validate_components(document):
    if sum(key in document for key in ('root', 'components', 'widget')) > 1:
        raise ValueError('Use exactly one of root, components or widget')
    if 'widget' in document:
        data = document['widget']
        check_keys(data, {'tree', 'objects', 'animations'}, 'widget')
        objects = validate_object_tree(data['objects'])
        if data['tree'] not in objects:
            raise ValueError('Unresolved WidgetTree root')
        animations = data.get('animations', [])
        if not isinstance(animations, list) or len(set(animations)) != len(animations) or any(a not in objects for a in animations):
            raise ValueError('Invalid widget animation references')
        validate_widget_links(document, objects)
    if 'components' not in document:
        return
    data = document['components']
    check_keys(data, {'objects', 'scs', 'templates', 'generatedTemplates'}, 'components')
    objects = validate_object_tree(data['objects'])
    if data['scs'] not in objects:
        raise ValueError('Unresolved component SCS root')
    for field in ('templates', 'generatedTemplates'):
        refs = data[field]
        if not isinstance(refs, list) or len(set(refs)) != len(refs) or any(r not in objects for r in refs):
            raise ValueError('Invalid component template references')


def validate_widget_links(document, objects):
    """Check declared panel/slot references before Unreal recursively visits the tree."""
    prefix = document['asset'] + '.'
    def resolve(value):
        if value in ('None', ''):
            return None
        if value.startswith('"'):
            value = json.loads(value)
        if "'" in value and value.endswith("'"):
            value = value.split("'", 1)[1][:-1]
        key = value[len(prefix):] if value.startswith(prefix) else value
        if key not in objects:
            raise ValueError(f'Unresolved widget hierarchy reference: {value}')
        return key
    tree = document['widget']['tree']
    resolve(objects[tree]['properties'].get('RootWidget', 'None'))
    edges, parents = {}, {}
    for key, obj in objects.items():
        props = obj['properties']
        for field in ('RootWidget', 'Parent', 'Slot', 'Content'):
            if field in props:
                resolve(props[field])
        if 'Slots' not in props:
            continue
        slots = props['Slots']
        if not (slots.startswith('(') and slots.endswith(')')):
            raise ValueError(f'Invalid widget Slots: {key}')
        seen = set()
        for value in split_fields(slots[1:-1]):
            slot = resolve(value)
            if not slot or slot in seen:
                raise ValueError(f'Duplicate or empty widget slot: {key}')
            seen.add(slot)
            slot_props = objects[slot]['properties']
            if 'Parent' in slot_props and resolve(slot_props['Parent']) != key:
                raise ValueError(f'Widget slot parent mismatch: {slot}')
            child = resolve(slot_props.get('Content', 'None'))
            if not child:
                continue
            if child in parents:
                raise ValueError(f'Widget has multiple slots: {child}')
            parents[child] = key
            if 'Slot' in objects[child]['properties'] and resolve(objects[child]['properties']['Slot']) != slot:
                raise ValueError(f'Widget slot/content mismatch: {child}')
            edges.setdefault(key, []).append(child)
    # Iterate, rather than recursively walking potentially malformed input.
    active, finished = set(), set()
    for start in edges:
        pending = [(start, False)]
        while pending:
            key, leaving = pending.pop()
            if leaving:
                active.remove(key); finished.add(key)
            elif key in active:
                raise ValueError(f'Cyclic widget hierarchy: {key}')
            elif key not in finished:
                active.add(key); pending.append((key, True))
                pending.extend((child, False) for child in edges.get(key, []))


def split_fields(text, separator=','):
    """Split Unreal tuples without splitting quoted strings or nested structures."""
    items, start, depth, quoted, escaped = [], 0, 0, False, False
    for i, char in enumerate(text):
        if escaped:
            escaped = False
        elif quoted and char == '\\':
            escaped = True
        elif char == '"':
            quoted = not quoted
        elif not quoted:
            if char == '(':
                depth += 1
            elif char == ')':
                depth -= 1
                if depth < 0:
                    raise ValueError('Unbalanced Unreal text')
            elif char == separator and depth == 0:
                if text[start:i].strip():
                    items.append(text[start:i].strip())
                start = i + 1
    if depth or quoted or escaped:
        raise ValueError('Unbalanced Unreal text')
    if text[start:].strip():
        items.append(text[start:].strip())
    return items


def fields(text):
    result = {}
    for part in split_fields(text):
        key, sep, value = part.partition('=')
        if not sep or key in result:
            raise ValueError(f'Invalid or duplicate native field: {part}')
        result[key] = value
    return result


def quote(value):
    if not isinstance(value, str):
        raise ValueError('Pin defaults must be strings')
    return json.dumps(value, ensure_ascii=False)


def unquote(value):
    return json.loads(value) if value.startswith('"') else value


def parse_nodes(text):
    nodes, current = [], None
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith('Begin Object '):
            if current is not None:
                raise ValueError('Nested native objects need explicit support; refusing to discard them')
            header = dict(re.findall(r'(\w+)=("(?:\\.|[^"\\])*"|\S+)', line[13:]))
            check_keys(header, {'Class', 'Name', 'ExportPath'}, 'object header')
            current = {'id': unquote(header['Name']), 'class': header['Class'], 'properties': {}, 'pins': [], 'custom': []}
        elif line == 'End Object':
            if current is None:
                raise ValueError('Unmatched End Object')
            nodes.append(current)
            current = None
        elif current is None:
            raise ValueError(f'Unexpected native line: {line[:100]}')
        elif line.startswith('CustomProperties Pin (') and line.endswith(')'):
            current['pins'].append(fields(line[len('CustomProperties Pin ('):-1]))
        elif line.startswith('CustomProperties '):
            current['custom'].append(line)
        else:
            key, sep, value = line.partition('=')
            if not sep or key in current['properties']:
                raise ValueError(f'Invalid or duplicate node property: {line[:100]}')
            current['properties'][key] = value
    if current is not None:
        raise ValueError('Unclosed object')
    if len({n['id'] for n in nodes}) != len(nodes):
        raise ValueError('Duplicate node names')
    return nodes


def pin_alias(node, pin):
    output = pin.get('Direction') == '"EGPD_Output"'
    name = unquote(pin['PinName'])
    alias = {'execute': 'exec', 'ReturnValue': 'result', 'self': 'target'}.get(name, name)
    if name.startswith('then_') and name[5:].isdigit():
        alias = 'then' + name[5:]
    if node['class'].endswith('.K2Node_CallFunction'):
        inputs = [unquote(p['PinName']) for p in node['pins']
                  if p.get('Direction') != '"EGPD_Output"' and p.get('PinType.PinCategory') != '"exec"'
                  and unquote(p['PinName']) != 'self' and p.get('bHidden') != 'True']
        if inputs == ['A', 'B']:
            alias = {'A': 'left', 'B': 'right'}.get(name, alias)
    return ('out:' if output else 'in:') + alias


def compact(document):
    check_keys(document, FIELDS, 'native envelope')
    validate_components(document)
    validate_interfaces(document)
    if document['schema'] != 2 or document['format'] != 'native-asset':
        raise ValueError('Expected schema 2 native-asset')
    result = copy.deepcopy(document)
    result['format'] = 'compact-asset'
    result['variables'] = [fields(v[1:-1]) for v in document['variables']]
    parsed = [parse_nodes(g['text']) for g in document['graphs']]
    # Intern repeated pin metadata by its full content, with a deterministic order.
    signatures = sorted({json.dumps({k: v for k, v in p.items() if k not in PIN_SEPARATE}, sort_keys=True)
                         for nodes in parsed for n in nodes for p in n['pins']})
    type_ids = {signature: f'p{i}' for i, signature in enumerate(signatures)}
    result['pinTemplates'] = {type_ids[s]: json.loads(s) for s in signatures}
    result['classes'] = {}
    for graph, nodes in zip(result['graphs'], parsed):
        check_keys(graph, {'name', 'kind', 'properties', 'text', 'nodeCount'}, 'native graph')
        if len(nodes) != graph['nodeCount']:
            raise ValueError('Native node count does not match the envelope')
        lookup, all_pins, pin_ids = {}, {}, set()
        for node in nodes:
            for pin in node['pins']:
                alias = pin_alias(node, pin)
                endpoint = node['id'] + '.' + alias
                key = (node['id'], pin['PinId'])
                if pin['PinId'] in pin_ids or endpoint in all_pins:
                    raise ValueError('Duplicate pin identity or ambiguous pin name')
                pin_ids.add(pin['PinId'])
                lookup[key], all_pins[endpoint] = endpoint, pin
        output, links = [], []
        for node in sorted(nodes, key=lambda n: n['id']):
            short = node['class'].rsplit('.', 1)[-1]
            kind = KINDS.get(short, short.removeprefix('K2Node_'))
            if kind in result['classes'] and result['classes'][kind] != node['class']:
                raise ValueError('Conflicting node class alias')
            result['classes'][kind] = node['class']
            item = {'id': node['id'], 'kind': kind, 'properties': node['properties'], 'pins': {}}
            defaults = {}
            for pin in node['pins']:
                endpoint = lookup[node['id'], pin['PinId']]
                alias = endpoint[len(node['id']) + 1:]
                signature = json.dumps({k: v for k, v in pin.items() if k not in PIN_SEPARATE}, sort_keys=True)
                p = {'id': pin['PinId'], 'type': type_ids[signature]}
                name = unquote(pin['PinName'])
                if name != alias.split(':', 1)[1]:
                    p['name'] = name
                if 'DefaultValue' in pin:
                    defaults[alias] = unquote(pin['DefaultValue'])
                for key in ('DefaultObject', 'DefaultTextValue'):
                    if key in pin:
                        p[key] = pin[key]
                item['pins'][alias] = p
                linked = pin.get('LinkedTo', '()')
                for reference in split_fields(linked[1:-1]):
                    other = tuple(reference.split())
                    if other not in lookup:
                        raise ValueError(f'Unresolved native pin link: {other}')
                    target = lookup[other]
                    target_alias = target[len(other[0]) + 1:]
                    if alias.startswith('out:') == target_alias.startswith('out:'):
                        raise ValueError('Only output-to-input links are supported')
                    reciprocal = all_pins[target].get('LinkedTo', '()')
                    if f'{node["id"]} {pin["PinId"]}' not in split_fields(reciprocal[1:-1]):
                        raise ValueError('Non-reciprocal native pin link')
                    if alias.startswith('out:'):
                        links.append({'from': endpoint, 'to': target})
            if defaults:
                item['defaults'] = defaults
            if node['custom']:
                item['custom'] = node['custom']
            output.append(item)
        del graph['text'], graph['nodeCount']
        graph['format'] = 'compact'
        graph['body'] = {'nodes': output, 'links': sorted(links, key=lambda l: (l['from'], l['to']))}
    return result


def native(document):
    check_keys(document, FIELDS | {'classes', 'pinTemplates'}, 'compact envelope')
    validate_components(document)
    validate_interfaces(document)
    if document['schema'] != 2 or document['format'] != 'compact-asset':
        raise ValueError('Expected schema 2 compact-asset')
    result = copy.deepcopy(document)
    result['format'] = 'native-asset'
    dependencies = set()
    result['variables'] = ['(' + ','.join(f'{k}={v}' for k, v in variable.items()) + ')' for variable in document['variables']]
    for graph in result['graphs']:
        check_keys(graph, {'name', 'kind', 'properties', 'format', 'body'}, 'compact graph')
        if graph['format'] != 'compact':
            raise ValueError('Expected compact graph')
        check_keys(graph['body'], {'nodes', 'links'}, 'graph body')
        nodes, links = graph['body']['nodes'], graph['body']['links']
        endpoints, connections, pin_ids = {}, {}, set()
        if len({n['id'] for n in nodes}) != len(nodes):
            raise ValueError('Duplicate node ID')
        for node in nodes:
            check_keys(node, {'id', 'kind', 'properties', 'pins', 'defaults', 'custom'}, 'node')
            if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', node['id']):
                raise ValueError('Schema-2 node IDs must be Unreal object names')
            for alias, pin in node['pins'].items():
                if not alias.startswith(('in:', 'out:')):
                    raise ValueError('Pin aliases require in:/out: direction')
                check_keys(pin, {'id', 'type', 'name', 'DefaultObject', 'DefaultTextValue'}, 'pin')
                if not re.fullmatch('[A-Fa-f0-9]{32}', pin['id']):
                    raise ValueError('Invalid pin GUID')
                if pin['id'].upper() in pin_ids:
                    raise ValueError('Duplicate pin GUID')
                pin_ids.add(pin['id'].upper())
                endpoint = node['id'] + '.' + alias
                endpoints[endpoint] = (node['id'], pin['id'])
                connections[endpoint] = []
        seen = set()
        for link in links:
            check_keys(link, {'from', 'to'}, 'link')
            a, b = link['from'], link['to']
            if a not in endpoints or b not in endpoints or '.out:' not in a or '.in:' not in b or (a, b) in seen:
                raise ValueError(f'Invalid or duplicate link: {link}')
            seen.add((a, b))
            connections[a].append(endpoints[b]); connections[b].append(endpoints[a])
        blocks = []
        for node in nodes:
            cls = document['classes'][node['kind']]
            lines = [f'Begin Object Class={cls} Name={quote(node["id"])}']
            lines += [f'   {k}={v}' for k, v in node['properties'].items()]
            if set(node.get('defaults', {})) - set(node['pins']):
                raise ValueError('Default refers to an unknown pin')
            for alias, pin in node['pins'].items():
                p = {'PinId': pin['id'], 'PinName': quote(pin.get('name', alias.split(':', 1)[1]))}
                if alias.startswith('out:'):
                    p['Direction'] = '"EGPD_Output"'
                template = document['pinTemplates'][pin['type']]
                if set(template) & PIN_SEPARATE:
                    raise ValueError('Pin template contains per-pin fields')
                p.update(template)
                if alias in node.get('defaults', {}):
                    p['DefaultValue'] = quote(node['defaults'][alias])
                for key in ('DefaultObject', 'DefaultTextValue'):
                    if key in pin:
                        p[key] = pin[key]
                if 'DefaultObject' in pin:
                    dependency = unquote(pin['DefaultObject'])
                    if dependency != 'None' and not dependency.startswith(document['asset'] + '.'):
                        dependencies.add(dependency)
                endpoint = node['id'] + '.' + alias
                if connections[endpoint]:
                    p['LinkedTo'] = '(' + ''.join(f'{n} {i},' for n, i in sorted(connections[endpoint])) + ')'
                lines.append('   CustomProperties Pin (' + ','.join(f'{k}={v}' for k, v in p.items()) + ',)')
            lines += ['   ' + line for line in node.get('custom', [])]
            lines.append('End Object')
            blocks.append('\n'.join(lines))
        del graph['format'], graph['body']
        graph['text'], graph['nodeCount'] = '\n'.join(blocks) + '\n', len(nodes)
    del result['classes'], result['pinTemplates']
    result['dependencies'] = sorted(dependencies)
    return result


def template(catalog, operation, node_id):
    entries = [e for e in catalog['entries'] if e['identity'] == operation]
    if len(entries) != 1:
        raise ValueError('Expected one catalogue operation')
    if len(parse_nodes(entries[0]['nativeInsertion'])) != 1:
        raise ValueError('Catalogue template must contain one node')
    # Use the codec itself so factory output follows the same complete field contract.
    envelope = {'schema': 2, 'format': 'native-asset', 'asset': '/Game/Template', 'parent': '/Script/Engine.Actor',
                'properties': {}, 'variables': [], 'defaults': {}, 'dependencies': [], 'graphs': [
                    {'name': 'EventGraph', 'kind': 'event', 'properties': {}, 'nodeCount': 1, 'text': entries[0]['nativeInsertion']}]}
    data = compact(envelope)
    n = data['graphs'][0]['body']['nodes'][0]
    n['id'] = node_id
    n['properties']['NodeGuid'] = uuid.uuid4().hex.upper()
    for pin in n['pins'].values():
        pin['id'] = uuid.uuid4().hex.upper()
    return {'node': n, 'classes': data['classes'], 'pinTemplates': data['pinTemplates'],
            'note': 'Merge pinTemplates with unique keys and remap node pin.type values before insertion.'}


def add_call(document, catalog, operation, node_id, graph_name):
    result = copy.deepcopy(document)
    graphs = [g for g in result['graphs'] if g['name'] == graph_name]
    if len(graphs) != 1 or any(n['id'] == node_id for n in graphs[0]['body']['nodes']):
        raise ValueError('Select one existing graph and a new node ID')
    item = template(catalog, operation, node_id)
    for kind, cls in item['classes'].items():
        if kind in result['classes'] and result['classes'][kind] != cls:
            raise ValueError('Conflicting class alias')
        result['classes'][kind] = cls
    remap = {}
    for key, value in item['pinTemplates'].items():
        existing = next((k for k, v in result['pinTemplates'].items() if v == value), None)
        if existing is None:
            i = len(result['pinTemplates'])
            while f'p{i}' in result['pinTemplates']:
                i += 1
            existing = f'p{i}'
            result['pinTemplates'][existing] = value
        remap[key] = existing
    for pin in item['node']['pins'].values():
        pin['type'] = remap[pin['type']]
    graphs[0]['body']['nodes'].append(item['node'])
    # Canonicalize pooled metadata and validate the inserted declaration.
    return compact(native(result))


def compare(expected, actual):
    """Canonical graph/field comparison. Does not drop unknown fields or node order within pins."""
    expected, actual = compact(native(expected)), compact(native(actual))
    differences = []
    def visit(a, b, path):
        if isinstance(a, dict) and isinstance(b, dict):
            for key in sorted(set(a) | set(b)):
                if key not in a or key not in b:
                    differences.append({'path': path + '/' + key, 'expected': a.get(key), 'actual': b.get(key)})
                else:
                    visit(a[key], b[key], path + '/' + key)
        elif isinstance(a, list) and isinstance(b, list):
            if len(a) != len(b):
                differences.append({'path': path + '/length', 'expected': len(a), 'actual': len(b)})
            else:
                for i, (left, right) in enumerate(zip(a, b)):
                    visit(left, right, path + '/' + str(i))
        elif a != b:
            differences.append({'path': path, 'expected': a, 'actual': b})
    visit(expected, actual, '')
    return differences


def register_widget(document, name):
    """Allocate editor identity for a newly declared widget/animation; keep all existing GUIDs."""
    result = copy.deepcopy(document)
    validate_components(result)
    widget = result['widget']
    objects = [o for o in widget['objects'] if o['name'] == name and
               (o['outer'] == widget['tree'] or o['id'] in widget.get('animations', []))]
    if len(objects) != 1:
        raise ValueError('Select one declared source widget or animation by name')
    field = 'WidgetVariableNameToGuidMap'
    value = result.setdefault('properties', {}).get(field, '()')
    entries = split_fields(value[1:-1]) if value != '()' else []
    for entry in entries:
        pair = split_fields(entry[1:-1])
        if len(pair) != 2:
            raise ValueError('Invalid widget GUID map')
        if json.loads(pair[0]) == name:
            return result
    entries.append('(' + json.dumps(name, ensure_ascii=False) + ', ' + uuid.uuid4().hex.upper() + ')')
    result['properties'][field] = '(' + ','.join(entries) + ')'
    return result


def batch_sources(manifest, folder):
    check_keys(manifest, {'assets'}, 'batch manifest')
    if not isinstance(manifest['assets'], list) or not manifest['assets']:
        raise ValueError('Batch assets must be a nonempty list of compact source paths')
    sources, identities = [], set()
    for path in manifest['assets']:
        if not isinstance(path, str) or not path:
            raise ValueError('Batch source paths must be nonempty strings')
        source = json.loads((folder / path).read_text(encoding='utf-8-sig'))
        if source['asset'] in identities:
            raise ValueError('Duplicate batch asset identity')
        identities.add(source['asset'])
        sources.append(source)
    for source in sources:
        parent_package = source['parent'].split('.', 1)[0]
        if parent_package in identities:
            raise ValueError('Batch parent classes must already exist; import the parent first: ' + parent_package)
    return sources


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('compact', 'native', 'template', 'add-call', 'compare', 'batch-native', 'batch-compare', 'register-widget'))
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--operation')
    parser.add_argument('--id')
    parser.add_argument('--graph', default='EventGraph')
    parser.add_argument('--catalog', type=Path)
    parser.add_argument('--actual', type=Path)
    args = parser.parse_args()
    try:
        if args.mode in ('template', 'add-call') and (not args.operation or not args.id):
            raise ValueError('template/add-call require --operation and --id')
        if args.mode == 'add-call' and args.catalog is None:
            raise ValueError('add-call requires --catalog')
        if args.mode in ('compare', 'batch-compare') and args.actual is None:
            raise ValueError('compare requires --actual')
        if args.input.resolve() == args.output.resolve():
            raise ValueError('Output must not overwrite input')
        data = json.loads(args.input.read_text(encoding='utf-8-sig'))
        if args.mode == 'register-widget':
            if not args.id:
                raise ValueError('register-widget requires --id NAME')
            output = register_widget(data, args.id)
        elif args.mode.startswith('batch-'):
            if any((args.input.parent / path).resolve() == args.output.resolve() for path in data.get('assets', [])):
                raise ValueError('Output must not overwrite a batch source')
            sources = batch_sources(data, args.input.parent)
            if args.mode == 'batch-native':
                output = {'assets': [native(source) for source in sources]}
            else:
                rows = []
                for index, source in enumerate(sources):
                    actual = compact(json.loads((args.actual / f'{index}.native.json').read_text(encoding='utf-8-sig')))
                    diffs = compare(source, actual)
                    rows.append({'asset': source['asset'], 'equal': not diffs, 'differences': diffs})
                    (args.actual / f'{index}.json').write_text(json.dumps(actual,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
                output = {'equal': all(row['equal'] for row in rows), 'assets': rows}
        elif args.mode == 'add-call':
            output = add_call(data, json.loads(args.catalog.read_text(encoding='utf-8-sig')), args.operation, args.id, args.graph)
        elif args.mode == 'compare':
            diffs = compare(data, json.loads(args.actual.read_text(encoding='utf-8-sig')))
            output = {'equal': not diffs, 'differences': diffs}
        else:
            output = template(data, args.operation, args.id) if args.mode == 'template' else (compact(data) if args.mode == 'compact' else native(data))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(output, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({'output': str(args.output.resolve())}))
        if args.mode == 'compare' and output['differences']:
            parser.exit(1, f'{len(output["differences"])} asset-text differences; see {args.output}\n')
        if args.mode == 'batch-compare' and not output['equal']:
            parser.exit(1, f'Batch asset-text differences; see {args.output}\n')
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(2, f'Cannot convert asset text: {error}\n')


if __name__ == '__main__':
    main()
