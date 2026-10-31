"""Native-only Win64 editor plugin preparation; no game or BPT knowledge."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[2]
KIND = 'Unreal editor support binaries'
MANIFEST = 'editor-support.json'
IDENTIFIER = re.compile(r'[A-Za-z][A-Za-z0-9_]*\Z')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def unlinked(path):
    """Never traverse redirected paths when replacing or removing owned trees."""
    path = Path(path).absolute()
    for candidate in (path, *path.parents):
        require(not candidate.is_symlink() and not candidate.is_junction(), f'Linked path is unsupported: {candidate}')
    return path.resolve()


def tree(path):
    path = unlinked(path)
    if not path.exists():
        return {}
    result = {}
    for file in path.rglob('*'):
        unlinked(file)
        if file.is_file():
            result[file.relative_to(path).as_posix()] = file.read_bytes()
    return result


def remove_owned(path, owner):
    path, owner = unlinked(path), unlinked(owner)
    require(path.is_relative_to(owner) and path != owner, 'Cleanup escaped its owned directory')
    tree(path)  # Check descendants before recursive removal, including junctions.
    if path.exists():
        shutil.rmtree(path)


def engine_identity(engine):
    value = read(engine / 'Engine/Binaries/Win64/UnrealEditor.modules').get('BuildId')
    require(isinstance(value, str) and value, 'Editor module build identity is unavailable')
    return value


def descriptor(path):
    require(path.suffix == '.uplugin' and IDENTIFIER.fullmatch(path.stem), 'Expected a named .uplugin file')
    value = read(path)
    require(not value.get('CanContainContent', False), 'Only native support plugins are supported; content plugins remain separate')
    modules = value.get('Modules', [])
    names = [m.get('Name') for m in modules]
    require(names and all(isinstance(n, str) and IDENTIFIER.fullmatch(n) for n in names), 'Invalid module names')
    require(len(set(n.casefold() for n in names)) == len(names), 'Duplicate module names')
    require(all(m.get('Type') in ('Runtime', 'Editor') and m.get('LoadingPhase', 'Default') == 'Default' for m in modules),
            'Only Runtime/Editor modules with Default loading are covered')
    return value, names


def package_info(package, build_id):
    files = tree(package)
    require(MANIFEST in files, 'Not a published editor-support package')
    info = read(package / MANIFEST)
    require(info.get('kind') == KIND and info.get('schema') == 1, 'Unsupported support package manifest')
    name = info.get('plugin')
    require(isinstance(name, str) and IDENTIFIER.fullmatch(name), 'Invalid package plugin name')
    require(info.get('engineBuildId') == build_id, 'Support plugin belongs to another editor build; rebuild it with the selected engine')
    desc, names = descriptor(package / (name + '.uplugin'))
    native = read(package / 'Binaries/Win64/UnrealEditor.modules')
    require(native.get('BuildId') == build_id and set(native.get('Modules', {})) == set(names), 'Native module manifest disagrees with the package')
    require(info.get('modules') == names, 'Descriptor module list changed')
    expected = info.get('files', {})
    require(isinstance(expected, dict) and set(files) == set(expected) | {MANIFEST}, 'Unexpected or missing package files')
    for relative, length in expected.items():
        require(relative == name + '.uplugin' or relative == 'Binaries/Win64/UnrealEditor.modules' or
                re.fullmatch(r'Binaries/Win64/[A-Za-z0-9_.-]+\.dll', relative), 'Package contains unsupported source, resource or path')
        require(type(length) is int and len(files[relative]) == length, f'Package file size changed: {relative}')
    for module, dll in native['Modules'].items():
        require(dll == 'UnrealEditor-' + module + '.dll' and 'Binaries/Win64/' + dll in expected,
                'Expected a clean editor module DLL, not a hot-reload binary')
    return info, desc, files


def run(command, log):
    with log.open('w', encoding='utf-8') as output:
        result = subprocess.run([str(x) for x in command], stdout=output, stderr=subprocess.STDOUT)
    require(result.returncode == 0, f'Command failed ({result.returncode}); see {log}')


def build_package(source, engine, output, runner=run):
    source = unlinked(source)
    desc, names = descriptor(source)
    require((source.parent / 'Source').is_dir(), 'PluginSource requires owned module sources')
    require(not (source.parent / 'Content').exists(), 'Native support must not contain an asset tree')
    for file in source.parent.rglob('*'):
        unlinked(file)
    require(not output.is_relative_to(source.parent), 'Build output must be outside the source plugin tree')
    # UAT may create Config/FilterPlugin.ini beside its input. Give it a private
    # source copy as well as a private HostProject; never rewrite the owner's input.
    # UBT still rejects action paths over 260 characters. Keep compiler scratch
    # shallow regardless of the caller's evidence/workspace directory depth.
    scratch = ROOT / 'Tmp/UnrealEditorSupport' / ('b-' + uuid.uuid4().hex[:8])
    scratch.mkdir(parents=True)
    write(output / 'build-scratch.json', dict(path=str(scratch)))
    private = scratch / 'input'
    shutil.copytree(source.parent, private, ignore=shutil.ignore_patterns('Binaries', 'Intermediate', 'Saved', '.git'))
    build_source = private / source.name
    built = scratch / 'package'
    batch = engine / 'Engine/Build/BatchFiles/RunUAT.bat'
    for path in (build_source, batch, built):
        require(not any(c in str(path) for c in '&|<>^%!"\r\n'), 'Unsupported command-shell characters in build path')
    runner(['cmd.exe', '/d', '/c', batch, 'BuildPlugin', '-Plugin=' + str(build_source), '-Package=' + str(built),
            '-NoTargetPlatforms', '-NoP4', '-UTF8Output'], output / 'build.log')
    package = output / 'package'
    package.mkdir()
    shutil.copy2(built / source.name, package / source.name)
    binary = package / 'Binaries/Win64'
    binary.mkdir(parents=True)
    for file in (built / 'Binaries/Win64').iterdir():
        if file.is_file() and (file.suffix == '.dll' or file.name == 'UnrealEditor.modules'):
            shutil.copy2(file, binary / file.name)
    info = dict(kind=KIND, schema=1, plugin=source.stem, modules=names, engineBuildId=engine_identity(engine),
                files={name: len(data) for name, data in tree(package).items()})
    write(package / MANIFEST, info)
    package_info(package, engine_identity(engine))
    remove_owned(scratch, ROOT / 'Tmp/UnrealEditorSupport')
    (output / 'build-scratch.json').unlink()
    return package


def recovery_path(project):
    return project.parent / 'Saved/UnrealEditorSupport/recovery.json'


def recover(project):
    journal = recovery_path(project)
    if not journal.exists():
        return False
    state = read(journal)
    name = state.get('plugin')
    require(isinstance(name, str) and IDENTIFIER.fullmatch(name), 'Invalid recovery plugin')
    require(state.get('project') == str(project), 'Recovery belongs to another project')
    owner = unlinked(journal.parent)
    stage = unlinked(Path(state['stage']))
    require(stage.parent == owner and stage.name.startswith('transaction-'), 'Invalid recovery staging directory')
    destination = unlinked(project.parent / 'Plugins' / name)
    # Keep recovery copies until every restore operation succeeds.
    before = stage / 'project-before.json'
    project.write_bytes(before.read_bytes())
    if state['existed']:
        previous = stage / 'previous'
        if previous.exists():
            remove_owned(destination, project.parent / 'Plugins')
            shutil.copytree(previous, destination)
    elif destination.exists():
        remove_owned(destination, project.parent / 'Plugins')
    journal.unlink()
    remove_owned(stage, owner)
    return True


def probe(project, engine, names, output, runner=run):
    report = output / 'loaded.json'
    script = output / 'load-plugin.py'
    script.write_text('import unreal, json, ctypes\nfrom pathlib import Path\n' +
                     'kernel = ctypes.WinDLL("kernel32", use_last_error=True)\n' +
                     'kernel.GetModuleHandleW.argtypes = [ctypes.c_wchar_p]\nkernel.GetModuleHandleW.restype = ctypes.c_void_p\n' +
                     f'for name in {names!r}:\n    unreal.load_module(name)\n' +
                     '    assert kernel.GetModuleHandleW("UnrealEditor-" + name + ".dll"), "Module DLL not loaded: " + name\n' +
                     f'Path({str(report)!r}).write_text(json.dumps({{"status":"passed","modules":{names!r}}}))\n', encoding='utf-8')
    runner([engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', project,
            '-EnablePlugins=PythonScriptPlugin', '-run=pythonscript', '-script=' + str(script),
            '-unattended', '-nop4', '-nosplash', '-nullrhi', '-ddc=NoZenLocalFallback',
            '-abslog=' + str(output / 'load-unreal.log')], output / 'load.log')
    require(read(report) == {'status': 'passed', 'modules': names}, 'Editor did not confirm all support modules')


def attach(project, package, engine, output, loader=probe):
    require(not recovery_path(project).exists(), 'Interrupted preparation requires -Recover before another update')
    info, desc, desired = package_info(package, engine_identity(engine))
    name, names = info['plugin'], info['modules']
    original = project.read_bytes()
    data = json.loads(original.decode('utf-8-sig'))
    require(not data.get('AdditionalPluginDirectories'), 'External plugin directories are outside the covered preparation path')
    conflicts = set(n.casefold() for n in names)
    require(not conflicts.intersection(m['Name'].casefold() for m in data.get('Modules', [])), 'Project already owns a support module; migrate it explicitly before attaching a plugin')
    plugins = unlinked(project.parent / 'Plugins')
    for other in plugins.rglob('*.uplugin') if plugins.exists() else []:
        unlinked(other)
        if other.parent != plugins / name:
            require(not conflicts.intersection(m['Name'].casefold() for m in read(other).get('Modules', [])), f'Another plugin provides the same module: {other}')
    destination = unlinked(plugins / name)
    old = tree(destination)
    existed = destination.exists()
    if existed:
        require(old and MANIFEST in old and read(destination / MANIFEST).get('kind') == KIND and
                read(destination / MANIFEST).get('plugin') == name, 'Refuse replacing an unmanaged plugin directory')
        package_info(destination, read(destination / MANIFEST)['engineBuildId'])
    references = data.setdefault('Plugins', [])
    matched = [p for p in references if p.get('Name', '').casefold() == name.casefold()]
    require(len(matched) <= 1, 'Duplicate plugin references in project descriptor')
    if matched:
        require(matched[0].get('Enabled', True), 'Project explicitly disables this plugin; resolve that choice before preparing')
    else:
        references.append({'Name': name, 'Enabled': True})
    updated = (json.dumps(data, indent=2) + '\n').encode('utf-8') if not matched else original
    if old == desired and updated == original:
        loader(project, engine, names, output)
        return 'unchanged'
    owner = unlinked(project.parent / 'Saved/UnrealEditorSupport')
    stage = owner / ('transaction-' + uuid.uuid4().hex)
    stage.mkdir(parents=True)
    (stage / 'project-before.json').write_bytes(original)
    replacement = stage / 'replacement'
    shutil.copytree(package, replacement)
    require(tree(replacement) == desired, 'Staged plugin copy differs from the package')
    require(project.read_bytes() == original and tree(destination) == old, 'Project changed during preparation; retry after reconciling edits')
    journal = recovery_path(project)
    write(journal, dict(project=str(project), plugin=name, stage=str(stage), existed=existed))
    try:
        plugins.mkdir(exist_ok=True)
        if existed:
            destination.rename(stage / 'previous')
        replacement.rename(destination)
        project.write_bytes(updated)
        loader(project, engine, names, output)
        require(tree(destination) == desired and project.read_bytes() == updated, 'Installed support changed during editor load')
    except BaseException as error:
        try:
            recover(project)
        except Exception as rollback:
            raise RuntimeError(f'Preparation failed: {error}; recovery incomplete: {rollback}; preserve {journal}') from error
        raise
    journal.unlink()
    remove_owned(stage, owner)
    return 'prepared'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', required=True)
    parser.add_argument('--engine', required=True)
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument('--source')
    choice.add_argument('--package')
    choice.add_argument('--recover', action='store_true')
    choice.add_argument('--project-dependencies', action='store_true')
    parser.add_argument('--output')
    args = parser.parse_args()
    output = None
    created_output = False
    try:
        project, engine = unlinked(args.project), unlinked(args.engine)
        require(project.is_file() and project.suffix == '.uproject', 'ProjectFile must be an existing .uproject')
        if args.recover:
            print(json.dumps(dict(status='recovered' if recover(project) else 'no-recovery-needed', project=str(project))))
            return 0
        output = unlinked(args.output or ROOT / 'Tmp/UnrealEditorSupport' / ('prepare-' + uuid.uuid4().hex))
        require(output.is_relative_to(ROOT / 'Tmp') and output != ROOT / 'Tmp', 'OutputRoot must be a fresh directory below repository Tmp')
        require(not output.exists(), 'OutputRoot already exists')
        output.mkdir(parents=True)
        created_output = True
        require(not recovery_path(project).exists(), 'Interrupted preparation requires -Recover')
        if args.project_dependencies:
            sys.path.insert(0, str(ROOT / 'tools'))
            from UnrealEditorSupport.project_support import prepare_project
            providers = prepare_project(project, engine, output)
            result = dict(status='prepared', project=str(project), providers=providers, evidence=str(output))
            write(output / 'result.json', result)
            print(json.dumps(result))
            return 0
        package = build_package(Path(args.source), engine, output) if args.source else unlinked(args.package)
        status = attach(project, package, engine, output)
        result = dict(status=status, project=str(project), plugin=read(package / MANIFEST)['plugin'],
                      package=str(package), built=bool(args.source), engineBuildId=engine_identity(engine),
                      loadedModules=read(output / 'loaded.json')['modules'], evidence=str(output))
        write(output / 'result.json', result)
        print(json.dumps(result))
        return 0
    except Exception as error:
        result = dict(status='failed', reason=str(error), evidence=str(output) if output else None)
        if created_output:
            write(output / 'failure.json', result)
        print(json.dumps(result))
        return 1


if __name__ == '__main__':
    sys.exit(main())
