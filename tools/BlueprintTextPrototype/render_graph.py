"""Render compact Blueprint JSON to offline SVG + HTML; Python standard library only.

Usage: python render_graph.py blueprint.json --output diagram.svg [--view full] [--direction LR]
Accepts the shared prototype envelope or the earlier standalone nodes/links JSON.
This is a structural view, not an Unreal compiler or a behavior simulator.
"""

import argparse
import html
import json
import re
import textwrap
from collections import defaultdict
from pathlib import Path


def escape(value):
    return html.escape(str(value), quote=True)


def load_graph(document, selection=None):
    if document.get("schema") != 1:
        raise ValueError("Expected compact schema 1.")
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
    edges = []
    for link in body["links"]:
        endpoints = []
        for key in ("from", "to"):
            node_id, separator, pin = link[key].rpartition(".")
            if not separator or node_id not in ids or not pin:
                raise ValueError(f"Unresolved {key} endpoint: {link[key]!r}")
            endpoints.append((node_id, pin))
        (source, output), (target, input_pin) = endpoints
        output, input_pin = output.removeprefix("out:"), input_pin.removeprefix("in:")
        # Known execution aliases only. Everything else is explicitly data/other.
        execution = input_pin in ("exec", "execute") or bool(re.fullmatch(r"then(?:_?\d+)?|else", output))
        edges.append((source, output, target, input_pin, execution))
    owner = document.get("asset", document.get("owner", "Blueprint"))
    return sorted(nodes, key=lambda n: n["id"]), sorted(edges), str(owner), str(name)


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


def node_lines(node, connected_inputs, hidden_inputs=()):
    kind = node.get("kind", "unknown")
    reference = node.get("operation", node.get("event", node.get("ref", kind)))
    short = reference.rsplit("::", 1)[-1].rsplit(".", 1)[-1]
    variable = node.get("variable", short)
    if kind in ("setVariable", "variableSet"):
        title = "Set " + variable
    elif kind in ("getVariable", "variableGet"):
        title = "Get " + variable
    elif kind in ("event", "customEvent"):
        title = "Event " + short.removeprefix("Receive")
    elif kind == "sequence":
        title = f"Sequence · {node.get('outputs', 2)} outputs"
    elif kind == "branch":
        title = "Branch"
    else:
        title = short
    lines = [title, node["id"]]
    lines.extend(hidden_inputs)
    values = node.get("arguments", node.get("defaults", {}))
    for key, value in sorted(values.items()):
        if key.removeprefix("in:") in connected_inputs:
            continue
        lines.append(f"{key.removeprefix('in:')} = {json.dumps(value, ensure_ascii=False)}")
    if node.get("enabled") is False:
        lines.append("DISABLED")
    if node.get("developmentOnly"):
        lines.append("DEVELOPMENT ONLY")
    if node.get("comment"):
        lines.append("Comment: " + node["comment"])
    return [part for row, line in enumerate(lines)
            for part in (textwrap.wrap(line, 29 if row == 0 else 42) or [""])]


def render(document, selection=None, view="simplified", direction="TB"):
    if view not in ("simplified", "full") or direction not in ("TB", "LR"):
        raise ValueError("Expected view simplified/full and direction TB/LR.")
    all_nodes, all_edges, owner, graph = load_graph(document, selection)
    hidden = {}
    if view == "simplified":
        for node in all_nodes:
            if node.get("enabled") is False:
                hidden[node["id"]] = "disabled " + node["id"]
            elif node.get("kind") == "self" or node.get("ref") == "/Script/BlueprintGraph.K2Node_Self":
                hidden[node["id"]] = "Self"
    nodes = [node for node in all_nodes if node["id"] not in hidden]
    edges = [edge for edge in all_edges if edge[0] not in hidden and edge[2] not in hidden]
    rank = layers(nodes, edges)
    groups = defaultdict(list)
    for node in nodes:
        groups[rank[node["id"]]].append(node)
    boxes, lines_by_id = {}, {}
    connected, hidden_inputs = defaultdict(set), defaultdict(list)
    for source, _, target, pin, _ in all_edges:
        connected[target].add(pin)
        if source in hidden and target not in hidden:
            hidden_inputs[target].append(f"{pin} ← {hidden[source]} (hidden)")
    for node in nodes:
        node_id = node["id"]
        lines_by_id[node_id] = node_lines(node, connected[node_id], hidden_inputs[node_id])
    rows, row_bottom = {}, {}
    y, row = 210, 0
    max_columns = min(3, max((len(m) for m in groups.values()), default=1))
    for column, members in sorted(groups.items()):
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
                    rows[node_id] = row
                row_bottom[row] = y + row_height
                y += row_height + 110
                row += 1
    right = max((x + w for x, y, w, h in boxes.values()), default=720)
    width = max(950, right + (320 if direction == "TB" else 80))
    bottom = max((y + h for x, y, w, h in boxes.values()), default=210)
    routed_count = sum(rank[target] != rank[source] + 1 for source, _, target, _, _ in edges)
    height = bottom + 90 + (routed_count * 25 if direction == "LR" else 0)
    title = owner.rsplit("/", 1)[-1] + " / " + graph
    marker_prefix = view + direction
    pieces = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" role="img" aria-label="{escape(title)}">',
              '<style>text{font-family:Segoe UI,Arial,sans-serif;fill:#20334b} .label{font-size:11px;paint-order:stroke;stroke:#f8fafc;stroke-width:5px;stroke-linejoin:round}</style>',
              '<rect width="100%" height="100%" fill="#f8fafc"/>']
    for name, color in (("exec", "#b36b13"), ("data", "#317b9b")):
        pieces.append(f'<defs><marker id="{marker_prefix}{name}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{color}"/></marker></defs>')
    pieces.append(f'<text x="40" y="34" font-size="22" font-weight="600">{escape(title)}</text>')
    pieces.append(f'<text x="40" y="59" font-size="12">{view} · {direction} · {len(nodes)} / {len(all_nodes)} nodes · {len(edges)} / {len(all_edges)} links · source unchanged</text>')
    pieces.append('<text x="40" y="82" font-size="12">Solid amber: execution (standard pin names). Dashed blue: data / other links.</text>')
    pieces.append('<text x="40" y="103" font-size="12">Structural view only. Timers / indirect calls are not inferred. Hover for exact details.</text>')
    pieces.append('<text x="40" y="124" font-size="12">Defaults of connected inputs are omitted; hover shows the original node fields.</text>')
    pieces.append(f'<text x="40" y="145" font-size="12">Hidden: {len(hidden)} disabled / Self nodes, {len(all_edges) - len(edges)} incident links. No bypass links are invented.</text>')
    declarations = document.get("variables", [])
    if declarations:
        pieces.append(f'<text x="40" y="166" font-size="12">{escape("Variables: " + ", ".join(v["name"] + ": " + v["type"] for v in declarations))}</text>')
    incoming, outgoing = defaultdict(list), defaultdict(list)
    for i, edge in enumerate(edges):
        outgoing[edge[0]].append(i)
        incoming[edge[2]].append(i)
    # Match endpoint order to the visible neighbors (e.g. both branch arms).
    for indices in outgoing.values():
        indices.sort(key=lambda i: (boxes[edges[i][2]][0 if direction == "TB" else 1], i))
    for indices in incoming.values():
        indices.sort(key=lambda i: (boxes[edges[i][0]][0 if direction == "TB" else 1], i))
    routed_index = 0
    for i, (source, output, target, input_pin, execution) in enumerate(edges):
        x, y, w, h = boxes[source]
        tx, ty, tw, th = boxes[target]
        transform = ''
        if direction == "TB":
            sx, sy = x + w * (outgoing[source].index(i) + 1) / (len(outgoing[source]) + 1), y + h
            ex, ey = tx + tw * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1), ty
            start_y = row_bottom[rows[source]] + 16 + (i % 5) * 8
            if rows[target] == rows[source] + 1:
                mid = (start_y + ey) / 2
                path = f'M{sx},{sy} L{sx},{start_y} C{sx},{mid} {ex},{mid} {ex},{ey}'
                lx, ly = (sx + ex) / 2, mid - 7
            else:
                lane = right + 30 + (routed_index % 8) * 25
                routed_index += 1
                end_y = ey - 20 - (i % 5) * 8
                path = f'M{sx},{sy} L{sx},{start_y} L{lane},{start_y} L{lane},{end_y} L{ex},{end_y} L{ex},{ey}'
                lx, ly = lane + 6, (start_y + ey) / 2
                transform = f' transform="rotate(90 {lx} {ly})"'
        elif rank[target] == rank[source] + 1:
            sx, sy = x + w, y + h * (outgoing[source].index(i) + 1) / (len(outgoing[source]) + 1)
            ex, ey = tx, ty + th * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1)
            mid = (sx + ex) / 2
            path = f'M{sx},{sy} C{mid},{sy} {mid},{ey} {ex},{ey}'
            lx, ly = mid, (sy + ey) / 2 - 7
        else:
            sx, sy = x + w, y + h * (outgoing[source].index(i) + 1) / (len(outgoing[source]) + 1)
            ex, ey = tx, ty + th * (incoming[target].index(i) + 1) / (len(incoming[target]) + 1)
            # Route skipped layers and cycles below the graph, never behind a node.
            lane = bottom + 30 + routed_index * 25
            routed_index += 1
            path = f'M{sx},{sy} L{sx+20},{sy} L{sx+20},{lane} L{ex-20},{lane} L{ex-20},{ey} L{ex},{ey}'
            lx, ly = (sx + ex) / 2, lane - 5
        style, color = ("exec", "#b36b13") if execution else ("data", "#317b9b")
        dash = '' if execution else ' stroke-dasharray="6 4"'
        pieces.append(f'<g class="graph-edge" data-source="{escape(source)}" data-target="{escape(target)}"><title>{escape(source+"."+output+" → "+target+"."+input_pin)}</title><path d="{path}" fill="none" stroke="{color}" stroke-width="2"{dash} marker-end="url(#{marker_prefix}{style})"/>')
        pieces.append(f'<text class="label" x="{lx}" y="{ly}" text-anchor="middle"{transform}>{escape(output+" → "+input_pin)}</text></g>')
    for node in nodes:
        x, y, w, h = boxes[node["id"]]
        kind = node.get("kind", "unknown")
        color = "#dce9fc" if kind in ("event", "customEvent") else "#f7e9d7" if kind in ("setVariable", "variableSet", "branch") else "#e0efea" if kind == "sequence" else "#e8edf4"
        opacity = ' opacity="0.55"' if node.get("enabled") is False else ''
        pieces.append(f'<g class="graph-node" data-node-id="{escape(node["id"])}"{opacity}><title>{escape(json.dumps(node, ensure_ascii=False, indent=2))}</title><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{color}" stroke="#8b9bae" stroke-width="1.5"/>')
        for row, line in enumerate(lines_by_id[node["id"]]):
            size = 16 if row == 0 else 12
            weight = ' font-weight="600"' if row == 0 else ''
            pieces.append(f'<text x="{x+14}" y="{y+24+row*19}" font-size="{size}"{weight}>{escape(line)}</text>')
        pieces.append('</g>')
    pieces.append('</svg>')
    return '\n'.join(pieces), title, len(nodes), len(edges)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--output", type=Path, required=True, help="Output .svg; adjacent .html is also written")
    parser.add_argument("--graph", help="Graph name when the envelope has several graphs")
    parser.add_argument("--view", choices=("simplified", "full"), default="simplified")
    parser.add_argument("--direction", choices=("TB", "LR"), default="TB", help="Top-to-bottom (default) or left-to-right")
    args = parser.parse_args()
    try:
        if args.output.suffix.lower() != ".svg":
            raise ValueError("--output must end in .svg")
        html_path = args.output.with_suffix(".html")
        full_path = args.output.with_name(args.output.stem + ".full.svg")
        if args.input.resolve() in (args.output.resolve(), html_path.resolve(), full_path.resolve()):
            raise ValueError("Output must not overwrite the input.")
        document = json.loads(args.input.read_text(encoding="utf-8-sig"))
        variants = {(view, direction): render(document, args.graph, view, direction)
                    for view in ("simplified", "full") for direction in ("TB", "LR")}
        svg, title, nodes, links = variants[args.view, args.direction]
        sections = ''.join(f'<section id="{view}-{direction}"' +
                           ('' if (view, direction) == (args.view, args.direction) else ' hidden') +
                           '>' + result[0] + '</section>' for (view, direction), result in variants.items())
        page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>''' + escape(title) + '''</title>
<style>body{margin:0;background:#f8fafc;font:14px 'Segoe UI',sans-serif;height:100vh;display:grid;grid-template-rows:auto 1fr}nav{padding:10px 18px;background:#fff;border-bottom:1px solid #ddd;display:flex;gap:8px;align-items:center;flex-wrap:wrap}button,select{padding:6px 10px}main{overflow:auto;min-height:0}svg{display:block}</style>
<style>.graph-node{cursor:pointer}.graph-node:focus-visible{outline:none}.graph-node:focus-visible>rect{stroke:#1d4ed8;stroke-width:3}.muted{opacity:.16}.graph-node.selected,.graph-node.neighbor{opacity:1}.graph-node.selected>rect{stroke:#1d4ed8;stroke-width:4}.graph-node.neighbor>rect{stroke-width:2.5}.graph-edge.related>path{stroke-width:3.5}.hint{color:#52657a;font-size:12px}</style>
<nav><label>View <select id="view" onchange="choose()"><option value="simplified">Simplified</option><option value="full">Full</option></select></label>
<label>Direction <select id="direction" onchange="choose()"><option value="TB">Top to bottom</option><option value="LR">Left to right</option></select></label>
<button onclick="zoom(.8)" aria-label="Zoom out">−</button><button onclick="zoom(1.25)" aria-label="Zoom in">+</button><button onclick="fit()">Fit width</button><button onclick="fit(true)">Fit graph</button>
<span class="hint">Click a node to highlight its connections. Click again, click the background, or press Escape to clear.</span></nav><main>''' + sections + '''</main>
<script>const box=document.querySelector('main'),view=document.getElementById('view'),direction=document.getElementById('direction');let svg,scale=1,selectedId=null;
function show(){const v=svg.viewBox.baseVal;svg.style.width=v.width*scale+'px';svg.style.height=v.height*scale+'px'}
function zoom(f){scale=Math.max(.05,Math.min(4,scale*f));show()}
function fit(all=false){const v=svg.viewBox.baseVal;scale=Math.min(1,Math.max(1,box.clientWidth-20)/v.width,all?box.clientHeight/v.height:1);show()}
function highlight(id){
    selectedId=id;
    const neighbors=new Set(id===null?[]:[id]);
    svg.querySelectorAll('.graph-edge').forEach(edge=>{
        const related=id!==null&&(edge.dataset.source===id||edge.dataset.target===id);
        edge.classList.toggle('related',related);edge.classList.toggle('muted',id!==null&&!related);
        if(related){neighbors.add(edge.dataset.source);neighbors.add(edge.dataset.target)}
    });
    svg.querySelectorAll('.graph-node').forEach(node=>{
        const selected=node.dataset.nodeId===id,neighbor=!selected&&neighbors.has(node.dataset.nodeId);
        node.classList.toggle('selected',selected);node.classList.toggle('neighbor',neighbor);
        node.classList.toggle('muted',id!==null&&!neighbors.has(node.dataset.nodeId));
        node.setAttribute('aria-pressed',String(selected));
    });
}
function toggleNode(node){highlight(selectedId===node.dataset.nodeId?null:node.dataset.nodeId)}
box.addEventListener('click',event=>{
    const node=event.target.closest('.graph-node');
    if(node&&svg.contains(node)){toggleNode(node)}else{highlight(null)}
});
document.addEventListener('keydown',event=>{
    if(event.key==='Escape'){highlight(null);return}
    const node=event.target.closest('.graph-node');
    if(node&&svg.contains(node)&&(event.key==='Enter'||event.key===' ')){event.preventDefault();toggleNode(node)}
});
document.querySelectorAll('.graph-node').forEach(node=>{
    node.setAttribute('tabindex','0');node.setAttribute('role','button');
    node.setAttribute('aria-label','Highlight connections for '+node.dataset.nodeId);
});
function choose(){const id=view.value+'-'+direction.value;document.querySelectorAll('section').forEach(s=>s.hidden=s.id!==id);svg=document.getElementById(id).querySelector('svg');highlight(null);box.scrollTop=0;box.scrollLeft=0;fit()}
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
