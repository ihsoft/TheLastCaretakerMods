<#
.SYNOPSIS
Check reflected native call templates through isolated export/edit/Apply/reopen.
Runs through the shared Unreal queue; no mod assets or game process are used.
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
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT call template acceptance';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputRoot must stay below Tmp/BlueprintText.'}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test evidence directory.'}
[void](New-Item -ItemType Directory -Path (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/CallTemplates.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Category":"Tests"}' | Set-Content -LiteralPath $project -Encoding UTF8
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$work=Join-Path $output 'workspace'
function RunEngine([string]$Mode,[string]$Name){
    if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation.'}
    $log=Join-Path $output ($Name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) ('-abslog='+$log) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed ($LASTEXITCODE); diagnostic: $log"}
}
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'CallTemplateFixture' 'fixture'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset '/Game/BPTCallTemplateFixture' -EngineRoot $EngineRoot -PythonPath $PythonPath
$operations=@('/Script/Engine.Actor::AddComponentByClass','/Script/Engine.SceneComponent::K2_AttachToComponent',
    '/Script/Engine.BoxComponent::SetBoxExtent','/Script/Engine.SceneComponent::GetChildrenComponents','/Script/Engine.KismetMathLibrary::MakeTransform',
    '/Script/Engine.KismetSystemLibrary::SetClassPropertyByName')
$before=Get-Content -LiteralPath (Join-Path $work 'workspace.json') -Raw | ConvertFrom-Json
$source=Join-Path $work $before.assets[0].source
$sourceBytes=[IO.File]::ReadAllBytes($source)
$assetFile=Join-Path $output 'Project/Content/BPTCallTemplateFixture.uasset'
$assetBytes=[IO.File]::ReadAllBytes($assetFile)
$templateResult=(& $tool -Action CallTemplates -WorkspaceRoot $work -Operation $operations -EngineRoot $EngineRoot -PythonPath $PythonPath) | ConvertFrom-Json
if(-not [Linq.Enumerable]::SequenceEqual([byte[]]$sourceBytes,[byte[]][IO.File]::ReadAllBytes($source)) -or
   -not [Linq.Enumerable]::SequenceEqual([byte[]]$assetBytes,[byte[]][IO.File]::ReadAllBytes($assetFile))){throw 'CallTemplates mutated source or project asset.'}
if(@($templateResult.operations | Where-Object { $_.identity -eq '/Script/Engine.Actor::AddComponentByClass' })[0].requiredConnections -notcontains 'RelativeTransform'){throw 'Required transform input was not reported.'}
& $PythonPath -B (Join-Path $PSScriptRoot 'call_template_fixture.py') $work $templateResult.catalog
if($LASTEXITCODE -ne 0){throw 'Cannot prepare call template source.'}
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'CallTemplateCheck' 'reopen-check'
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
$validSource=[IO.File]::ReadAllBytes($source)
$validAsset=[IO.File]::ReadAllBytes($assetFile)
@'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);d=json.loads(p.read_text())
g=next(g for g in d['graphs'] if g['name']=='Probe')
g['body']['links']=[l for l in g['body']['links'] if l['to']!='Add.in:RelativeTransform']
p.write_text(json.dumps(d,indent=2)+'\n',encoding='utf-8')
'@ | & $PythonPath -B - $source
if($LASTEXITCODE -ne 0){throw 'Cannot prepare missing reference input case.'}
$referenceRejected=$false
try{& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath *> (Join-Path $output 'reference-rejection.log')}
catch{$referenceRejected=$true}
finally{[IO.File]::WriteAllBytes($source,$validSource)}
if(-not $referenceRejected -or (Get-Content (Join-Path $output 'reference-rejection.log') -Raw) -notmatch 'Relative\s*Transform'){
    throw 'Expected missing required transform input rejection.'
}
if(-not [Linq.Enumerable]::SequenceEqual([byte[]]$validAsset,[byte[]][IO.File]::ReadAllBytes($assetFile))){throw 'Failed Apply did not preserve the accepted asset.'}
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
$rejected=$false
try{& $tool -Action CallTemplates -WorkspaceRoot $work -Operation '/Script/Engine.KismetSystemLibrary::Delay' -EngineRoot $EngineRoot -PythonPath $PythonPath *> (Join-Path $output 'latent-rejection.log')}
catch{$rejected=$true}
if(-not $rejected -or (Get-Content (Join-Path $output 'latent-rejection.log') -Raw) -notmatch 'Latent'){throw 'Expected latent template rejection.'}
$internalRejected=$false
try{& $tool -Action CallTemplates -WorkspaceRoot $work -Operation '/Script/Engine.KismetSystemLibrary::SetObjectPropertyByName' -EngineRoot $EngineRoot -PythonPath $PythonPath *> (Join-Path $output 'internal-rejection.log')}
catch{$internalRejected=$true}
if(-not $internalRejected -or (Get-Content (Join-Path $output 'internal-rejection.log') -Raw) -notmatch 'outside the tested allowlist'){throw 'Expected unrelated internal setter rejection.'}
$result=[pscustomobject]@{status='passed';templates=6;strictApply=$true;freshProcessCheck=$true;classPropertyWrite=$true;classPropertyNoOpsAndNull=$true;classPropertyInstanceIsolation=$true;unrelatedInternalRejected=$internalRejected;explicitFalsePreserved=$true;purePinsPreserved=$true;constReferenceConnected=$true;missingReferenceRejected=$referenceRejected;latentRejected=$true;sourceIsolation=$true;evidence=$output}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
$result | ConvertTo-Json -Compress
