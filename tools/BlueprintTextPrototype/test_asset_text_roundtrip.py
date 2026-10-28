"""Engine acceptance on a prepared disposable project. Warn before running Unreal.

Preserves the project's original target Blueprints, runs the public import wrapper,
retains rebuilt evidence, and restores originals even on failure. No cook/install.
"""
import argparse
import json
from pathlib import Path
import subprocess


def diagnostics(output):
    # PowerShell wraps error messages at console width. Use native logs as well.
    paths = [output / 'worker.log', *sorted(output.rglob('*-unreal.log'))]
    return '\n'.join(path.read_text(encoding='utf-8', errors='replace') for path in paths)


def check(project, manifest, output, *, single=False, expect_error=None, keep_existing=False):
    repo = Path(__file__).resolve().parents[2]
    owner = (repo / 'Tmp/BlueprintTextPrototype').resolve()
    project, manifest, output = (Path(p).resolve() for p in (project, manifest, output))
    content = (project.parent / 'Content').resolve()
    if not project.is_relative_to(owner) or not output.is_relative_to(owner):
        raise ValueError('Project and acceptance output must be below Tmp/BlueprintTextPrototype')
    if project.suffix != '.uproject' or not project.is_file():
        raise ValueError('Project must be an existing prepared .uproject')
    if keep_existing and not expect_error:
        raise ValueError('keep-existing is only for an expected rejection')
    data = json.loads(manifest.read_text(encoding='utf-8-sig'))
    sources = [manifest] if single else [manifest.parent / p for p in data['assets']]
    targets = {}
    for source in sources:
        asset = json.loads(source.read_text(encoding='utf-8-sig'))['asset']
        if not asset.startswith('/Game/'):
            raise ValueError('Acceptance only handles /Game Blueprint targets')
        relative = Path(asset[6:] + '.uasset')
        target = (content / relative).resolve()
        if not target.is_relative_to(content) or not target.is_relative_to(owner):
            raise ValueError('Target escapes the disposable project Content directory')
        if any(target.with_suffix(ext).exists() for ext in ('.uexp', '.ubulk')):
            raise ValueError('Acceptance expects ordinary uncooked Blueprint packages without sidecars')
        targets[target] = relative
    output.mkdir(parents=True, exist_ok=False)
    originals, moved, retained = {}, [], []
    for target in targets:
        originals[target] = target.read_bytes() if target.exists() else None
    code = None
    try:
        if not keep_existing:
            for target, relative in targets.items():
                if target.exists():
                    backup = output / 'original' / relative
                    backup.parent.mkdir(parents=True, exist_ok=True)
                    target.rename(backup); moved.append((target, backup))
        command = ['powershell.exe', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File',
                   str(repo / 'tools/BlueprintTextPrototype/Invoke-BlueprintAssetText.ps1'),
                   '-Action', 'Import' if single else 'ImportBatch', '-ProjectFile', str(project),
                   '-InputPath', str(manifest), '-OutputPath', str(output / ('reopened.json' if single else 'comparison.json'))]
        with (output / 'worker.log').open('w', encoding='utf-8') as log:
            code = subprocess.run(command, cwd=repo, stdout=log, stderr=subprocess.STDOUT).returncode
    finally:
        if not keep_existing:
            # Restore only targets whose originals were moved, or initially absent targets.
            managed = {target for target, _ in moved} | {t for t, original in originals.items() if original is None}
            for target in managed:
                if target.exists():
                    rebuilt = output / 'rebuilt' / targets[target]
                    rebuilt.parent.mkdir(parents=True, exist_ok=True)
                    target.rename(rebuilt); retained.append(str(rebuilt))
            for target, backup in reversed(moved):
                backup.rename(target)
        for target, original in originals.items():
            if (target.read_bytes() if target.exists() else None) != original:
                raise RuntimeError(f'Original package was not preserved: {target}; recovery data: {output}')
    log = diagnostics(output)
    passed = code == 0 if expect_error is None else code != 0 and expect_error in log and not retained
    result = dict(passed=passed, exitCode=code, originalsRestored=True, expectedError=expect_error,
                  rebuilt=retained, output=str(output))
    (output / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    if not passed:
        raise RuntimeError(f'Round-trip acceptance failed; see {output / "worker.log"}')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', required=True, type=Path)
    parser.add_argument('--input', required=True, type=Path, help='Batch manifest, or compact source with --single')
    parser.add_argument('--output', required=True, type=Path, help='New run folder below Tmp/BlueprintTextPrototype')
    parser.add_argument('--single', action='store_true')
    parser.add_argument('--expect-error', help='Require failure, this diagnostic text, and no saved candidate')
    parser.add_argument('--keep-existing', action='store_true', help='Test refusal to overwrite existing targets')
    args = parser.parse_args()
    try:
        print(json.dumps(check(args.project, args.input, args.output, single=args.single,
                               expect_error=args.expect_error, keep_existing=args.keep_existing)))
    except (ValueError, KeyError, OSError, RuntimeError) as exc:
        parser.exit(1, f'{exc}\n')


if __name__ == '__main__':
    main()
