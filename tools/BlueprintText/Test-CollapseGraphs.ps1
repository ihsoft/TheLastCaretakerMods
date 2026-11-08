<#
.SYNOPSIS
Test offline Collapse and strict engine roundtrip in an isolated project.
Exercises executable arithmetic, side-effect order, pure extraction, terminal
execution and reparenting of existing graphs with typed/defaulted pins.
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
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT Collapse regression';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputRoot must stay below Tmp/BlueprintText.'}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test evidence directory.'}
[void](New-Item -ItemType Directory -Path (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/CollapseGraphs.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Category":"Tests"}' | Set-Content -LiteralPath $project -Encoding UTF8
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$work=Join-Path $output 'workspace'
function RunEngine([string]$Mode,[string]$Name){
    if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation.'}
    $log=Join-Path $output ($Name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) '-ExpectedOffset=3' ('-abslog='+$log) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed ($LASTEXITCODE); diagnostic: $log"}
}
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'NestedGraphFixture' 'fixture'
RunEngine 'NestedGraphCheck' 'original-check'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BPTNestedFixture','/Game/BPTNestedActor') -EngineRoot $EngineRoot -PythonPath $PythonPath
$plan=& $PythonPath -B (Join-Path $PSScriptRoot 'collapse_fixture_plan.py') $work | ConvertFrom-Json
if($LASTEXITCODE -ne 0){throw 'Cannot select fixture operations'}
$reports=@()
foreach($operation in $plan){
    $reports+=& $tool -Action Collapse -WorkspaceRoot $work -Asset $operation.asset -Graph $operation.graph -Node $operation.nodes -Name $operation.name -PythonPath $PythonPath | ConvertFrom-Json
}
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'NestedGraphCheck' 'collapsed-check'
$unchanged=& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath | ConvertFrom-Json
if($unchanged.status -ne 'unchanged'){throw 'Repeated Apply must be unchanged'}
$preview=& $tool -Action Preview -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath | ConvertFrom-Json
$result=[pscustomobject]@{status='passed';operations=$reports;sideEffectOrder=$true;returnedValues=$true;eventAndConstruction=$true;existingTypedDefaults=$true;noOpApply=$true;preview=$preview;evidence=$output}
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
$result | ConvertTo-Json -Depth 12 -Compress
