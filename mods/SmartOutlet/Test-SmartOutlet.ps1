[CmdletBinding()]
param([string]$EngineRoot = 'P:/UnrealEngine/UE_5.8', [string]$OutputRoot)
$ErrorActionPreference = 'Stop'
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Test SmartOutlet'
    return
}
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../../tools/VoyageModPipeline.psm1')
Invoke-VoyageModEditorTest -ModRoot $PSScriptRoot -EngineRoot $EngineRoot -OutputRoot $OutputRoot `
    -ReportName 'checks-validation.json' -ExpectedSuiteCount 6 -Run {
    param($run)
    $settings = Join-Path $run.output 'SmartOutlet.ini'
    Copy-Item -LiteralPath (Join-Path $run.modRoot 'Assets/SmartOutlet.ini') -Destination $settings
    $previousOutput = $env:SMARTOUTLET_TEST_OUTPUT
    try {
        $env:SMARTOUTLET_TEST_OUTPUT = $run.output
        & (Join-Path $run.repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Test SmartOutlet Blueprints' `
            -FilePath (Join-Path $run.engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
            -ArgumentList @($run.project, '-run=pythonscript', ('-script=' + (Join-Path $run.projectRoot 'Tests/RunAll.py')),
                '-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $run.output 'checks-validation.log')))
    } finally { $env:SMARTOUTLET_TEST_OUTPUT = $previousOutput }
    Remove-Item -LiteralPath $settings
} | ConvertTo-Json -Depth 8
