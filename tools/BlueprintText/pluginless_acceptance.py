"""Integration-test helper: ordinary editor load/compile/save/reopen without BPT."""
import shutil
from blueprint_workspace import HERE, read, write


def verify(workspace, assets):
    project = workspace.project
    provider = project.parent / 'Plugins/BlueprintTextWorkspace'
    if not provider.is_dir() or (project.parent / 'Source').exists():
        raise ValueError('Pluginless acceptance requires a ready-provider working project')
    original = project.read_bytes()
    descriptor = read(project)
    descriptor['Plugins'] = [p for p in descriptor.get('Plugins', []) if p['Name'] != 'BlueprintTextWorkspace']
    detached = workspace.run / 'detached-BlueprintTextWorkspace'
    workspace.run.mkdir(parents=True, exist_ok=True)
    if detached.exists():
        raise ValueError('Pluginless acceptance already ran in this evidence directory')
    # Both verified absolute paths are inside this temporary workspace/project.
    provider.rename(detached)
    try:
        write(project, descriptor)
        for stage in ('save', 'reopen'):
            directory = workspace.run / ('pluginless-' + stage)
            directory.mkdir()
            shutil.copyfile(HERE / 'Tests/pluginless_probe.py', directory / 'probe.py')
            write(directory / 'request.json', dict(assets=assets, result='result.json'))
            workspace.command([workspace.engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', project,
                '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities', '-run=pythonscript',
                '-script=' + str(directory / 'probe.py'), '-unattended', '-nop4', '-nosplash', '-nullrhi',
                '-abslog=' + str(directory / 'unreal.log')], 'pluginless-' + stage)
            result = read(directory / 'result.json')
            log = (directory / 'unreal.log').read_text(encoding='utf-8', errors='replace')
            if result != dict(status='passed', assets=assets) or any(s in log for s in (
                    'LogBlueprint: Error:', 'Failed to find script package', 'LogPython: Error:')):
                raise ValueError('Pluginless editor check failed: ' + str(directory))
    finally:
        project.write_bytes(original)
        detached.rename(provider)
