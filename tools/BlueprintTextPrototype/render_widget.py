"""Offline widget-tree preview from schema-2 JSON and Unreal-exported resources."""
import argparse
import base64
import json
import math
import struct
from pathlib import Path

from asset_text_codec import split_fields


def parse(value):
    """Read the native-text subset needed for layout; retain unknown values."""
    if not isinstance(value, str):
        return value
    if value.startswith('(') and value.endswith(')'):
        fields = split_fields(value[1:-1])
        if all('=' in f for f in fields):
            return {k: parse(v) for k, v in (f.split('=', 1) for f in fields)}
        return [parse(f) for f in fields]
    if value.startswith(('NSLOCTEXT(', 'INVTEXT(')):
        return parse(split_fields(value[value.index('(') + 1:-1])[-1])
    if value.startswith('"'):
        return json.loads(value)
    if value in ('True', 'False'):
        return value == 'True'
    try:
        number = float(value)
        return number if math.isfinite(number) else value
    except ValueError:
        return value


def resource_path(value):
    # Accept native Class'/Package.Object' and plain object paths.
    value = str(value)
    return value.split("'", 1)[1][:-1] if "'" in value and value.endswith("'") else value


def widget_tree(document):
    if document.get('schema') != 2 or 'widget' not in document:
        raise ValueError('Expected schema-2 source with a widget object tree')
    objects = {}
    for obj in document['widget']['objects']:
        if obj['id'] in objects:
            raise ValueError(f"Duplicate widget object: {obj['id']}")
        objects[obj['id']] = obj
    asset = document['asset']
    prefix = asset + '.'

    def ref(value):
        path = resource_path(parse(value))
        key = path[len(prefix):] if path.startswith(prefix) else path
        if key not in objects:
            raise ValueError(f'Unresolved widget reference: {value}')
        return key

    widgets, seen = [], set()
    root = ref(objects[document['widget']['tree']]['properties']['RootWidget'])
    pending = [(root, {}, None)]
    while pending:
        key, slot, parent = pending.pop()
        if key in seen:
            raise ValueError(f'Cycle or multiple parents in widget tree: {key}')
        seen.add(key)
        obj = objects[key]
        props = {k: parse(v) for k, v in obj['properties'].items()}
        item = dict(id=key, name=obj['name'], kind=obj['class'].split('.')[-1],
                    props=props, raw=obj['properties'], slot=slot, parent=parent, children=[])
        widgets.append(item)
        slots = obj['properties'].get('Slots', '()')
        if not slots.startswith('(') or not slots.endswith(')'):
            raise ValueError(f'Malformed Slots: {key}')
        children = []
        for value in split_fields(slots[1:-1]):
            slot_obj = objects[ref(value)]
            content = slot_obj['properties'].get('Content', 'None')
            if content == 'None':
                continue
            child = ref(content)
            item['children'].append(child)
            children.append((child, {k: parse(v) for k, v in slot_obj['properties'].items()}, key))
        pending.extend(reversed(children))
    return root, widgets


def load_resources(path, asset):
    manifest = json.loads(path.read_text(encoding='utf-8-sig'))
    if manifest.get('schema') != 1 or manifest.get('asset') != asset:
        raise ValueError('Preview resource manifest schema/asset does not match the widget source')
    textures = {}
    folder = path.parent.resolve()
    for resource, item in manifest['textures'].items():
        file = (folder / item['file']).resolve()
        if not file.is_relative_to(folder):
            raise ValueError(f'Resource file escapes bundle: {item["file"]}')
        raw = file.read_bytes()
        if len(raw) < 24 or raw[:8] != b'\x89PNG\r\n\x1a\n':
            raise ValueError(f'Expected exported PNG: {file}')
        width, height = struct.unpack('>II', raw[16:24])
        if (width, height) != (item['width'], item['height']) or not width or not height:
            raise ValueError(f'PNG dimensions differ from resource manifest: {file}')
        textures[resource] = dict(src='data:image/png;base64,' + base64.b64encode(raw).decode('ascii'),
                                  width=width, height=height, source=resource)
    return textures, manifest.get('unsupported', [])


def render(source, output, resources=None):
    source, output = Path(source), Path(output)
    if source.resolve() == output.resolve():
        raise ValueError('Output must not overwrite widget source')
    document = json.loads(source.read_text(encoding='utf-8-sig'))
    root, widgets = widget_tree(document)
    manifest = Path(resources) if resources else source.with_suffix('.resources') / 'manifest.json'
    textures, unsupported = load_resources(manifest, document['asset'])
    # Retain the original resource strings for inspection; the viewer lookup uses them.
    lookup = {}
    warnings = [f"{item['resource']}: {item['reason']}" for item in unsupported]
    supported = {'InvalidationBox', 'CanvasPanel', 'HorizontalBox', 'VerticalBox',
                 'ScaleBox', 'TextBlock', 'Image', 'RadialSlider'}
    for widget in widgets:
        if widget['kind'] not in supported:
            warnings.append(f"{widget['name']}: {widget['kind']} is a placeholder; custom content is not expanded")
        brush = widget['props'].get('Brush', {})
        resource = brush.get('ResourceObject')
        if resource and resource != 'None':
            path = resource_path(resource)
            if path in textures:
                lookup[resource] = textures[path]
            else:
                warnings.append(f"{widget['name']}: no preview image for {path}")
    data = dict(asset=document['asset'], source=str(source), root=root, widgets=widgets,
                textures=lookup, warnings=sorted(set(warnings)))
    template = Path(__file__).with_name('widget_viewer.html').read_text(encoding='utf-8')
    html = template.replace('__DATA__', json.dumps(data, ensure_ascii=False, allow_nan=False).replace('<', '\\u003c'))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(html, encoding='utf-8')
    return dict(html=str(output), widgets=len(widgets), textures=len(lookup), warnings=data['warnings'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--resources', type=Path, help='Manifest; default: <source stem>.resources/manifest.json')
    args = parser.parse_args()
    try:
        print(json.dumps(render(args.source, args.output, args.resources)))
    except (ValueError, KeyError, TypeError, OSError) as exc:
        parser.exit(2, f'Widget preview failed: {exc}\n')


if __name__ == '__main__':
    main()
