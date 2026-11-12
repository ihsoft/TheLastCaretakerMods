<# Isolated declaration defaults, explicit equal/zero/false/empty, reconstruct and cook acceptance. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,[string]$EngineRoot='P:\UnrealEngine\UE_5.8',[string]$PythonPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT optional argument acceptance';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)){throw 'OutputRoot must be a new directory under Tmp/BlueprintText.'}
[void](New-Item -ItemType Directory -Path (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/Arguments.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Category":"Tests"}' | Set-Content -LiteralPath $project -Encoding UTF8
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$work=Join-Path $output 'workspace'
function Engine([string]$Mode,[string]$Name,[int]$Expected=7){
    $log=Join-Path $output ($Name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) ('-Expected='+$Expected) ('-abslog='+$log) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE){throw "$Mode failed ($LASTEXITCODE); see $log"}
}
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
Engine 'ArgumentFixture' 'fixture'
Engine 'ArgumentCheck' 'initial'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BPTArgumentCallee','/Game/BPTArgumentCaller') -EngineRoot $EngineRoot -PythonPath $PythonPath
foreach($stage in @('move','default')){
    & $PythonPath -B (Join-Path $PSScriptRoot 'argument_fixture.py') $work $stage
    if($LASTEXITCODE){throw 'Argument fixture edit failed.'}
    & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
    $expected=if($stage -eq 'move'){7}else{9}
    Engine 'ArgumentCheck' $stage $expected
}
Engine 'ArgumentReconstruct' 'reconstruct' 9
Engine 'ArgumentCheck' 'reconstructed-reopen' 9
Engine 'ArgumentRejectCorruption' 'reject-corruption'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot (Join-Path $output 'fresh') -Asset @('/Game/BPTArgumentCallee','/Game/BPTArgumentCaller') -EngineRoot $EngineRoot -PythonPath $PythonPath
& $tool -Action Apply -WorkspaceRoot (Join-Path $output 'fresh') -EngineRoot $EngineRoot -PythonPath $PythonPath
$fresh=Join-Path $output 'fresh'
$manifest=Get-Content (Join-Path $fresh 'workspace.json') -Raw | ConvertFrom-Json
$source=Join-Path $fresh (@($manifest.assets | Where-Object { $_.source -match 'BPTArgumentCaller' })[0].source)
$sourceBytes=[IO.File]::ReadAllBytes($source)
$assets=@{}
foreach($name in @('BPTArgumentCaller','BPTArgumentCallee')){$assets[$name]=[IO.File]::ReadAllBytes((Join-Path $output ('Project/Content/'+$name+'.uasset')))}
& $PythonPath -B (Join-Path $PSScriptRoot 'argument_fixture.py') $fresh 'missing'
if($LASTEXITCODE){throw 'Cannot prepare missing argument test.'}
$rejected=$false
try { & $tool -Action Apply -WorkspaceRoot $fresh -EngineRoot $EngineRoot -PythonPath $PythonPath *> (Join-Path $output 'missing-required.log') }
catch { $rejected=$true }
finally { [IO.File]::WriteAllBytes($source,$sourceBytes) }
if(-not $rejected -or (Get-Content (Join-Path $output 'missing-required.log') -Raw) -notmatch 'Missing required argument'){throw 'Expected a missing required argument diagnostic.'}
foreach($name in $assets.Keys){
    if(-not [Linq.Enumerable]::SequenceEqual([byte[]]$assets[$name],[byte[]][IO.File]::ReadAllBytes((Join-Path $output ('Project/Content/'+$name+'.uasset'))))){throw 'Failed Apply did not restore selected packages.'}
}
& $tool -Action Apply -WorkspaceRoot $fresh -EngineRoot $EngineRoot -PythonPath $PythonPath
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=cook' '-targetplatform=Windows' '-unattended' '-nop4' '-nullrhi' '-SkipZenStore' '-CookAll' '-NoDefaultMaps' ('-OutputDir='+(Join-Path $output 'cooked')) ('-abslog='+(Join-Path $output 'cook-unreal.log')) *> (Join-Path $output 'cook.log')
if($LASTEXITCODE){throw "Cook failed; see $output/cook.log"}
$cooked=Join-Path $output 'cooked/Arguments/Content/BPTArgumentCaller.uasset'
& (Join-Path $PSScriptRoot '../Invoke-VoyageBoundedTool.ps1') -Executable (Join-Path $PSScriptRoot '../../.tools/bin/UAssetGUI.exe') -Arguments @('--portable','tojson',$cooked,(Join-Path $output 'cooked-caller.json'),'5.8') -OutputRoot (Join-Path $output 'uasset-read') -MemoryLimitMB 1024 -TimeoutSeconds 60 | Out-Null
$cookedText=Get-Content -LiteralPath (Join-Path $output 'cooked-caller.json') -Raw
if($cookedText -match 'BlueprintTextCallFunction|/Script/BlueprintText|BPTArgumentOverrides|BPTDeclarationDefaults|BlueprintText.OptionalParameters'){throw 'Editor argument state leaked into cooked package.'}
@{status='passed';changedApplies=2;freshReopen=$true;explicitEqualPreserved=$true;zeroFalseEmpty=$true;crossBlueprint=$true;localAndNative=$true;reconstruction=$true;corruptionRejected=$true;missingRequiredRollback=$true;editorStateStrippedByCook=$true;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
