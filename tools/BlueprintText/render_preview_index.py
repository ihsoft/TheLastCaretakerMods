"""Batch offline graph/widget previews and an index from an explicit asset list.

Manifest: {title?, description?, demo?, assets: [source-path | {
  source?, before?, placeholder?: bool, status?: unchanged|added|changed|removed,
  note?, reviews?: {graph-name: {added?: [id], changed?: [id], removed?: [id]}},
  resources?, beforeResources?
}]}. Paths are relative to the manifest. Changed entries require both snapshots;
removed entries require only before. No automatic semantic diff or Unreal launch.
Review UI appears only when at least one asset has added/changed/removed status.
Output must be a new or empty directory. Existing input files are never edited.
"""
import argparse
import copy
import json
import subprocess
import sys
from html import escape
from pathlib import Path

from render_graph import load_graph, review_document
from render_widget import render as render_widget
from preview_references import property_references, package as reference_package, label as reference_label

STATUSES = ('unchanged', 'added', 'changed', 'removed')


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def write(path, document):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding='utf-8')


def keys(value, allowed, context):
    if not isinstance(value, dict) or set(value) - set(allowed):
        raise ValueError(f'Invalid or unknown {context} fields')


def graphs(document):
    result = {}
    for graph in document.get('graphs', []):
        name = graph['name']
        if name in result:
            raise ValueError(f'Duplicate graph: {name}')
        load_graph(document, name)
        result[name] = graph
    return result


def plan(manifest):
    manifest = Path(manifest).resolve()
    data = read(manifest)
    keys(data, {'title', 'description', 'demo', 'assets'}, 'manifest')
    for field in ('title', 'description'):
        if field in data and not isinstance(data[field], str):
            raise ValueError(f'{field} must be text')
    if not isinstance(data.get('demo', False), bool):
        raise ValueError('demo must be boolean')
    if not isinstance(data.get('assets'), list) or not data['assets']:
        raise ValueError('assets must be a nonempty list')
    entries, identities = [], set()
    for item in data['assets']:
        item = {'source': item} if isinstance(item, str) else dict(item)
        keys(item, {'source', 'before', 'placeholder', 'status', 'note', 'reviews', 'resources', 'beforeResources', 'context'}, 'asset')
        context = item.get('context')
        if context is not None:
            keys(context, {'asset', 'assetClass', 'role', 'reason', 'selected', 'dependencies', 'referencedBy'}, 'project context')
            for field in ('asset', 'assetClass', 'role', 'reason'):
                if not isinstance(context.get(field), str):
                    raise ValueError('Project context requires text ' + field)
            if not context['asset'] or context['role'] not in ('stub', 'unclassified') or not isinstance(context.get('selected'), bool):
                raise ValueError('Invalid project context identity, role or selection')
            for field in ('dependencies', 'referencedBy'):
                if not isinstance(context.get(field), list) or any(not isinstance(v, str) for v in context[field]):
                    raise ValueError('Project context requires a list of package paths: ' + field)
            if context['role'] == 'stub':
                item['placeholder'] = True
        status = item.get('status', 'unchanged')
        if status not in STATUSES:
            raise ValueError(f'Unknown status: {status}')
        expected = {'removed': {'before'}, 'changed': {'source', 'before'}}.get(status, {'source'})
        if context is not None and not any(k in item for k in ('source', 'before')):
            if status != 'unchanged' or not context['reason']:
                raise ValueError('Informational assets require unchanged status and an explicit reason')
            expected = set()
        if {key for key in ('source', 'before') if key in item} != expected:
            raise ValueError(f'{status} requires exactly these snapshots: {sorted(expected)}')
        if not isinstance(item.get('placeholder', False), bool) or not isinstance(item.get('note', ''), str):
            raise ValueError('placeholder must be boolean; note must be text')
        docs, paths = {}, {}
        for field in expected:
            if not isinstance(item[field], str) or not item[field]:
                raise ValueError(f'{field} must be a file path')
            paths[field] = (manifest.parent / item[field]).resolve()
            docs[field] = read(paths[field])
            if docs[field].get('schema') not in (1, 2) or not docs[field].get('asset'):
                raise ValueError('Expected a compact Blueprint envelope with asset identity')
            graphs(docs[field])
        document = docs.get('source', docs.get('before'))
        asset = document['asset'] if document else context['asset']
        if context and context['asset'] != asset:
            raise ValueError('Project context does not match Blueprint identity')
        if document and document.get('editorRole') == 'stub':
            item['placeholder'] = True
        if asset in identities or any(d['asset'] != asset for d in docs.values()):
            raise ValueError(f'Duplicate or mismatched asset identity: {asset}')
        identities.add(asset)
        reviews = item.get('reviews', {})
        if not isinstance(reviews, dict) or (reviews and status != 'changed'):
            raise ValueError('Per-graph reviews require a changed Blueprint')
        for name, marks in reviews.items():
            review_document(docs['source'], docs['before'], marks, name)
        bundles = {}
        for field, snapshot in (('resources', 'source'), ('beforeResources', 'before')):
            if field in item:
                if snapshot not in docs or not isinstance(item[field], str):
                    raise ValueError(f'{field} requires its matching snapshot and a path')
                bundles[snapshot] = (manifest.parent / item[field]).resolve()
        entries.append(dict(item=item, docs=docs, paths=paths, bundles=bundles, asset=asset, status=status))
    return data, entries


def graph_preview(document, name, directory, stem, marks=None, before=None, asset_context=None):
    source = directory / f'{stem}.json'
    write(source, document)
    command = [sys.executable, '-B', str(Path(__file__).with_name('render_graph.py')),
               str(source), '--graph', name, '--output', str(directory / f'{stem}.svg')]
    if asset_context:
        command += ['--asset-context', str(asset_context)]
    if marks is not None:
        review = directory / f'{stem}.review.json'
        write(review, marks)
        command += ['--review', str(review)]
        if before is not None:
            old = directory / f'{stem}.before.json'
            write(old, before)
            command += ['--before', str(old)]
    completed = subprocess.run(command, capture_output=True, text=True)
    if completed.returncode:
        raise ValueError(f'Graph preview failed for {name}: {completed.stderr.strip()}')
    return directory / f'{stem}.html'


def one_graph(document, graph):
    # Keep each viewer's input small without changing its shared pin/class tables.
    result = {key: value for key, value in document.items() if key != 'graphs'}
    result['graphs'] = [graph]
    return result


def generate(manifest, output):
    data, entries = plan(manifest)
    reviewing = any(entry['status'] != 'unchanged' for entry in entries)
    output = Path(output).resolve()
    if output.exists() and (not output.is_dir() or any(output.iterdir())):
        raise ValueError('Output directory must be new or empty')
    output.mkdir(parents=True, exist_ok=True)
    cards, inventory, other_assets = [], [], []
    anchors = {e['asset']: f'asset-{i:03d}' for i, e in enumerate(entries)}
    asset_context = {e['asset']: dict(stub=e['item'].get('placeholder', False),
                                    missing=e['item'].get('context', {}).get('reason', '').startswith('Missing dependency'),
                                    href='../index.html#' + anchors[e['asset']]) for e in entries}
    asset_context_path = output / 'asset-context.json'
    write(asset_context_path, asset_context)

    def reference(target):
        asset = reference_package(target)
        label = escape(reference_label(target, asset_context))
        title = escape(target, quote=True)
        return f'<a href="#{anchors[asset]}" title="{title}">{label}</a>' if asset in anchors else f'<code title="{title}">{label}</code>'

    def link(path, label):
        return f'<a href="{escape(path.relative_to(output).as_posix(), quote=True)}">{escape(label)}</a>'

    for number, entry in enumerate(entries):
        item, docs, status = entry['item'], entry['docs'], entry['status']
        context = item.get('context')
        placeholder = item.get('placeholder', False)
        # A dependency stub's empty editor graphs are not the game's editable logic.
        # Explicitly selected stubs can still be reviewed when editing their contract.
        shown = bool(docs) and (not context or not placeholder or context['selected'])
        if not shown:
            reason = context['reason'] or 'Editor stub; game implementation is not present.'
            badge = ' <span class="badge">STUB</span>' if placeholder else ''
            kind = context['assetClass'].rsplit('.', 1)[-1] or 'Not in project'
            other_assets.append(f'<li id="{anchors[entry["asset"]]}" title="{escape(reason, quote=True)}"><code>{escape(entry["asset"])}</code><span>{escape(kind)}{badge}</span></li>')
            inventory.append(dict(asset=entry['asset'], status=status, placeholder=placeholder, graphs=0, displayed=False, context=context))
            continue
        directory = output / f'blueprint-{number:03d}'
        directory.mkdir()
        snapshots, widgets, graph_links = [], [], []
        for snapshot, doc in docs.items():
            saved = directory / f'{snapshot}.json'
            write(saved, doc)
            snapshots.append(link(saved, 'Before source' if snapshot == 'before' else 'Source'))
            if 'widget' in doc:
                tree = next(o for o in doc['widget']['objects'] if o['id'] == doc['widget']['tree'])
                if tree['properties'].get('RootWidget', 'None') == 'None':
                    widgets.append(f'{snapshot.title()}: no local widget layout.')
                else:
                    bundle = entry['bundles'].get(snapshot, entry['paths'][snapshot].with_suffix('.resources') / 'manifest.json')
                    if not bundle.is_file():
                        widgets.append(f'{snapshot.title()}: widget preview needs an exported resource manifest (-PreviewResources).')
                    else:
                        target = directory / f'{snapshot}.widget.html'
                        render_widget(entry['paths'][snapshot], target, bundle)
                        widgets.append(link(target, 'Before widget preview' if snapshot == 'before' else 'Widget preview'))
        current, old = docs.get('source'), docs.get('before')
        current_graphs, old_graphs = graphs(current or {}), graphs(old or {})
        names = list(current_graphs) + [n for n in old_graphs if n not in current_graphs]
        for index, name in enumerate(names):
            after = one_graph(current, current_graphs[name]) if name in current_graphs else None
            before = one_graph(old, old_graphs[name]) if name in old_graphs else None
            marks = item.get('reviews', {}).get(name)
            caption = name
            if after is None:
                after = copy.deepcopy(before)
                after['graphs'][0]['body'] = {'nodes': [], 'links': []}
                marks = {'removed': [n['id'] for n in old_graphs[name]['body']['nodes']]}
                caption += ' — Removed (Before)'
            elif before is None and status in ('added', 'changed'):
                marks = {'added': [n['id'] for n in current_graphs[name]['body']['nodes']]}
                caption += ' — Added'
            target = graph_preview(after, name, directory, f'graph-{index:03d}', marks, before, asset_context_path)
            links = link(target, caption)
            if before is not None and name in current_graphs:
                target = graph_preview(before, name, directory, f'graph-{index:03d}-before', asset_context=asset_context_path)
                links += ' · ' + link(target, 'Before')
            counted = current_graphs.get(name, old_graphs.get(name))['body']
            version = 'Before' if name not in current_graphs else 'Current'
            stats = f'<span class="graph-stats" title="{version} full graph">{len(counted["nodes"])} nodes · {len(counted["links"])} links</span>'
            graph_links.append('<li><span>' + links + '</span>' + stats + '</li>')
        label = entry['asset'].rsplit('/', 1)[-1]
        placeholder = item.get('placeholder', False)
        warning = '<p class="placeholder-warning">STUB — Editor dependency placeholder; not the complete game asset.</p>' if placeholder else ''
        badge = f'<span class="badge {status}">{status.title()}</span>' if reviewing else ''
        information = ''
        if context:
            scope = 'Selected for editing' if context['selected'] else 'Available for BPT editing'
            badge += f'<span class="badge">{scope}</span>'
            if context['reason']:
                information = f'<p>{escape(context["reason"])}</p>'
        # These are actual editable fields, never the registry's flat dependency list.
        for snapshot, doc in docs.items():
            uses = [(field, target) for field, target in property_references(doc)
                    if reference_package(target) != entry['asset']]
            if uses:
                heading = 'Before: asset references in properties' if snapshot == 'before' else 'Asset references in properties'
                information += f'<details class="property-references"><summary>{heading}</summary><ul>' + ''.join(
                    '<li><code>' + escape(field) + '</code><span>→ ' + reference(target) + '</span></li>'
                    for field, target in uses) + '</ul></details>'
        cards.append(f'<section id="{anchors[entry["asset"]]}" class="blueprint {status}{" placeholder" if placeholder else ""}" data-status="{status}"><h2>{escape(label)}{badge}</h2><code>{escape(entry["asset"])}</code>{warning}{information}<p>{escape(item.get("note", ""))}</p><ul>{"".join(graph_links)}</ul><p>{" · ".join(widgets)}</p><p>{" · ".join(snapshots)}</p></section>')
        inventory.append(dict(asset=entry['asset'], status=status, placeholder=placeholder, graphs=len(names), displayed=True, **({'context':context} if context else {})))
    counts = {status: sum(e['status'] == status for e in entries) for status in STATUSES}
    legend = ('<nav aria-label="Blueprint review status">' +
              ' '.join(f'<span class="badge {s}">{s.title()}: {counts[s]}</span>' for s in STATUSES) + '</nav>') if reviewing else ''
    template = Path(__file__).with_name('preview_index.html').read_text(encoding='utf-8')
    replacements = dict(TITLE=escape(data.get('title', 'Blueprint previews')), DESCRIPTION=escape(data.get('description', '')),
                        DEMO='<p class="demo">DEMO — fictional changes for review. No mod assets were edited or rebuilt.</p>' if data.get('demo') else '',
                        LEGEND=legend, CARDS=''.join(cards),
                        OTHER_ASSETS=('<section class="other-assets"><h2>Other assets (' + str(len(other_assets)) + ')</h2>'
                                      '<p>Reference inventory only. These entries do not expose editable BPT graphs. STUB means an editor placeholder, not the game implementation.</p><ul>'
                                      + ''.join(other_assets) + '</ul></section>') if other_assets else '',
                        REVIEW_NOTICE=' Review badges and colored left borders indicate explicitly supplied changes.' if reviewing else '',
                        REVIEW_FOOTER=' Statuses and node review lists are supplied explicitly; this page does not claim an automatic or complete diff.' if reviewing else '')
    # One substitution pass: user text cannot become another template marker.
    import re
    page = re.sub(r'__(TITLE|DESCRIPTION|DEMO|LEGEND|CARDS|OTHER_ASSETS|REVIEW_NOTICE|REVIEW_FOOTER)__', lambda m: replacements[m[1]], template)
    (output / 'index.html').write_text(page, encoding='utf-8')
    summary = dict(index=str(output / 'index.html'), mode='review' if reviewing else 'browse', assets=len(entries),
                   editableAssets=len(cards), listedAssets=len(other_assets),
                   graphs=sum(a['graphs'] for a in inventory), counts=counts, items=inventory)
    write(output / 'summary.json', summary)
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--output', required=True, type=Path, help='New or empty preview directory')
    args = parser.parse_args()
    try:
        result = generate(args.manifest, args.output)
        print(json.dumps({k: v for k, v in result.items() if k != 'items'}))
    except (ValueError, TypeError, KeyError, OSError, StopIteration) as exc:
        parser.exit(2, f'Blueprint preview batch failed: {exc}\n')


if __name__ == '__main__':
    main()
