<#
.SYNOPSIS
Exercise collapsed graph export, strict Apply, executable semantics and offline preview.
Uses an isolated project and the shared Unreal queue; no mod assets are loaded.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='P:\UnrealEngine\UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT nested graph regression';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputRoot must stay below Tmp/BlueprintText.'}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test evidence directory.'}
[void](New-Item -ItemType Directory -Path (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/NestedGraphs.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Category":"Tests"}' | Set-Content -LiteralPath $project -Encoding UTF8
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$work=Join-Path $output 'workspace'
function RunEngine([string]$Mode,[string]$Name,[int]$Offset=3){
    if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation.'}
    $log=Join-Path $output ($Name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) ('-ExpectedOffset='+$Offset) ('-abslog='+$log) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed ($LASTEXITCODE); diagnostic: $log"}
}
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'NestedGraphFixture' 'fixture'
RunEngine 'NestedGraphCheck' 'original-check'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BPTNestedFixture','/Game/BPTNestedActor') -EngineRoot $EngineRoot -PythonPath $PythonPath
# Malformed ownership must fail before package mutation, through the public tool.
$manifest=Get-Content -LiteralPath (Join-Path $work 'workspace.json') -Raw | ConvertFrom-Json
$source=Join-Path $work $manifest.assets[0].source
$original=[IO.File]::ReadAllText($source)
$assetFile=Join-Path $output 'Project/Content/BPTNestedFixture.uasset'
$bytes=[IO.File]::ReadAllBytes($assetFile)
$stamp=[IO.File]::GetLastWriteTimeUtc($assetFile)
@'
import json, sys
from pathlib import Path
p=Path(sys.argv[1]); d=json.loads(p.read_text(encoding='utf-8'))
next(g for g in d['graphs'] if 'owner' in g)['owner']['graph']='F'*32
p.write_text(json.dumps(d),encoding='utf-8')
'@ | & $PythonPath -B - $source
if($LASTEXITCODE -ne 0){throw 'Cannot prepare invalid ownership case'}
$rejected=$false
try { & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath *> (Join-Path $output 'expected-rejection.log') }
catch { $rejected=$true }
finally { [IO.File]::WriteAllText($source,$original,[Text.UTF8Encoding]::new($false)) }
if(-not $rejected -or (Get-Content -LiteralPath (Join-Path $output 'expected-rejection.log') -Raw) -notmatch 'Missing parent graph'){throw 'Expected an ownership rejection from public Apply'}
if(-not [Collections.StructuralComparisons]::StructuralEqualityComparer.Equals($bytes,[IO.File]::ReadAllBytes($assetFile)) -or $stamp -ne [IO.File]::GetLastWriteTimeUtc($assetFile)){throw 'Rejected ownership modified the fixture package'}
@'
import json, sys
from pathlib import Path
work=Path(sys.argv[1])
manifest=json.loads((work/'workspace.json').read_text(encoding='utf-8'))
path=work/manifest['assets'][0]['source']
data=json.loads(path.read_text(encoding='utf-8'))
graphs=data['graphs']
assert len([g for g in graphs if g['name']=='Details'])==2
parent=next(g for g in graphs if g['name']=='RunProbe')
child=next(g for g in graphs if g.get('owner',{}).get('graph')==parent['properties']['GraphGuid'])
inner=next(g for g in graphs if g.get('owner',{}).get('graph')==child['properties']['GraphGuid'])
add=next(n for n in inner['body']['nodes'] if n['kind']=='call')
assert add['defaults']['in:right']=='3'
add['defaults']['in:right']='4'
inner['name']='AdjustedArithmetic'
path.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
'@ | & $PythonPath -B - $work
if($LASTEXITCODE -ne 0){throw 'Nested fixture source edit failed'}
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'NestedGraphCheck' 'edited-check' 4
$unchanged=& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath | ConvertFrom-Json
if($unchanged.status -ne 'unchanged'){throw 'Repeated Apply must be unchanged'}
$unchanged | ConvertTo-Json -Compress
& $tool -Action Preview -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
$result=[pscustomobject]@{status='passed';nestedLevels=2;duplicateNames=$true;branchExits=$true;sideEffectOrder=$true;eventAndConstruction=$true;typedBoundaries=$true;invalidOwnerRejectedBeforeMutation=$true;noOpApply=$true;freshProcessChecks=2;evidence=$output}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
$result | ConvertTo-Json -Compress
