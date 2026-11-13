[CmdletBinding()]
param([string]$OutputRoot)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $PSScriptRoot -OutputRoot $OutputRoot -Kind test
$report = Join-Path $run 'test-report.json'
$tests = @()
try {
    foreach ($suite in @('Test-ItemAssetRefresh.ps1','Test-MooringMeshRefresh.ps1','Test-ProjectCopy.ps1','Test-SettingsInstallation.ps1','Test-SourceUpdate.ps1','Test-ItemUpdateScope.ps1','Test-SourceUpdateInterruption.ps1')) {
        & (Join-Path $PSScriptRoot ('Build/' + $suite)) > (Join-Path $run ($suite + '.log'))
        $tests += $suite
    }
    Import-Module (Join-Path $repo 'tools/VoyageModSettings/VoyageModSettings.psm1') -Force
    $null = Read-VoyageModSettingsSchema -SchemaPath (Join-Path $PSScriptRoot 'Settings/StableGyro.settings.json') -DefaultIniPath (Join-Path $PSScriptRoot 'Assets/StableGyro.ini')
    $tests += 'INI/schema'
    @{status='passed';tests=$tests; limitations='Offline only: no Unreal, cook, installation into the real game, or gameplay.'} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $report -Encoding UTF8
} catch {
    @{status='failed';tests=$tests;error=$_.Exception.Message} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $report -Encoding UTF8
    throw "StableGyro tests failed; inspect $report : $_"
}
[pscustomobject]@{status='passed';reportPath=$report} | ConvertTo-Json -Compress
