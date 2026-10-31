"""Declarative project dependencies and source-aware reuse of ready plugins.

Engine callers hold the shared queue. The generic binary tool owns installation
and recovery; this layer only composes owner inputs and selects build or reuse.
"""
import base64
from pathlib import Path

from . import editor_support as support


def source_tree(source, overlays=()):
    source = support.unlinked(source).resolve()
    support.descriptor(source)
    ignored = {'Binaries', 'Intermediate', 'Saved', '.git'}
    files = {name: value for name, value in support.tree(source.parent).items()
             if not any(part in ignored for part in Path(name).parts)}
    support.require(not any(Path(name).parts[0] == 'Content' for name in files), 'Support source cannot contain assets')
    names = {name.casefold() for name in files}
    support.require(len(names) == len(files), 'Case-insensitive source ownership collision')
    for overlay in overlays:
        support.require(set(overlay) == {'source', 'destination'}, 'Overlay needs source and destination only')
        relative = Path(overlay['destination'])
        support.require(not relative.is_absolute() and relative.parts and relative.parts[0] == 'Source'
                        and '..' not in relative.parts, 'Overlay destination must be below Source')
        origin = support.unlinked(source.parent / overlay['source']).resolve()
        support.require(origin.is_dir(), 'Overlay source directory is missing: ' + str(origin))
        for name, value in support.tree(origin).items():
            target = (relative / name).as_posix()
            support.require(target.casefold() not in names, 'Source ownership collision: ' + target)
            names.add(target.casefold())
            files[target] = value
    return source, files


def prepare_provider(project, engine, output, source, overlays=(), *, builder=None, loader=None):
    builder = builder or support.build_package
    loader = loader or support.probe
    project, engine, output = map(support.unlinked, (project, engine, output))
    support.require(not output.exists(), 'Provider output already exists')
    support.require(output.is_relative_to(support.ROOT / 'Tmp'), 'Provider output must be below repository Tmp')
    source, files = source_tree(source, overlays)
    name = source.stem
    support.require(not support.recovery_path(project).exists(), 'Interrupted preparation requires -Recover')
    identity = support.engine_identity(engine)
    snapshot = dict(schema=1, engineBuildId=identity,
                    files={key: base64.b64encode(value).decode('ascii') for key, value in files.items()})
    cache = support.unlinked(project.parent / 'Saved/UnrealEditorSupport/inputs' / (name + '.json'))
    installed = project.parent / 'Plugins' / name
    output.mkdir(parents=True)
    reused = cache.is_file() and support.read(cache) == snapshot and installed.is_dir()
    if reused:
        package = installed
    else:
        composed = output / 'provider'
        for relative, value in files.items():
            path = composed / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(value)
        package = builder(composed / source.name, engine, output)
    status = support.attach(project, package, engine, output, loader)
    # Never mark changed sources current after a failed build, attach or load.
    temporary = cache.with_suffix('.writing')
    support.write(temporary, snapshot)
    temporary.replace(cache)
    if not reused:
        support.remove_owned(output / 'provider', output)
    result = dict(status=status, plugin=name, package=str(package), built=not reused,
                  project=str(project), engineBuildId=identity,
                  loadedModules=support.read(package / support.MANIFEST)['modules'], evidence=str(output))
    support.write(output / 'result.json', result)
    return result


def prepare_project(project, engine, output):
    """An absent declaration means no owner-supplied native dependencies."""
    project = support.unlinked(project)
    declaration = support.unlinked(project.parent / 'EditorSupport.json')
    if not declaration.exists():
        return []
    data = support.read(declaration)
    support.require(set(data) == {'schema', 'plugins'} and data['schema'] == 1
                    and isinstance(data['plugins'], list), 'Invalid EditorSupport.json')
    entries, names, modules = [], set(), set()
    # Validate the complete declaration before changing any plugin.
    for entry in data['plugins']:
        support.require(isinstance(entry, dict) and set(entry) <= {'source', 'overlays'} and 'source' in entry,
                        'Project support entries need source and optional overlays')
        source, _ = source_tree(project.parent / entry['source'], entry.get('overlays', []))
        _, provided = support.descriptor(source)
        support.require(source.stem.casefold() not in names, 'Duplicate project support plugin')
        support.require(not modules.intersection(n.casefold() for n in provided), 'Duplicate project support module')
        names.add(source.stem.casefold()); modules.update(n.casefold() for n in provided)
        entries.append((source, entry.get('overlays', [])))
    return [prepare_provider(project, engine, Path(output) / source.stem, source, overlays)
            for source, overlays in entries]
