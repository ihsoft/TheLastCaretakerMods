<#
.SYNOPSIS
Import the reusable INI parser through BPT and execute editor-only behavior tests.
Warn before running. Uses the shared Unreal queue; never cooks or installs in game.
RunOnly reuses a previously prepared test project with the two test parser assets.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [switch]$RunOnly,
    [string]$ProjectFile,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT INI parser tests';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputRoot must stay below Tmp/BlueprintText'}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test directory'}
if($RunOnly){
    if(-not $ProjectFile){throw 'RunOnly requires ProjectFile'}
    $project=(Resolve-Path -LiteralPath $ProjectFile).Path
    if(-not $project.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'ProjectFile must stay below Tmp/BlueprintText'}
}elseif($ProjectFile){throw 'ProjectFile requires RunOnly; a full test creates its own project'}
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
[void](New-Item -ItemType Directory -Path $output)
if(-not $RunOnly){
    $projectFolder=Join-Path $output 'Project'
    [void](New-Item -ItemType Directory -Path $projectFolder)
    $project=Join-Path $projectFolder 'IniParserTest.uproject'
    @{FileVersion=3;EngineAssociation='5.8';Plugins=@(@{Name='Fab';Enabled=$false})} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $project -Encoding UTF8
    $support=Join-Path $PSScriptRoot 'Tests/IniParser/EditorSupport/IniParserTestSupport.uplugin'
    @{schema=1;plugins=@(@{source=$support})} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $projectFolder 'EditorSupport.json') -Encoding UTF8
    $workspace=Join-Path $output 'workspace'
    $source=Join-Path $PSScriptRoot 'Library/IniFileParser/BP_IniFileParser.json'
    foreach($name in @('IniTestA','IniTestB')){
        & (Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1') -Action AddSource -ProjectFile $project -WorkspaceRoot $workspace -SourcePath $source -Asset ('/Game/Mods/'+$name+'/BP_IniFileParser') -PythonPath $PythonPath
    }
    & (Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1') -Action Apply -WorkspaceRoot $workspace -EngineRoot $EngineRoot -PythonPath $PythonPath
}
if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation'}
$log=Join-Path $output 'test-unreal.log'
$report=Join-Path $output 'result.json'
$cases=Join-Path $PSScriptRoot 'Tests/IniParser/cases.json'
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=IniParserTest' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Input='+$cases) ('-Output='+$report) ('-abslog='+$log) *> (Join-Path $output 'test.log')
if($LASTEXITCODE -ne 0){
    $errors=@(Select-String -LiteralPath $log -Pattern 'LogTemp: Error:|LogBlueprint: Error:|Ensure condition failed:' | Select-Object -Last 3 | ForEach-Object {$_.Line})
    throw "INI parser tests failed ($LASTEXITCODE): $($errors -join ' | '). Evidence: $output"
}
Get-Content -LiteralPath $report -Raw
