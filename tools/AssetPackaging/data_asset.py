"""Cooked DataAsset format conversion and semantic comparison; consumer identities are parameters."""
import argparse
import json
from pathlib import Path

def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))

def require(condition, message):
    if not condition:
        raise ValueError(message)

def kind(p):
    return p['$type'].split(',')[0].split('.')[-1]

def reference(asset, index):
    if not index:
        return None
    obj = asset['Imports'][-index-1] if index < 0 else asset['Exports'][index-1]
    parent = reference(asset, obj['OuterIndex'])
    return (parent+'.' if parent else '')+obj['ObjectName']

def quote(value):
    return json.dumps(value, ensure_ascii=False)

def canonical(asset):
    require(len({e['ObjectName'] for e in asset['Exports']}) == len(asset['Exports']),
            'Duplicate export names need a qualified-identity reader')
    def strip(v):
        if isinstance(v,dict): return {k:strip(x) for k,x in v.items() if k!='$type'}
        if isinstance(v,list): return [strip(x) for x in v]
        return v
    def node(p):
        t, v = kind(p), p.get('Value')
        if t == 'StructPropertyData': return {'struct':p['StructType'],'fields':fields(v)}
        if t == 'ArrayPropertyData': return {'array':[node(x) for x in v]}
        if t == 'MapPropertyData':
            require(not p.get('KeysToRemove'), 'Unsupported map removal')
            return {'map':sorted([[node(k),node(val)] for k,val in v],key=lambda x:json.dumps(x[0],sort_keys=True))}
        if t == 'ObjectPropertyData': return {'object':reference(asset,v)}
        if t == 'TextPropertyData':
            require(p['HistoryType'] in ('Base','StringTableEntry'), 'Unsupported text history')
            keys=('HistoryType','Flags','Namespace','Value','CultureInvariantString') if p['HistoryType']=='Base' else ('HistoryType','Flags','TableId','Value')
            return {'text':{k:p.get(k) for k in keys}}
        if t == 'EnumPropertyData': return {'enum':p['EnumType'],'value':v.split('::')[-1]}
        return {'type':t,'value':strip(v)}
    def fields(props):
        require(len({p['Name'] for p in props})==len(props), 'Duplicate fields')
        return {p['Name']:node(p) for p in props}
    return {e['ObjectName']:{'class':reference(asset,e['ClassIndex']), 'outer':reference(asset,e['OuterIndex']),
        'template':reference(asset,e['TemplateIndex']), 'super':reference(asset,e['SuperIndex']),
        'flags':e['ObjectFlags'], 'extras':e.get('Extras'), 'fields':fields(e['Data'])}
        for e in asset['Exports']}

def compare(left, right, prefix=''):
    diffs=[]
    if isinstance(left,dict) and isinstance(right,dict):
        for key in sorted(left.keys()|right.keys()):
            if key not in left: diffs.append(prefix+'/'+key+': added '+repr(right[key]))
            elif key not in right: diffs.append(prefix+'/'+key+': missing')
            else: diffs.extend(compare(left[key],right[key],prefix+'/'+key))
    elif left != right:
        diffs.append(prefix+': '+repr(left)+' != '+repr(right))
    return diffs

parser=argparse.ArgumentParser()
parser.add_argument('mode', choices=['compare','compare-native','convert'])
parser.add_argument('--source',required=True)
parser.add_argument('--actual')
parser.add_argument('--contract')
parser.add_argument('--package',required=True)
parser.add_argument('--class',dest='expected_class',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
a=read(args.source)
package,root_class=args.package,args.expected_class
root_name=package.rsplit('/',1)[1]
roots=[e for e in a['Exports'] if e['OuterIndex']==0]
require(len(roots)==1 and reference(a,roots[0]['ClassIndex'])==root_class,'Wrong cooked class')
require(roots[0]['ObjectName']==root_name and a['FolderName']==package,'Wrong cooked identity')
if args.mode=='convert':
    require(all('Data' in e for e in a['Exports']), 'Expected complete fresh cook')
    require('PKG_Cooked' in a['PackageFlags'] and not a['HasUnversionedProperties'],'Expected tagged cook')
    result=a
    contract=read(args.contract)
    require(set(contract)=={'IsUnversioned','LegacyFileVersion','ObjectVersion','ObjectVersionUE5','FileVersionLicenseeUE',
          'CustomVersionContainer','PackageFlags','HasUnversionedProperties','CustomSerializationFlags'},'Wrong format contract')
    result.update(contract)
    def unversioned_enums(node):
        if isinstance(node,dict):
            if node.get('$type','').split(',')[0].endswith('.EnumPropertyData'):
                prefix=node['EnumType']+'::'
                if node['Value'].startswith(prefix): node['Value']=node['Value'][len(prefix):]
            for value in node.values(): unversioned_enums(value)
        elif isinstance(node,list):
            for value in node: unversioned_enums(value)
    if contract['HasUnversionedProperties']: unversioned_enums(result['Exports'])
    for imp in result['Imports']:
        imp['PackageName']='None'
    # Canonical fromjson with the reviewed usmap builds the entire unversioned
    # property stream. This JSON is intermediate input, never a header patch.
elif args.mode=='compare-native':
    native=read(args.actual)
    def normalize_native(node):
        if isinstance(node,dict):
            if 'nativeHistory' in node:
                text=node['text']
                if text['HistoryType']=='Base': expected='NSLOCTEXT('+', '.join(quote(text[k]) for k in ('Namespace','Value','CultureInvariantString'))+')'
                else: expected='LOCTABLE('+', '.join(quote(text[k]) for k in ('TableId','Value'))+')'
                require(node['nativeHistory']==expected,'Native history differs from text identity')
            normalized={k:normalize_native(v) for k,v in node.items() if k!='nativeHistory'}
            for k in ('object','outer'):
                v=normalized.get(k)
                if isinstance(v,str) and v.startswith(package+'.'):
                    normalized[k]=v[len(package)+1:].replace(':','.')
            if 'map' in normalized: normalized['map'].sort(key=lambda x:json.dumps(x[0],sort_keys=True))
            return normalized
        if isinstance(node,list): return [normalize_native(x) for x in node]
        return node
    expected=canonical(a)
    if 'objects' in native:
        expected={'objects':{name:{k:e[k] for k in ('class','outer','fields')} for name,e in expected.items()}}
    else:
        expected={k:expected[root_name][k] for k in ('class','fields')}
    diffs=compare(expected,normalize_native(native))
    result={'status':'failed' if diffs else 'passed','differences':diffs}
else:
    b=read(args.actual)
    diffs=compare(canonical(a),canonical(b))
    result={'status':'failed' if diffs else 'passed','differences':diffs}
Path(args.output).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result if args.mode.startswith('compare') else {'status':'prepared','mode':args.mode}))
if args.mode.startswith('compare') and diffs: raise SystemExit(1)
