"""Run a repository script in the supported Windows PowerShell environment."""
import json
import os
from pathlib import Path
import subprocess
import sys


def windows_powershell():
    """Do not resolve the host or its modules through the caller's PATH."""
    if os.name != 'nt':
        raise RuntimeError('Repository PowerShell scripts require Windows')
    windows = Path(os.environ['SystemRoot'])
    # Sysnative bypasses filesystem redirection for a 32-bit Python caller.
    system = 'Sysnative' if os.environ.get('PROCESSOR_ARCHITEW6432') else 'System32'
    home = windows / system / 'WindowsPowerShell/v1.0'
    executable = home / 'powershell.exe'
    if not executable.is_file():
        raise RuntimeError(f'Windows PowerShell is missing: {executable}')
    env = dict(os.environ)
    for key in list(env):
        if key.lower() == 'psmodulepath':
            del env[key]
    program_files = os.environ.get('ProgramW6432', os.environ['ProgramFiles'])
    env['PSModulePath'] = os.pathsep.join([
        str(windows / 'System32/WindowsPowerShell/v1.0/Modules'),
        str(Path(program_files) / 'WindowsPowerShell/Modules'),
    ])
    return str(executable), env


def run_windows_powershell(arguments):
    executable, env = windows_powershell()
    options = ['-NoLogo', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass']
    # Check the actual executable before any target script starts.
    probe = subprocess.run([executable, *options, '-Command',
        "if ($PSVersionTable.PSEdition -ne 'Desktop' -or "
        "$PSVersionTable.PSVersion.Major -ne 5 -or "
        "$PSVersionTable.PSVersion.Minor -ne 1 -or "
        "-not [Environment]::Is64BitProcess) { exit 78 }; "
        "[Console]::Write($PSVersionTable.PSVersion.ToString())"],
        env=env, capture_output=True, text=True)
    if probe.returncode:
        raise RuntimeError(f'Expected 64-bit Windows PowerShell 5.1 at {executable}; '
                           f'host check failed ({probe.returncode}): {probe.stderr.strip()}')
    print(json.dumps({'repositoryPowerShell': {
        'executable': executable, 'version': probe.stdout.strip(), 'edition': 'Desktop',
    }}), file=sys.stderr, flush=True)
    return subprocess.call([executable, *options, *arguments], env=env)


def run_command(command):
    """Queue adapter: pin Windows PowerShell; leave other executables explicit."""
    if Path(command[0]).name.lower() in ('powershell', 'powershell.exe'):
        return run_windows_powershell(command[1:])
    return subprocess.call(command)


def main():
    if len(sys.argv) < 2:
        print('Usage: python -B tools/run_powershell.py script.ps1 [script arguments]', file=sys.stderr)
        return 2
    script = Path(sys.argv[1]).resolve()
    if script.suffix.lower() != '.ps1' or not script.is_file():
        print(f'Expected an existing .ps1 script: {script}', file=sys.stderr)
        return 2
    try:
        return run_windows_powershell(['-File', str(script), *sys.argv[2:]])
    except (OSError, RuntimeError) as error:
        print(f'Repository PowerShell launch failed: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
