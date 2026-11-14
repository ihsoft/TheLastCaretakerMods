[CmdletBinding()]
param([string]$EngineRoot = 'K:/Epic Games/UE_5.8', [string]$OutputRoot)
$ErrorActionPreference = 'Stop'
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Test Railgun'
    return
}
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../../tools/VoyageModPipeline.psm1')
Import-Module (Join-Path $PSScriptRoot '../../tools/VoyageModSettings/VoyageModSettings.psm1') -Force
$null = Read-VoyageModSettingsSchema -SchemaPath (Join-Path $PSScriptRoot 'Settings/Railgun.settings.json') -DefaultIniPath (Join-Path $PSScriptRoot 'Assets/Railgun.ini')
Invoke-VoyageModEditorTest -ModRoot $PSScriptRoot -EngineRoot $EngineRoot -OutputRoot $OutputRoot `
    -ReportName 'settings-validation.json' -Run {
    param($run)
    & (Join-Path $run.repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Test Railgun settings' `
        -FilePath (Join-Path $run.engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
        -ArgumentList @($run.project, '-run=TestRailgunSettings', ('-Schema=' + (Join-Path $run.modRoot 'Settings/Railgun.settings.json')),
            ('-Output=' + $run.reportPath), '-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',
            ('-abslog=' + (Join-Path $run.output 'editor.log')))
} | ConvertTo-Json -Depth 8
