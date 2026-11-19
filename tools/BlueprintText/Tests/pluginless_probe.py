"""Executed by Unreal's built-in Python plugin, with BPT physically absent."""
import json
from pathlib import Path
import unreal

root = Path(__file__).parent
request = json.loads((root / 'request.json').read_text())
loaded = []
for package in request['assets']:
    asset = unreal.load_asset(package)
    if not isinstance(asset, unreal.Blueprint):
        raise RuntimeError('Blueprint did not load: ' + package)
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Blueprint did not save: ' + package)
    loaded.append(package)
(root / request['result']).write_text(json.dumps(dict(status='passed', assets=loaded)))
