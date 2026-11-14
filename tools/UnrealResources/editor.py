"""Unreal worker for explicit Texture2D/SoundWave authoring and preview export."""
import json
import math
import os
from pathlib import Path
import sys

import unreal

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).parent))
from request import TEXTURE_SETTINGS, SOUND_SETTINGS

ENUMS = {'compression_settings': unreal.TextureCompressionSettings,
         'lod_group': unreal.TextureGroup, 'mip_gen_settings': unreal.TextureMipGenSettings,
         'filter': unreal.TextureFilter}


def encode(value):
    if value is None or isinstance(value, (str, bool, int, float)):
        return value
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    return value.name


def describe(asset):
    texture = isinstance(asset, unreal.Texture2D)
    if not texture and not isinstance(asset, unreal.SoundWave):
        raise ValueError(f'Unsupported resource class: {asset.get_class().get_name()}')
    result = {'asset': asset.get_path_name(), 'type': asset.get_class().get_name(),
              'settings': {name: encode(asset.get_editor_property(name))
                           for name in (TEXTURE_SETTINGS if texture else SOUND_SETTINGS)}}
    if texture:
        result.update(width=asset.blueprint_get_size_x(), height=asset.blueprint_get_size_y())
        if result['width'] <= 0 or result['height'] <= 0:
            raise ValueError(f'Empty texture: {result["asset"]}')
    else:
        result.update(duration=asset.get_editor_property('duration'), channels=asset.get_editor_property('num_channels'))
        if result['channels'] <= 0 or result['duration'] <= 0:
            raise ValueError(f'Empty sound: {result["asset"]}')
    return result


def load(entry):
    asset = unreal.load_asset(entry['asset'])
    identity = entry['asset'] + '.' + entry['asset'].rsplit('/', 1)[1]
    if asset is None or asset.get_class().get_name() != entry['type'] or asset.get_path_name() != identity:
        raise ValueError(f'Missing asset or wrong type: {entry["asset"]}, expected {entry["type"]}')
    return asset


def settings_for(entry):
    result = {}
    for name, value in entry.get('settings', {}).items():
        if name in ENUMS:
            if not hasattr(ENUMS[name], value):
                raise ValueError(f'Unknown {name}: {value}')
            value = getattr(ENUMS[name], value)
        elif name == 'sound_class_object':
            value = unreal.load_asset(value)
            if not isinstance(value, unreal.SoundClass):
                raise ValueError('sound_class_object must identify a project SoundClass')
        result[name] = value
    return result


def check_input(entry, record):
    for key, expected in entry['expected'].items():
        if not math.isclose(record[key], expected, rel_tol=1e-5, abs_tol=1e-4):
            raise ValueError(f'{entry["asset"]}: imported {key}={record[key]}, input requires {expected}')


def run(request, phase):
    output = Path(request['output'])
    entries = request['resources']
    if phase == 'reopen':
        previous = json.loads((output / 'execute.json').read_text(encoding='utf-8'))['resources']
        records = [describe(load(entry)) for entry in entries]
        if records != previous:
            raise ValueError('Saved resource readback differs in a fresh editor process')
        return records

    action = request['action']
    # Validate every destination and setting before the first write.
    assets, settings = [], []
    for entry in entries:
        exists = unreal.EditorAssetLibrary.does_asset_exist(entry['asset'])
        if action == 'Import' and entry['mode'] == 'create':
            if exists:
                raise ValueError(f'Create destination exists: {entry["asset"]}')
            assets.append(None)
        else:
            assets.append(load(entry))
        settings.append(settings_for(entry))

    records = []
    for entry, asset, overrides in zip(entries, assets, settings):
        if action == 'Import':
            # Preserve consumer settings while replacing resource content at the same identity.
            names = TEXTURE_SETTINGS if entry['type'] == 'Texture2D' else SOUND_SETTINGS
            preserved = {key: asset.get_editor_property(key) for key in names} if asset else {}
            task = unreal.AssetImportTask()
            task.filename = entry['source']
            task.destination_path, task.destination_name = entry['asset'].rsplit('/', 1)
            task.automated = True
            task.replace_existing = entry['mode'] == 'replace'
            task.replace_existing_settings = False
            task.save = False
            task.factory = unreal.TextureFactory() if entry['type'] == 'Texture2D' else unreal.SoundFactory()
            if entry['type'] == 'SoundWave':
                task.factory.set_editor_property('auto_create_cue', False)
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            imported = task.get_objects()
            identity = entry['asset'] + '.' + entry['asset'].rsplit('/', 1)[1]
            if len(imported) != 1 or imported[0].get_path_name() != identity:
                raise ValueError(f'Import did not produce the exact requested resource: {entry["asset"]}')
            asset = load(entry)
            for key, value in (preserved | overrides).items():
                asset.set_editor_property(key, value)
            record = describe(asset)
            check_input(entry, record)
            if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
                raise ValueError(f'Could not save {entry["asset"]}')
        else:
            record = describe(asset)
            if action == 'Export':
                extension = '.png' if entry['type'] == 'Texture2D' else '.wav'
                path = output / 'exports' / (entry['asset'].removeprefix('/Game/') + extension)
                path.parent.mkdir(parents=True, exist_ok=True)
                task = unreal.AssetExportTask()
                task.object = asset
                task.filename = str(path)
                task.automated = True
                task.prompt = False
                task.replace_identical = False
                task.exporter = unreal.TextureExporterPNG() if entry['type'] == 'Texture2D' else unreal.SoundExporterWAV()
                if not unreal.Exporter.run_asset_export_task(task) or not path.is_file() or not path.stat().st_size:
                    raise ValueError(f'Export failed: {entry["asset"]}')
                record['export'] = str(path)
        records.append(record)
    return records


request = json.loads(Path(os.environ['UNREAL_RESOURCE_REQUEST']).read_text(encoding='utf-8'))
phase = os.environ['UNREAL_RESOURCE_PHASE']
report = Path(request['output']) / (phase + '.json')
try:
    records = run(request, phase)
    report.write_text(json.dumps({'status': 'passed', 'resources': records}, indent=2) + '\n', encoding='utf-8')
except Exception as error:
    report.write_text(json.dumps({'status': 'failed', 'error': str(error)}, indent=2) + '\n', encoding='utf-8')
    raise
