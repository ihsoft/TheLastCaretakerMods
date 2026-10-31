# Complete conversion and independent validation for the bounded VoyageItem profile.
import argparse
import json
import re
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('mode', choices=['prepare','compare'])
parser.add_argument('--source', required=True)
parser.add_argument('--recipe', required=True)
parser.add_argument('--schema')
parser.add_argument('--cooked')
parser.add_argument('--output', required=True)
args=parser.parse_args()
def require(condition, message):
    if not condition:
        raise ValueError(message)

recipe=json.loads(Path(args.recipe).read_text(encoding='utf-8-sig'))
require(set(recipe)=={'schemaVersion','profile','asset','patches'}, 'Unknown or missing recipe fields')
require(recipe['schemaVersion']==1 and recipe['profile']=='voyage-item-default-generator-v1', 'Unsupported recipe/profile')
package=recipe['asset']
require(isinstance(package,str) and re.fullmatch(r'/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+',package), 'Invalid asset package')
name=package.rsplit('/',1)[1]
require(isinstance(recipe['patches'],list) and len(recipe['patches'])<=1, 'Only one soft-class replacement is supported')
for patch in recipe['patches']:
    require(set(patch)=={'property','expect','value'} and patch['property']=='DroppedActor', 'Only DroppedActor is supported by this profile')
    for key in ('expect','value'):
        require(isinstance(patch[key],str) and re.fullmatch(r'/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+\.[A-Za-z0-9_]+_C',patch[key]), 'Invalid soft class path')
asset=json.loads(Path(args.source).read_text(encoding='utf-8-sig'))
require(len(asset['Exports'])==2, 'Profile requires exactly two fully parsed exports')
require([e['ObjectName'] for e in asset['Exports']]==['BP_CraftingLocationGenerator_Default_C_0',name], 'Unsupported export shape/order')
require(all('Data' in e for e in asset['Exports']), 'Unparsed export')
def class_path(index):
    require(index<0, 'Unsupported local export class')
    entry=asset['Imports'][-index-1]
    return (class_path(entry['OuterIndex'])+'.' if entry['OuterIndex'] else '')+entry['ObjectName']
require(class_path(asset['Exports'][1]['ClassIndex'])=='/Script/Voyage.VoyageItem', 'Unsupported root class')
require(class_path(asset['Exports'][0]['ClassIndex'])=='/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.BP_CraftingLocationGenerator_Default_C', 'Unsupported inner class')
require(asset['Exports'][0]['OuterIndex']==2 and asset['Exports'][1]['OuterIndex']==0, 'Unsupported object ownership')

if args.mode=="prepare":
    variables=json.loads(Path(args.schema).read_text(encoding="utf-8-sig"))
    require(isinstance(variables,dict) and 'UberGraphFrame' in variables, 'Generator layout lacks UberGraphFrame')
    allowed={'BoolProperty','DoubleProperty','ObjectProperty','TextProperty','StructProperty<Vector>','StructProperty<Rotator>','StructProperty<HitResult>'}
    for field,kind in variables.items():
        require(field=='UberGraphFrame' or kind in allowed, 'Unsupported generator field '+field+': '+kind)
    def ref(index):
        if index == 0: return 'None'
        if index > 0:
            e=asset['Exports'][index-1]
            return package + '.' + asset['Exports'][1]['ObjectName'] + ((':' + e['ObjectName']) if index == 1 else '')
        imp=asset['Imports'][-index-1]
        return ref(imp['OuterIndex']) + '.' + imp['ObjectName'] if imp['OuterIndex'] else imp['ObjectName']
    def quote(s): return json.dumps(s, ensure_ascii=False)
    def value(p):
        t=p['$type'].split(',')[0].split('.')[-1]
        v=p.get('Value')
        if t in ('DoublePropertyData','FloatPropertyData','IntPropertyData'): return str(v)
        if t=='BytePropertyData': return str(p['Value'])
        if t=='BoolPropertyData': return 'True' if v else 'False'
        if t=='EnumPropertyData': return quote(v)
        if t=='NamePropertyData':
            assert re.fullmatch(r'[A-Za-z0-9_]+',v)
            return v
        if t=='ObjectPropertyData': return quote(ref(v)) if v else 'None'
        if t=='SoftObjectPropertyData':
            path=v['AssetPath']; name=path['PackageName']
            return 'None' if name=='None' else quote(name+'.'+path['AssetName']+(':'+v['SubPathString'] if v['SubPathString'] else ''))
        if t=='VectorPropertyData': return '('+','.join(k+'='+str(v[k]) for k in ('X','Y','Z'))+')'
        if t=='StructPropertyData':
            if p['StructType']=='Vector': return value(v[0])
            return '('+','.join(x['Name']+'='+value(x) for x in v)+')'
        if t=='ArrayPropertyData': return '('+','.join(value(x) for x in v)+')'
        if t=='MapPropertyData':
            assert not p.get('KeysToRemove')
            return '('+','.join('('+value(k)+','+value(val)+')' for k,val in v)+')'
        if t=='TextPropertyData':
            if p['HistoryType']=='StringTableEntry':
                require(p['Flags']==0, 'Unsupported string-table text flags')
                return 'LOCTABLE('+quote(p['TableId'])+','+quote(v)+')'
            if p['HistoryType']=='Base':
                require(p['Flags']=='Immutable', 'Unsupported base text flags')
                return 'NSLOCTEXT('+','.join(map(quote,[p['Namespace'],v,p['CultureInvariantString']]))+')'
            if p['HistoryType']=='OrderedFormat':
                require(p['Flags']==0, 'Unsupported formatted text flags')
                assert all(x['Type']=='Text' for x in p['Arguments'])
                return 'LOCGEN_FORMAT_ORDERED('+','.join([value(p['SourceFmt'])]+[value(x['Value']) for x in p['Arguments']])+')'
        raise ValueError(p)

    spec={'innerName':asset['Exports'][0]['ObjectName'],'package':package,'blueprint':'/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default',
          'properties':[{p['Name']:value(p) for p in e['Data']} for e in asset['Exports']],
          'textProperties':[{p['Name']:p for p in e['Data'] if 'TextPropertyData' in p['$type']} for e in asset['Exports']],
          'variables':variables,
          'dependencies':[],'tables':{}}
    for i,imp in enumerate(asset['Imports']):
        if imp['ClassName'] in ('Texture2D','VoyageItemCategoryAsset','VoyageItemMaterial','VoyageItemPart'):
            spec['dependencies'].append({'path':ref(-i-1),'class':imp['ClassName']})

    for patch in recipe['patches']:
        require(spec['properties'][1].get(patch['property'])==quote(patch['expect']), 'Unexpected original '+patch['property'])
        spec['properties'][1][patch['property']]=quote(patch['value'])
    def collect_tables(obj):
        if isinstance(obj,dict):
            if obj.get('HistoryType')=='StringTableEntry':
                spec['tables'].setdefault(obj['TableId'],{})[obj['Value']]=obj['Value']
            for child in obj.values(): collect_tables(child)
        elif isinstance(obj,list):
            for child in obj: collect_tables(child)
    collect_tables(asset['Exports'])
    Path(args.output).write_text(json.dumps(spec,indent=2),encoding='utf-8')
    print('Prepared complete project DataAsset input')
else:
    source=asset
    cooked=json.loads(Path(args.cooked).read_text(encoding="utf-8-sig"))
    def normalize(asset):
        def reference(index):
            if not index: return None
            obj=asset['Imports'][-index-1] if index<0 else asset['Exports'][index-1]
            parent=reference(obj['OuterIndex'])
            return (parent+'.' if parent else '')+obj['ObjectName']
        def node(p):
            t=p['$type'].split(',')[0].split('.')[-1]
            if t=='StructPropertyData': return {'struct':p['StructType'],'fields':fields(p['Value'])}
            if t=='ObjectPropertyData': return {'object':reference(p['Value'])}
            if t=='ArrayPropertyData': return {'array':[node(v) for v in p['Value']]}
            if t=='MapPropertyData':
                assert not p.get('KeysToRemove')
                return {'map':sorted([[node(k),node(v)] for k,v in p['Value']],key=lambda pair:json.dumps(pair[0],sort_keys=True))}
            if t=='TextPropertyData':
                result={'history':p['HistoryType'],'flags':p['Flags'],'value':p.get('Value')}
                for key in ('Namespace','CultureInvariantString','TableId'):
                    if p.get(key) is not None: result[key]=p[key]
                if p.get('SourceFmt'): result['format']=node(p['SourceFmt'])
                if p.get('Arguments'): result['arguments']=[{'type':x['Type'],'value':node(x['Value'])} for x in p['Arguments']]
                return {'text':result}
            def strip(value):
                if isinstance(value,dict): return {k:strip(v) for k,v in value.items() if k!='$type'}
                if isinstance(value,list): return [strip(v) for v in value]
                return value
            return {'type':t,'value':strip(p.get('Value'))}
        def fields(props): return {p['Name']:node(p) for p in props}
        return {e['ObjectName']:{'class':reference(e['ClassIndex']),'outer':reference(e['OuterIndex']),'template':reference(e['TemplateIndex']),'flags':e['ObjectFlags'],'extras':e.get('Extras'),'fields':fields(e['Data'])} for e in asset['Exports']}

    a,b=normalize(source),normalize(cooked)
    # Independently construct the expected cooked representation from the original.
    for patch in recipe['patches']:
        prop=a[name]['fields'][patch['property']]
        require(prop['type']=='SoftObjectPropertyData', 'Expected a soft class reference')
        path=prop['value']['AssetPath']
        require(path['PackageName']+'.'+path['AssetName']==patch['expect'] and not prop['value']['SubPathString'], 'Unexpected original patch value')
        path['PackageName'],path['AssetName']=patch['value'].split('.')

    differences=[]
    def diff(a,b,path=''):
        if type(a)!=type(b): differences.append(f'{path}: {a!r} != {b!r}'); return
        if isinstance(a,dict):
            for k in a.keys()|b.keys():
                if k not in a: differences.append(path+'/'+k+': ADDED')
                elif k not in b: differences.append(path+'/'+k+': MISSING')
                else: diff(a[k],b[k],path+'/'+k)
        elif isinstance(a,list):
            if len(a)!=len(b): differences.append(path+': length mismatch')
            for i,(x,y) in enumerate(zip(a,b)): diff(x,y,path+'/'+str(i))
        elif a!=b: differences.append(f'{path}: {a!r} != {b!r}')
    diff(a,b)

    versions=lambda a: sorted((v['Key'],v['Version']) for v in a['CustomVersionContainer'])
    version_equal=versions(source)==versions(cooked) and all(source[k]==cooked[k] for k in ('ObjectVersion','ObjectVersionUE5','IsUnversioned','PackageFlags'))
    result={'status':'passed' if not differences and version_equal else 'failed','differences':differences,'versionContractEqual':version_equal,'exports':len(b),'allowedChanges':[p['property'] for p in recipe['patches']]}
    Path(args.output).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result))
    raise SystemExit(0 if result['status']=='passed' else 1)
