<#
.SYNOPSIS
Check native CDO subobjects through changed BPT Apply/reopen and cooked readback.
Uses an isolated native fixture and the shared Unreal queue; never installs a mod.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$EngineRoot='P:/UnrealEngine/UE_5.8', [string]$PythonPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT native CDO defaults acceptance';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $output)){throw 'Use a fresh directory below Tmp/BlueprintText.'}
[void](New-Item -ItemType Directory (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/NativeDefaults.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8"}' | Set-Content $project
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$fixture=Join-Path $PSScriptRoot 'Tests/NativeDefaults'
& (Join-Path $PSScriptRoot '../Prepare-UnrealEditorSupport.ps1') -ProjectFile $project -PluginSource (Join-Path $fixture 'EditorSupport/NativeDefaultsFixture.uplugin') -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'native-support')
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
function CheckNative([string]$Stage){
    $env:BPT_NATIVE_STAGE=$Stage;$env:BPT_NATIVE_OUTPUT=$output
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-run=pythonscript' ('-script='+(Join-Path $fixture 'engine_fixture.py')) '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-abslog='+(Join-Path $output ($Stage+'-unreal.log'))) *> (Join-Path $output ($Stage+'.log'))
    if($LASTEXITCODE -ne 0){throw "Native $Stage check failed; see $output/$Stage.log"}
}
CheckNative 'create'
CheckNative 'original'
$work=Join-Path $output 'workspace'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BP_NativeDefaults','/Game/BP_NativeDefaultsChild') -EngineRoot $EngineRoot -PythonPath $PythonPath
$manifest=Get-Content (Join-Path $work 'workspace.json') -Raw | ConvertFrom-Json
$source=Join-Path $work $manifest.assets[0].source
$original=Join-Path $output 'intact-source.json'
Copy-Item -LiteralPath $source -Destination $original
# A legacy input must fail before any package is replaced.
$beforeFiles=@{}
foreach($asset in $manifest.assets){$doc=Get-Content (Join-Path $work $asset.source) -Raw | ConvertFrom-Json;$file=Join-Path (Split-Path $project) ('Content/'+$doc.asset.Substring(6)+'.uasset');$beforeFiles[$file]=[IO.File]::ReadAllBytes($file)}
@'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);d=json.loads(p.read_text());assert d.pop('nativeDefaults')['objects'];p.write_text(json.dumps(d,indent=2)+'\n')
'@ | & $PythonPath -B - $source
if($LASTEXITCODE){throw 'Cannot prepare legacy regression'}
$rejected=$false
try { & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath } catch { $rejected=$true }
if(-not $rejected){throw 'Legacy native-default source was accepted'}
$failure=Get-ChildItem (Join-Path $work '.workspace/runs') -Filter failure.json -Recurse | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if(-not $failure -or (Get-Content $failure.FullName -Raw | ConvertFrom-Json).error -notmatch 'Legacy source omits nativeDefaults'){throw 'Legacy input failed for an unexpected reason'}
foreach($file in $beforeFiles.Keys){if([Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) -cne [Convert]::ToBase64String($beforeFiles[$file])){throw 'Legacy rejection changed an asset'}}
if(Test-Path (Join-Path $work '.workspace/recovery.json')){throw 'Legacy rejection entered the write transaction'}
Copy-Item -LiteralPath $original -Destination $source
foreach($stage in @('roundtrip','zero','restored')){
@'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);d=json.loads(Path(sys.argv[2]).read_text());stage=sys.argv[3]
if stage=='roundtrip':
    g=next(g for g in d['graphs'] if g['body']['nodes'])
    props=g['body']['nodes'][0]['properties'];props['NodePosX']=str(int(props.get('NodePosX','0'))+16)
elif stage=='zero':
    row=next(o for o in d['nativeDefaults']['objects'] if o['name']=='ModuleComponent')
    row['properties'].pop('ItemAsset',None)
    row['properties']['ConfigData']='(bAutoStartModule=False,ResourceBandwidthInput=0.000000,MaxResourceAmount=0)'
    nested=next(o for o in d['nativeDefaults']['objects'] if o['name']=='Nested')
    nested['properties']['Value']='0'
p.write_text(json.dumps(d,indent=2)+'\n',encoding='utf-8')
'@ | & $PythonPath -B - $source $original $stage
    if($LASTEXITCODE){throw 'Cannot prepare native default edit'}
    & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
    CheckNative $stage
}
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=cook' '-targetplatform=Windows' '-unattended' '-nop4' '-nullrhi' '-SkipZenStore' '-CookAll' '-NoDefaultMaps' ('-OutputDir='+(Join-Path $output 'cooked')) ('-abslog='+(Join-Path $output 'cook-unreal.log')) *> (Join-Path $output 'cook.log')
if($LASTEXITCODE){throw "Cook failed; see $output/cook.log"}
$cooked=Join-Path $output 'cooked/NativeDefaults/Content'
foreach($pair in @(@('parent','BP_NativeDefaults'),@('child','BP_NativeDefaultsChild'))){
    & (Join-Path $PSScriptRoot '../Invoke-VoyageBoundedTool.ps1') -Executable (Join-Path $PSScriptRoot '../../.tools/bin/UAssetGUI.exe') -Arguments @('--portable','tojson',(Join-Path $cooked ($pair[1]+'.uasset')),(Join-Path $output ('cooked-'+$pair[0]+'.json')),'5.8') -OutputRoot (Join-Path $output 'uasset-read') -MemoryLimitMB 1024 -TimeoutSeconds 60 | Out-Null
}
& $PythonPath -B (Join-Path $fixture 'check_cooked.py') $output
if($LASTEXITCODE){throw 'Independent cooked property check failed'}
@{status='passed';changedApplies=3;selectedAssets=2;legacyRejectedBeforeWrite=$true;independentCookedCheck=$true;project=$project;workspace=$work;original=$original;cooked=$cooked;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
