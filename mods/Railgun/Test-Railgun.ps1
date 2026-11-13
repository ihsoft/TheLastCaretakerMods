# Run through Invoke-UnrealQueued.ps1, keeping the reservation for the whole test.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'K:/Epic Games/UE_5.8',
    [string]$OutputRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$owner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/Railgun'))
if (-not $OutputRoot) { $OutputRoot = Join-Path $owner ('test-settings-' + [guid]::NewGuid().ToString('N')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not $output.StartsWith($owner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot must stay below Tmp/Railgun.'
}
if (Test-Path -LiteralPath $output) { throw 'Use a fresh test output directory.' }
$schema = Join-Path $PSScriptRoot 'Settings/Railgun.settings.json'
Import-Module (Join-Path $repo 'tools/VoyageModSettings/VoyageModSettings.psm1') -Force
$null = Read-VoyageModSettingsSchema -SchemaPath $schema -DefaultIniPath (Join-Path $PSScriptRoot 'Assets/Railgun.ini')
$project = & (Join-Path $PSScriptRoot 'New-RailgunProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$projectRoot = Split-Path -Parent $project
$workspace = Join-Path $repo ('Tmp/BlueprintText/Railgun-test-' + [guid]::NewGuid().ToString('N'))
try {
    & (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare `
        -ProjectFile $project -WorkspaceRoot $workspace -EngineRoot $EngineRoot
    $reportPath = Join-Path $output 'settings-validation.json'
    & (Join-Path $repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Test Railgun settings copy' `
        -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
        -ArgumentList @($project, '-run=TestRailgunSettings', ('-Schema=' + $schema),
            ('-Output=' + $reportPath), '-unattended', '-nop4', '-nosplash', '-nullrhi',
            '-ddc=NoZenLocalFallback', ('-abslog=' + (Join-Path $output 'editor.log'))) > (Join-Path $output 'editor-console.log')
    $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    if ($report.status -ne 'passed') { throw "Settings assertions failed: $reportPath" }
    $resolvedProject = (Resolve-Path -LiteralPath $projectRoot).Path
    if ($resolvedProject -ne (Join-Path $output 'Project') -or
        ((Get-Item -LiteralPath $resolvedProject -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Unsafe test project cleanup path.'
    }
    Remove-Item -LiteralPath $resolvedProject -Recurse -Force
    [ordered]@{ status='passed'; report=$reportPath; evidence=$output; bptWorkspace=$workspace } | ConvertTo-Json
}
catch {
    Write-Warning "Test failed; temporary project and diagnostics remain at $output (BPT workspace: $workspace)"
    throw
}
