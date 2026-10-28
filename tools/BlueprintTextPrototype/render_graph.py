"""Render compact Blueprint JSON to offline SVG + HTML; Python standard library only.

Usage: python render_graph.py blueprint.json --output diagram.svg [--view full] [--direction LR]
Accepts the shared prototype envelope or the earlier standalone nodes/links JSON.
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


def escape(value):
    return html.escape(str(value), quote=True)


def load_graph(document, selection=None):
    if document.get("schema") not in (1, 2):
        raise ValueError("Expected compact schema 1 or 2.")
    if "graphs" in document:
        graphs = [g for g in document["graphs"] if not selection or g["name"] == selection]
        if len(graphs) != 1:
            raise ValueError("Select exactly one graph with --graph NAME.")
        graph = graphs[0]
        if graph.get("format") != "compact":
            raise ValueError("Native graph text is not supported. Supply a compact export.")
        body, name = graph["body"], graph["name"]
    else:
        body, name = document, document.get("graph", "Graph")
        if selection and selection != name:
            raise ValueError(f"Graph {selection!r} was not found.")
    nodes = body["nodes"]
    if len(nodes) > 500:
        raise ValueError("This simple viewer supports at most 500 nodes per graph.")
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
            if not separator or node_id not in ids or not pin:
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
        if (old_owner, old_name) != (owner, name):
            raise ValueError('Before and current sources must identify the same asset and graph.')
    elif any(s in ('changed', 'removed') for s in marked.values()):
        raise ValueError('Changed and removed nodes require --before source.')
    current_by_id, old_by_id = ({n['id']: n for n in ns} for ns in (current, old))
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

    def snapshot(node_id, by_id, links):
        if node_id not in by_id:
            return None
        return {'node': copy.deepcopy(by_id[node_id]),
                'links': [{'from': e[0] + '.' + e[1], 'to': e[2] + '.' + e[3]}
                          for e in links if node_id in (e[0], e[2])]}

    for node in result['nodes']:
        node_id = node['id']
        if node_id in boundary:
            node['_reviewBoundary'] = True
        if node_id in marked:
            node['_reviewStatus'] = marked[node_id]
            if marked[node_id] == 'changed':
                node['_reviewPreviousDescription'] = description_fields(
                    old_by_id[node_id], before.get('pinTemplates', {}), before.get('parent'))
            node['_reviewDetails'] = {'Status': marked[node_id],
                                     'Before': snapshot(node_id, old_by_id, old_edges),
                                     'After': snapshot(node_id, current_by_id, edges)}
    if len(result['nodes']) > 500:
        raise ValueError('Review including removed nodes exceeds the 500-node limit.')
    return result


def layers(nodes, edges):
    """Collapse cycles before assigning dependency layers."""
    adjacent = {node["id"]: [] for node in nodes}
    for source, _, target, _, _ in edges:
        adjacent[source].append(target)
    indices, low, stack, active, components = {}, {}, [], set(), []

    def visit(node):
        indices[node] = low[node] = len(indices)
        stack.append(node)
        active.add(node)
        for target in sorted(set(adjacent[node])):
            if target not in indices:
                visit(target)
                low[node] = min(low[node], low[target])
            elif target in active:
                low[node] = min(low[node], indices[target])
        if low[node] == indices[node]:
            component = []
            while True:
                member = stack.pop()
                active.remove(member)
                component.append(member)
                if member == node:
                    break
            components.append(sorted(component))

    for node in adjacent:
        if node not in indices:
            visit(node)
    membership = {node: i for i, component in enumerate(components) for node in component}
    rank = [0] * len(components)
    # Tarjan emits sink components first, so reverse order is topological.
    for i in reversed(range(len(components))):
        for node in components[i]:
            for target in adjacent[node]:
                j = membership[target]
                if i != j:
                    rank[j] = max(rank[j], rank[i] + 1)
    return {node: rank[membership[node]] for node in adjacent}


def native_string(value):
    return json.loads(value) if value.startswith('"') else value


def pin_category(node, alias, templates):
    pin = node.get('pins', {}).get(alias, {})
    return native_string(templates.get(pin.get('type'), {}).get('PinType.PinCategory', '""'))


def node_fill(node, templates, execution_nodes):
    kind = node.get('kind', 'unknown')
    if kind in ('event', 'customEvent', 'FunctionEntry'):
        return '#dce9fc'
    if (any(pin_category(node, alias, templates) == 'exec' for alias in node.get('pins', {}))
            or node['id'] in execution_nodes
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


def fold_chains(nodes, edges, original_edges, templates, parent=None, mode='data'):
    """Contract linear data or execution edges; retain original nodes and boundaries."""
    by_id = {n['id']: n for n in nodes}
    incoming, outgoing = defaultdict(list), defaultdict(list)
    for edge in original_edges:
        outgoing[edge[0]].append(edge)
        incoming[edge[2]].append(edge)

    def eligible(node):
        if node.get('_reviewStatus') or node.get('_reviewBoundary'):
            return False
        if mode == 'action':
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
        if edge[4] == (mode == 'action') and target and eligible(target) and len(inputs) == 1:
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
        steps = [node_lines(n, {e[3] for e in incoming[n['id']]}, pin_templates=templates, parent=parent)
                 for n in chain]
        if mode == 'action':
            steps = [[line for line in step if line != 'No data outputs'] for step in steps]
        replacement = dict(chain[-1], _foldedNodes=chain, _foldedLinks=links, _foldedSteps=steps, _chainKind=mode)
        if mode == 'action':
            step_by_id = {n['id']: i for i, n in enumerate(chain)}
            for source, output, target, input_pin, execution in links:
                if not execution:
                    step = steps[step_by_id[target]]
                    index = next((i for i, line in enumerate(step) if line.startswith(('Returns:', 'Output '))), len(step))
                    step.insert(index, f'Input: {input_pin} ← Step {step_by_id[source]+1}: {output}')
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
        if mode == 'action' and target in remap and not execution:
            input_pin = f'Step {step}: {input_pin}'
        elif target != new_target:
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


def output_lines(node, pin_templates):
    if "pins" not in node:
        return []  # Schema 1 has no complete signature; do not infer a return type.
    lines = []
    for alias, pin in node["pins"].items():
        if not alias.startswith("out:"):
            continue
        metadata = pin_templates.get(pin.get("type"), {})
        if native_string(metadata.get("PinType.PinCategory", '""')) == "exec":
            continue
        name = pin.get("name", alias[4:])
        prefix = "Returns" if name == "ReturnValue" or alias == "out:result" else "Output " + name
        lines.append(prefix + ": " + pin_type_label(metadata))
    return lines or ["No data outputs"]


def edge_output_type(node, output, pin_templates):
    """Resolve the producing pin, including an earlier step of an action chain."""
    if '_foldedNodes' in node:
        match = re.fullmatch(r'Step (\d+): (.+)', output)
        if match:
            index = int(match.group(1)) - 1
            if not 0 <= index < len(node['_foldedNodes']):
                return None
            node, output = node['_foldedNodes'][index], match.group(2)
    pin = node.get('pins', {}).get('out:' + output, {})
    metadata = pin_templates.get(pin.get('type'), {})
    if not metadata.get('PinType.PinCategory'):
        return None  # Unknown legacy signature: do not infer a type from the consumer.
    return pin_type_label(metadata)


def unconnected_outputs(node, connected, pin_templates):
    """Declared visible pins only; count original links, including folded/hidden ones."""
    if 'pins' in node:
        outputs = [alias[4:] for alias, pin in node['pins'].items() if alias.startswith('out:')
                   and pin_templates.get(pin.get('type'), {}).get('bHidden') != 'True']
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
    return [name for name in outputs if name not in connected and name not in terminal]


def node_lines(node, connected_inputs, hidden_inputs=(), pin_templates=None, parent=None,
               step_inputs=(), step_outputs=(), wrap=True):
    if '_expression' in node and not any(step_outputs):
        lines = [node['_expression'], f"Expression · {len(node['_foldedNodes'])} nodes (Details / Full)"]
        lines.extend(output_lines(node, pin_templates or {}))
        return [part for i, line in enumerate(lines) for part in wrap_node_line(line, i == 0)]
    if '_foldedSteps' in node:
        label = 'Action chain' if node.get('_chainKind') == 'action' else 'Data chain'
        lines = [f"{label} · {len(node['_foldedNodes'])} nodes"]
        for i, step in enumerate(node['_foldedSteps'], 1):
            step = list(step)
            if i <= len(step_inputs):
                index = next((j for j, line in enumerate(step[1:], 1)
                              if line.startswith(('Input:', 'Returns:', 'Output ', 'No data outputs'))), len(step))
                step[index:index] = ['Input: ' + hint for hint in step_inputs[i-1]]
            lines.extend([f'{i}. {step[0]}'] + step[1:])
            if i <= len(step_outputs):
                lines.extend(step_outputs[i-1])
        return [part for i, line in enumerate(lines) for part in wrap_node_line(line, i == 0)]
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
    else:
        title = readable_name(short)
    if kind == "call":
        label = lookup_function_label(node, pin_templates, parent)
        title = label['title'] if label else ("NOT" if short == "Not_PreBool" else readable_name(title))
    # All IDs remain in the tooltip and link identity, not the visible node label.
    lines = [title] + details
    lines.extend('Input: ' + hint for hint in hidden_inputs)
    values = node.get("arguments", node.get("defaults", {}))
    for key, value in sorted(values.items()):
        metadata = (pin_templates or {}).get(node.get("pins", {}).get(key, {}).get("type"), {})
        if key.startswith("out:") or key.removeprefix("in:") in connected_inputs or metadata.get("bDefaultValueIsIgnored") == "True":
            continue
        lines.append(f"Input: {key.removeprefix('in:')} = {json.dumps(value, ensure_ascii=False)}")
    if kind == "call":
        lines.extend(output_lines(node, pin_templates or {}))
    if is_disabled(node):
        lines.append("DISABLED")
    if node.get("developmentOnly"):
        lines.append("DEVELOPMENT ONLY")
    if node.get("comment"):
        lines.append("Comment: " + node["comment"])
    if not wrap:
        return lines
    return [part for row, line in enumerate(lines)
            for part in wrap_node_line(line, row == 0)]


def description_key(line, index):
    """Match semantic fields before wrapping, independent of ordering and links."""
    if index == 0:
        return 'title'
    if line.startswith('Input: ') and ' = ' in line:
        return line.partition(' = ')[0]
    if ':' in line:
        return line.partition(':')[0]
    return line


def description_fields(node, templates, parent):
    # Include stored defaults regardless of wiring, so a link-only edit does not
    # turn an unchanged default into a changed description when it becomes visible.
    return {description_key(line, i): line for i, line in enumerate(
        node_lines(node, set(), pin_templates=templates, parent=parent, wrap=False))}


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


def render(document, selection=None, view="simplified", direction="TB", review=None, before=None):
    if review is not None:
        document = review_document(document, before, review, selection)
    if view not in ("simplified", "full") or direction not in ("TB", "LR"):
        raise ValueError("Expected view simplified/full and direction TB/LR.")
    all_nodes, all_edges, owner, graph = load_graph(document, selection)
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
        nodes, edges, actions_folded = fold_chains(nodes, edges, all_edges, document.get("pinTemplates", {}), document.get("parent"), 'action')
        folded += actions_folded
    rank = layers(nodes, edges)
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
        notes = [[f'○ {output} — unconnected' for output in
                  unconnected_outputs(part, connected_outputs[part['id']], document.get('pinTemplates', {}))]
                 for part in parts]
        if '_foldedNodes' in node:
            lines_by_id[node_id] = node_lines(node, connected[node_id], pin_templates=document.get('pinTemplates', {}),
                                             parent=document.get('parent'), step_inputs=[hidden_inputs[p['id']] for p in parts],
                                             step_outputs=notes)
            title_rows[node_id] = len(wrap_node_line(node['_expression'], True)) if '_expression' in node and not any(notes) else 1
        else:
            templates, parent = document.get('pinTemplates', {}), document.get('parent')
            raw_lines = node_lines(node, connected[node_id], hidden_inputs[node_id], templates, parent, wrap=False)
            title_rows[node_id] = len(wrap_node_line(raw_lines[0], True))
            previous = node.get('_reviewPreviousDescription', {})
            current = description_fields(node, templates, parent) if previous else {}
            lines_by_id[node_id], changed_lines[node_id] = [], {}
            for index, line in enumerate(raw_lines):
                key = description_key(line, index)
                changed = key in current and current[key] != previous.get(key)
                for part in wrap_node_line(line, index == 0):
                    if changed:
                        changed_lines[node_id][len(lines_by_id[node_id])] = previous.get(key, '(not present)')
                    lines_by_id[node_id].append(part)
            for note in notes[0]:
                lines_by_id[node_id].extend(textwrap.wrap(note, 42))
        if node.get('_reviewStatus'):
            status = node['_reviewStatus']
            badge_row = title_rows[node_id]
            lines_by_id[node_id].insert(badge_row, {'added': '+ Added', 'changed': '~ Changed', 'removed': '− Removed'}[status])
            changed_lines[node_id] = {i + (i >= badge_row): value for i, value in changed_lines.get(node_id, {}).items()}
    parent_edges = defaultdict(list)
    for edge in edges:
        parent_edges[edge[2]].append(edge)
    y = 210
    max_columns = min(3, max((len(m) for m in groups.values()), default=1))
    for column, members in sorted(groups.items()):
        members = order_layer(members, parent_edges, boxes, direction)
        if direction == "LR":
            y = 210
            for node in members:
                height = 28 + 19 * len(lines_by_id[node["id"]])
                boxes[node["id"]] = (40 + column * 500, y, 330, height)
                y += height + 60
        else:
            # Bound row width; large root sets continue downward, not off-screen.
            for start in range(0, len(members), 3):
                chunk = members[start:start + 3]
                offset = (max_columns - len(chunk)) * 190
                row_height = max(28 + 19 * len(lines_by_id[n["id"]]) for n in chunk)
                for index, node in enumerate(chunk):
                    node_id = node["id"]
                    height = 28 + 19 * len(lines_by_id[node_id])
                    boxes[node_id] = (40 + offset + index * 380, y, 330, height)
                y += row_height + 110
    right = max((x + w for x, y, w, h in boxes.values()), default=720)
    width = max(950, right + 80)
    bottom = max((y + h for x, y, w, h in boxes.values()), default=210)
    height = bottom + 90
    title = owner.rsplit("/", 1)[-1] + " / " + graph
    marker_prefix = view + direction
    pieces = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" role="img" aria-label="{escape(title)}">',
              '<style>text{font-family:Segoe UI,Arial,sans-serif;fill:#20334b} .label{font-size:11px;paint-order:stroke;stroke:#f8fafc;stroke-width:5px;stroke-linejoin:round}.graph-edge:hover>.edge-line{stroke-width:3.5}.graph-edge:hover{opacity:1}</style>',
              '<rect width="100%" height="100%" fill="#f8fafc"/>']
    for name, color in (("exec", "#b36b13"), ("data", "#317b9b"), ("removed", "#b42318")):
        pieces.append(f'<defs><marker id="{marker_prefix}{name}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{color}"/></marker></defs>')
    pieces.append(f'<text x="40" y="34" font-size="22" font-weight="600">{escape(title)}</text>')
    pieces.append(f'<text x="40" y="59" font-size="12">{view} · {direction} · {len(nodes)} / {len(all_nodes)} nodes · {len(edges)} / {len(all_edges)} links · source unchanged</text>')
    pieces.append('<text x="40" y="82" font-size="12">Nodes: blue entry, amber execution, grey data/other. Links: solid amber execution, dashed blue data/other.</text>')
    pieces.append('<text x="40" y="103" font-size="12">Structural view only. Timers / indirect calls are not inferred. Details button: node data. Hover arrows: pin details.</text>')
    pieces.append('<text x="40" y="124" font-size="12">Only unconnected input defaults are shown. Output defaults are not computed results; Details retains raw fields.</text>')
    pieces.append(f'<text x="40" y="145" font-size="12">Hidden: {len(hidden)} disabled / Self nodes, {hidden_links} incident links. Folded: {folded} nodes into chains; see Details / Full.</text>')
    declarations = document.get("variables", [])
    if declarations:
        declaration_text = "Variables: " + ", ".join(variable_label(v) for v in declarations)
        visible_text = textwrap.shorten(declaration_text, width=max(30, int((width - 80) / 7)), placeholder=" …")
        pieces.append(f'<text x="40" y="166" font-size="12"><title>{escape(declaration_text)}</title>{escape(visible_text)}</text>')
    if document.get('_review'):
        pieces.append('<text x="40" y="187" font-size="12">REVIEW · + green: added · ~ purple: changed · − red dashed: removed · Details: Before / After · explicit marks only</text>')
    incoming, outgoing = defaultdict(list), defaultdict(list)
    for i, edge in enumerate(edges):
        outgoing[edge[0]].append(i)
        incoming[edge[2]].append(i)
    nodes_by_id = {node['id']: node for node in nodes}
    node_kinds = {node['id']: node.get('kind') for node in nodes}
    # Data precedes execution at both ends; geometry never changes pin order.
    for indices in outgoing.values():
        indices.sort(key=lambda i: (edges[i][4], pin_order(edges[i][1]), edges[i][2], edges[i][3], i))
    for indices in incoming.values():
        indices.sort(key=lambda i: (edges[i][4], pin_order(edges[i][3]), edges[i][0], edges[i][1], i))
    for i, (source, output, target, input_pin, execution) in enumerate(edges):
        x, y, w, h = boxes[source]
        tx, ty, tw, th = boxes[target]
        transform = ''
        if direction == "TB":
            sx, sy = x + w * (outgoing[source].index(i) + 1) / (len(outgoing[source]) + 1), y + h
            ex, ey = tx + tw * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1), ty
            bend = max(35, (ey - sy) / 2)
            path = f'M{sx},{sy} C{sx},{sy+bend} {ex},{ey-bend} {ex},{ey}'
            if source == target:
                # Keep a self-loop visible beside its own node, not at a graph-wide lane.
                side = x + w + 55
                path = f'M{sx},{sy} C{side},{sy+40} {side},{ey-40} {ex},{ey}'
            lx, ly = (sx + ex) / 2, (sy + ey) / 2 - 7
        else:
            sx, sy = x + w, y + h * (outgoing[source].index(i) + 1) / (len(outgoing[source]) + 1)
            ex, ey = tx, ty + th * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1)
            bend = max(35, (ex - sx) / 2)
            path = f'M{sx},{sy} C{sx+bend},{sy} {ex-bend},{ey} {ex},{ey}'
            if source == target:
                side = y + h + 55
                path = f'M{sx},{sy} C{sx+40},{side} {ex-40},{side} {ex},{ey}'
            lx, ly = (sx + ex) / 2, (sy + ey) / 2 - 7
        label_text, anchor = output + ' → ' + ('' if execution else 'Input: ') + input_pin, 'middle'
        branch_choice = execution and node_kinds[source] == 'branch' and output in ('then', 'else')
        if branch_choice:
            # Branch choice belongs at the originating pin, not on a distant routing lane.
            label_text, transform = output, ''
            if direction == 'TB':
                lx, ly = sx, sy + 13
            else:
                lx, ly, anchor = sx + 10, sy - 7, 'start'
        style, color = ("exec", "#b36b13") if execution else ("data", "#317b9b")
        dash = '' if execution else ' stroke-dasharray="6 4"'
        removed_link = (source, output, target, input_pin, execution) in document.get('_reviewRemovedLinks', [])
        if removed_link:
            color, dash = '#b42318', ' stroke-dasharray="4 5" opacity="0.55"'
            style = 'removed'
        tooltip = source + '.' + output + ' → ' + target + '.' + input_pin
        if not execution:
            output_type = edge_output_type(nodes_by_id[source], output, document.get('pinTemplates', {}))
            typed_output = output + (f' ({output_type})' if output_type else '')
            tooltip = f'Output: {typed_output}\nInput: {input_pin}\nFrom: {source}\nTo: {target}'
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
        color = node_fill(node, document.get('pinTemplates', {}), execution_nodes)
        opacity = ' opacity="0.55"' if node.get("enabled") is False else ''
        status = node.get('_reviewStatus', '')
        stroke = {'added': '#15803d', 'changed': '#7c3aed', 'removed': '#b42318'}.get(status, '#8b9bae')
        dash = ' stroke-dasharray="7 4"' if status == 'removed' else ''
        if status == 'removed':
            opacity = ' opacity="0.6"'
        details = node.get('_reviewDetails', node)
        review_html = ''
        if status == 'changed':
            review_html = ' data-review-details="' + escape(render_details(details['Before'], details['After'])) + '"'
        ids = [n['id'] for n in node.get('_foldedNodes', [node])]
        pieces.append(f'<g class="graph-node" data-node-id="{escape(node["id"])}" data-node-ids="{escape(json.dumps(ids))}" data-review-status="{status}"{review_html}{opacity}><title>{escape(json.dumps(details, ensure_ascii=False, indent=2))}</title><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{color}" stroke="{stroke}" stroke-width="{3 if status else 1.5}"{dash}/>')
        for row, line in enumerate(lines_by_id[node["id"]]):
            header = row < title_rows[node['id']]
            size = 16 if header else 12
            weight = ' font-weight="600"' if header else ''
            fit = f' textLength="{w-70 if row == 0 else w-28}" lengthAdjust="spacingAndGlyphs"' if header and len(line) > 25 else ''
            previous = changed_lines.get(node['id'], {}).get(row)
            decoration, tooltip = '', ''
            if previous is not None:
                decoration = ' class="review-changed-line" style="fill:#7c3aed;font-weight:600"'
                tooltip = '<title>' + escape('Previously: ' + previous) + '</title>'
            pieces.append(f'<text x="{x+14}" y="{y+24+row*19}" font-size="{size}"{weight}{fit}{decoration}>{escape(line)}{tooltip}</text>')
        copy_label = 'Copy node ID' if len(ids) == 1 else f'Copy all {len(ids)} node IDs'
        pieces.append(f'<g class="copy-node-id" role="button" tabindex="0" aria-label="{copy_label}" transform="translate({x+w-27},{y+5})"><title>{copy_label}</title><rect width="22" height="22" rx="4" fill="#fff" stroke="#64748b"/><path d="M6 6h9v11H6z M9 3h9v11" fill="none" stroke="#334155" stroke-width="1.5"/></g>')
        pieces.append(f'<g class="node-info" role="button" tabindex="0" aria-label="Show node details" transform="translate({x+w-53},{y+5})"><title>Show node details</title><rect width="22" height="22" rx="4" fill="#fff" stroke="#64748b"/><text x="11" y="16" text-anchor="middle" font-size="16" font-weight="600">i</text></g>')
        pieces.append('</g>')
    pieces.append('</svg>')
    if direction == 'TB':
        layout = {'nodes': [dict(id=n['id'], rank=rank[n['id']], kind=n.get('kind'),
                                x=boxes[n['id']][0], y=boxes[n['id']][1],
                                w=boxes[n['id']][2], h=boxes[n['id']][3]) for n in nodes],
                  'edges': edges}
        pieces[0] = pieces[0].replace('<svg ', '<svg data-layout="' + escape(json.dumps(layout, separators=(',', ':'))) + '" ', 1)
    return '\n'.join(pieces), title, len(nodes), len(edges)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--output", type=Path, required=True, help="Output .svg; adjacent .html is also written")
    parser.add_argument("--graph", help="Graph name when the envelope has several graphs")
    parser.add_argument('--review', type=Path, help='Explicit added/changed/removed node ID lists')
    parser.add_argument('--before', type=Path, help='Original compact source for review Before details and removed nodes')
    parser.add_argument("--view", choices=("simplified", "full"), default="simplified")
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
        if args.before and not args.review:
            raise ValueError('--before requires --review.')
        if args.review:
            review = json.loads(args.review.read_text(encoding='utf-8-sig'))
            before = json.loads(args.before.read_text(encoding='utf-8-sig')) if args.before else None
            document = review_document(document, before, review, args.graph)
        variants = {(view, direction): render(document, args.graph, view, direction)
                    for view in ("simplified", "full") for direction in ("TB", "LR")}
        svg, title, nodes, links = variants[args.view, args.direction]
        sections = ''.join(f'<section id="{view}-{direction}"' +
                           ('' if (view, direction) == (args.view, args.direction) else ' hidden') +
                           '>' + result[0] + '</section>' for (view, direction), result in variants.items())
        page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>''' + escape(title) + '''</title>
<style>body{margin:0;background:#f8fafc;font:14px 'Segoe UI',sans-serif;height:100vh;display:grid;grid-template-rows:auto 1fr}nav{padding:10px 18px;background:#fff;border-bottom:1px solid #ddd;display:flex;gap:8px;align-items:center;flex-wrap:wrap}button,select{padding:6px 10px}main{overflow:auto;min-height:0;position:relative}svg{display:block}</style>
<style>.graph-node{cursor:pointer}.graph-node:focus-visible{outline:none}.graph-node:focus-visible>rect{stroke:#1d4ed8;stroke-width:3}.muted{opacity:.16}.graph-node.selected,.graph-node.neighbor{opacity:1}.graph-node.selected>rect{stroke:#1d4ed8;stroke-width:4}.graph-node.neighbor>rect{stroke-width:2.5}.graph-edge.related>.edge-line,.graph-edge.inspected>.edge-line{stroke-width:3.5}.graph-edge.inspected{opacity:1}.hint{color:#52657a;font-size:12px}.copy-node-id{cursor:copy}.node-info{cursor:pointer}.copy-node-id:hover>rect,.copy-node-id:focus>rect,.node-info:hover>rect,.node-info:focus>rect{fill:#dbeafe;stroke:#1d4ed8}.copy-node-id:focus,.node-info:focus{outline:none}#edge-tooltip{position:fixed;z-index:30;max-width:min(440px,calc(100vw - 24px));max-height:60vh;overflow:auto;padding:10px 12px;border:1px solid #94a3b8;border-radius:6px;background:#fff;box-shadow:0 4px 18px #20334b33;white-space:pre-wrap;overflow-wrap:anywhere;user-select:text;font:12px/1.5 Consolas,monospace}#copy-status{font-size:12px;color:#17614b}#node-details{position:absolute;z-index:20;width:min(520px,calc(100vw - 24px));max-height:70vh;overflow:auto;background:#fff;border:1px solid #94a3b8;border-radius:8px;box-shadow:0 6px 28px #20334b44;user-select:text}#node-details[hidden]{display:none}.detail-toolbar{position:sticky;top:0;display:flex;gap:6px;background:#f1f5f9;padding:8px;border-bottom:1px solid #cbd5e1}#close-details{margin-left:auto}#detail-ids{padding:10px 12px 0;overflow-wrap:anywhere;font-weight:600}#detail-text{margin:0;padding:12px;white-space:pre-wrap;overflow-wrap:anywhere;font:12px/1.5 Consolas,monospace;cursor:text}</style>
<style>.detail-row{display:block;min-height:1.5em}.detail-mark{display:inline-block;width:1.5em;user-select:none}.detail-changed{color:#7c3aed}.detail-added{color:#15803d}.detail-removed{color:#b42318;text-decoration:line-through}.detail-technical{color:#64748b}</style>
<nav><label>View <select id="view" onchange="choose()"><option value="simplified">Simplified</option><option value="full">Full</option></select></label>
<label>Direction <select id="direction" onchange="choose()"><option value="TB">Top to bottom</option><option value="LR">Left to right</option></select></label>
<button onclick="zoom(.8)" aria-label="Zoom out">−</button><button onclick="zoom(1.25)" aria-label="Zoom in">+</button><button onclick="fit()">Fit width</button><button onclick="fit(true)">Fit graph</button>
<label>Nodes per row <input id="nodes-per-row" type="number" min="1" max="12" step="1" value="3" style="width:4em;padding:6px" onchange="changeColumns()" title="Maximum nodes per row in top-to-bottom view"></label>
<span class="hint">Click node: highlight. Click arrow: details. Copy icon: node IDs. Info button: selectable details. Escape: clear.</span><span id="copy-status" role="status" aria-live="polite"></span></nav><main>''' + sections + '''
<aside id="node-details" hidden aria-label="Node details"><div class="detail-toolbar"><select id="detail-version" aria-label="Details version" hidden><option>After</option><option>Before</option></select><button id="copy-detail-ids">Copy IDs</button><button id="copy-details">Copy contents</button><button id="close-details" aria-label="Close details">×</button></div><div id="detail-ids"></div><pre id="detail-text" tabindex="0"></pre></aside></main><div id="edge-tooltip" role="tooltip" hidden></div>
<script>''' + (Path(__file__).with_name('graph_layout.js').read_text(encoding='utf-8')) + '\n' + (Path(__file__).with_name('graph_viewer.js').read_text(encoding='utf-8')) + '''
''' + f'view.value="{args.view}";direction.value="{args.direction}";' + '''choose();</script></html>'''
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
