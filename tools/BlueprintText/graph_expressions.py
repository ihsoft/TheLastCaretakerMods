"""Bounded, presentation-only expressions; source nodes stay in Details."""
import json
import re
from collections import Counter, defaultdict, deque

from asset_text_codec import fields


MATH_OWNER = '"/Script/CoreUObject.Class\'/Script/Engine.KismetMathLibrary\'"'
OPERATIONS = {}
for suffix, scalar in [('DoubleDouble', 'double'), ('FloatFloat', 'float')]:
    for name, operator in [('Add', '+'), ('Subtract', '-'), ('Multiply', '*'), ('Divide', '/')]:
        OPERATIONS[name + '_' + suffix] = (operator, ('A', 'B'), (scalar, scalar), scalar)
    for name, operator in [('Less', '<'), ('LessEqual', '<='), ('Greater', '>'), ('GreaterEqual', '>='),
                           ('EqualEqual', '=='), ('NotEqual', '!=')]:
        OPERATIONS[name + '_' + suffix] = (operator, ('A', 'B'), (scalar, scalar), 'bool')
for name, operator in [('BooleanAND', 'AND'), ('BooleanOR', 'OR'), ('BooleanXOR', 'XOR')]:
    OPERATIONS[name] = (operator, ('A', 'B'), ('bool', 'bool'), 'bool')
OPERATIONS['Not_PreBool'] = ('NOT', ('A',), ('bool',), 'bool')
OPERATIONS['SelectFloat'] = ('select', ('bPickA', 'A', 'B'), ('bool', 'double', 'double'), 'double')
OPERATIONS['FMin'] = ('min', ('A', 'B'), ('double', 'double'), 'double')
OPERATIONS['FMax'] = ('max', ('A', 'B'), ('double', 'double'), 'double')
OPERATIONS['MakeVector'] = ('Vector', ('X', 'Y', 'Z'), ('double',) * 3, 'Vector')
OPERATIONS['SelectVector'] = ('select', ('bPickA', 'A', 'B'), ('bool', 'Vector', 'Vector'), 'Vector')
OPERATIONS['Dot_VectorVector'] = ('dot', ('A', 'B'), ('Vector', 'Vector'), 'double')
OPERATIONS['Add_VectorVector'] = ('+', ('A', 'B'), ('Vector', 'Vector'), 'Vector')
OPERATIONS['Subtract_VectorVector'] = ('-', ('A', 'B'), ('Vector', 'Vector'), 'Vector')
OPERATIONS['Multiply_VectorFloat'] = ('*', ('A', 'B'), ('Vector', 'double'), 'Vector')
OPERATIONS['VInterpTo_Constant'] = ('VInterpTo_Constant', ('Current', 'Target', 'DeltaTime', 'InterpSpeed'),
                                    ('Vector', 'Vector', 'float', 'float'), 'Vector')

VECTOR_TYPE = '"/Script/CoreUObject.ScriptStruct\'/Script/CoreUObject.Vector\'"'
NUMBER = r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?'
CALL_OPERATORS = ('select', 'min', 'max', 'float', 'double', 'Vector', 'dot', 'NOT', 'VInterpTo_Constant')
ARGUMENT_LABELS = {'select': ('', 'true: ', 'false: '),
                   'VInterpTo_Constant': ('current: ', 'target: ', 'deltaTime: ', 'interpSpeed: ')}


def unquote(value):
    return json.loads(value) if value.startswith('"') else value


def member(node, field):
    try:
        return fields(node.get('properties', {}).get(field, '()')[1:-1])
    except ValueError:
        return {}


def expression_type(pin, templates):
    metadata = templates.get(pin.get('type'), {})
    category = unquote(metadata.get('PinType.PinCategory', '""'))
    if (metadata.get('PinType.ContainerType', 'None') != 'None'
            or metadata.get('PinType.bIsReference', 'False') != 'False'):
        return None
    if category == 'struct':
        return 'Vector' if metadata.get('PinType.PinSubCategoryObject') == VECTOR_TYPE else None
    return unquote(metadata.get('PinType.PinSubCategory', '""')) if category == 'real' else category


def describe(node, templates):
    props = node.get('properties', {})
    if (node.get('kind') != 'call' or props.get('bDefaultsToPureFunc') != 'True'
            or node.get('_graphLink') or node.get('enabled') is False
            or props.get('EnabledState', 'Enabled') != 'Enabled' or node.get('comment')
            or props.get('NodeComment') or node.get('developmentOnly')):
        return None
    ref = member(node, 'FunctionReference')
    operation = OPERATIONS.get(unquote(ref.get('MemberName', '""')))
    if ref.get('MemberParent') != MATH_OWNER or operation is None:
        return None
    inputs, outputs = {}, []
    for alias, pin in node.get('pins', {}).items():
        name = pin.get('name', alias.split(':', 1)[1])
        if alias.startswith('in:') and name == 'self':
            continue  # Only the unconnected static-library target is omitted.
        if alias.startswith('out:'):
            outputs.append(alias)
        elif alias.startswith('in:') and name not in inputs:
            inputs[name] = alias
        else:
            return None
    symbol, names, types, result = operation
    if set(inputs) != set(names) or len(outputs) != 1:
        return None
    if any(expression_type(node['pins'][inputs[n]], templates) != t for n, t in zip(names, types)):
        return None
    if expression_type(node['pins'][outputs[0]], templates) != result:
        return None
    return symbol, [inputs[n] for n in names], outputs[0], types


def literal(node, alias, kind, templates):
    pin = node['pins'][alias]
    if templates.get(pin['type'], {}).get('bDefaultValueIsIgnored') == 'True':
        return None
    value = node.get('defaults', {}).get(alias)
    if not isinstance(value, str):
        return None
    if kind == 'bool':
        return value if value in ('true', 'false') else None
    if kind == 'Vector':
        parts = [part.strip() for part in value.split(',')]
        if len(parts) == 3 and all(re.fullmatch(NUMBER, part) for part in parts):
            return {'op': 'Vector', 'args': parts}
        return None  # Do not reconstruct missing components or guess other struct syntax.
    if kind in ('float', 'double') and re.fullmatch(NUMBER, value):
        return value
    return None


def formula(symbol, args):
    if symbol in ARGUMENT_LABELS:
        return symbol + '(' + ', '.join(label + arg for label, arg in zip(ARGUMENT_LABELS[symbol], args)) + ')'
    if symbol in ('min', 'max', 'Vector', 'dot'):
        return symbol + '(' + ', '.join(args) + ')'
    if symbol in ('float', 'double'):
        return symbol + '(' + args[0] + ')'
    if symbol == 'NOT':
        return 'NOT (' + args[0] + ')'
    return '(' + (' ' + symbol + ' ').join(args) + ')'


def flat_expression(tree):
    return tree if isinstance(tree, str) else formula(tree['op'], [flat_expression(a) for a in tree['args']])


def format_expression(tree, width=60, label='Output:'):
    """Wrap the operation tree, never a token stream or an identifier.

    Short expressions stay inline. Operators travel with their right operand;
    nested groups and wrapped argument lists use two-space continuations.
    An indivisible identifier may exceed the target width; the card accommodates it.
    """
    def layout(node, indent=0, prefix='', suffix=''):
        pad = ' ' * indent
        single = pad + prefix + flat_expression(node) + suffix
        if len(single) <= width or isinstance(node, str):
            return [single]
        op, args = node['op'], node['args']
        if op in CALL_OPERATORS:
            lines = [pad + prefix + ('NOT ' if op == 'NOT' else op) + '(']
            labels = ARGUMENT_LABELS.get(op, ('',) * len(args))
            for i, (arg, arg_label) in enumerate(zip(args, labels)):
                comma = ',' if i < len(args) - 1 else ''
                compact = arg_label + flat_expression(arg) + comma
                # Pack adjacent short arguments without changing their grouping.
                if len(lines) > 1 and len(lines[-1]) + 1 + len(compact) + len(suffix) + 1 <= width:
                    lines[-1] += ' ' + compact
                else:
                    lines.extend(layout(arg, indent + 2, arg_label, comma))
            closing = ')' + suffix
            if len(lines[-1]) + len(closing) <= width:
                lines[-1] += closing
            else:
                lines.append(pad + closing)
            return lines
        lines = layout(args[0], indent, prefix + '(')
        lines.extend(layout(args[1], indent + 2, op + ' ', ')' + suffix))
        return lines

    if len(label) + 1 + len(flat_expression(tree)) <= width:
        return [label + ' ' + flat_expression(tree)]
    return [label] + layout(tree, 2)


def fold_expressions(nodes, edges, original_edges, templates, children=None, removed_links=(),
                     max_nodes=8, max_chars=240):
    """Fold exclusive trees, retaining shared values and all unknown operations."""
    children = children or {}
    by_id = {n['id']: n for n in nodes}
    incoming, outgoing = defaultdict(list), defaultdict(list)
    for edge in original_edges:
        incoming[edge[2]].append(edge)
        outgoing[edge[0]].append(edge)
    protected = {e[i] for e in removed_links for i in (0, 2)}
    specs = {}
    for node in nodes:
        spec = describe(node, templates)
        if not spec or node['id'] in protected:
            continue
        wired = defaultdict(list)
        for e in incoming[node['id']]:
            wired['in:' + e[3]].append(e)
        if set(wired) - set(spec[1]) or any(len(v) != 1 for v in wired.values()):
            continue
        if any(alias not in wired and literal(node, alias, kind, templates) is None
               for alias, kind in zip(spec[1], spec[3])):
            continue
        compatible = True
        for alias, kind in zip(spec[1], spec[3]):
            if alias not in wired:
                continue
            source, output = wired[alias][0][:2]
            actual = expression_type(by_id.get(source, {}).get('pins', {}).get('out:' + output, {}), templates)
            if actual != kind and {actual, kind} != {'float', 'double'}:
                compatible = False
        if not compatible:
            continue
        specs[node['id']] = spec
    # Cycles and their dependents remain explicit. No recursive cycle guessing.
    indegree = {nid: sum(e[0] in specs for e in incoming[nid]) for nid in specs}
    ready = deque(n for n in specs if not indegree[n])
    order = []
    while ready:
        nid = ready.popleft()
        order.append(nid)
        for e in outgoing[nid]:
            if e[2] in indegree:
                indegree[e[2]] -= 1
                if not indegree[e[2]]:
                    ready.append(e[2])
    specs = {nid: specs[nid] for nid in order}
    # A literal constructor has no live reads. It may be repeated in formulas,
    # unlike a shared computed vector. Preserve its original node in Details.
    constant_vectors = {nid: {'op': 'Vector', 'args': [literal(by_id[nid], alias, kind, templates)
                        for alias, kind in zip(spec[1], spec[3])]}
                        for nid, spec in specs.items() if spec[0] == 'Vector' and not incoming[nid]}
    symbols, counts, anonymous = {}, Counter(), set()
    for index, node in enumerate(nodes, 1):
        for alias, pin in node.get('pins', {}).items():
            if not alias.startswith('out:'):
                continue
            name = pin.get('name', alias[4:])
            endpoint = node['id'], alias[4:]
            if node['id'] in specs:
                name = f'value{index}'
                anonymous.add(endpoint)
                uses = outgoing[node['id']]
                if len(uses) == 1 and by_id.get(uses[0][2], {}).get('kind') in ('Tunnel', 'FunctionResult'):
                    target = by_id[uses[0][2]]
                    name = target['pins'].get('in:' + uses[0][3], {}).get('name', uses[0][3])
                    anonymous.discard(endpoint)
            elif node.get('kind') in ('getVariable', 'variableGet'):
                name = unquote(member(node, 'VariableReference').get('MemberName', json.dumps(node.get('variable', name))))
                targets = [e for e in incoming[node['id']] if e[3] in ('target', 'self')]
                if targets:
                    source = by_id.get(targets[0][0], {})
                    receiver = source.get('variable') or unquote(member(source, 'VariableReference').get('MemberName', json.dumps(targets[0][1])))
                    name = receiver + '.' + name
            elif node['id'] in children:
                name = children[node['id']]['name'] + '.' + name
            if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9.]*', name):
                name = f'input{index}'
                anonymous.add(endpoint)
            symbols[node['id'], alias[4:]] = name
            counts[name] += 1
    # Distinct reads with the same name must not look like one captured value.
    for endpoint, name in list(symbols.items()):
        if counts[name] > 1:
            symbols[endpoint] = name + '_' + endpoint[0]
    assigned, replacements, remap, inlined_links = set(), {}, {}, set()

    def build(nid, allow):
        symbol, aliases, output, types = specs[nid]
        args, members, inputs, constants = [], [nid], {}, {}
        for alias, kind in zip(aliases, types):
            wire = [e for e in incoming[nid] if e[3] == alias[3:]]
            if not wire:
                args.append(literal(by_id[nid], alias, kind, templates))
                continue
            edge = wire[0]
            source, out = edge[:2]
            if (source in constant_vectors
                    and by_id[source].get('_reviewStatus') == by_id[nid].get('_reviewStatus')
                    and (source in constants or allow > len(members) + len(constants))):
                args.append(constant_vectors[source])
                constants[source] = by_id[source]
                continue
            expanded = None
            if (source in specs and source not in assigned and len(outgoing[source]) == 1
                    and by_id[source].get('_reviewStatus') == by_id[nid].get('_reviewStatus')
                    and allow - len(members) - len(constants) > 0):
                candidate = build(source, allow - len(members) - len(constants))
                if len(flat_expression(candidate[0])) + sum(len(flat_expression(a)) for a in args) < max_chars:
                    expanded = candidate
            if expanded:
                expression, parts, leaves, literals = expanded
                args.append(expression); members.extend(parts); inputs.update(leaves); constants.update(literals)
            else:
                name = symbols.get((source, out), source + '.' + out)
                args.append(name)
                pin = by_id.get(source, {}).get('pins', {}).get('out:' + out, {})
                inputs[source, out] = dict(name=name, source=source, pin=out, type=pin.get('type'))
            source_pin = by_id[source]['pins']['out:' + out]
            if expression_type(source_pin, templates) != kind:
                args[-1] = {'op': kind, 'args': [args[-1]]}
        expression = {'op': symbol, 'args': args}
        if len(flat_expression(expression)) > max_chars and len(members) > 1:
            return build(nid, 1)
        return expression, members, inputs, constants

    for nid in reversed(order):
        if nid in assigned:
            continue
        if nid in constant_vectors and outgoing[nid] and all(e in inlined_links for e in outgoing[nid]):
            assigned.add(nid)
            continue
        expression, members, inputs, constants = build(nid, max_nodes)
        # Fallback aliases belong to this expression's boundary, not graph order.
        # Rename leaves structurally, so value1 never rewrites value10 or a literal.
        reserved = {p['name'] for endpoint, p in inputs.items() if endpoint not in anonymous}
        renamed, number = {}, 1
        for endpoint, pin in inputs.items():
            if endpoint not in anonymous:
                continue
            while f'value{number}' in reserved:
                number += 1
            name = f'value{number}'
            renamed[pin['name']] = name
            pin['name'] = name
            reserved.add(name)
            number += 1

        def rename_leaves(tree):
            if isinstance(tree, str):
                return renamed.get(tree, tree)
            return dict(tree, args=[rename_leaves(arg) for arg in tree['args']])

        expression = rename_leaves(expression)
        parts = [by_id[x] for x in members] + list(constants.values())
        group = set(members) | constants.keys()
        inlined_links.update(e for e in original_edges if e[0] in constants and e[2] in members)
        replacement = dict(by_id[nid], _formula=flat_expression(expression), _formulaTree=expression,
                           _formulaInputs=list(inputs.values()),
                           _formulaResult=symbols[nid, specs[nid][2][4:]], _foldedNodes=parts,
                           _foldedLinks=[e for e in original_edges if e[0] in group and e[2] in group],
                           _boundaryLinks=[e for e in original_edges if (e[0] in group) != (e[2] in group)
                                           and e[0] not in constants])
        if replacement.get('_reviewStatus'):
            replacement['_reviewDetails'] = {'Status': replacement['_reviewStatus']}
            for version in ('Before', 'After'):
                snapshots = {n['id']: n.get('_reviewDetails', {}).get(version) for n in parts}
                replacement['_reviewDetails'][version] = {'Nodes': snapshots} if any(snapshots.values()) else None
        replacements[nid] = replacement
        assigned.update(members)
        for i, part in enumerate(members, 1):
            remap[part] = nid, i
    result_edges, input_groups = [], {}
    for source, output, target, pin, execution in edges:
        if (source, output, target, pin, execution) in inlined_links:
            continue
        new_source = remap.get(source, (source, 0))[0]
        new_target, step = remap.get(target, (target, 0))
        if new_source == new_target and source != target:
            continue
        if target in remap:
            original = (source, output, target, pin, execution)
            pin = f'Step {step}: {pin}'
            key = (source, output, new_target, execution)
            uses = replacements[new_target].setdefault('_expressionInputLinks', {})
            if key in input_groups:
                uses[input_groups[key]].append(original)
                continue
            input_groups[key] = pin
            uses[pin] = [original]
        result_edges.append((new_source, output, new_target, pin, execution))
    return ([replacements.get(n['id'], n) for n in nodes if n['id'] not in assigned or n['id'] in replacements],
            result_edges, len(assigned) - len(replacements))
