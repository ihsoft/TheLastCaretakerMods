# HAND-WRITTEN BUILD TOOL with game-derived relocation contracts validated for
# Steam build 23962331, VoyageSteam-Win64-Shipping.exe SHA-256
# 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D.
# Revalidate per GAME_DERIVED_SOURCES.md after every game update.

param(
    [string]$CookedRoot = "$PSScriptRoot\Saved\Cooked\Windows\Voyage",

    [Parameter(Mandatory = $true)]
    [string]$OriginalForkliftDirectory,

    [Parameter(Mandatory = $true)]
    [string]$OriginalHudDirectory,

    [Parameter(Mandatory = $true)]
    [string]$ScriptObjects,

    [string]$Retoc = "R:\Codex\ToolCache\rust-retoc-master\source\target\release\retoc.exe",

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'DonkLiftKeyboardControl_P'
)

$ErrorActionPreference = 'Stop'

$originalPackage = '/Game/Blueprints/Vehicles/BP_Forklift_Possesable'
$renamedPackage = '/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original'
if ($originalPackage.Length -ne $renamedPackage.Length) {
    throw 'The original and renamed package paths must have identical byte lengths.'
}
$originalHudPackage = '/Game/UI/Game/HUD/BP_VoyageIngameForklift'
$renamedHudPackage = '/Game/Mods/DonkLift/HUD_Forklift_Original'
if ($originalHudPackage.Length -ne $renamedHudPackage.Length) {
    throw 'The original and renamed HUD package paths must have identical byte lengths.'
}

$cooked = (Resolve-Path -LiteralPath $CookedRoot).Path
$originalDirectory = (Resolve-Path -LiteralPath $OriginalForkliftDirectory).Path
$originalHud = (Resolve-Path -LiteralPath $OriginalHudDirectory).Path
$scriptObjectsPath = (Resolve-Path -LiteralPath $ScriptObjects).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
$childStage = Join-Path $stage 'Voyage\Content\Blueprints\Vehicles'
$parentStage = Join-Path $stage 'Voyage\Content\Mods\DonkLiftKeyboard'
$helperStage = Join-Path $stage 'Voyage\Content\Mods\DonkLiftKeyboardControl'
$contextStage = Join-Path $stage 'Voyage\Content\Game\Input\Vehicle'
$hudChildStage = Join-Path $stage 'Voyage\Content\UI\Game\HUD'
$hudParentStage = Join-Path $stage 'Voyage\Content\Mods\DonkLift'
New-Item -ItemType Directory -Path $childStage, $parentStage, $helperStage, $contextStage, $hudChildStage, $hudParentStage, $package -Force | Out-Null

$childSource = Join-Path $cooked 'Content\Blueprints\Vehicles'
$helperSource = Join-Path $cooked 'Content\Mods\DonkLiftKeyboardControl'
foreach ($assetName in @('BP_Forklift_Possesable.uasset', 'BP_Forklift_Possesable.uexp')) {
    Copy-Item -LiteralPath (Join-Path $childSource $assetName) -Destination $childStage
}
foreach ($assetName in @(
    'ModActor.uasset',
    'ModActor.uexp',
    'IAV_DonkLiftBrake.uasset',
    'IAV_DonkLiftBrake.uexp',
    'IAV_DonkLiftCenterSteering.uasset',
    'IAV_DonkLiftCenterSteering.uexp'
)) {
    Copy-Item -LiteralPath (Join-Path $helperSource $assetName) -Destination $helperStage
}
foreach ($assetName in @('IMC_Forklift_Keyboard.uasset', 'IMC_Forklift_Keyboard.uexp')) {
    Copy-Item -LiteralPath (Join-Path $cooked 'Content\Game\Input\Vehicle' $assetName) -Destination $contextStage
}
foreach ($assetName in @('BP_VoyageIngameForklift.uasset', 'BP_VoyageIngameForklift.uexp')) {
    Copy-Item -LiteralPath (Join-Path $cooked 'Content\UI\Game\HUD' $assetName) -Destination $hudChildStage
}

function Copy-RelocatedAsset {
    param(
        [Parameter(Mandatory = $true)] [string]$SourceDirectory,
        [Parameter(Mandatory = $true)] [string]$SourceAssetName,
        [Parameter(Mandatory = $true)] [string]$DestinationDirectory,
        [Parameter(Mandatory = $true)] [string]$DestinationAssetName,
        [Parameter(Mandatory = $true)] [string]$SourcePackageName,
        [Parameter(Mandatory = $true)] [string]$DestinationPackageName
    )

    $sourceUasset = Join-Path $SourceDirectory ($SourceAssetName + '.uasset')
    $sourceUexp = Join-Path $SourceDirectory ($SourceAssetName + '.uexp')
    if (-not (Test-Path -LiteralPath $sourceUasset -PathType Leaf) -or
        -not (Test-Path -LiteralPath $sourceUexp -PathType Leaf)) {
        throw "Original cooked asset is incomplete: $SourceDirectory\$SourceAssetName"
    }

    $bytes = [IO.File]::ReadAllBytes($sourceUasset)
    $find = [Text.Encoding]::ASCII.GetBytes($SourcePackageName)
    $replace = [Text.Encoding]::ASCII.GetBytes($DestinationPackageName)
    $matches = [Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le $bytes.Length - $find.Length; $offset++) {
        $isMatch = $true
        for ($index = 0; $index -lt $find.Length; $index++) {
            if ($bytes[$offset + $index] -ne $find[$index]) {
                $isMatch = $false
                break
            }
        }
        if ($isMatch) {
            $matches.Add($offset)
            $offset += $find.Length - 1
        }
    }
    if ($matches.Count -ne 2) {
        throw "Expected exactly two embedded package paths in $SourceAssetName; found $($matches.Count)."
    }
    foreach ($offset in $matches) {
        [Array]::Copy($replace, 0, $bytes, $offset, $replace.Length)
    }

    [IO.File]::WriteAllBytes((Join-Path $DestinationDirectory ($DestinationAssetName + '.uasset')), $bytes)
    Copy-Item -LiteralPath $sourceUexp -Destination (Join-Path $DestinationDirectory ($DestinationAssetName + '.uexp'))
}

Copy-RelocatedAsset `
    -SourceDirectory $originalDirectory `
    -SourceAssetName 'BP_Forklift_Possesable' `
    -DestinationDirectory $parentStage `
    -DestinationAssetName 'BP_Forklift_Original' `
    -SourcePackageName $originalPackage `
    -DestinationPackageName $renamedPackage
Copy-RelocatedAsset `
    -SourceDirectory $originalHud `
    -SourceAssetName 'BP_VoyageIngameForklift' `
    -DestinationDirectory $hudParentStage `
    -DestinationAssetName 'HUD_Forklift_Original' `
    -SourcePackageName $originalHudPackage `
    -DestinationPackageName $renamedHudPackage
Copy-Item -LiteralPath $scriptObjectsPath -Destination (Join-Path $stage 'scriptobjects.bin')

$utoc = Join-Path $package ($ContainerName + '.utoc')
& $retocPath to-zen --version UE5_7 $stage $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc verify failed with exit code $LASTEXITCODE"
}

$outputs = @(
    Join-Path $package ($ContainerName + '.pak')
    Join-Path $package ($ContainerName + '.ucas')
    $utoc
)
foreach ($path in $outputs) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Expected package file was not produced: $path"
    }
}

Write-Host 'Autonomous DonkLift control and HUD package built successfully.'
Get-FileHash -Algorithm SHA256 -LiteralPath $outputs |
    Select-Object Path, Hash |
    Format-Table -AutoSize
