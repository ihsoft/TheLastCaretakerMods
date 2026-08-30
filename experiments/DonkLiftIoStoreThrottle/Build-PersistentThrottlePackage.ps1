# HAND-WRITTEN RESEARCH TOOL for a package layout validated against
# Steam build 23962331 (Unreal Engine 5.7.4),
# VoyageSteam-Win64-Shipping.exe SHA-256
# 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D.
# Re-extract scriptobjects/assets and revalidate the container after a game update.

param(
    [Parameter(Mandatory = $true)]
    [string] $AssetDirectory,

    [Parameter(Mandatory = $true)]
    [string] $ScriptObjects,

    [Parameter(Mandatory = $true)]
    [string] $Retoc,

    [Parameter(Mandatory = $true)]
    [string] $OutputDirectory,

    [string] $ContainerName = 'DonkLiftPersistentThrottle_P'
)

$ErrorActionPreference = 'Stop'

$assetRoot = (Resolve-Path -LiteralPath $AssetDirectory).Path
$scriptObjectsPath = (Resolve-Path -LiteralPath $ScriptObjects).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $outputRoot) {
    throw "Output directory already exists: $outputRoot"
}

$sourceUasset = Join-Path $assetRoot 'BP_Forklift_Possesable.uasset'
$sourceUexp = Join-Path $assetRoot 'BP_Forklift_Possesable.uexp'
if (-not (Test-Path -LiteralPath $sourceUasset) -or
    -not (Test-Path -LiteralPath $sourceUexp)) {
    throw "Patched forklift asset is incomplete: $assetRoot"
}

$stageRoot = Join-Path $outputRoot 'stage'
$assetStage = Join-Path $stageRoot 'Voyage\Content\Blueprints\Vehicles'
$packageRoot = Join-Path $outputRoot 'package'
New-Item -ItemType Directory -Path $assetStage, $packageRoot -Force | Out-Null

Copy-Item -LiteralPath $sourceUasset, $sourceUexp -Destination $assetStage
Copy-Item -LiteralPath $scriptObjectsPath -Destination (Join-Path $stageRoot 'scriptobjects.bin')

$outputUtoc = Join-Path $packageRoot ($ContainerName + '.utoc')
& $retocPath to-zen --version UE5_7 $stageRoot $outputUtoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}

$outputs = @(
    Join-Path $packageRoot ($ContainerName + '.pak')
    Join-Path $packageRoot ($ContainerName + '.ucas')
    $outputUtoc
)
foreach ($path in $outputs) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected package file was not produced: $path"
    }
}

Write-Host 'Persistent-throttle IoStore package built successfully'
Get-FileHash -Algorithm SHA256 -LiteralPath $outputs |
    Select-Object Path, Hash |
    Format-Table -AutoSize
