"""Export native assets from two Git revisions and render the ordinary BPT review."""
import argparse
import io
import json
import stat
import subprocess
import zipfile
from pathlib import Path

from blueprint_workspace import HERE, OWNER, Workspace, inside, project_operation, read, target, write

REPO = HERE.parents[1]


def blueprint_records(assets, exports):
    from asset_text_codec import compare
    records = []
    for asset in assets:
        old, new = exports['before'].get(asset), exports['after'].get(asset)
        if old and new:
            if compare(read(old), read(new)):
                records.append(dict(before=str(old), source=str(new), status='changed'))
            else:
                records.append(dict(source=str(new), status='unchanged'))
        elif new:
            records.append(dict(source=str(new), status='added'))
        else:
            records.append(dict(before=str(old), status='removed'))
    return records


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args])


def revision(value):
    return git('rev-parse', '--verify', '--end-of-options', value + '^{commit}').decode().strip()


def extract(commit, relative, destination):
    relative = Path(relative).as_posix()
    if relative.startswith('/') or '..' in Path(relative).parts:
        raise ValueError('Git source must stay inside the repository')
    blob = git('archive', '--format=zip', commit, '--', relative)
    with zipfile.ZipFile(io.BytesIO(blob)) as archive:
        for member in archive.infolist():
            if stat.S_ISLNK(member.external_attr >> 16):
                raise ValueError('Linked Git project inputs are unsupported: ' + member.filename)
            if member.is_dir():
                continue
            path = inside(destination, member.filename)
            content = archive.read(member)
            if path.exists() and path.read_bytes() != content:
                raise ValueError('Git snapshot path collision: ' + str(path))
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)


def snapshot(commit, project_relative, root):
    """Materialize project and its declared native source/overlay dependencies."""
    project_relative = Path(project_relative)
    if project_relative.suffix != '.uproject':
        raise ValueError('ProjectFile must identify a tracked .uproject')
    extract(commit, project_relative.parent, root)
    project = inside(root, project_relative)
    declaration = project.parent / 'EditorSupport.json'
    if declaration.exists():
        data = read(declaration)
        if data.get('schema') != 1:
            raise ValueError('Unsupported project dependency schema')
        for entry in data['plugins']:
            original = (REPO / project_relative.parent / entry['source']).resolve()
            relative = original.relative_to(REPO)
            extract(commit, relative.parent, root)
            entry['source'] = str(inside(root, relative))
            for overlay in entry.get('overlays', []):
                overlay_relative = (original.parent / overlay['source']).resolve().relative_to(REPO)
                extract(commit, overlay_relative, root)
                overlay['source'] = str(inside(root, overlay_relative))
        write(declaration, data)
    return project


def review(root, project_relative, before, after, assets, kind, engine):
    root = inside(OWNER, Path(root).resolve())
    if root.exists():
        raise ValueError('Git review requires a new output directory')
    project_relative = Path(project_relative)
    if project_relative.is_absolute():
        project_relative = project_relative.resolve().relative_to(REPO)
    commits = dict(before=revision(before), after=revision(after))
    root.mkdir(parents=True)
    exports = {}
    for side, commit in commits.items():
        project = snapshot(commit, project_relative, root / 'snapshots' / side)
        selected = [a for a in assets if target(project, a).exists()]
        exports[side] = {}
        if not selected:
            continue
        if kind == 'DataAsset':
            from data_asset_workspace import DataAssetWorkspace
            workspace = DataAssetWorkspace(root / side, engine)
        else:
            workspace = Workspace(root / side, engine)
        with project_operation(project):
            workspace.export(project, selected)
        for item in read(workspace.manifest)['assets']:
            path = inside(workspace.root, item['source'])
            exports[side][read(path)['asset']] = path
    if any(a not in exports['before'] and a not in exports['after'] for a in assets):
        raise ValueError('A selected asset is absent from both revisions')
    label = f'{before} ({commits["before"][:12]}) → {after} ({commits["after"][:12]})'
    if kind == 'Blueprint':
        from render_preview_index import generate
        records = blueprint_records(assets, exports)
        manifest = root / 'preview.json'
        write(manifest, dict(title='Git asset review', description=label, assets=records))
        generate(manifest, root / 'preview')
    else:
        from data_asset_preview import render
        # Structural/property inventory changes retain the existing renderer's
        # explicit unsupported diagnostic; never label them unchanged.
        if set(exports['before']) != set(exports['after']):
            raise ValueError('DataAsset Git comparison currently requires the asset in both revisions')
        items = [dict(document=read(exports['after'][a]), before=read(exports['before'][a]), pending=False) for a in assets]
        page, _ = render(items, label)
        (root / 'preview').mkdir()
        (root / 'preview/index.html').write_text(page, encoding='utf-8')
    result = dict(status='previewed', revisions=commits, project=project_relative.as_posix(), kind=kind,
                  assets=assets, engine=str(engine), previewUrl=(root / 'preview/index.html').as_uri())
    write(root / 'result.json', result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--project', required=True)
    parser.add_argument('--before', required=True)
    parser.add_argument('--after', default='HEAD')
    parser.add_argument('--asset', action='append', required=True)
    parser.add_argument('--kind', choices=('Blueprint', 'DataAsset'), default='Blueprint')
    parser.add_argument('--engine', default='P:/UnrealEngine/UE_5.8')
    args = parser.parse_args()
    if len(set(args.asset)) != len(args.asset):
        raise ValueError('Select unique asset packages')
    print(json.dumps(review(args.output, args.project, args.before, args.after, args.asset, args.kind, args.engine)))


if __name__ == '__main__':
    main()
