<#
.SYNOPSIS
Exercise UObject BPT relocation and Blueprint construction in a disposable prepared project.
Warn before running: launches Unreal through the shared queue. No cooking or installation.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='K:\Epic Games\UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT UObject integration tests';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$project=(Resolve-Path -LiteralPath $ProjectFile).Path
$output=[IO.Path]::GetFullPath($OutputRoot)
foreach($path in @($project,$output)){
    if(-not $path.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'Test paths must stay below Tmp/BlueprintText.'}
}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test evidence directory.'}
[void](New-Item -ItemType Directory -Path $output)
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$codec=Join-Path $PSScriptRoot 'asset_text_codec.py'
function ConvertText([string[]]$Arguments){
    & $PythonPath -B $codec @Arguments | Out-Null
    if($LASTEXITCODE -ne 0){throw "Codec failed: $($Arguments[0]); evidence: $output"}
}
function RunEngine([string]$Mode,[string]$Asset,[string]$Name){
    if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation.'}
    $log=Join-Path $output ($Name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) ('-Asset='+$Asset) ('-Output='+(Join-Path $output ($Name+'.native.json'))) ('-abslog='+$log) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){
        $errors=@(Select-String -LiteralPath $log -Pattern 'LogTemp: Error:|LogBlueprint: Error:|Ensure condition failed:' | Select-Object -Last 3 | ForEach-Object {$_.Line})
        throw "$Mode failed ($LASTEXITCODE): $($errors -join ' | '). Evidence: $output"
    }
}
function RetireFixture([string]$Name){
    # Only move the two fixtures just produced by this test; never touch mod content.
    if($Name -notin @('BPTObjectSource','BPTObjectConsumer')){throw 'Unknown test fixture'}
    $file=[IO.Path]::GetFullPath((Join-Path (Split-Path $project -Parent) ('Content/'+$Name+'.uasset')))
    if(-not $file.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture escaped test ownership'}
    Move-Item -LiteralPath $file -Destination (Join-Path $output ($Name+'.original.uasset'))
}
RunEngine 'ObjectFixture' '' 'source'
& (Join-Path $PSScriptRoot 'Invoke-BlueprintAssetText.ps1') -Action Export -ProjectFile $project -Asset '/Game/BPTObjectSource' -OutputPath (Join-Path $output 'source.json') -EngineRoot $EngineRoot -PythonPath $PythonPath
$first='/Game/Mods/TestA/Settings/BPTObjectSource'
$second='/Game/Mods/TestB/Settings/OtherPackage'
ConvertText @('relocate',(Join-Path $output 'source.json'),'--asset',$first,'--output',(Join-Path $output 'mod-a.json'))
ConvertText @('relocate',(Join-Path $output 'source.json'),'--asset',$second,'--output',(Join-Path $output 'mod-b.json'))
RetireFixture 'BPTObjectSource'
$workspace=Join-Path $output 'workspace'
& (Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1') -Action AddSource -ProjectFile $project -WorkspaceRoot $workspace -SourcePath (Join-Path $output 'source.json') -Asset $first -PythonPath $PythonPath
& (Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1') -Action Apply -WorkspaceRoot $workspace -EngineRoot $EngineRoot -PythonPath $PythonPath
foreach($name in @('mod-b')){
    & (Join-Path $PSScriptRoot 'Invoke-BlueprintAssetText.ps1') -Action Import -ProjectFile $project -InputPath (Join-Path $output ($name+'.json')) -OutputPath (Join-Path $output ($name+'.reopened.json')) -EngineRoot $EngineRoot -PythonPath $PythonPath | Out-Null
}
RunEngine 'ObjectConsumer' $first 'consumer'
& (Join-Path $PSScriptRoot 'Invoke-BlueprintAssetText.ps1') -Action Export -ProjectFile $project -Asset '/Game/BPTObjectConsumer' -OutputPath (Join-Path $output 'consumer.json') -EngineRoot $EngineRoot -PythonPath $PythonPath
RetireFixture 'BPTObjectConsumer'
& (Join-Path $PSScriptRoot 'Invoke-BlueprintAssetText.ps1') -Action Import -ProjectFile $project -InputPath (Join-Path $output 'consumer.json') -OutputPath (Join-Path $output 'consumer.reopened.json') -EngineRoot $EngineRoot -PythonPath $PythonPath | Out-Null
RunEngine 'ObjectCheck' $first 'check'
if(Test-Path -LiteralPath (Join-Path (Split-Path $project -Parent) 'Content/BPTObjectSource.uasset')){throw 'Original package unexpectedly recreated'}
$result=[pscustomobject]@{status='passed';relocatedSources=2;actorConsumerRoundtrip=$true;blueprintExecution=$true;originalPackageAbsent=$true;evidence=$output}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
$result | ConvertTo-Json -Compress
