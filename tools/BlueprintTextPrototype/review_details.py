"""Small field-based review presentation; arrays are atomic except incident links."""
import html
import json


MISSING = object()


def canonical(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False)


def equal(before, after, path=()):
    if before is MISSING or after is MISSING:
        return before is after
    if isinstance(before, dict) and isinstance(after, dict):
        return before.keys() == after.keys() and all(equal(before[k], after[k], (*path, k)) for k in before)
    if path == ('links',) and isinstance(before, list) and isinstance(after, list):
        return sorted(map(canonical, before)) == sorted(map(canonical, after))
    return canonical(before) == canonical(after)


def technical(path):
    return bool(path) and (path[-1].lower() in ('nodeguid', 'nodeposx', 'nodeposy', 'persistentguid')
                          or (path[-1] == 'id' and 'pins' in path))


def changes(before, after, path):
    if equal(before, after, path):
        return []
    if isinstance(before, dict) and isinstance(after, dict):
        return [p for key in dict.fromkeys([*before, *after])
                for p in changes(before.get(key, MISSING), after.get(key, MISSING), (*path, key))]
    return [path]


def render_details(before, after):
    """Escaped HTML for After with deleted fields; copying uses separate raw JSON."""
    rows = []

    def line(text, old, new, path, mark=True):
        status = '' if not mark or equal(old, new, path) else (
            'added' if old is MISSING else 'removed' if new is MISSING else 'changed')
        symbol = {'added': '+', 'removed': '−', 'changed': '~'}.get(status, ' ')
        css = 'detail-' + status if status else ''
        if status and all(technical(p) for p in changes(old, new, path)):
            css += ' detail-technical'
        previous = '(not present)' if old is MISSING else (
            'object (see Before)' if isinstance(old, dict) and isinstance(new, dict)
            else json.dumps(old, ensure_ascii=False, indent=2))
        title = ('Previously: ' + previous) if status else ''
        rows.append(f'<span class="detail-row {css}" title="{html.escape(title, quote=True)}">'
                    f'<span class="detail-mark" aria-hidden="true">{symbol}</span>{html.escape(text)}</span>')

    def walk(old, new, path=(), depth=0, key=None, comma=''):
        value = old if new is MISSING else new
        prefix = '  ' * depth + (json.dumps(key, ensure_ascii=False) + ': ' if key is not None else '')
        if isinstance(value, dict):
            line(prefix + '{', old, new, path)
            old_map = old if isinstance(old, dict) else {}
            new_map = new if isinstance(new, dict) else {}
            keys = list(dict.fromkeys([*old_map, *new_map]))
            for i, k in enumerate(keys):
                walk(old_map.get(k, MISSING), new_map.get(k, MISSING), (*path, k), depth+1,
                     k, ',' if i < len(keys)-1 else '')
            line('  ' * depth + '}' + comma, old, new, path, False)
        elif path == ('links',) and isinstance(old, list) and isinstance(new, list):
            line(prefix + '[', old, new, path)
            # Identity is the endpoint pair, not position in the array.
            old_links = {canonical(e): e for e in old}
            new_links = {canonical(e): e for e in new}
            keys = sorted(old_links.keys() | new_links.keys())
            for i, k in enumerate(keys):
                a, b = old_links.get(k, MISSING), new_links.get(k, MISSING)
                text = '  ' * (depth+1) + json.dumps(a if b is MISSING else b, ensure_ascii=False)
                line(text + (',' if i < len(keys)-1 else ''), a, b, path)
            line('  '*depth + ']' + comma, old, new, path, False)
        else:
            # Other arrays remain a single field, without index-by-index markings.
            line(prefix + json.dumps(value, ensure_ascii=False) + comma, old, new, path)

    walk(before, after)
    return ''.join(rows)
