# Explicit authoring operation on a prepared disposable Unreal project.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Inspect','Export','Import')][string]$Action,
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][string]$RequestPath,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot 'unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation "Resource $Action"
    return
}
$python = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python -B (Join-Path $PSScriptRoot 'UnrealResources/request.py') `
    --action $Action --project $ProjectFile --request $RequestPath --output $OutputRoot
if ($LASTEXITCODE -ne 0) { throw 'Resource request rejected; Unreal was not launched.' }
$output = [IO.Path]::GetFullPath($OutputRoot)
$request = Get-Content -LiteralPath (Join-Path $output 'request.json') -Raw | ConvertFrom-Json
Import-Module (Join-Path $PSScriptRoot 'VoyageModPipeline.psm1') -Force
$projectRoot = Split-Path -Parent $request.project
$before = @{}
if (Test-Path -LiteralPath (Join-Path $projectRoot 'Content') -PathType Container) {
    $before = Get-VoyageProjectContentSnapshot -ProjectRoot $projectRoot
}
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$previousRequest = $env:UNREAL_RESOURCE_REQUEST
$previousPhase = $env:UNREAL_RESOURCE_PHASE
try {
    $env:UNREAL_RESOURCE_REQUEST = Join-Path $output 'request.json'
    $phases = @('execute')
    if ($Action -eq 'Import') { $phases += 'reopen' }
    foreach ($phase in $phases) {
        $env:UNREAL_RESOURCE_PHASE = $phase
        & $editor $request.project '-run=pythonscript' `
            ('-script=' + (Join-Path $PSScriptRoot 'UnrealResources/editor.py')) `
            '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
            '-unattended' '-nop4' '-nosplash' '-nullrhi' `
            ('-abslog=' + (Join-Path $output ($phase + '.log'))) *> (Join-Path $output ($phase + '-console.log'))
        if ($LASTEXITCODE -ne 0) { throw "Resource $phase failed ($LASTEXITCODE). Diagnostics: $output" }
        $report = Get-Content -LiteralPath (Join-Path $output ($phase + '.json')) -Raw | ConvertFrom-Json
        if ($report.status -cne 'passed') { throw "Resource $phase failed. Diagnostics: $output" }
    }
    $allowed = @()
    if ($Action -eq 'Import') {
        $allowed = @($request.resources | ForEach-Object {
            'Content/' + $_.asset.Substring('/Game/'.Length) + '.uasset'
        })
    }
    Assert-VoyageProjectContentUpdateScope -ProjectRoot $projectRoot -Before $before -AllowedPaths $allowed
    [ordered]@{ status='passed'; action=$Action; reportPath=(Join-Path $output ($phases[-1] + '.json')); outputRoot=$output } | ConvertTo-Json
} finally {
    $env:UNREAL_RESOURCE_REQUEST = $previousRequest
    $env:UNREAL_RESOURCE_PHASE = $previousPhase
}
