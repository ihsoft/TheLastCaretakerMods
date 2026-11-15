"""Offline DataAsset review: comparison and complete state share one baseline."""
import html


def compare(before, after):
    # A comparison is not an edit request: read-only values may differ too.
    from data_asset_workspace import validate
    validate(before); validate(after)
    if any(before[k] != after[k] for k in ('asset', 'objectName', 'class')):
        raise ValueError('Comparison DataAsset identity/class differs from the selected asset')
    if before['objects'].keys() != after['objects'].keys():
        raise ValueError('Comparison owned-object inventory differs; export using matching project declarations')
    result = []
    for key, obj in before['objects'].items():
        current = after['objects'][key]
        if obj['class'] != current['class'] or obj['properties'].keys() != current['properties'].keys():
            raise ValueError('Comparison property inventory/class differs: ' + key)
        for name, old in obj['properties'].items():
            new = current['properties'][name]
            if old['type'] != new['type']:
                raise ValueError('Comparison property type differs: ' + key + ' / ' + name)
            if old['value'] != new['value']:
                result.append(dict(object=key, property=name, before=old['value'], after=new['value']))
    return result


def render(items, comparison_label):
    esc = lambda value: html.escape(str(value))
    display = lambda value: '\n'.join(value) if isinstance(value, list) else value
    cell = lambda value: '<td><pre>'+esc(display(value) or '(empty)')+'</pre></td>'
    cards, count = [], 0
    has_comparison = any(item['before'] is not None for item in items)
    for item in items:
        doc, before = item['document'], item['before']
        delta = compare(before, doc) if before is not None else []
        count += len(delta)
        changed = {(row['object'], row['property']): row for row in delta}
        label = lambda row: row['property'] if row['object'] == '.' else row['object'] + ' / ' + row['property']
        rows = ''.join('<tr><th>'+esc(label(row))+'</th>'+cell(row['before'])+cell(row['after'])+'</tr>' for row in delta)
        summary = (f'{len(delta)} changed '+('property' if len(delta) == 1 else 'properties')) if before is not None else 'No comparison baseline'
        change_view = ('<table><thead><tr><th>Property</th><th>Before</th><th>After</th></tr></thead><tbody>'+rows+'</tbody></table>' if delta else
                       '<p>No differences from the comparison baseline.</p>' if before is not None else '<p>No comparison baseline. Use Full asset to inspect the saved state.</p>')
        objects = []
        for key, obj in doc['objects'].items():
            fields = []
            for name, row in obj['properties'].items():
                change = changed.get((key, name))
                value = '<pre>'+esc(display(row['value']) or '(empty)')+'</pre>'
                if change:
                    value += '<details class="previous"><summary>Before</summary><pre>'+esc(display(change['before']) or '(empty)')+'</pre></details>'
                fields.append('<tr'+(' class="changed"' if change else '')+'><th>'+esc(name)+
                              (' <span class="badge">Changed</span>' if change else '')+
                              '<small>'+esc(row['type'])+(' · read-only' if not row['editable'] else '')+'</small></th><td>'+value+'</td></tr>')
            title = 'Asset' if key == '.' else key
            objects.append('<details class="object" open><summary>'+esc(title)+' · '+str(len(obj['properties']))+' properties</summary><p>'+esc(obj['class'])+'</p><table><tbody>'+''.join(fields)+'</tbody></table></details>')
        status = 'Pending changes to saved project' if item['pending'] else 'Matches saved project'
        cards.append('<section><h2>'+esc(doc['objectName'])+'</h2><p class="path">'+esc(doc['asset'])+'</p><p>'+status+' · '+summary+'</p>'+
                     '<div data-mode="changes"'+('' if has_comparison else ' hidden')+'>'+change_view+'</div>'+
                     '<div data-mode="full"'+(' hidden' if has_comparison else '')+'>'+''.join(objects)+'</div></section>')
    baseline = 'Compared with: '+comparison_label if has_comparison else 'No comparison baseline'
    page = '''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DataAsset review</title><style>
body{font:16px system-ui;background:#171c22;color:#e8edf3;margin:32px auto;padding:0 24px;max-width:1400px}
section{background:#252f3b;border-radius:12px;padding:24px;margin:20px 0}h2{margin-top:0}p,summary,small{color:#b4c5d4}.path{overflow-wrap:anywhere}
small{display:block;font-weight:400;margin-top:6px}summary{cursor:pointer}table{width:100%;border-collapse:collapse;table-layout:fixed}
th,td{text-align:left;vertical-align:top;padding:12px;border-bottom:1px solid #526271;overflow-wrap:anywhere}details.object{margin-top:20px}
pre{white-space:pre-wrap;overflow-wrap:anywhere;font:14px ui-monospace,monospace;margin:0;max-height:24rem;overflow:auto}
.changed{background:#30423e}.changed th{border-left:3px solid #7ad5ad}.badge{font-size:12px;color:#a2f1cd;font-weight:500}.previous{margin-top:12px}.previous summary{font-size:13px}
nav{display:flex;gap:8px;margin:20px 0}button{font:inherit;border:1px solid #64788e;border-radius:8px;background:#252f3b;color:#e8edf3;padding:10px 20px;cursor:pointer}
button[aria-pressed=true]{background:#d0e4fa;color:#172638;border-color:#d0e4fa}button:disabled{opacity:.5;cursor:default}button:focus-visible{outline:3px solid #7ad5ad;outline-offset:3px}
[hidden]{display:none!important}.hint{font-size:14px}
</style></head><body><h1>DataAsset review</h1>'''
    page += '<p id="comparison">'+esc(baseline)+'</p><nav aria-label="Review mode">'
    page += '<button type="button" data-view="changes" aria-pressed="'+str(has_comparison).lower()+'"'+('' if has_comparison else ' disabled')+'>Changes</button>'
    page += '<button type="button" data-view="full" aria-pressed="'+str(not has_comparison).lower()+'">Full asset</button></nav>'
    page += '<p class="hint">Changes shows differences from the named baseline. Full asset shows every property and highlights those differences.</p>'+''.join(cards)
    page += '''<script>
for(const button of document.querySelectorAll('[data-view]'))button.addEventListener('click',()=>{
  for(const other of document.querySelectorAll('[data-view]'))other.setAttribute('aria-pressed',String(other===button));
  for(const panel of document.querySelectorAll('[data-mode]'))panel.hidden=panel.dataset.mode!==button.dataset.view;
});
</script></body></html>'''
    return page, count
