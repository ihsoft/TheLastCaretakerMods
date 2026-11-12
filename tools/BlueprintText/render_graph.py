"""Render compact Blueprint JSON to offline SVG + HTML; Python standard library only.

Usage: python render_graph.py blueprint.json --output diagram.svg [--view full] [--direction LR]
Accepts the shared Blueprint text envelope or the earlier standalone nodes/links JSON.
This is a structural view, not an Unreal compiler or a behavior simulator.
"""

import argparse
import copy
import html
import json
import re
import textwrap
from collections import defaultdict
from pathlib import Path
from function_labels import lookup as lookup_function_label
from review_details import render_details
from preview_references import node_references, package as reference_package, label as reference_label
from preview_references import is_type_reference, is_implicit_target, with_type_reference_details
from graph_hierarchy import select_graph, graph_key
from asset_text_codec import split_fields
from graph_expressions import fold_expressions, format_expression

VIEW_MODES = ('simplified', 'full', 'expressions')


def escape(value):
    return html.escape(str(value), quote=True)


def load_graph(document, selection=None):
    if document.get("schema") not in (1, 2):
        raise ValueError("Expected compact schema 1 or 2.")
    if "graphs" in document:
        graph = select_graph(document, selection)
        if graph.get("format") != "compact":
            raise ValueError("Native graph text is not supported. Supply a compact export.")
        body, name = graph["body"], graph["name"]
    else:
        body, name = document, document.get("graph", "Graph")
        if selection and selection not in (name, document.get('_graphId')):
            raise ValueError(f"Graph {selection!r} was not found.")
    nodes = body["nodes"]
    ids = [node["id"] for node in nodes]
    if any(not isinstance(i, str) or not i for i in ids) or len(set(ids)) != len(ids):
        raise ValueError("Node IDs must be nonempty unique strings.")
    by_id = {node['id']: node for node in nodes}
    templates = document.get('pinTemplates', {})
    edges = []
    for link in body["links"]:
        endpoints = []
        for key in ("from", "to"):
            node_id, separator, pin = link[key].rpartition(".")
            if not separator or node_id not in by_id or not pin:
                raise ValueError(f"Unresolved {key} endpoint: {link[key]!r}")
            endpoints.append((node_id, pin))
        (source, output), (target, input_pin) = endpoints
        output_type = pin_category(by_id[source], output if output.startswith('out:') else 'out:' + output, templates)
        input_type = pin_category(by_id[target], input_pin if input_pin.startswith('in:') else 'in:' + input_pin, templates)
        output, input_pin = output.removeprefix("out:"), input_pin.removeprefix("in:")
        # Native pin types take precedence; schema 1 falls back to standard aliases.
        if output_type or input_type:
            execution = output_type == 'exec' or input_type == 'exec'
        else:
            execution = input_pin in ("exec", "execute") or bool(re.fullmatch(r"then(?:_?\d+)?|else", output))
        edges.append((source, output, target, input_pin, execution))
    owner = document.get("asset", document.get("owner", "Blueprint"))
    return sorted(nodes, key=lambda n: n["id"]), sorted(edges), str(owner), str(name)


def review_document(document, before, review, selection=None):
    """Overlay explicit review marks; never infer a complete diff or mutate source."""
    if not isinstance(review, dict) or set(review) - {'added', 'changed', 'removed'}:
        raise ValueError('Review must contain only added, changed and removed ID lists.')
    marked = {}
    for status, ids in review.items():
        if not isinstance(ids, list):
            raise ValueError(f'Review {status} must be an ID list.')
        for node_id in ids:
            if not isinstance(node_id, str) or not node_id or node_id in marked:
                raise ValueError('Review IDs must be nonempty strings, unique across all lists.')
            marked[node_id] = status
    current, edges, owner, name = load_graph(document, selection)
    old, old_edges = [], []
    if before is not None:
        old, old_edges, old_owner, old_name = load_graph(before, selection)
        same_identity = ('graphs' in document and 'graphs' in before and
                         graph_key(select_graph(document, selection)) == graph_key(select_graph(before, selection)))
        if old_owner != owner or (old_name != name and not same_identity):
            raise ValueError('Before and current sources must identify the same asset and graph.')
    elif any(s in ('changed', 'removed') for s in marked.values()):
        raise ValueError('Changed and removed nodes require --before source.')
    current_by_id, old_by_id = ({n['id']: n for n in ns} for ns in (current, old))
    current_terminal = terminal_exit_ids(current, edges, document.get('pinTemplates', {}))
    old_terminal = terminal_exit_ids(old, old_edges, (before or {}).get('pinTemplates', {}))
    for node_id, status in marked.items():
        if status == 'removed':
            valid = node_id in old_by_id and node_id not in current_by_id
        elif status == 'added':
            valid = node_id in current_by_id and node_id not in old_by_id
        else:
            valid = node_id in current_by_id and node_id in old_by_id
        if not valid:
            raise ValueError(f'Review {status} ID does not match the sources: {node_id}')
    result = copy.deepcopy(document)
    result.pop('graphs', None)
    result.update(graph=name, nodes=copy.deepcopy(current), links=[], _review=True)
    result['_graphId'] = graph_key(select_graph(document, selection)) if 'graphs' in document else document.get('_graphId', name)
    templates = result.setdefault('pinTemplates', {})
    old_templates = {}
    for key, value in (before or {}).get('pinTemplates', {}).items():
        mapped = 'before:' + key
        while mapped in templates:
            mapped = 'before:' + mapped
        templates[mapped] = copy.deepcopy(value)
        old_templates[key] = mapped
    removed = {i for i, s in marked.items() if s == 'removed'}
    for node_id in sorted(removed):
        node = copy.deepcopy(old_by_id[node_id])
        for pin in node.get('pins', {}).values():
            if pin.get('type') in old_templates:
                pin['type'] = old_templates[pin['type']]
        result['nodes'].append(node)
    visible = {n['id'] for n in result['nodes']}
    ghosts = [e for e in old_edges if (e[0] in removed or e[2] in removed)
              and e[0] in visible and e[2] in visible]
    boundary = {i for e in ghosts for i in (e[0], e[2])}
    result['_reviewRemovedLinks'] = ghosts
    for source, output, target, input_pin, _ in edges + ghosts:
        result['links'].append({'from': source + '.out:' + output, 'to': target + '.in:' + input_pin})

    def snapshot(node_id, by_id, links, pin_templates):
        if node_id not in by_id:
            return None
        return {'node': with_type_reference_details(copy.deepcopy(by_id[node_id]), pin_templates),
                'links': [{'from': e[0] + '.' + e[1], 'to': e[2] + '.' + e[3]}
                          for e in links if node_id in (e[0], e[2])]}

    for node in result['nodes']:
        node_id = node['id']
        node['_graphNeverContinues'] = node_id in (old_terminal if node_id in removed else current_terminal)
        if node_id in boundary:
            node['_reviewBoundary'] = True
        if node_id in marked:
            node['_reviewStatus'] = marked[node_id]
            if marked[node_id] == 'changed':
                node['_reviewPreviousDescription'] = description_fields(
                    old_by_id[node_id], before.get('pinTemplates', {}), before.get('parent'))
            node['_reviewDetails'] = {'Status': marked[node_id],
                                     'Before': snapshot(node_id, old_by_id, old_edges, (before or {}).get('pinTemplates', {})),
                                     'After': snapshot(node_id, current_by_id, edges, document.get('pinTemplates', {}))}
    return result


def layers(nodes, edges, templates=None):
    """Keep execution layers stable; place data near its earliest consumer."""
    adjacent = {node["id"]: [] for node in nodes}
    for source, _, target, _, _ in edges:
        adjacent[source].append(target)
    indices, low, stack, active, components = {}, {}, [], set(), []

    adjacent = {node: sorted(set(targets)) for node, targets in adjacent.items()}

    def enter(node):
        indices[node] = low[node] = len(indices)
        stack.append(node)
        active.add(node)

    # Explicit DFS frames preserve Tarjan's traversal order without Python recursion.
    for root in adjacent:
        if root in indices:
            continue
        enter(root)
        frames = [(root, iter(adjacent[root]))]
        while frames:
            node, targets = frames[-1]
            target = next(targets, None)
            if target is not None:
                if target not in indices:
                    enter(target)
                    frames.append((target, iter(adjacent[target])))
                elif target in active:
                    low[node] = min(low[node], indices[target])
                continue
            frames.pop()
            if low[node] == indices[node]:
                component = []
                while True:
                    member = stack.pop()
                    active.remove(member)
                    component.append(member)
                    if member == node:
                        break
                components.append(sorted(component))
            if frames:
                parent = frames[-1][0]
                low[parent] = min(low[parent], low[node])
    membership = {node: i for i, component in enumerate(components) for node in component}
    rank = [0] * len(components)
    # Tarjan emits sink components first, so reverse order is topological.
    for i in reversed(range(len(components))):
        for node in components[i]:
            for target in adjacent[node]:
                j = membership[target]
                if i != j:
                    rank[j] = max(rank[j], rank[i] + 1)
    execution_nodes = {endpoint for source, _, target, _, execution in edges
                       if execution for endpoint in (source, target)}
    for node in nodes:
        if (node.get('_chainKind') == 'action' or node.get('_graphNeverContinues')
                or node.get('kind') in ('event', 'customEvent', 'FunctionEntry', 'FunctionResult',
                                        'sequence', 'branch', 'setVariable', 'variableSet')
                or any(pin_category(part, alias, templates or {}) == 'exec'
                       for part in node.get('_foldedNodes', [node]) for alias in part.get('pins', {}))):
            execution_nodes.add(node['id'])
    # SCCs are already in consumer-first order. Move data components together,
    # stopping before their earliest consumer; never move an execution component.
    for i, component in enumerate(components):
        if any(node in execution_nodes for node in component):
            continue
        consumers = {membership[target] for node in component for target in adjacent[node]
                     if membership[target] != i}
        if consumers:
            rank[i] = min(rank[j] for j in consumers) - 1
    return {node: rank[membership[node]] for node in adjacent}


def native_string(value):
    return json.loads(value) if value.startswith('"') else value


def text_literal(value):
    """Display stored FText source text; retain unsupported expressions verbatim."""
    if not isinstance(value, str):
        return value
    expression = value.strip()
    match = re.fullmatch(r'(NSLOCTEXT|LOCTEXT|INVTEXT)\((.*)\)', expression, re.DOTALL)
    try:
        if match:
            fields = split_fields(match[2])
            if len(fields) == {'NSLOCTEXT': 3, 'LOCTEXT': 2, 'INVTEXT': 1}[match[1]]:
                strings = [json.loads(field) for field in fields]
                if all(isinstance(item, str) for item in strings):
                    return strings[-1]
        elif expression.startswith('"'):
            decoded = json.loads(expression)
            if isinstance(decoded, str):
                return decoded
    except ValueError:
        pass  # Unknown native syntax must remain visible, not disappear.
    return value


def pin_category(node, alias, templates):
    pin = node.get('pins', {}).get(alias, {})
    return native_string(templates.get(pin.get('type'), {}).get('PinType.PinCategory', '""'))


def terminal_exit_ids(nodes, edges, templates):
    """An impure collapsed graph may have a required exit tunnel with no pins."""
    execution_entry = any(
        (node.get('kind') == 'FunctionEntry' or
         (node.get('kind') == 'Tunnel' and node.get('properties', {}).get('bCanHaveOutputs') == 'True'))
        and any(alias.startswith('out:') and pin_category(node, alias, templates) == 'exec'
                for alias in node.get('pins', {})) for node in nodes)
    connected_execution = {target for _, _, target, _, execution in edges if execution}
    return {node['id'] for node in nodes
            if node.get('kind') == 'Tunnel'
            and node.get('properties', {}).get('bCanHaveInputs') == 'True'
            and node.get('properties', {}).get('bCanHaveOutputs') != 'True'
            and (execution_entry or any(alias.startswith('in:') and pin_category(node, alias, templates) == 'exec'
                                        for alias in node.get('pins', {})))
            and node['id'] not in connected_execution}


def node_fill(node, templates, execution_nodes):
    kind = node.get('kind', 'unknown')
    if kind in ('event', 'customEvent', 'FunctionEntry'):
        return '#dce9fc'
    if (any(pin_category(node, alias, templates) == 'exec' for alias in node.get('pins', {}))
            or node['id'] in execution_nodes or node.get('_graphNeverContinues')
            or kind in ('setVariable', 'variableSet', 'branch', 'sequence', 'FunctionResult')):
        return '#f7e9d7'
    return '#e8edf4'


def readable_name(value):
    value = re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', value)
    return re.sub(r'([A-Z])([A-Z][a-z])', r'\1 \2', value).replace('_', ' ')


def wrap_node_line(line, header=False):
    # Display identifiers as words before wrapping; never split a title word
    # into an arbitrary suffix. SVG constrains exceptional unsplittable titles.
    if header:
        line = readable_name(line)
    return textwrap.wrap(line, 25 if header else 42,
                         break_long_words=not header, break_on_hyphens=not header) or ['']


def object_label(value):
    return native_string(value).rstrip("'").rsplit('.', 1)[-1]


def member_name(node, field="FunctionReference"):
    match = re.search(r'MemberName="([^"]+)"', node.get("properties", {}).get(field, ""))
    return match.group(1) if match else ""


def fold_chains(nodes, edges, original_edges, templates, parent=None, mode='data', self_ids=()):
    """Contract linear data or execution edges; retain original nodes and boundaries."""
    by_id = {n['id']: n for n in nodes}
    incoming, outgoing = defaultdict(list), defaultdict(list)
    for edge in original_edges:
        outgoing[edge[0]].append(edge)
        incoming[edge[2]].append(edge)

    def eligible(node):
        status = node.get('_reviewStatus')
        if (mode != 'action' and status) or (node.get('_reviewBoundary') and status != 'removed'):
            return False
        if mode == 'action':
            # Defaults and a technical Self target do not introduce data dependencies.
            if any(not e[4] and not (e[0] in self_ids and e[3] in ('target', 'self'))
                   for e in incoming[node['id']]):
                return False
            if node.get('kind') not in ('call', 'setVariable', 'variableSet', 'AddDelegate', 'RemoveDelegate', 'ClearDelegate', 'CallDelegate'):
                return False
            execution = {alias for alias in node.get('pins', {}) if pin_category(node, alias, templates) == 'exec'}
            # Require a declared straight-through signature, not merely one wired arm.
            return execution == {'in:exec', 'out:then'} and len([e for e in incoming[node['id']] if e[4]]) <= 1
        if node.get('kind') in ('getVariable', 'variableGet'):
            return not any(e[4] for e in incoming[node['id']] + outgoing[node['id']])
        if node.get('kind') != 'call' or node.get('properties', {}).get('bDefaultsToPureFunc') != 'True':
            return False
        categories = [pin_category(node, p, templates) for p in node.get('pins', {})]
        return bool(categories) and all(c and c != 'exec' for c in categories)

    successor, predecessor = {}, {}
    for node in nodes:
        outputs = [e for e in outgoing[node['id']] if e[4]] if mode == 'action' else outgoing[node['id']]
        if not eligible(node) or len(outputs) != 1:
            continue
        edge = outputs[0]
        target = by_id.get(edge[2])
        inputs = [e for e in incoming[edge[2]] if e[4]] if mode == 'action' else incoming[edge[2]]
        if (edge[4] == (mode == 'action') and target and eligible(target) and len(inputs) == 1
                and node.get('_reviewStatus') == target.get('_reviewStatus')):
            successor[node['id']] = target['id']
            predecessor[target['id']] = node['id']
    removed, replacements, remap = set(), {}, {}
    for start in nodes:
        if start['id'] in predecessor:  # Cycles have no root, so remain explicit.
            continue
        chain = [start]
        while chain[-1]['id'] in successor:
            chain.append(by_id[successor[chain[-1]['id']]])
        if len(chain) < 2:
            continue
        last = chain[-1]['id']
        ids = {n['id'] for n in chain}
        links = [e for e in original_edges if e[0] in ids and e[2] in ids]
        steps = [node_lines(n, {e[3] for e in incoming[n['id']]}, pin_templates=templates, parent=parent,
                            connected_outputs={e[1] for e in outgoing[n['id']]}, wrap=False)
                 for n in chain]
        for part, step in zip(chain, steps):
            previous = part.get('_reviewPreviousDescription', {})
            current = description_fields(part, templates, parent) if previous else {}
            for index, line in enumerate(step):
                key = description_key(line, index)
                if key in current and current[key] != previous.get(key):
                    step[index] = CardLine(line, reference=getattr(line, 'reference', None),
                                           previous=previous.get(key, '(not present)'))
            if part.get('_graphLink'):
                step[0] = CardLine(step[0], reference=part['_graphLink'], previous=getattr(step[0], 'previous', None))
        replacement = dict(chain[-1], _foldedNodes=chain, _foldedLinks=links, _foldedSteps=steps, _chainKind=mode)
        if replacement.get('_reviewStatus'):
            replacement['_reviewDetails'] = {'Status': replacement['_reviewStatus']}
            for version in ('Before', 'After'):
                snapshots = {n['id']: n['_reviewDetails'][version] for n in chain}
                replacement['_reviewDetails'][version] = (
                    {'Steps': snapshots} if any(snapshots.values()) else None)
        variable = start.get('variable') or member_name(start, 'VariableReference')
        if (mode == 'data' and start.get('kind') in ('getVariable', 'variableGet') and not incoming[start['id']] and variable
                and all(member_name(n) == 'Not_PreBool' for n in chain[1:])
                and all(e[3] not in ('target', 'self') for e in links)
                and all(len([p for p in n.get('pins', {}) if p.startswith('in:') and p != 'in:target']) == 1 for n in chain[1:])):
            expression = variable
            for _ in chain[1:]:
                expression = 'NOT (' + expression + ')'
            replacement['_expression'] = expression
        replacements[last] = replacement
        removed.update(ids - {last})
        for i, n in enumerate(chain):
            remap[n['id']] = (last, i + 1)
    result_edges = []
    for source, output, target, input_pin, execution in edges:
        new_source, source_step = remap.get(source, (source, 0))
        new_target, step = remap.get(target, (target, 0))
        if new_source == new_target and source != target:
            continue
        if mode == 'action' and source in remap and not execution:
            output = f'Step {source_step}: {output}'
        if target != new_target:
            input_pin = f'Step {step}: {input_pin}'
        result_edges.append((new_source, output, new_target, input_pin, execution))
    # Tooltip includes exact pre-fold boundary endpoints as well as internal links.
    for last, node in replacements.items():
        ids = {n['id'] for n in node['_foldedNodes']}
        node['_boundaryLinks'] = [e for e in original_edges if (e[0] in ids) != (e[2] in ids)]
    return ([replacements.get(n['id'], n) for n in nodes if n['id'] not in removed],
            result_edges, len(removed))


def fold_data_chains(nodes, edges, original_edges, templates, parent=None):
    return fold_chains(nodes, edges, original_edges, templates, parent, 'data')


def pin_type_label(metadata):
    category = native_string(metadata.get("PinType.PinCategory", '"unknown"'))
    subtype = native_string(metadata.get("PinType.PinSubCategory", '""'))
    obj = native_string(metadata.get("PinType.PinSubCategoryObject", '""'))
    object_name = obj.rstrip("'").rsplit('.', 1)[-1] if obj and obj != "None" else ""
    if category == "real":
        label = subtype or "real"
    elif category in ("object", "softobject"):
        label = (object_name or "Object") + (" soft reference" if category == "softobject" else " reference")
    elif category in ("class", "softclass"):
        label = category + "<" + (object_name or "Object") + ">"
    elif object_name and category in ("struct", "enum", "byte"):
        label = object_name
    else:
        label = category
    container = native_string(metadata.get("PinType.ContainerType", "None"))
    if container in ("Array", "Set"):
        label = container.lower() + "<" + label + ">"
    elif container == "Map":
        # Map values use FEdGraphTerminalType rather than FEdGraphPinType.
        terminal = metadata.get("PinType.PinValueType", "")
        value = {"PinType." + name: raw for name, raw in re.findall(
            r'Terminal(Category|SubCategory|SubCategoryObject)=("(?:\\.|[^"\\])*"|[^,)]+)', terminal)}
        value = {key.replace("PinType.", "PinType.Pin", 1): raw for key, raw in value.items()}
        label = "map<" + label + ", " + pin_type_label(value) + ">"
    return label


class CardLine(str):
    """Keep display semantics through wrapping and folded-step descriptions."""
    def __new__(cls, text, unconnected_output=False, reference=None, previous=None, code=False, step_start=False):
        line = super().__new__(cls, text)
        line.unconnected_output = unconnected_output
        line.reference = reference
        line.previous = previous
        line.code = code
        line.step_start = step_start
        return line


def wrap_card_line(line, title=False):
    return [CardLine(part, getattr(line, 'unconnected_output', False), getattr(line, 'reference', None),
                     getattr(line, 'previous', None), step_start=index == 0 and getattr(line, 'step_start', False))
            for index, part in enumerate(wrap_node_line(line, title))]


def card_section_gaps(lines):
    """Separate inputs from outputs without inserting fake text rows."""
    if any(getattr(line, 'step_start', False) for line in lines):
        # A chain is grouped by operation, not by the pins within an operation.
        seen_step, gaps = False, []
        for line in lines:
            start = getattr(line, 'step_start', False)
            gaps.append(8 if start and seen_step else 0)
            seen_step = seen_step or start
        return gaps
    section, gaps = None, []
    for line in lines:
        current = ('input' if line.startswith(('Input:', 'Parameter:')) else
                   'output' if line.startswith(('Output:', 'Output (')) else None)
        gaps.append(8 if section == 'input' and current == 'output' else 0)
        if current:
            section = current
        elif re.match(r'\d+\. ', line):
            section = None
    return gaps


def output_default(node, alias, templates):
    metadata = templates.get(node.get('pins', {}).get(alias, {}).get('type'), {})
    if metadata.get('bDefaultValueIsIgnored') == 'True':
        return None
    return node.get('parameterDefaults', {}).get(alias, node.get('defaults', {}).get(alias))


def hidden_advanced_output(node, alias, templates, connected):
    metadata = templates.get(node.get('pins', {}).get(alias, {}).get('type'), {})
    return (node.get('properties', {}).get('AdvancedPinDisplay') == 'Hidden'
            and metadata.get('bAdvancedView') == 'True'
            and native_string(metadata.get('PinType.PinCategory', '""')) != 'exec'
            and alias[4:] not in connected)


def output_lines(node, pin_templates, connected=()):
    if "pins" not in node:
        return []  # Schema 1 has no complete signature; do not infer a return type.
    lines = []
    show_signature = node.get('kind') in ('call', 'getVariable', 'variableGet')
    properties = node.get('properties', {})
    graph_entry = node.get('kind') == 'FunctionEntry' or (
        node.get('kind') == 'Tunnel' and properties.get('bCanHaveOutputs') == 'True'
        and properties.get('bCanHaveInputs') != 'True')
    graph_exit = node.get('kind') == 'FunctionResult' or (
        node.get('kind') == 'Tunnel' and properties.get('bCanHaveInputs') == 'True'
        and properties.get('bCanHaveOutputs') != 'True')
    for alias, pin in node["pins"].items():
        if not alias.startswith('in:' if graph_exit else 'out:'):
            continue
        metadata = pin_templates.get(pin.get("type"), {})
        if native_string(metadata.get("PinType.PinCategory", '""')) == "exec" or metadata.get('bHidden') == 'True':
            continue
        name = pin.get("name", alias.split(':', 1)[1])
        if graph_exit:
            # Inputs of the exit node declare results of the enclosing graph.
            # Like entry parameters, these signatures remain visible when unwired.
            lines.append(CardLine(f'Output: {name} ({pin_type_label(metadata)})'))
            continue
        if graph_entry:
            # These outputs are the graph's inputs; their signature stays visible
            # even when unused or when Show unconnected is disabled.
            lines.append(CardLine(f'Parameter: {name} ({pin_type_label(metadata)})'))
            continue
        if hidden_advanced_output(node, alias, pin_templates, connected):
            continue
        if node.get('kind') == 'BreakStruct':
            lines.append(CardLine(f'Field: {name} ({pin_type_label(metadata)})', alias[4:] not in connected))
            continue
        prefix = "Output: " if name == "ReturnValue" or alias == "out:result" or node.get('kind') in ('getVariable', 'variableGet') else "Output: " + name + " — "
        default = output_default(node, alias, pin_templates)
        if alias[4:] not in connected and default is not None:
            # Stored pin defaults are not evaluated runtime output values.
            lines.append(CardLine(f'Output: {name} = {json.dumps(default, ensure_ascii=False)} (default; {pin_type_label(metadata)})', True))
        elif show_signature:
            lines.append(CardLine(prefix + pin_type_label(metadata), alias[4:] not in connected))
    return lines


def edge_pin_details(node, name, direction, pin_templates):
    """Resolve each endpoint independently, including a folded chain step."""
    if '_foldedNodes' in node:
        match = re.fullmatch(r'Step (\d+): (.+)', name)
        if match:
            index = int(match.group(1)) - 1
            if not 0 <= index < len(node['_foldedNodes']):
                return node['id'], name, None
            node, name = node['_foldedNodes'][index], match.group(2)
    pin = node.get('pins', {}).get(direction + ':' + name, {})
    metadata = pin_templates.get(pin.get('type'), {})
    # Unknown legacy signatures stay untyped; never borrow the opposite pin's type.
    label = pin_type_label(metadata) if metadata.get('PinType.PinCategory') else None
    return node['id'], pin.get('name', name), label


def edge_output_type(node, output, pin_templates):
    return edge_pin_details(node, output, 'out', pin_templates)[2]


def unconnected_outputs(node, connected, pin_templates):
    """Declared visible pins only; count original links, including folded/hidden ones."""
    if 'pins' in node:
        outputs = [alias[4:] for alias, pin in node['pins'].items() if alias.startswith('out:')
                   and pin_templates.get(pin.get('type'), {}).get('bHidden') != 'True'
                   and not hidden_advanced_output(node, alias, pin_templates, connected)]
        execution_outputs = [alias[4:] for alias in node['pins'] if alias.startswith('out:')
                             and pin_category(node, alias, pin_templates) == 'exec']
    elif node.get('kind') == 'sequence':
        outputs = [f'then{i}' for i in range(node.get('outputs', 2))]
        execution_outputs = outputs
    else:
        outputs = []  # No complete signature in older call declarations: do not guess.
        execution_outputs = []
    # An unwired sole execution output simply ends this path; retain data and branch warnings.
    terminal = set(execution_outputs) if len(execution_outputs) == 1 else set()
    return [name for name in outputs if name not in connected and name not in terminal
            and (name in execution_outputs or output_default(node, 'out:' + name, pin_templates) is None)]


def node_lines(node, connected_inputs, hidden_inputs=(), pin_templates=None, parent=None,
               step_inputs=(), step_outputs=(), wrap=True, connected_outputs=()):
    if '_formula' in node:
        lines = [f'Expression · {len(node["_foldedNodes"])} operations']
        result_pin = next(p for alias, p in node['pins'].items() if alias.startswith('out:'))
        result_type = pin_type_label((pin_templates or {}).get(result_pin['type'], {}))
        formula_lines = [CardLine(row, code=True) for row in format_expression(
            node['_formulaTree'], label='Output (' + result_type + '):')]
        lines.extend('Input: ' + p['name'] + ' (' + pin_type_label((pin_templates or {}).get(p['type'], {})) + ')'
                     for p in node['_formulaInputs'])
        lines.extend(formula_lines)
        if not wrap:
            return lines
        width = max(330, max(int(len(line) * 7.3 + 29) for line in formula_lines))
        return [part for i, line in enumerate(lines)
                for part in ([line] if getattr(line, 'code', False) else
                             wrap_card_line(line, True) if i == 0 else
                             [CardLine(text) for text in textwrap.wrap(line, int((width - 28) / 6.5),
                                 break_long_words=False, break_on_hyphens=False)])]
    if '_expression' in node and not any(step_outputs):
        lines = [node['_expression'], f"Expression · {len(node['_foldedNodes'])} nodes (Details / Full)"]
        lines.extend(output_lines(node, pin_templates or {}, connected_outputs))
        return [part for i, line in enumerate(lines) for part in wrap_card_line(line, i == 0)]
    if '_foldedSteps' in node:
        label = 'Action chain' if node.get('_chainKind') == 'action' else 'Data chain'
        lines = [f"{label} · {len(node['_foldedNodes'])} nodes"]
        for i, step in enumerate(node['_foldedSteps'], 1):
            step = list(step)
            if i <= len(step_inputs):
                index = next((j for j, line in enumerate(step[1:], 1)
                              if line.startswith(('Input:', 'Output:'))), len(step))
                step[index:index] = ['Input: ' + hint for hint in step_inputs[i-1]]
            lines.extend([CardLine(f'{i}. {readable_name(step[0])}', reference=getattr(step[0], 'reference', None),
                                   previous=getattr(step[0], 'previous', None), step_start=True)] + step[1:])
            if i <= len(step_outputs):
                lines.extend(step_outputs[i-1])
        return [part for i, line in enumerate(lines) for part in wrap_card_line(line, i == 0)]
    kind = node.get("kind", "unknown")
    reference = node.get("operation", node.get("event", node.get("ref", kind)))
    properties = node.get("properties", {})
    member = re.search(r'MemberName="([^"]+)"', properties.get("FunctionReference", properties.get("VariableReference", properties.get("EventReference", ""))))
    if member:
        reference = member.group(1)
    short = reference.rsplit("::", 1)[-1].rsplit(".", 1)[-1]
    variable = node.get("variable", short)
    details = []
    if kind in ("setVariable", "variableSet"):
        title = "Set " + variable
    elif kind in ("getVariable", "variableGet"):
        title = "Get " + variable
    elif kind in ("event", "customEvent"):
        title = "Event " + short.removeprefix("Receive")
    elif kind == "sequence":
        outputs = node.get('outputs', len([p for p in node.get('pins', {}) if p.startswith('out:then')]) or 2)
        title = f"Sequence · {outputs} outputs"
    elif kind == "branch":
        title = "Branch"
    elif kind == "BreakStruct":
        title = "Break " + readable_name(object_label(properties['StructType'])) if properties.get('StructType') else "Break Struct"
    elif kind in ("cast", "DynamicCast"):
        title = "Cast to " + object_label(properties.get("TargetType", '"unknown type"'))
        details.append("Check type; return typed reference")
    elif kind == "CreateDelegate":
        title = "Create callback"
        details.append("Function: " + native_string(properties.get("SelectedFunctionName", '"unknown"')))
        details.append("Creates a reference; does not call it")
    elif kind in ("AddDelegate", "RemoveDelegate", "ClearDelegate", "CallDelegate"):
        title = {"AddDelegate": "Bind event handler", "RemoveDelegate": "Unbind event handler",
                 "ClearDelegate": "Unbind all event handlers", "CallDelegate": "Broadcast event"}[kind]
        delegate = re.search(r'MemberName="([^"]+)"', properties.get("DelegateReference", ""))
        details.append("Event: " + (delegate.group(1) if delegate else "unknown"))
    elif kind == "MacroInstance":
        macro = re.search(r'MacroGraph="([^"]+)"', properties.get("MacroGraphReference", ""))
        title = "Macro: " + (readable_name(macro.group(1).rstrip("'").rsplit(':', 1)[-1]) if macro else "unknown")
    elif kind in ("FunctionEntry", "FunctionResult"):
        title = "Function entry" if kind == "FunctionEntry" else "Return from function"
        if member:
            details.append("Function: " + short)
    elif kind == "Tunnel":
        entry = properties.get('bCanHaveOutputs') == 'True'
        exit_node = properties.get('bCanHaveInputs') == 'True'
        title = "Graph Entry" if entry and not exit_node else "Graph Exit" if exit_node and not entry else "Tunnel"
    else:
        title = readable_name(short)
    if kind == "call":
        label = lookup_function_label(node, pin_templates, parent)
        title = label['title'] if label else ("NOT" if short == "Not_PreBool" else readable_name(title))
    # All IDs remain in the tooltip and link identity, not the visible node label.
    lines = [title] + details
    lines.extend('Input: ' + hint for hint in hidden_inputs)
    values = {**node.get('defaults', {}), **node.get('arguments', {})}
    for alias, pin in node.get('pins', {}).items():
        if alias.startswith('in:') and 'DefaultTextValue' in pin:
            values[alias] = text_literal(pin['DefaultTextValue'])
    for key, value in sorted(values.items()):
        metadata = (pin_templates or {}).get(node.get("pins", {}).get(key, {}).get("type"), {})
        if key.startswith("out:") or key.removeprefix("in:") in connected_inputs or metadata.get("bDefaultValueIsIgnored") == "True":
            continue
        reference = node.get('_inputReferences', {}).get(key)
        suffix = (' [STUB]' if reference.get('stub') else ' [not in project]' if reference.get('missing') else '') if reference else ''
        lines.append(CardLine(f"Input: {key.removeprefix('in:')} = {json.dumps(value, ensure_ascii=False)}" + suffix,
                              reference=reference))
    lines.extend(output_lines(node, pin_templates or {}, connected_outputs))
    if is_disabled(node):
        lines.append("DISABLED")
    if node.get("developmentOnly"):
        lines.append("DEVELOPMENT ONLY")
    if node.get("comment"):
        lines.append("Comment: " + node["comment"])
    if not wrap:
        return lines
    return [part for row, line in enumerate(lines)
            for part in wrap_card_line(line, row == 0)]


def description_key(line, index):
    """Match semantic fields before wrapping, independent of ordering and links."""
    if index == 0:
        return 'title'
    if line.startswith(('Parameter: ', 'Field: ')):
        return line.rsplit(' (', 1)[0]
    if line.startswith(('Input: ', 'Output: ')) and ' = ' in line:
        return line.partition(' = ')[0]
    if line.startswith('Output: ') and ' — ' in line:
        return line.partition(' — ')[0]
    if line.startswith('Output: ') and ' (' in line:
        return line.rsplit(' (', 1)[0]
    if ':' in line:
        return line.partition(':')[0]
    return line


def description_fields(node, templates, parent):
    # Include stored defaults regardless of wiring, so a link-only edit does not
    # turn an unchanged default into a changed description when it becomes visible.
    return {description_key(line, i): line for i, line in enumerate(
        node_lines({k:v for k,v in node.items() if k != '_inputReferences'}, set(),
                   pin_templates=templates, parent=parent, wrap=False))}


def with_input_references(node, templates, owner, context):
    """Link a displayed literal at its input, including within folded steps."""
    candidates = defaultdict(list)
    for field, target in node_references(node, templates):
        parts = field.split(' / ')
        if (len(parts) == 2 and parts[0] in ('defaults', 'arguments') and
                parts[1].startswith('in:') and reference_package(target) != owner):
            candidates[parts[1]].append(dict(node=node['id'], field=field, target=target,
                                            **context.get(reference_package(target), {})))
    # A line containing several asset paths keeps its separate per-target links.
    refs = {alias: values[0] for alias, values in candidates.items() if len(values) == 1}
    return dict(node, _inputReferences=refs) if refs else node


def is_disabled(node):
    return node.get("enabled") is False or node.get("properties", {}).get("EnabledState") == "Disabled"


def variable_label(variable):
    if "name" in variable:
        return variable["name"] + ": " + variable["type"]
    name = variable.get("VarName", '"?"')
    name = json.loads(name) if name.startswith('"') else name
    native_type = variable.get("VarType", "")
    category = re.search(r'PinCategory="([^"]+)"', native_type)
    subtype = re.search(r'PinSubCategory="([^"]+)"', native_type)
    kind = category.group(1) if category else "?"
    if kind == "real" and subtype:
        kind = subtype.group(1)
    return name + ": " + kind


def pin_order(pin):
    # Natural numeric order also handles Sequence then_2 / then_10 and folded steps.
    name = re.sub(r'\d+', lambda match: match[0].zfill(12), pin)
    return ({'then': 0, 'else': 1}.get(pin, 2), name)


def order_layer(members, parent_edges, boxes, direction):
    def key(node):
        parents = parent_edges[node['id']]
        execution = [edge for edge in parents if edge[4]]
        available = [edge for edge in (execution or parents) if edge[0] in boxes]
        if not available:
            return (float('inf'), (2, ''), node['id'])
        axis = 0 if direction == 'TB' else 1
        center = sum(boxes[e[0]][axis] + boxes[e[0]][axis + 2] / 2 for e in available) / len(available)
        return (center, min(pin_order(e[1]) for e in available), node['id'])
    return sorted(members, key=key)


def render(document, selection=None, view="simplified", direction="TB", review=None, before=None, asset_context=None, graph_context=None, marker_namespace=""):
    asset_context = asset_context or {}
    graph_context = graph_context or {}
    if review is not None:
        document = review_document(document, before, review, selection)
    if view not in VIEW_MODES or direction not in ("TB", "LR"):
        raise ValueError("Expected view simplified/full/expressions and direction TB/LR.")
    all_nodes, all_edges, owner, graph = load_graph(document, selection)
    all_nodes = [with_type_reference_details(n, document.get('pinTemplates', {})) for n in all_nodes]
    all_nodes = [with_input_references(n, document.get('pinTemplates', {}), owner, asset_context) for n in all_nodes]
    terminal = terminal_exit_ids(all_nodes, all_edges, document.get('pinTemplates', {}))
    all_nodes = [dict(n, _graphNeverContinues=True)
                 if n['id'] in terminal and '_graphNeverContinues' not in n else n for n in all_nodes]
    # Attach navigation to view copies; source and review snapshots stay untouched.
    all_nodes = [dict(n, _graphLink=target) if (target := graph_context.get(
        'beforeCalls' if n.get('_reviewStatus') == 'removed' else 'calls', {}).get(n['id'])) else n
        for n in all_nodes]
    hidden = {}
    if view == "simplified":
        for node in all_nodes:
            if node.get('_reviewStatus') or node.get('_reviewBoundary'):
                continue
            if is_disabled(node):
                hidden[node["id"]] = "disabled node"
            elif node.get("kind") == "self" or node.get("ref") == "/Script/BlueprintGraph.K2Node_Self":
                hidden[node["id"]] = "Self"
    nodes = [node for node in all_nodes if node["id"] not in hidden]
    edges = [edge for edge in all_edges if edge[0] not in hidden and edge[2] not in hidden]
    hidden_links, folded = len(all_edges) - len(edges), 0
    if view == "simplified":
        nodes, edges, folded = fold_data_chains(nodes, edges, all_edges, document.get("pinTemplates", {}), document.get("parent"))
        self_ids = {n['id'] for n in all_nodes if n.get('kind') == 'self' or n.get('ref') == '/Script/BlueprintGraph.K2Node_Self'}
        nodes, edges, actions_folded = fold_chains(nodes, edges, all_edges, document.get("pinTemplates", {}), document.get("parent"), 'action', self_ids)
        folded += actions_folded
    elif view == 'expressions':
        nodes, edges, folded = fold_expressions(nodes, edges, all_edges, document.get('pinTemplates', {}),
                                              graph_context.get('children'), document.get('_reviewRemovedLinks', []))
    rank = layers(nodes, edges, document.get('pinTemplates', {}))
    groups = defaultdict(list)
    for node in nodes:
        groups[rank[node["id"]]].append(node)
    boxes, lines_by_id, changed_lines, title_rows = {}, {}, {}, {}
    connected, connected_outputs, hidden_inputs = defaultdict(set), defaultdict(set), defaultdict(list)
    for source, output, target, pin, _ in all_edges:
        connected[target].add(pin)
        connected_outputs[source].add(output)
        if source in hidden and target not in hidden:
            hidden_inputs[target].append(f"{pin} ← {hidden[source]} (hidden)")
    for node in nodes:
        node_id = node["id"]
        parts = node.get('_foldedNodes', [node])
        notes = [[CardLine(('Branch: ' if pin_category(part, 'out:' + output, document.get('pinTemplates', {})) == 'exec'
                           or part.get('kind') == 'sequence' else 'Output: ') + f'{output} — unconnected', True) for output in
                  unconnected_outputs(part, connected_outputs[part['id']], document.get('pinTemplates', {}))]
                 for part in parts]
        if '_foldedNodes' in node:
            lines_by_id[node_id] = node_lines(node, connected[node_id], pin_templates=document.get('pinTemplates', {}),
                                             parent=document.get('parent'), step_inputs=[hidden_inputs[p['id']] for p in parts],
                                             step_outputs=notes, connected_outputs=connected_outputs[node_id])
            title_rows[node_id] = len(wrap_node_line(node['_expression'], True)) if '_expression' in node and not any(notes) else 1
            changed_lines[node_id] = {i: line.previous for i, line in enumerate(lines_by_id[node_id])
                                     if getattr(line, 'previous', None) is not None}
        else:
            templates, parent = document.get('pinTemplates', {}), document.get('parent')
            raw_lines = node_lines(node, connected[node_id], hidden_inputs[node_id], templates, parent, wrap=False,
                                   connected_outputs=connected_outputs[node_id])
            if node.get('_graphNeverContinues'):
                raw_lines[0] = 'Graph Never Continues'
                if any(alias.startswith('in:') and pin_category(node, alias, templates) not in ('exec', None, '')
                       for alias in node.get('pins', {})):
                    raw_lines.append('Warning: result outputs cannot be returned')
            child = graph_context.get('children', {}).get(node_id)
            if child:
                raw_lines[0] = child['name']
                raw_lines.append('Nested graph · ' + str(child['nodes']) + ' local nodes')
                if child.get('changed') and node.get('_reviewStatus') not in ('added', 'removed'):
                    raw_lines.append('~ Nested implementation changed')
            title_rows[node_id] = len(wrap_node_line(raw_lines[0], True))
            previous = node.get('_reviewPreviousDescription', {})
            current = description_fields(node, templates, parent) if previous else {}
            lines_by_id[node_id], changed_lines[node_id] = [], {}
            for index, line in enumerate(raw_lines):
                key = description_key(line, index)
                changed = key in current and current[key] != previous.get(key)
                for part in wrap_card_line(line, index == 0):
                    if changed:
                        changed_lines[node_id][len(lines_by_id[node_id])] = previous.get(key, '(not present)')
                    lines_by_id[node_id].append(part)
            for note in notes[0]:
                lines_by_id[node_id].extend(CardLine(part, note.unconnected_output) for part in textwrap.wrap(note, 42))
        # Keep each target at its actual node/pin/property, including folded steps.
        seen_references = {(ref.get('node'), ref['field'], ref['target'])
                           for line in lines_by_id[node_id]
                           if (ref := getattr(line, 'reference', None)) and ref.get('node')}
        for part in node.get('_foldedNodes', [node]):
            for field, target in node_references(part, document.get('pinTemplates', {})):
                if view != 'full' and (is_type_reference(field) or is_implicit_target(part, document.get('pinTemplates', {}), field)):
                    continue
                if reference_package(target) == owner or (part['id'], field, target) in seen_references:
                    continue
                seen_references.add((part['id'], field, target))
                location = field.replace('pins / ', '').replace(' / DefaultObject', '').replace('defaults / ', '').replace('properties / ', '')
                if '_foldedNodes' in node:
                    location = part['id'] + ' / ' + location
                info = dict(target=target, field=field, **asset_context.get(reference_package(target), {}))
                line = CardLine(location + ' → ' + reference_label(target, asset_context), reference=info)
                lines_by_id[node_id].extend(wrap_card_line(line))
        if node.get('_reviewStatus'):
            status = node['_reviewStatus']
            badge_row = title_rows[node_id]
            lines_by_id[node_id].insert(badge_row, {'added': '+ Added', 'changed': '~ Changed', 'removed': '− Removed'}[status])
            changed_lines[node_id] = {i + (i >= badge_row): value for i, value in changed_lines.get(node_id, {}).items()}
    parent_edges = defaultdict(list)
    for edge in edges:
        parent_edges[edge[2]].append(edge)
    card_widths = {n['id']: max(330, max((int(len(line) * 7.3 + 29)
                   for line in lines_by_id[n['id']] if getattr(line, 'code', False)), default=330))
                   for n in nodes}
    section_gaps = {nid: card_section_gaps(lines) for nid, lines in lines_by_id.items()}
    card_heights = {nid: 28 + 19 * len(lines) + sum(section_gaps[nid])
                    for nid, lines in lines_by_id.items()}
    compact_lines = {nid: [line for line in lines if not getattr(line, 'unconnected_output', False)]
                     for nid, lines in lines_by_id.items()}
    compact_gaps = {nid: card_section_gaps(lines) for nid, lines in compact_lines.items()}
    widest = max(card_widths.values(), default=330)
    row_step, column_step = widest + 50, widest + 170
    y = 240
    max_columns = min(3, max((len(m) for m in groups.values()), default=1))
    for column, members in sorted(groups.items()):
        members = order_layer(members, parent_edges, boxes, direction)
        if direction == "LR":
            y = 240
            for node in members:
                height = card_heights[node["id"]]
                boxes[node["id"]] = (40 + column * column_step, y, card_widths[node['id']], height)
                y += height + 60
        else:
            # Bound row width; large root sets continue downward, not off-screen.
            for start in range(0, len(members), 3):
                chunk = members[start:start + 3]
                offset = (max_columns - len(chunk)) * row_step / 2
                row_height = max(card_heights[n["id"]] for n in chunk)
                for index, node in enumerate(chunk):
                    node_id = node["id"]
                    height = card_heights[node_id]
                    boxes[node_id] = (40 + offset + index * row_step, y, card_widths[node_id], height)
                y += row_height + 110
    right = max((x + w for x, y, w, h in boxes.values()), default=720)
    width = max(950, right + 80)
    bottom = max((y + h for x, y, w, h in boxes.values()), default=240)
    height = bottom + 90
    breadcrumbs = graph_context.get('breadcrumbs', [])
    title_parts = [dict(name=owner.rsplit('/', 1)[-1],
                        **({'href': breadcrumbs[0]['href']} if breadcrumbs else {})),
                   *breadcrumbs[1:], dict(name=graph)]
    title = ' / '.join(part['name'] for part in title_parts)
    header_width = max(950, 80 + len(title) * 13)
    width = max(width, header_width)
    marker_prefix = marker_namespace + view + direction
    pieces = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" role="img" aria-label="{escape(title)}">',
              '<style>text{font-family:Segoe UI,Arial,sans-serif;fill:#20334b} .label{font-size:11px;paint-order:stroke;stroke:#f8fafc;stroke-width:5px;stroke-linejoin:round}.graph-edge:hover>.edge-line{stroke-width:3.5}.graph-edge:hover{opacity:1}</style>',
              '<style>.open-child-graph{cursor:pointer}.open-child-graph:focus{outline:none}.open-child-graph:hover>rect,.open-child-graph:focus-visible>rect{fill:#dbeafe;stroke:#1d4ed8;stroke-width:2}.graph-reference{text-decoration:underline;cursor:pointer}.graph-reference:hover text{fill:#1d4ed8}.graph-node.selected>.nested-graph-frame,.graph-node.neighbor>.nested-graph-frame,.graph-node:focus-visible>.nested-graph-frame{stroke-width:1}</style>',
              '<rect width="100%" height="100%" fill="#f8fafc"/>']
    for name, color in (("exec", "#b36b13"), ("data", "#317b9b"), ("removed", "#b42318")):
        pieces.append(f'<defs><marker id="{marker_prefix}{name}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{color}"/></marker></defs>')
    title_markup = ' / '.join(
        '<a href="' + escape(part['href']) + '" tabindex="0"><tspan style="text-decoration:underline">' +
        escape(part['name']) + '</tspan></a>' if 'href' in part else '<tspan>' + escape(part['name']) + '</tspan>'
        for part in title_parts)
    pieces.append(f'<text class="graph-title" x="40" y="34" font-size="22" font-weight="600" aria-label="Graph hierarchy">{title_markup}</text>')
    pieces.append(f'<text x="40" y="59" font-size="12">{view} · {direction} · {len(nodes)} / {len(all_nodes)} nodes · {len(edges)} / {len(all_edges)} links · source unchanged</text>')
    pieces.append('<g class="graph-legend" role="group" aria-label="Node and link legend">')
    for x, label, color in ((40, 'Entry / event', '#dce9fc'), (197, 'Execution step', '#f7e9d7'), (354, 'Data / other', '#e8edf4')):
        pieces.append(f'<rect x="{x}" y="73" width="145" height="36" rx="8" fill="{color}" stroke="#8b9bae" stroke-width="1.5"/><text x="{x+72.5}" y="96" text-anchor="middle" font-size="13" font-weight="600">{label}</text>')
    for x, label, style, color, dash in ((525, 'Execution', 'exec', '#b36b13', ''), (727, 'Data', 'data', '#317b9b', ' stroke-dasharray="6 4"')):
        pieces.append(f'<path d="M{x},91 H{x+52}" fill="none" stroke="{color}" stroke-width="2.5"{dash} marker-end="url(#{marker_prefix}{style})"/><text x="{x+65}" y="96" font-size="13" font-weight="600">{label}</text>')
    pieces.append('</g>')
    pieces.append('<text x="40" y="133" font-size="12">Structural view only. Timers / indirect calls are not inferred. Details button: node data. Hover arrows: pin details.</text>')
    pieces.append('<text x="40" y="154" font-size="12">Unconnected pin defaults are marked as defaults, not runtime results. Details retains raw fields.</text>')
    pieces.append(f'<text x="40" y="175" font-size="12">Hidden: {len(hidden)} disabled / Self nodes, {hidden_links} incident links. Folded: {folded} nodes into {"expressions" if view == "expressions" else "chains"}; see Details / Full.</text>')
    declarations = document.get("variables", [])
    if declarations:
        declaration_text = "Variables: " + ", ".join(variable_label(v) for v in declarations)
        visible_text = textwrap.shorten(declaration_text, width=max(30, int((width - 80) / 7)), placeholder=" …")
        pieces.append(f'<text x="40" y="196" font-size="12"><title>{escape(declaration_text)}</title>{escape(visible_text)}</text>')
    if document.get('_review'):
        pieces.append('<text x="40" y="217" font-size="12">REVIEW · + green: added · ~ purple: changed · − red dashed: removed · Details: Before / After · explicit marks only</text>')
    incoming, outgoing = defaultdict(list), defaultdict(list)
    for i, edge in enumerate(edges):
        outgoing[edge[0]].append(i)
        incoming[edge[2]].append(i)
    nodes_by_id = {node['id']: node for node in nodes}
    # Data precedes execution at both ends; geometry never changes pin order.
    for indices in outgoing.values():
        indices.sort(key=lambda i: (edges[i][4], pin_order(edges[i][1]), edges[i][2], edges[i][3], i))
    for indices in incoming.values():
        indices.sort(key=lambda i: (edges[i][4], pin_order(edges[i][3]), edges[i][0], edges[i][1], i))
    output_ports = {node_id: list(dict.fromkeys(edges[i][1] for i in indices))
                    for node_id, indices in outgoing.items()}
    for i, (source, output, target, input_pin, execution) in enumerate(edges):
        x, y, w, h = boxes[source]
        tx, ty, tw, th = boxes[target]
        transform = ''
        ports = output_ports[source]
        source_fraction = (ports.index(output) + 1) / (len(ports) + 1)
        if direction == "TB":
            sx, sy = x + w * source_fraction, y + h
            ex, ey = tx + tw * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1), ty
            bend = max(35, (ey - sy) / 2)
            path = f'M{sx},{sy} C{sx},{sy+bend} {ex},{ey-bend} {ex},{ey}'
            if source == target:
                # Keep a self-loop visible beside its own node, not at a graph-wide lane.
                side = x + w + 55
                path = f'M{sx},{sy} C{side},{sy+40} {side},{ey-40} {ex},{ey}'
            lx, ly = (sx + ex) / 2, (sy + ey) / 2 - 7
        else:
            sx, sy = x + w, y + h * source_fraction
            ex, ey = tx, ty + th * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1)
            bend = max(35, (ex - sx) / 2)
            path = f'M{sx},{sy} C{sx+bend},{sy} {ex-bend},{ey} {ex},{ey}'
            if source == target:
                side = y + h + 55
                path = f'M{sx},{sy} C{sx+40},{side} {ex-40},{side} {ex},{ey}'
            lx, ly = (sx + ex) / 2, (sy + ey) / 2 - 7
        label_text, anchor = output + ' → ' + ('' if execution else 'Input: ') + input_pin, 'middle'
        branch_choice = execution
        if branch_choice:
            # Branch choice belongs at the originating pin, not on a distant routing lane.
            label_text, transform = output, ''
            if direction == 'TB':
                lx, ly = sx, sy + 13
            else:
                lx, ly, anchor = sx + 10, sy - 7, 'start'
        style, color = ("exec", "#b36b13") if execution else ("data", "#317b9b")
        dash = '' if execution else ' stroke-dasharray="6 4"'
        removed_link = ((source, output, target, input_pin, execution) in document.get('_reviewRemovedLinks', [])
                        or any(nodes_by_id[n].get('_reviewStatus') == 'removed' for n in (source, target)))
        if removed_link:
            color, dash = '#b42318', ' stroke-dasharray="4 5" opacity="0.55"'
            style = 'removed'
        templates = document.get('pinTemplates', {})
        source_id, output_name, output_type = edge_pin_details(nodes_by_id[source], output, 'out', templates)
        target_id, input_name, input_type = edge_pin_details(nodes_by_id[target], input_pin, 'in', templates)
        typed_output = output_name + (f' ({output_type})' if output_type else '')
        typed_input = input_name + (f' ({input_type})' if input_type else '')
        tooltip = f'{source_id} -> {target_id}\nFrom: {typed_output}\nTo: {typed_input}'
        uses = nodes_by_id[target].get('_expressionInputLinks', {}).get(input_pin, [])
        if len(uses) > 1:
            targets = []
            for _, _, original_target, original_pin, _ in uses:
                part = next(n for n in nodes_by_id[target]['_foldedNodes'] if n['id'] == original_target)
                _, name, kind = edge_pin_details(part, original_pin, 'in', templates)
                targets.append(original_target + ': ' + name + (f' ({kind})' if kind else ''))
            tooltip = f'{source_id} -> {target}\nFrom: {typed_output}\nTo:\n' + '\n'.join(targets)
        if removed_link:
            tooltip = 'Removed connection (Before)\n' + tooltip
        pieces.append(f'<g class="graph-edge" data-source="{escape(source)}" data-target="{escape(target)}"><title>{escape(tooltip)}</title><path class="edge-line" d="{path}" fill="none" stroke="{color}" stroke-width="2"{dash} marker-end="url(#{marker_prefix}{style})"/>')
        # A solid transparent hit area covers gaps in dashed lines and stays usable at any zoom.
        pieces.append(f'<path class="edge-hit" d="{path}" fill="none" stroke="transparent" stroke-width="14" vector-effect="non-scaling-stroke" pointer-events="stroke"/>')
        if branch_choice:
            pieces.append(f'<text class="label" x="{lx}" y="{ly}" text-anchor="{anchor}"{transform}>{escape(label_text)}</text>')
        pieces.append('</g>')
    execution_nodes = {endpoint for source, _, target, _, execution in all_edges if execution for endpoint in (source, target)}
    for node in nodes:
        x, y, w, h = boxes[node["id"]]
        child = graph_context.get('children', {}).get(node['id'])
        # A folded display chain has no graph of its own. Only its individual
        # step links navigate; never promote one step's target to the whole card.
        destination = (child or node.get('_graphLink')) if '_foldedNodes' not in node else None
        callers = graph_context.get('callers', []) if node['id'] in graph_context.get('entryNodes', []) else []
        color = node_fill(node, document.get('pinTemplates', {}), execution_nodes)
        opacity = ' opacity="0.55"' if node.get("enabled") is False else ''
        status = node.get('_reviewStatus', '')
        nested_changed = bool(child and child.get('changed') and status not in ('added', 'removed'))
        stroke = {'added': '#15803d', 'changed': '#7c3aed', 'removed': '#b42318'}.get(
            status, '#7c3aed' if nested_changed else '#8b9bae')
        dash = ' stroke-dasharray="7 4"' if status == 'removed' else ''
        if status == 'removed':
            opacity = ' opacity="0.6"'
        details = node.get('_reviewDetails', node)
        review_html = ''
        if status == 'changed':
            review_html = ' data-review-details="' + escape(render_details(details['Before'], details['After'])) + '"'
        ids = [n['id'] for n in node.get('_foldedNodes', [node])]
        anchor = f' id="node-{escape(node["id"])}"' if view == "simplified" and direction == "TB" else ""
        pieces.append(f'<g class="graph-node" data-node-id="{escape(node["id"])}" data-node-ids="{escape(json.dumps(ids))}"{anchor} data-review-status="{status}"{review_html}{opacity}><title>{escape(json.dumps(details, ensure_ascii=False, indent=2))}</title><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{color}" stroke="{stroke}" stroke-width="{3 if status or nested_changed else 1.5}"{dash}/>')
        if destination:
            pieces.append(f'<rect class="nested-graph-frame" x="{x+4}" y="{y+4}" width="{w-8}" height="{h-8}" rx="8" fill="none" stroke="{stroke}" stroke-width="1" pointer-events="none"{dash}/>')
        hidden_offset, section_offset = 0, 0
        visible_gaps = iter(compact_gaps[node['id']])
        for row, line in enumerate(lines_by_id[node["id"]]):
            header = row < title_rows[node['id']]
            size = 16 if header else 12
            weight = ' font-weight="600"' if header else ''
            title_width = w - (96 if destination or callers else 70) if row == 0 else w - 28
            fit = f' textLength="{title_width}" lengthAdjust="spacingAndGlyphs"' if header and len(line) > 25 else ''
            previous = changed_lines.get(node['id'], {}).get(row)
            decoration, tooltip = '', ''
            if previous is not None:
                decoration = ' class="review-changed-line" style="fill:#7c3aed;font-weight:600"'
                tooltip = '<title>' + escape('Previously: ' + previous) + '</title>'
            elif line == '~ Nested implementation changed':
                decoration = ' class="review-nested-change" style="fill:#7c3aed;font-weight:600"'
            elif line.startswith('Warning: result outputs'):
                decoration = ' class="graph-result-warning" style="fill:#b45309;font-weight:600"'
            gap = section_gaps[node['id']][row]
            section_offset += gap
            if not getattr(line, 'unconnected_output', False):
                hidden_offset += gap - next(visible_gaps)
            compact_style = f'--compact-dy:{-hidden_offset}px;'
            code_style = ''
            if getattr(line, 'code', False):
                compact_style += 'white-space:pre;font-family:Consolas,monospace;'
                code_style = ' font-family="Consolas, monospace" xml:space="preserve"'
            if 'style="' in decoration:
                decoration = decoration.replace('style="', 'style="' + compact_style)
            else:
                decoration += f' style="{compact_style}"'
            output_flag = ' data-unconnected-output="true"' if getattr(line, 'unconnected_output', False) else ''
            reference = getattr(line, 'reference', None)
            if reference:
                tooltip += '<title>' + escape(reference['field'] + '\n' + reference['target']) + '</title>'
            label = re.match(r'^(Input:|Output(?: \([^)]*\))?:|Parameter:|Field:)', line) if not header else None
            content = (f'<tspan font-weight="600" style="font-family:Segoe UI,Arial,sans-serif">{escape(label[0])}</tspan>' + escape(line[len(label[0]):])
                       if label else escape(line))
            text = f'<text data-card-row="{row}"{output_flag} x="{x+14}" y="{y+24+row*19+section_offset}" font-size="{size}"{weight}{fit}{code_style}{decoration}>{content}{tooltip}</text>'
            if reference and reference.get('href'):
                reference_class = 'graph-reference' if reference.get('kind') == 'graph' else 'asset-reference'
                text = '<a class="' + reference_class + '" href="' + escape(reference['href']) + '">' + text + '</a>'
            pieces.append(text)
            hidden_offset += (19 + gap) if output_flag else 0
        copy_label = 'Copy node ID' if len(ids) == 1 else f'Copy all {len(ids)} node IDs'
        pieces.append(f'<g class="copy-node-id" role="button" tabindex="0" aria-label="{copy_label}" transform="translate({x+w-27},{y+5})"><title>{copy_label}</title><rect width="22" height="22" rx="4" fill="#fff" stroke="#64748b"/><path d="M6 6h9v11H6z M9 3h9v11" fill="none" stroke="#334155" stroke-width="1.5"/></g>')
        pieces.append(f'<g class="node-info" role="button" tabindex="0" aria-label="Show node details" transform="translate({x+w-53},{y+5})"><title>Show node details</title><rect width="22" height="22" rx="4" fill="#fff" stroke="#64748b"/><text x="11" y="16" text-anchor="middle" font-size="16" font-weight="600">i</text></g>')
        if destination:
            label = escape('Open graph: ' + destination.get('target', destination['name']))
            pieces.append(f'<a class="open-child-graph" href="{escape(destination["href"])}" tabindex="0" aria-label="{label}" transform="translate({x+w-79},{y+5})"><title>{label}</title><rect width="22" height="22" rx="4" fill="#fff" stroke="#64748b"/><path d="M12 4h6v14h-6 M3 11h11 M10 7l4 4-4 4" fill="none" stroke="#334155" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></a>')
        if callers:
            label = escape('Go to calling graph: ' + callers[0]['name'] if len(callers) == 1 else f'Choose calling graph ({len(callers)})')
            tag = 'a' if len(callers) == 1 else 'g'
            action = f'href="{escape(callers[0]["href"])}"' if tag == 'a' else 'role="button" aria-haspopup="true" aria-expanded="false" aria-controls="caller-menu"'
            pieces.append(f'<{tag} class="open-caller-graph" {action} tabindex="0" aria-label="{label}" transform="translate({x+w-79},{y+5})"><title>{label}</title><rect width="22" height="22" rx="4" fill="#fff" stroke="#64748b"/><path transform="translate(22 0) scale(-1 1)" d="M12 4h6v14h-6 M3 11h11 M10 7l4 4-4 4" fill="none" stroke="#334155" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></{tag}>')
        pieces.append('</g>')
    pieces.append('</svg>')
    if direction in ('TB', 'LR'):
        layout = {'direction': direction, 'minWidth': header_width, 'nodes': [dict(id=n['id'], rank=rank[n['id']], kind=n.get('kind'),
                                x=boxes[n['id']][0], y=boxes[n['id']][1],
                                w=boxes[n['id']][2], h=boxes[n['id']][3],
                                compactH=28 + 19 * len(compact_lines[n['id']])
                                    + sum(compact_gaps[n['id']])) for n in nodes],
                  'edges': edges}
        pieces[0] = pieces[0].replace('<svg ', '<svg data-layout="' + escape(json.dumps(layout, separators=(',', ':'))) + '" ', 1)
    return '\n'.join(pieces), title, len(nodes), len(edges)


def caller_navigation(callers):
    if len(callers) < 2:
        return ''
    links = []
    for caller in callers:
        count = len(caller['nodes'])
        detail = f'<small>{count} calls</small>' if count > 1 else ''
        links.append(f'<li><a href="{escape(caller["href"])}" title="{escape(caller["asset"])}">{escape(caller["name"])}{detail}</a></li>')
    return '<aside id="caller-menu" hidden aria-label="Calling graphs"><ul>' + ''.join(links) + '</ul></aside>'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--output", type=Path, required=True, help="Output .svg; adjacent .html is also written")
    parser.add_argument("--graph", help="Stable GraphGuid or unambiguous graph name")
    parser.add_argument('--review', type=Path, help='Explicit added/changed/removed node ID lists')
    parser.add_argument('--before', type=Path, help='Original compact source for review Before details and removed nodes')
    parser.add_argument('--asset-context', type=Path, help='Tool-owned asset roles and preview links; never part of editable BPT')
    parser.add_argument('--graph-context', type=Path, help='Tool-owned nested navigation; never part of editable BPT')
    parser.add_argument("--view", choices=VIEW_MODES, default="simplified")
    parser.add_argument("--direction", choices=("TB", "LR"), default="TB", help="Top-to-bottom (default) or left-to-right")
    args = parser.parse_args()
    try:
        if args.output.suffix.lower() != ".svg":
            raise ValueError("--output must end in .svg")
        html_path = args.output.with_suffix(".html")
        full_path = args.output.with_name(args.output.stem + ".full.svg")
        if any(p and p.resolve() in (args.output.resolve(), html_path.resolve(), full_path.resolve())
               for p in (args.input, args.review, args.before)):
            raise ValueError("Output must not overwrite the input.")
        document = json.loads(args.input.read_text(encoding="utf-8-sig"))
        asset_context = json.loads(args.asset_context.read_text(encoding='utf-8')) if args.asset_context else {}
        graph_context = json.loads(args.graph_context.read_text(encoding='utf-8')) if args.graph_context else {}
        final_document = document
        has_review = any(c.get('changed') for c in graph_context.get('children', {}).values())
        if args.before and not args.review:
            raise ValueError('--before requires --review.')
        if args.review:
            review = json.loads(args.review.read_text(encoding='utf-8-sig'))
            before = json.loads(args.before.read_text(encoding='utf-8-sig')) if args.before else None
            reviewed = review_document(document, before, review, args.graph)
            if any(review.values()):
                has_review = True
                document = reviewed
        variants = {(view, direction): render(document, args.graph, view, direction, asset_context=asset_context, graph_context=graph_context)
                    for view in VIEW_MODES for direction in ("TB", "LR")}
        svg, title, nodes, links = variants[args.view, args.direction]
        sections = ''.join(f'<section id="{view}-{direction}"' +
                           ('' if (view, direction) == (args.view, args.direction) else ' hidden') +
                           '>' + result[0] + '</section>' for (view, direction), result in variants.items())
        # A whole new/deleted graph has one snapshot, not a comparison to toggle.
        has_review = has_review and graph_context.get('comparisonAvailable', args.before is not None)
        if has_review:
            final_context = copy.deepcopy(graph_context)
            for child in final_context.get('children', {}).values():
                child.pop('changed', None)
            sections += ''.join(f'<section id="final-{view}-{direction}" hidden>' +
                render(final_document, args.graph, view, direction,
                       asset_context=asset_context, graph_context=final_context, marker_namespace='final-')[0] + '</section>'
                for view in VIEW_MODES for direction in ('TB', 'LR'))
        page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>''' + escape(title) + '''</title>
<style>body{margin:0;background:#f8fafc;font:14px 'Segoe UI',sans-serif;height:100vh;display:grid;grid-template-rows:auto 1fr}nav{padding:10px 18px;background:#fff;border-bottom:1px solid #ddd;display:flex;gap:8px;align-items:center;flex-wrap:wrap}button,select{padding:6px 10px}main{overflow:auto;min-height:0;position:relative}svg{display:block}</style>
<style>.graph-node{cursor:pointer}.graph-node:focus-visible{outline:none}.graph-node:focus-visible>rect{stroke:#1d4ed8;stroke-width:3}svg.has-selection .graph-node:not(.selected):not(.neighbor),svg.has-selection .graph-edge:not(.related):not(.inspected):not(:hover){opacity:.16}.graph-node.selected,.graph-node.neighbor{opacity:1}.graph-node.selected>rect{stroke:#1d4ed8;stroke-width:4}.graph-node.neighbor>rect{stroke-width:2.5}.graph-edge.related>.edge-line,.graph-edge.inspected>.edge-line{stroke-width:3.5}.graph-edge.inspected{opacity:1}.hint{color:#52657a;font-size:12px}.copy-node-id{cursor:copy}.node-info{cursor:pointer}.copy-node-id:hover>rect,.copy-node-id:focus>rect,.node-info:hover>rect,.node-info:focus>rect{fill:#dbeafe;stroke:#1d4ed8}.copy-node-id:focus,.node-info:focus{outline:none}#edge-tooltip{position:fixed;z-index:30;max-width:min(440px,calc(100vw - 24px));max-height:60vh;overflow:auto;padding:10px 12px;border:1px solid #94a3b8;border-radius:6px;background:#fff;box-shadow:0 4px 18px #20334b33;white-space:pre-wrap;overflow-wrap:anywhere;user-select:text;font:12px/1.5 Consolas,monospace}#copy-status{font-size:12px;color:#17614b}#node-details{position:absolute;z-index:20;width:min(520px,calc(100vw - 24px));max-height:70vh;overflow:auto;background:#fff;border:1px solid #94a3b8;border-radius:8px;box-shadow:0 6px 28px #20334b44;user-select:text}#node-details[hidden]{display:none}.detail-toolbar{position:sticky;top:0;display:flex;gap:6px;background:#f1f5f9;padding:8px;border-bottom:1px solid #cbd5e1}#close-details{margin-left:auto}#detail-ids{padding:10px 12px 0;overflow-wrap:anywhere;font-weight:600}#detail-text{margin:0;padding:12px;white-space:pre-wrap;overflow-wrap:anywhere;font:12px/1.5 Consolas,monospace;cursor:text}</style>
<style>.detail-row{display:block;min-height:1.5em}.detail-mark{display:inline-block;width:1.5em;user-select:none}.detail-changed{color:#7c3aed}.detail-added{color:#15803d}.detail-removed{color:#b42318;text-decoration:line-through}.detail-technical{color:#64748b}</style>
<nav><label>View <select id="view" onchange="choose()"><option value="simplified">Simplified</option><option value="full">Full</option><option value="expressions">Expressions</option></select></label>
<label>Direction <select id="direction" onchange="choose()"><option value="TB">Top to bottom</option><option value="LR">Left to right</option></select></label>
<button onclick="zoom(.8)" aria-label="Zoom out">−</button><button onclick="zoom(1.25)" aria-label="Zoom in">+</button><button onclick="fit()">Fit width</button><button onclick="fit(true)">Fit graph</button>
<label>Nodes per row <input id="nodes-per-row" type="number" min="1" max="12" step="1" value="3" style="width:4em;padding:6px" onchange="changeColumns()" title="Maximum nodes per row in top-to-bottom view"></label>
<label title="Show unconnected outputs on every node, including execution branches and output defaults."><input id="unconnected" type="checkbox" checked onchange="changeUnconnected()">Show unconnected</label>
<style>svg.hide-unconnected [data-unconnected-output="true"]{display:none}svg.hide-unconnected [data-card-row]{transform:translateY(var(--compact-dy,0px))}</style>
<span id="copy-status" role="status" aria-live="polite"></span></nav><main>''' + sections + '''
<aside id="node-details" hidden aria-label="Node details"><div class="detail-toolbar"><select id="detail-version" aria-label="Details version" hidden><option>After</option><option>Before</option></select><button id="copy-detail-ids">Copy IDs</button><button id="copy-details">Copy contents</button><button id="close-details" aria-label="Close details">×</button></div><div id="detail-ids"></div><pre id="detail-text" tabindex="0"></pre></aside></main><div id="edge-tooltip" role="tooltip" hidden></div>
<script>''' + (Path(__file__).with_name('graph_layout.js').read_text(encoding='utf-8')) + '\n' + (Path(__file__).with_name('graph_viewer.js').read_text(encoding='utf-8')) + '''
''' + f'view.value="{args.view}";direction.value="{args.direction}";' + '''restoreViewerState();</script></html>'''
        if has_review:
            page = page.replace('>Show unconnected</label>', '>Show unconnected</label>\n<label title="Turn off to see only the final graph, without review marks or removed nodes."><input id="show-changes" type="checkbox" checked onchange="changeReview()">Show changes</label>', 1)
        page = page.replace('<aside id="node-details"', caller_navigation(graph_context.get('callers', [])) + '<aside id="node-details"', 1)
        page = page.replace('<nav>', '<style>.open-caller-graph{cursor:pointer}.open-caller-graph:focus{outline:none}.open-caller-graph:hover>rect,.open-caller-graph:focus-visible>rect{fill:#dbeafe;stroke:#1d4ed8;stroke-width:2}#caller-menu{position:absolute;z-index:40;min-width:280px;width:max-content;max-width:min(650px,85vw);max-height:60vh;overflow:auto;padding:5px;border:1px solid #94a3b8;border-radius:6px;background:#fff;box-shadow:0 4px 18px #20334b33}#caller-menu[hidden]{display:none}#caller-menu ul{list-style:none;margin:0;padding:0}#caller-menu a{display:block;padding:8px;color:#175ba4;overflow-wrap:anywhere;text-decoration:none}#caller-menu a:hover,#caller-menu a:focus-visible{background:#dbeafe}#caller-menu small{display:block;color:#52657a}</style><nav>', 1)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(svg, encoding="utf-8")
        full_path.write_text(variants["full", args.direction][0], encoding="utf-8")
        html_path.write_text(page, encoding="utf-8")
        print(json.dumps({"svg": str(args.output.resolve()), "fullSvg": str(full_path.resolve()),
                          "html": str(html_path.resolve()), "view": args.view, "direction": args.direction,
                          "nodes": nodes, "links": links}))
    except (ValueError, KeyError, TypeError, AttributeError, OSError, RecursionError) as error:
        parser.exit(2, f"Cannot render graph: {error}\n")


if __name__ == "__main__":
    main()
