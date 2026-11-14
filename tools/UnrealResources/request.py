"""Validate bounded resource requests without loading Unreal or changing assets."""
import argparse
import json
from pathlib import Path
import re
import struct
import wave

TEXTURE_SETTINGS = {
    'srgb': bool, 'never_stream': bool, 'compression_settings': str,
    'lod_group': str, 'mip_gen_settings': str, 'filter': str,
}
SOUND_SETTINGS = {'volume': float, 'pitch': float, 'looping': bool, 'sound_class_object': str}


def fields(value, allowed, required):
    if not isinstance(value, dict) or set(value) - set(allowed) or set(required) - set(value):
        raise ValueError(f'Expected fields {sorted(allowed)}, required {sorted(required)}; got {value!r}')


def below_tmp(path):
    root = Path(__file__).resolve().parents[2] / 'Tmp'
    absolute = Path(path).absolute()
    resolved = absolute.resolve()
    if not resolved.is_relative_to(root.resolve()) or resolved == root.resolve():
        raise ValueError(f'Working paths must be below repository Tmp: {path}')
    current = absolute
    while current != current.parent:
        if current.is_symlink() or current.is_junction():
            raise ValueError(f'Linked working paths are not supported: {current}')
        current = current.parent
    return resolved


def source_info(path, resource_type):
    if resource_type == 'Texture2D':
        with path.open('rb') as stream:
            header = stream.read(24)
        if len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n' or header[12:16] != b'IHDR':
            raise ValueError(f'Expected a PNG image: {path}')
        width, height = struct.unpack('>II', header[16:24])
        if not width or not height:
            raise ValueError('PNG dimensions must be positive')
        return {'width': width, 'height': height}
    with wave.open(str(path), 'rb') as stream:
        if stream.getcomptype() != 'NONE' or not stream.getnframes() or not stream.getframerate():
            raise ValueError('Expected nonempty PCM WAV audio')
        return {'channels': stream.getnchannels(), 'duration': stream.getnframes() / stream.getframerate()}


def prepare(action, project, request, output):
    project = below_tmp(project)
    output = below_tmp(output)
    if project.suffix.lower() != '.uproject' or not project.is_file():
        raise ValueError('ProjectFile must name an existing disposable .uproject')
    if output.exists() or output.is_relative_to(project.parent) or project.is_relative_to(output):
        raise ValueError('Output must be fresh and separate from the working project')
    request = Path(request).resolve()
    data = json.loads(request.read_text(encoding='utf-8-sig'))
    fields(data, {'schemaVersion', 'resources'}, {'schemaVersion', 'resources'})
    if data['schemaVersion'] != 1 or not isinstance(data['resources'], list) or not data['resources']:
        raise ValueError('Expected schemaVersion 1 and a nonempty resources array')
    seen = set()
    resources = []
    for entry in data['resources']:
        allowed = {'asset', 'type'} | ({'source', 'mode', 'settings'} if action == 'Import' else set())
        required = allowed - {'settings'}
        fields(entry, allowed, required)
        asset = entry['asset']
        if not isinstance(asset, str) or not re.fullmatch(r'/Game/(?:[A-Za-z_][A-Za-z0-9_]*/)*[A-Za-z_][A-Za-z0-9_]*', asset):
            raise ValueError(f'Expected an exact /Game/ package path: {asset!r}')
        if asset.casefold() in seen:
            raise ValueError(f'Duplicate resource destination: {asset}')
        seen.add(asset.casefold())
        if entry['type'] not in ('Texture2D', 'SoundWave'):
            raise ValueError('Supported resource types: Texture2D, SoundWave')
        result = dict(entry)
        if action == 'Import':
            if entry['mode'] not in ('create', 'replace'):
                raise ValueError('Import mode must explicitly be create or replace')
            source = (request.parent / entry['source']).resolve()
            extension = '.png' if entry['type'] == 'Texture2D' else '.wav'
            if not source.is_file() or source.suffix.lower() != extension:
                raise ValueError(f'Expected an existing {extension} source: {source}')
            result['source'] = str(source)
            result['expected'] = source_info(source, entry['type'])
            settings = entry.get('settings', {})
            types = TEXTURE_SETTINGS if entry['type'] == 'Texture2D' else SOUND_SETTINGS
            fields(settings, types, set())
            for name, value in settings.items():
                expected_type = types[name]
                valid = type(value) is expected_type
                if expected_type is float:
                    import math
                    valid = type(value) in (int, float) and math.isfinite(value) and value >= 0
                    if name == 'pitch':
                        valid = valid and value > 0
                if not valid:
                    raise ValueError(f'Invalid setting {name}: {value!r}')
            result['settings'] = settings
        resources.append(result)
    return {'action': action, 'project': str(project), 'output': str(output), 'resources': resources}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--action', required=True, choices=['Inspect', 'Export', 'Import'])
    parser.add_argument('--project', required=True)
    parser.add_argument('--request', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    prepared = prepare(args.action, args.project, args.request, args.output)
    destination = Path(prepared['output'])
    destination.mkdir(parents=True)
    (destination / 'request.json').write_text(json.dumps(prepared, indent=2) + '\n', encoding='utf-8')
