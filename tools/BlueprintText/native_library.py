"""Copy a native UObject/function-library Blueprint into a consumer project."""
from pathlib import Path

from blueprint_workspace import read, write, target


def copy_asset(workspace, source, destination):
    source = Path(source).resolve()
    contents = [p for p in source.parents if p.name == 'Content' and list(p.parent.glob('*.uproject'))]
    if len(contents) != 1 or source.suffix != '.uasset' or not source.is_file():
        raise ValueError('SourcePath must be an uncooked .uasset below a source project Content directory')
    source_asset = '/Game/' + source.relative_to(contents[0]).with_suffix('').as_posix()
    staged = target(workspace.project, source_asset)
    output = target(workspace.project, destination)
    if staged == output or staged.exists() or output.exists():
        raise ValueError('CopyAsset requires absent source staging and destination packages in the working project')
    if workspace.manifest.exists() or (workspace.root.exists() and any(p.name != '.workspace' for p in workspace.root.iterdir())):
        raise ValueError('CopyAsset requires a fresh workspace')
    # Validate the single-package source boundary, including split bulk files.
    source_project = next(contents[0].parent.glob('*.uproject'))
    target(source_project, source_asset)
    original = source.read_bytes()
    workspace.prepare(workspace.project)
    journal = workspace.internal / 'recovery.json'
    write(journal, dict(project=str(workspace.project), baseline={}, assets=[
        dict(asset=a, backup=None) for a in (source_asset, destination)]))
    try:
        staged.parent.mkdir(parents=True, exist_ok=True)
        staged.write_bytes(original)
        workspace.engine_run('CopyBlueprint', 'native-copy', Input=source_asset, Asset=destination)
        if staged.read_bytes() != original or source.read_bytes() != original:
            raise ValueError('Native copy changed the source package')
        staged.unlink()
        # Fresh export occurs with the library package physically absent.
        workspace.export(workspace.project, [destination], prepared=True)
        baseline_path = workspace.internal / 'baseline.json'
        baseline = read(baseline_path)
        baseline[destination]['before'] = None
        write(baseline_path, baseline)
        journal.unlink()
    except BaseException:
        workspace.recover()
        raise
    return dict(status='asset-copied', asset=destination, source=str(source), applied=True)
