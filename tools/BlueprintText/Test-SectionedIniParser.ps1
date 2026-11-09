<#
.SYNOPSIS
Import sectioned and flat INI sources and execute both through the shared Unreal queue.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT sectioned INI parser tests';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputRoot must stay below Tmp/BlueprintText'}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test directory'}
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$projectFolder=Join-Path $output 'Project'
[void](New-Item -ItemType Directory -Path $projectFolder)
$project=Join-Path $projectFolder 'SectionedIniTest.uproject'
@{FileVersion=3;EngineAssociation='5.8';Plugins=@(@{Name='Fab';Enabled=$false})} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $project -Encoding UTF8
$support=Join-Path $PSScriptRoot 'Tests/IniParser/EditorSupport/IniParserTestSupport.uplugin'
@{schema=1;plugins=@(@{source=$support})} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $projectFolder 'EditorSupport.json') -Encoding UTF8
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$workspace=Join-Path $output 'workspace'
foreach($parser in @('BP_IniFileParser','BP_SectionedIniFileParser')){
    foreach($name in @('IniTestA','IniTestB')){
        & $tool -Action AddSource -ProjectFile $project -WorkspaceRoot $workspace -SourcePath (Join-Path $PSScriptRoot "Library/IniFileParser/$parser.json") -Asset "/Game/Mods/$name/$parser" -PythonPath $PythonPath
    }
}
& $tool -Action Apply -WorkspaceRoot $workspace -EngineRoot $EngineRoot -PythonPath $PythonPath
& $PythonPath -B (Join-Path $PSScriptRoot 'Tests/IniParser/sectioned_cases.py') (Join-Path $output 'typed-cases.json')
if($LASTEXITCODE -ne 0){throw 'Cannot prepare sectioned typed cases'}
function RunCases([string]$Name,[string]$Cases,[bool]$Sectioned){
    if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation'}
    $folder=Join-Path $output $Name
    [void](New-Item -ItemType Directory -Path $folder)
    $log=Join-Path $folder 'test-unreal.log'
    $report=Join-Path $folder 'result.json'
    $extra=@();if($Sectioned){$extra+= '-Sectioned'}
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=IniParserTest' '-unattended' '-nop4' '-nosplash' '-nullrhi' ("-Input=$Cases") ("-Output=$report") ("-abslog=$log") @extra *> (Join-Path $folder 'test.log')
    if($LASTEXITCODE -ne 0){throw "$Name failed ($LASTEXITCODE); diagnostic: $log"}
    Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
}
$flat=RunCases 'flat' (Join-Path $PSScriptRoot 'Tests/IniParser/cases.json') $false
$typed=RunCases 'typed' (Join-Path $output 'typed-cases.json') $true
$sections=RunCases 'sections' (Join-Path $PSScriptRoot 'Tests/IniParser/sectioned-cases.json') $true
& $tool -Action Apply -WorkspaceRoot $workspace -EngineRoot $EngineRoot -PythonPath $PythonPath
$result=@{status='passed';flat=$flat;typed=$typed;sections=$sections;evidence=$output}
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
$result | ConvertTo-Json -Depth 10 -Compress
