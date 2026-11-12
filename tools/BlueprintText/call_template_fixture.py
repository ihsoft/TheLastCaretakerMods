"""Isolated acceptance edits using reflected call templates, never hand-built pin metadata."""
import json
from pathlib import Path
import sys

from asset_text_codec import add_call, compact, native


OPERATIONS = {
    'Add': '/Script/Engine.Actor::AddComponentByClass',
    'Attach': '/Script/Engine.SceneComponent::K2_AttachToComponent',
    'Extent': '/Script/Engine.BoxComponent::SetBoxExtent',
    'ChildrenCall': '/Script/Engine.SceneComponent::GetChildrenComponents',
    'Transform': '/Script/Engine.KismetMathLibrary::MakeTransform',
}
SET_CLASS = '/Script/Engine.KismetSystemLibrary::SetClassPropertyByName'


def edited(document, catalog):
    graph = next(g for g in document['graphs'] if g['name'] == 'Probe')
    original = graph['body']['nodes']
    entry = next(n for n in original if document['classes'][n['kind']].endswith('.K2Node_FunctionEntry'))
    result = next(n for n in original if document['classes'][n['kind']].endswith('.K2Node_FunctionResult'))
    graph['body'] = dict(nodes=[entry, result], links=[])
    for identity, operation in OPERATIONS.items():
        document = add_call(document, catalog, operation, identity, 'Probe')
    graph = next(g for g in document['graphs'] if g['name'] == 'Probe')
    nodes = {n['id']: n for n in graph['body']['nodes']}

    def alias(identity, direction, name):
        return next(a for a, p in nodes[identity]['pins'].items()
                    if a.startswith(direction + ':') and p.get('name', a.split(':', 1)[1]) == name)

    def link(a, pin_a, b, pin_b):
        graph['body']['links'].append({'from': a + '.' + alias(a, 'out', pin_a), 'to': b + '.' + alias(b, 'in', pin_b)})

    def value(node, pin, value):
        key = alias(node, 'in', pin)
        import call_arguments
        field = 'arguments' if call_arguments.governed(nodes[node], key, document['pinTemplates']) else 'defaults'
        nodes[node].setdefault(field, {})[key] = value

    nodes['Add']['pins'][alias('Add', 'in', 'Class')]['DefaultObject'] = '"/Script/Engine.BoxComponent"'
    value('Add', 'bDeferredFinish', 'false')
    value('Attach', 'bWeldSimulatedBodies', 'false')
    value('Extent', 'bUpdateOverlaps', 'false')
    value('Extent', 'InBoxExtent', '24.000000,24.000000,24.000000')
    link(entry['id'], 'then', 'Add', 'execute')
    link('Add', 'then', 'Attach', 'execute')
    link('Attach', 'then', 'Extent', 'execute')
    link('Extent', 'then', result['id'], 'execute')
    link('Transform', 'ReturnValue', 'Add', 'RelativeTransform')
    for identity in ('Attach', 'Extent', 'ChildrenCall'):
        link(entry['id'], 'TargetBox', identity, 'self')
    link(entry['id'], 'ParentComponent', 'Attach', 'Parent')
    link('ChildrenCall', 'Children', result['id'], 'Children')
    document = add_call(document, catalog, SET_CLASS, 'SetClass', 'SetClassProbe')
    graph = next(g for g in document['graphs'] if g['name'] == 'SetClassProbe')
    nodes = {n['id']: n for n in graph['body']['nodes']}
    entry = next(n for n in nodes.values() if document['classes'][n['kind']].endswith('.K2Node_FunctionEntry'))
    result = next(n for n in nodes.values() if document['classes'][n['kind']].endswith('.K2Node_FunctionResult'))
    graph['body']['links'] = []
    link(entry['id'], 'then', 'SetClass', 'execute')
    link('SetClass', 'then', result['id'], 'execute')
    for pin in ('Object', 'PropertyName', 'Value'):
        link(entry['id'], pin, 'SetClass', pin)
    return compact(native(document))


def main():
    work, catalog_path = map(Path, sys.argv[1:3])
    manifest = json.loads((work / 'workspace.json').read_text(encoding='utf-8'))
    source = work / manifest['assets'][0]['source']
    document = edited(json.loads(source.read_text()), json.loads(catalog_path.read_text()))
    source.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
