# HAND-WRITTEN BUILD TOOL: extracts the base-game asset required by the
# DonkLift package without allowing the installed override to shadow it.

param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc = 'R:\Codex\ToolCache\rust-retoc-master\source\target\release\retoc.exe',

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    $processes = ($running | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ', '
    throw "Voyage must be closed before preparing original assets: $processes"
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$paks = Join-Path $root 'Voyage\Content\Paks'
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$additionalContainers = @(
    Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc' |
        Where-Object { $_.Name -ne 'global.utoc' -and $_.Name -notlike 'pakchunk*.utoc' }
)
$unexpectedContainers = @(
    $additionalContainers |
        Where-Object { $_.Name -ne 'DonkLiftKeyboardControl_P.utoc' }
)
if ($unexpectedContainers.Count -gt 0) {
    $names = ($unexpectedContainers.Name | Sort-Object) -join ', '
    throw "Unexpected additional IoStore containers must be handled explicitly: $names"
}

$installedUtoc = Join-Path $paks 'DonkLiftKeyboardControl_P.utoc'
$disabledUtoc = Join-Path $paks 'DonkLiftKeyboardControl_P.utoc.disabled-for-extraction'
if (Test-Path -LiteralPath $disabledUtoc) {
    throw "Temporary UTOC path already exists: $disabledUtoc"
}
$installedHash = if (Test-Path -LiteralPath $installedUtoc -PathType Leaf) {
    (Get-FileHash -Algorithm SHA256 -LiteralPath $installedUtoc).Hash
} else {
    $null
}

$extractor = Join-Path $PSScriptRoot '..\..\tools\Extract-VoyagePackage.ps1'
try {
    if ($installedHash) {
        Move-Item -LiteralPath $installedUtoc -Destination $disabledUtoc
    }

    & $extractor `
        -Filter 'Vehicles/BP_Forklift_Possesable' `
        -GameRoot $root `
        -Retoc $Retoc `
        -OutputRoot $output
}
finally {
    if (Test-Path -LiteralPath $disabledUtoc -PathType Leaf) {
        Move-Item -LiteralPath $disabledUtoc -Destination $installedUtoc
    }
}

if ($installedHash) {
    if (-not (Test-Path -LiteralPath $installedUtoc -PathType Leaf)) {
        throw 'The installed DonkLift UTOC was not restored after extraction.'
    }
    $restoredHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $installedUtoc).Hash
    if ($restoredHash -cne $installedHash) {
        throw 'The restored DonkLift UTOC does not match the installed file.'
    }
}

$manifests = @(Get-ChildItem -LiteralPath $output -File -Recurse -Filter 'extraction-manifest.json')
if ($manifests.Count -ne 1) {
    throw "Expected exactly one extraction manifest; found $($manifests.Count)."
}

Write-Host 'Prepared fresh DonkLift base-game inputs:'
$manifests | Sort-Object FullName | Select-Object -ExpandProperty DirectoryName
