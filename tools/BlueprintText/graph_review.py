"""Structural graph comparison shared by preview badges and node overlays."""
import copy
from collections import Counter

from graph_hierarchy import graph_key


def node_signature(document, node):
    value = copy.deepcopy(node)
    value['kind'] = document.get('classes', {}).get(node.get('kind'), node.get('kind'))
    for pin in value.get('pins', {}).values():
        if 'type' in pin:
            pin['type'] = document.get('pinTemplates', {}).get(pin['type'], pin['type'])
    # Native pin order is represented state (for example, Sequence execution).
    # Keep it in comparison signatures without changing source or Details data.
    if 'pins' in value:
        value['pins'] = list(value['pins'].items())
    return value


def graph_signature(document, graph):
    value = copy.deepcopy(graph)
    # A move changes the caller/location, not the child's own implementation.
    if 'owner' in value:
        value['owner'].pop('graph', None)
        value['owner'].pop('node', None)
    value['body']['nodes'] = sorted(
        (node_signature(document, n) for n in graph['body']['nodes']), key=lambda n: n['id'])
    value['body']['links'].sort(key=lambda e: (e['from'], e['to']))
    return value


def review_marks(before, after):
    """Compare stable node identities, resolved payloads and incident links.

    Whole added/removed graphs are handled by the preview's graph inventory.
    Node ordering and table alias spelling do not count as changes.
    """
    result = {}
    old_graphs = {graph_key(g): g for g in before.get('graphs', [])}
    for graph in after.get('graphs', []):
        old = old_graphs.get(graph_key(graph))
        if old is None:
            continue
        left, right = ({n['id']: n for n in g['body']['nodes']} for g in (old, graph))

        def incident(g, identity):
            return Counter((e['from'], e['to']) for e in g['body']['links']
                           if identity in (e['from'].rpartition('.')[0], e['to'].rpartition('.')[0]))

        marks = dict(added=sorted(right.keys() - left.keys()), removed=sorted(left.keys() - right.keys()),
                     changed=sorted(k for k in left.keys() & right.keys()
                                    if node_signature(before, left[k]) != node_signature(after, right[k])
                                    or incident(old, k) != incident(graph, k)))
        if any(marks.values()):
            result[graph_key(graph)] = marks
    return result
