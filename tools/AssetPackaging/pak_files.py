"""Prepare and verify a PAK of explicitly mapped extra files, without interpreting their content."""
import argparse
import filecmp
import json
from pathlib import Path
import re
import shutil


ROOT = Path(__file__).resolve().parents[2]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def working_path(path):
    absolute = Path(path).absolute()
    resolved = absolute.resolve()
    tmp = (ROOT / 'Tmp').resolve()
    require(resolved.is_relative_to(tmp) and resolved != tmp, 'Output must be below repository Tmp')
    for parent in [absolute, *absolute.parents]:
        require(not parent.is_symlink() and not parent.is_junction(), 'Linked output path: ' + str(parent))
    return resolved


def prepare(manifest, output):
    manifest = Path(manifest).resolve()
    output = working_path(output)
    require(not output.exists(), 'Use a fresh output directory')
    data = json.loads(manifest.read_text(encoding='utf-8-sig'))
    require(set(data) == {'schemaVersion', 'files'} and data['schemaVersion'] == 1,
            'Expected schemaVersion 1 and files')
    require(isinstance(data['files'], list) and data['files'], 'Supply at least one file')
    files, destinations = [], set()
    for entry in data['files']:
        require(set(entry) == {'source', 'mountPath'}, 'A file needs source and mountPath only')
        source = (manifest.parent / entry['source']).resolve()
        require(source.is_file(), 'Missing source: ' + str(source))
        require(not any(c in str(source) for c in '\r\n"'), 'Source cannot be encoded in a response file')
        target = entry['mountPath']
        require(isinstance(target, str) and target.startswith('../../../'), 'mountPath must start with ../../../')
        parts = target[9:].split('/')
        require(parts and all(p and p not in ('.', '..') and not p.endswith((' ', '.'))
                             and not re.search(r'[\\:"<>|?*\x00-\x1f]', p) for p in parts),
                'mountPath must be a canonical relative file path')
        require(all(not re.fullmatch(r'(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\..*)?', p, re.I)
                    for p in parts), 'Device names are not supported')
        key = target.casefold()
        require(key not in destinations, 'Duplicate mountPath: ' + target)
        destinations.add(key)
        files.append({'source': str(source), 'mountPath': target})
    for target in destinations:
        require(not any(other.startswith(target + '/') for other in destinations), 'File/directory mount collision')
    output.mkdir(parents=True)
    request = {'files': files, 'pakPath': str(output / 'files.pak')}
    (output / 'request.json').write_text(json.dumps(request, indent=2), encoding='utf-8')
    # Unreal's response reader accepts UTF-8 with BOM, including non-ASCII source paths.
    (output / 'response.txt').write_text(''.join('"{}" "{}"\n'.format(f['source'], f['mountPath']) for f in files),
                                       encoding='utf-8-sig')
    return request


def check_list(output, log):
    output = working_path(output)
    request = json.loads((output / 'request.json').read_text(encoding='utf-8'))
    text = Path(log).read_text(encoding='utf-8-sig')
    mounts = re.findall(r'Listing .* with mount point "([^"]*)"', text)
    require(len(mounts) == 1, 'Missing or ambiguous PAK mount point')
    mount = mounts[0].replace('\\', '/')
    entries = re.findall(r'LogPakFile: Display: "([^"]+)" offset:', text)
    require(entries, 'Missing PAK entries')
    actual = [mount + entry.replace('\\', '/') for entry in entries]
    expected = [f['mountPath'] for f in request['files']]
    require(sorted(actual) == sorted(expected), 'PAK mount/file list differs from the request')
    # Exact matching above ensures safe extraction paths from this produced PAK.
    require(all(p and p not in ('.', '..') for entry in entries for p in entry.replace('\\', '/').split('/')),
            'Unsafe PAK extraction path')
    return request, mount


def verify(output, log):
    output = working_path(output)
    request, mount = check_list(output, log)
    extracted = output / 'readback'
    actual = sorted(p.relative_to(extracted).as_posix() for p in extracted.rglob('*') if p.is_file())
    expected = sorted(f['mountPath'][len(mount):] for f in request['files'])
    require(actual == expected, 'Extracted file set differs from the request')
    for entry in request['files']:
        saved = extracted / entry['mountPath'][len(mount):]
        require(filecmp.cmp(entry['source'], saved, shallow=False), 'PAK bytes differ: ' + entry['mountPath'])
    result = {'status': 'passed', 'pakPath': request['pakPath'], 'fileCount': len(expected),
              'mountPoint': mount, 'reportPath': str(output / 'result.json')}
    shutil.rmtree(working_path(extracted))
    (output / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['prepare', 'check-list', 'verify'])
    parser.add_argument('--output', required=True)
    parser.add_argument('--manifest')
    parser.add_argument('--log')
    args = parser.parse_args()
    if args.action == 'prepare':
        result = prepare(args.manifest, args.output)
    elif args.action == 'check-list':
        check_list(args.output, args.log)
        result = {'status': 'passed'}
    else:
        result = verify(args.output, args.log)
    print(json.dumps(result))
