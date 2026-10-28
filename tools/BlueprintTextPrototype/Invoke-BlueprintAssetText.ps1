<#
.SYNOPSIS
Export/rebuild schema-2 Actor Blueprints in an isolated editor project.
Export writes a complete compact source. Import requires an absent target asset,
compiles/saves it, then exports it in a fresh process and compares every represented field.
ExportLabels reads English Engine function UI metadata without changing assets.
Warn the user before any action: all launch headless Unreal. No cooking or installation.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Export','Import','ExportLabels')][string]$Action,
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$InputPath,
    [string]$Asset,
    [string]$EngineRoot='K:\Epic Games\UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintTextPrototype')).TrimEnd('\')+'\'
$project=(Resolve-Path -LiteralPath $ProjectFile).Path
$output=[IO.Path]::GetFullPath($OutputPath)
foreach($path in @($project,$output)){
    if(-not $path.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'Project and output must stay below Tmp/BlueprintTextPrototype.'}
}
if([IO.Path]::GetExtension($project) -ne '.uproject'){throw 'ProjectFile must be a prepared .uproject'}
if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'Close Unreal Editor before this operation'}
if([string]::IsNullOrWhiteSpace($PythonPath)){
    $PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    if(-not(Test-Path -LiteralPath $PythonPath)){ $PythonPath=(Get-Command python -ErrorAction Stop).Source }
}
$codec=Join-Path $PSScriptRoot 'asset_text_codec.py'
$evidence=Join-Path (Split-Path $output -Parent) ('asset-text-'+$Action.ToLowerInvariant()+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
[void](New-Item -ItemType Directory -Path $evidence -Force)
function ConvertText([string[]]$arguments){
    & $PythonPath -B $codec @arguments | Out-Null
    if($LASTEXITCODE -ne 0){throw "Asset text conversion/comparison failed; evidence: $evidence"}
}
function RunEngine([string]$name,[string[]]$arguments){
    $log=Join-Path $evidence ($name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-abslog='+$log) @arguments *> (Join-Path $evidence ($name+'.log'))
    if($LASTEXITCODE -ne 0){
        $errors=@(Select-String -LiteralPath $log -Pattern 'LogTemp: Error:' | Select-Object -Last 3 | ForEach-Object {$_.Line})
        throw "Asset text $name failed ($LASTEXITCODE): $($errors -join ' | '). Evidence: $evidence"
    }
}
if($Action -eq 'ExportLabels'){
    RunEngine 'labels' @('-Mode=Labels',('-Output='+$output))
}elseif($Action -eq 'Export'){
    if([string]::IsNullOrWhiteSpace($Asset)){throw 'Export requires Asset'}
    $native=Join-Path $evidence 'export.native.json'
    RunEngine 'export' @('-Mode=Export',('-Asset='+$Asset),('-Output='+$native))
    ConvertText @('compact',$native,'--output',$output)
}else{
    $input=(Resolve-Path -LiteralPath $InputPath).Path
    if($input -eq $output){throw 'Output must not overwrite source'}
    # Native pin names are case-sensitive; PowerShell JSON objects are not.
    $Asset=& $PythonPath -c "import json,sys; print(json.load(open(sys.argv[1], encoding='utf-8-sig'))['asset'])" $input
    if($LASTEXITCODE -ne 0){throw "Cannot read asset identity; source: $input"}
    $native=Join-Path $evidence 'import.native.json'
    ConvertText @('native',$input,'--output',$native)
    RunEngine 'import' @('-Mode=Import',('-Input='+$native))
    $reopened=Join-Path $evidence 'reopened.native.json'
    RunEngine 'reopen' @('-Mode=Export',('-Asset='+$Asset),('-Output='+$reopened))
    ConvertText @('compact',$reopened,'--output',$output)
    ConvertText @('compare',$input,'--actual',$output,'--output',(Join-Path $evidence 'comparison.json'))
}
[pscustomobject]@{status='passed';action=$Action;asset=$Asset;output=$output;evidence=$evidence}
