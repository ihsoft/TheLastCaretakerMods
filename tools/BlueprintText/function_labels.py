"""Frozen English Unreal UI labels, resolved by class and function identity.

Refresh: export with Invoke-BlueprintAssetText.ps1 -Action ExportLabels, then
python -B function_labels.py <export.json> --output Metadata/function-labels.json
The publisher accepts Engine metadata only and writes deterministic JSON.
"""
import argparse
from functools import lru_cache
import json
from pathlib import Path
import re

SNAPSHOT = Path(__file__).parent / 'Metadata/function-labels.json'


def validate(data):
    if data.get('schema') != 1 or data.get('culture') != 'en' or not data.get('engineVersion'):
        raise ValueError('Expected a versioned schema-1 English Unreal labels export')
    if not data.get('functions'):
        raise ValueError('Function labels export is empty')
    for identity, entry in data['functions'].items():
        if not re.fullmatch(r'/Script/Engine\.\w+::\w+', identity):
            raise ValueError('Only native Engine function identities belong in this snapshot')
        if set(entry) - {'title', 'compactTitle'} or not isinstance(entry.get('title'), str) or not entry['title']:
            raise ValueError('Invalid function label: ' + identity)
        if 'compactTitle' in entry and not isinstance(entry['compactTitle'], str):
            raise ValueError('Invalid compact title: ' + identity)
    for owner, parent in data['parents'].items():
        if not owner.startswith('/Script/Engine.') or not parent.startswith('/Script/'):
            raise ValueError('Unexpected class ancestry in labels export')
    return data


@lru_cache(maxsize=1)
def load_labels():
    return validate(json.loads(SNAPSHOT.read_text(encoding='utf-8')))


def script_class(raw):
    if "'" in raw:
        raw = raw.split("'", 2)[1]  # Class'path': resolve the path, not the wrapper class.
    match = re.search(r'/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+', raw)
    return match.group(0) if match else None


def lookup(node, pin_templates=None, parent=None, catalog=None):
    """Never resolve by bare function name: custom functions may share names."""
    catalog = load_labels() if catalog is None else catalog
    operation = node.get('operation', '')
    if '::' in operation:
        owner, function = operation.rsplit('::', 1)
    else:
        reference = node.get('properties', {}).get('FunctionReference', '')
        member = re.search(r'MemberName="([^"]+)"', reference)
        if not member:
            return None
        function = member.group(1)
        explicit = re.search(r'MemberParent="((?:\\.|[^"\\])*)"', reference)
        if explicit:
            owner = script_class(explicit.group(1))
        else:
            if 'MemberGuid=' in reference:
                return None  # Local Blueprint member identity, not native metadata.
            target = node.get('pins', {}).get('in:target', {})
            raw_class = (pin_templates or {}).get(target.get('type'), {}).get('PinType.PinSubCategoryObject', '')
            owner = script_class(raw_class)
            # A Blueprint-local custom function is not a native parent function.
            if not owner and 'bSelfContext=True' in reference and 'MemberGuid=' not in reference:
                owner = parent
    visited = set()
    while owner and owner not in visited:
        visited.add(owner)
        found = catalog['functions'].get(owner + '::' + function)
        if found:
            return found
        owner = catalog['parents'].get(owner)
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    try:
        if args.input.resolve() == args.output.resolve():
            raise ValueError('Output must not overwrite the source export')
        data = validate(json.loads(args.input.read_text(encoding='utf-8-sig')))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(data, ensure_ascii=False, sort_keys=True, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({'output': str(args.output), 'functions': len(data['functions']), 'engineVersion': data['engineVersion']}))
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(2, f'Cannot publish function labels: {error}\n')


if __name__ == '__main__':
    main()
