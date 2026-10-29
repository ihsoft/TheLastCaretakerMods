<#
.SYNOPSIS
Export/rebuild supported schema-2 Actor/Widget Blueprints in an isolated editor project.
Export writes a complete compact source. Import requires an absent target asset,
compiles/saves it, then exports it in a fresh process and compares every represented field.
ImportBatch takes {"assets":["relative/source.json",...]} and reconstructs mutually
referencing Blueprints together. OutputPath receives their comparison report;
reopened compact sources are kept in the adjacent evidence directory.
ExportLabels reads English Engine function UI metadata without changing assets.
Export -PreviewResources also extracts widget brush Texture2D source images to
<output stem>.resources/manifest.json and PNGs during the same engine invocation.
Materials and unavailable source images are listed as unsupported in the manifest.
Warn the user before any action: all launch headless Unreal. No cooking or installation.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Export','Import','ImportBatch','ExportLabels')][string]$Action,
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$InputPath,
    [string]$Asset,
    [switch]$PreviewResources,
    [string]$EngineRoot='K:\Epic Games\UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation "Blueprint asset text: $Action";return}
$owners=@('BlueprintText','BlueprintTextPrototype') | ForEach-Object {[IO.Path]::GetFullPath((Join-Path $PSScriptRoot ('../../Tmp/'+$_))).TrimEnd('\')+'\'}
$project=(Resolve-Path -LiteralPath $ProjectFile).Path
$output=[IO.Path]::GetFullPath($OutputPath)
if($PreviewResources -and $Action -ne 'Export'){throw 'PreviewResources requires Export'}
foreach($path in @($project,$output)){
    if(-not @($owners | Where-Object {$path.StartsWith($_,[StringComparison]::OrdinalIgnoreCase)}).Count){throw 'Project and output must stay below Tmp/BlueprintText.'}
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
        $errors=@(Select-String -LiteralPath $log -Pattern 'LogTemp: Error:|LogBlueprint: Error:|Ensure condition failed:' | Select-Object -Last 3 | ForEach-Object {$_.Line})
        throw "Asset text $name failed ($LASTEXITCODE): $($errors -join ' | '). Evidence: $evidence"
    }
}
if($Action -eq 'ExportLabels'){
    RunEngine 'labels' @('-Mode=Labels',('-Output='+$output))
}elseif($Action -eq 'Export'){
    if([string]::IsNullOrWhiteSpace($Asset)){throw 'Export requires Asset'}
    $native=Join-Path $evidence 'export.native.json'
    $arguments=@('-Mode=Export',('-Asset='+$Asset),('-Output='+$native))
    if($PreviewResources){$arguments+=('-PreviewResources='+[IO.Path]::ChangeExtension($output,'resources'))}
    RunEngine 'export' $arguments
    ConvertText @('compact',$native,'--output',$output)
}elseif($Action -eq 'ImportBatch'){
    $sourceManifest=(Resolve-Path -LiteralPath $InputPath).Path
    if($sourceManifest -eq $output){throw 'Output must not overwrite source'}
    $native=Join-Path $evidence 'batch.native.json'
    ConvertText @('batch-native',$sourceManifest,'--output',$native)
    RunEngine 'import' @('-Mode=ImportBatch',('-Input='+$native))
    $reopened=Join-Path $evidence 'reopened'
    [void](New-Item -ItemType Directory -Path $reopened)
    RunEngine 'reopen' @('-Mode=ExportBatch',('-Input='+$native),('-Output='+$reopened))
    ConvertText @('batch-compare',$sourceManifest,'--actual',$reopened,'--output',$output)
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
