"""Geometry for intact connected fragments; mirrored by graph_layout.js."""
from collections import defaultdict
import re


def pin_order(pin):
    return ({'then': 0, 'else': 1}.get(pin, 2),
            re.sub(r'\d+', lambda m: m[0].zfill(12), pin))


def port_fractions(nodes, edges):
    result = []
    for endpoint, pin in ((0, 1), (2, 3)):
        ports = {n['id']: {} for n in nodes}
        for e in edges:
            ports[e[endpoint]][e[pin]] = (bool(e[4]), *pin_order(e[pin]))
        result.append({node: {p: (i + 1) / (len(values) + 1)
                             for i, p in enumerate(sorted(values, key=values.get))}
                       for node, values in ports.items()})
    return result


def segments_cross(a, b, c, d):
    """Strict interior crossing in (flow rank, cross-axis port) coordinates."""
    turn = lambda p, q, r: (q[0]-p[0])*(r[1]-p[1]) - (q[1]-p[1])*(r[0]-p[0])
    return turn(a, b, c)*turn(a, b, d) < -1e-7 and turn(c, d, a)*turn(c, d, b) < -1e-7


def layout_boxes(nodes, edges, direction='TB', columns=8):
    """Keep primary exec chains rigid; pack components within a soft row width."""
    horizontal = direction == 'LR'
    by = {n['id']: n for n in nodes}
    incoming, outgoing, adjacent = ({n: [] for n in by} for _ in range(3))
    for i, e in enumerate(edges):
        outgoing[e[0]].append(i)
        incoming[e[2]].append(i)
        adjacent[e[0]].append(e[2])
        adjacent[e[2]].append(e[0])
    outputs, inputs = port_fractions(nodes, edges)
    cross = lambda n: by[n]['h' if horizontal else 'w']
    flow = lambda n: by[n]['w' if horizontal else 'h']
    delta = lambda e: cross(e[0]) * outputs[e[0]][e[1]] - cross(e[2]) * inputs[e[2]][e[3]]
    forward = lambda i: edges[i][4] and by[edges[i][0]]['rank'] < by[edges[i][2]]['rank']
    edge_key = lambda i: (edges[i][0], *pin_order(edges[i][1]), edges[i][2], edges[i][3])
    primary_in = {n: min(filter(forward, incoming[n]), key=edge_key, default=None) for n in by}
    primary_out = {n: min(filter(forward, outgoing[n]), key=edge_key, default=None) for n in by}
    execution_nodes = {n for n in by if any(edges[i][4] for i in incoming[n] + outgoing[n])}
    # Keep linear pure computations together; shared producers are not assigned
    # arbitrarily to one consumer. Cycles remain separate, finite placements.
    data_in = {}
    for n in by:
        sources = {edges[i][0] for i in incoming[n] if edges[i][0] not in execution_nodes}
        if n not in execution_nodes and len(sources) == 1 and {edges[i][0] for i in incoming[n]} == sources:
            source = next(iter(sources))
            if by[source]['rank'] < by[n]['rank'] and {edges[i][2] for i in outgoing[source]} == {n}:
                data_in[n] = min((i for i in incoming[n] if edges[i][0] == source), key=edge_key)
    seen, components = set(), []
    for root in sorted(by):
        if root in seen:
            continue
        pending, component = [root], []
        seen.add(root)
        while pending:
            node = pending.pop()
            component.append(node)
            for other in sorted(adjacent[node]):
                if other not in seen:
                    seen.add(other)
                    pending.append(other)
        components.append(component)
    boxes, shelf_x, shelf_y, shelf_height = {}, 40, 240, 0
    row_width = max(1, min(12, columns)) * 390 - 60
    for component in components:
        ordered = sorted(component, key=lambda n: (by[n]['rank'], n))
        chain_of, offsets, chains = {}, {}, []
        for node in ordered:
            i = primary_in[node] if node in execution_nodes else data_in.get(node)
            if i is not None and (node not in execution_nodes or primary_out[edges[i][0]] == i):
                source = edges[i][0]
                chain_of[node] = chain_of[source]
                offsets[node] = offsets[source] + delta(edges[i])
            else:
                chain_of[node], offsets[node] = len(chains), 0
                chains.append([])
            chains[chain_of[node]].append(node)
        is_exec = lambda chain: chain[0] in execution_nodes
        def chain_key(chain):
            execution = is_exec(chain)
            root = chain[0]
            i = primary_in[root]
            return (not execution, by[root]['rank'] if execution else -by[chain[-1]]['rank'],
                    edge_key(i) if i is not None else (root, -1, '', '', ''), root)
        occupied, placed = defaultdict(list), {}
        exec_edges = [edges[i] for n in component for i in outgoing[n] if edges[i][4]]
        ordered_chains = sorted(chains, key=chain_key)
        schedule = [(chain, False) for chain in ordered_chains] + [
            (chain, True) for _ in range(2) for chain in ordered_chains if not is_exec(chain)]
        for chain, refine in schedule:
            previous = placed[chain[0]] - offsets[chain[0]] if refine else None
            if refine:
                for n in chain:
                    del placed[n]
                occupied = defaultdict(list)
                for n, position in placed.items():
                    occupied[by[n]['rank']].append((position, position + cross(n)))
            incoming_anchors, outgoing_anchors = [], []
            for n in chain:
                for i in incoming[n]:
                    e = edges[i]
                    if e[0] in placed:
                        incoming_anchors.append((e[4], placed[e[0]] + delta(e) - offsets[n]))
                for i in outgoing[n]:
                    e = edges[i]
                    if e[2] in placed:
                        outgoing_anchors.append((e[4], placed[e[2]] - delta(e) - offsets[n]))
            execution = is_exec(chain)
            anchors = ((incoming_anchors or outgoing_anchors) if execution else
                       (outgoing_anchors or incoming_anchors))
            preferred = [v for execution, v in anchors if execution] or [v for _, v in anchors]
            preferred.sort()
            desired = preferred[(len(preferred) - 1) // 2] if preferred else 0
            forbidden = [(lo - offsets[n] - cross(n) - 60, hi - offsets[n] + 60)
                         for n in chain for lo, hi in occupied[by[n]['rank']]]
            candidates = [desired] + [v for interval in forbidden for v in interval]
            if refine:
                candidates.append(previous)
            if not execution:
                candidates += [v for _, v in incoming_anchors + outgoing_anchors]
            # Secondary execution branches follow the earlier output, not the
            # nearest free space on the opposite side of the primary branch.
            candidates = [v for v in candidates if not execution or v >= desired]
            valid = lambda v: all(v <= lo + 1e-7 or v >= hi - 1e-7 for lo, hi in forbidden)
            incident = {i for n in chain for i in incoming[n] + outgoing[n]
                        if edges[i][0] in placed or edges[i][2] in placed}
            def cost(value):
                if execution:
                    return (abs(value-desired), value)
                position = lambda n: placed[n] if n in placed else value + offsets[n]
                def endpoints(e):
                    return ((by[e[0]]['rank'], position(e[0]) + cross(e[0])*outputs[e[0]][e[1]]),
                            (by[e[2]]['rank'], position(e[2]) + cross(e[2])*inputs[e[2]][e[3]]))
                crossings = 0
                for i in sorted(incident):
                    e = edges[i]
                    for spine in exec_edges:
                        if e[0] in (spine[0], spine[2]) or e[2] in (spine[0], spine[2]):
                            continue
                        crossings += segments_cross(*endpoints(e), *endpoints(spine))
                distance = lambda values: sum(abs(value-v) for _, v in values)/max(1, len(values))
                # Consumers define the computation's location; sources still
                # contribute, while crossing execution carries a card-width cost.
                if refine:
                    # Revisit groups after all their neighbors exist. Each move
                    # reduces the same graph-wide distance/crossing objective.
                    score = sum(abs(value-v) for _, v in incoming_anchors + outgoing_anchors) + 390*crossings
                else:
                    score = 2*distance(outgoing_anchors) + distance(incoming_anchors) + 390*crossings
                return (round(score, 6), abs(value-(previous if refine else desired)), value)
            shift = min(filter(valid, candidates), key=cost)
            for n in chain:
                placed[n] = shift + offsets[n]
                occupied[by[n]['rank']].append((placed[n], placed[n] + cross(n)))
        main, cursor = {}, 0
        for rank in sorted({by[n]['rank'] for n in component}):
            main[rank] = cursor
            cursor += max(flow(n) for n in component if by[n]['rank'] == rank) + (170 if horizontal else 110)
        minimum = min(placed.values())
        local = {n: ((main[by[n]['rank']], placed[n] - minimum) if horizontal
                     else (placed[n] - minimum, main[by[n]['rank']])) for n in component}
        # Reserve space around same-rank/back edges, including self loops.
        margin = 80 if any(by[e[0]]['rank'] >= by[e[2]]['rank']
                           for n in component for i in outgoing[n] for e in [edges[i]]) else 0
        width = max(local[n][0] + by[n]['w'] for n in component) + 2 * margin
        height = max(local[n][1] + by[n]['h'] for n in component) + 2 * margin
        if shelf_x > 40 and shelf_x - 40 + width > row_width:
            shelf_x, shelf_y, shelf_height = 40, shelf_y + shelf_height + 110, 0
        for n, (x, y) in local.items():
            boxes[n] = (x + shelf_x + margin, y + shelf_y + margin, by[n]['w'], by[n]['h'])
        shelf_x += width + 60
        shelf_height = max(shelf_height, height)
    return boxes
