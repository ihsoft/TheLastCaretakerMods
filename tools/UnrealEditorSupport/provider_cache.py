"""Disposable shared provider packages. Callers hold the shared Unreal queue."""
import hashlib
import json
import os
import re
import shutil

from . import editor_support as support

CACHE_ROOT = support.ROOT / 'artifacts/UnrealEditorSupport/providers'
MAX_BYTES = 2 * 1024 ** 3
MAX_ENTRIES = 32
KIND = 'Shared Unreal editor provider'


def key(snapshot):
    # Byte identity controls cache lookup, independent of the consumer's path.
    return hashlib.sha256(json.dumps(snapshot, sort_keys=True, separators=(',', ':')).encode('utf-8')).hexdigest()


def package_key(files):
    digest = hashlib.sha256()
    for name, value in sorted(files.items()):
        encoded = name.encode('utf-8')
        digest.update(len(encoded).to_bytes(8, 'big')); digest.update(encoded)
        digest.update(len(value).to_bytes(8, 'big')); digest.update(value)
    return digest.hexdigest()


def find(snapshot):
    owner = support.unlinked(CACHE_ROOT)
    entry = support.unlinked(owner / key(snapshot))
    if not entry.exists():
        return None
    # A damaged cache entry is disposable; an installed project is not.
    # Reject redirected paths before considering any removal.
    support.tree(entry)
    try:
        record = support.read(entry / 'entry.json')
        support.require(record.get('kind') == KIND and record.get('schema') == 1
                        and record.get('inputs') == snapshot, 'Invalid provider cache inputs')
        info, _, files = support.package_info(entry / 'package', snapshot['engineBuildId'])
        support.require(info['schema'] == snapshot['packageSchema']
                        and package_key(files) == record.get('packageKey'), 'Changed provider cache package')
    except (ValueError, FileNotFoundError, KeyError, TypeError):
        support.remove_owned(entry, owner)
        return None
    os.utime(entry, None)
    return entry / 'package'


def publish(snapshot, package, output):
    """Publish only after attach/load succeeds, using a complete atomic entry."""
    _, _, files = support.package_info(package, snapshot['engineBuildId'])
    record = dict(kind=KIND, schema=1, inputs=snapshot, packageKey=package_key(files))
    size = sum(map(len, files.values())) + len((json.dumps(record, indent=2) + '\n').encode('utf-8'))
    if size > MAX_BYTES:
        return  # A large provider remains usable without retaining it globally.
    owner = support.unlinked(CACHE_ROOT)
    owner.mkdir(parents=True, exist_ok=True)
    entry = support.unlinked(owner / key(snapshot))
    if entry.exists():
        return
    stage = support.unlinked(output / 'cache-entry')
    support.require(not stage.exists(), 'Provider cache stage already exists')
    try:
        shutil.copytree(package, stage / 'package')
        support.require(support.tree(stage / 'package') == files, 'Provider cache copy differs from validated package')
        support.write(stage / 'entry.json', record)
        stage.rename(entry)
    finally:
        support.remove_owned(stage, output)


def prune():
    """Bound retained bytes and variants; never follow links or clean unknown names."""
    owner = support.unlinked(CACHE_ROOT)
    if not owner.exists():
        return
    entries = []
    for entry in owner.iterdir():
        if not re.fullmatch(r'[0-9a-f]{64}', entry.name):
            continue
        support.unlinked(entry)
        if entry.is_dir():
            size = 0
            for path in entry.rglob('*'):
                support.unlinked(path)
                if path.is_file():
                    size += path.stat().st_size
            entries.append((entry.stat().st_mtime_ns, entry, size))
    total = sum(size for _, _, size in entries)
    count = len(entries)
    for _, entry, size in sorted(entries):
        if count <= MAX_ENTRIES and total <= MAX_BYTES:
            break
        support.remove_owned(entry, owner)
        count -= 1
        total -= size
