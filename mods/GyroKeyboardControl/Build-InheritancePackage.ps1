# INTERNAL BUILD STAGE with game-derived DroppedActor patch contracts validated for
# Steam build 25191271, VoyageSteam-Win64-Shipping.exe SHA-256
# 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
# Revalidate per GAME_DERIVED_SOURCES.md after every game update.

param(
    [string]$CookedRoot = "$PSScriptRoot\Saved\Cooked\Windows\Voyage",

    [Parameter(Mandatory = $true)]
    [string]$OriginalItemDirectory,

    [Parameter(Mandatory = $true)]
    [string]$ScriptObjects,

    [string]$Retoc,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'GyroKeyboardControl_P'
)

$ErrorActionPreference = 'Stop'

$expectedSteamBuildId = '25191271'
$expectedExecutableSha256 = '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B'
$retocCompatibilityVersion = 'UE5_8'
$expectedItemFilter = 'Data/Assets/Modules/DA_Item_Module_GyroCopter'
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}

$stockDroppedActorPackage = '/Game/Blueprints/Vehicles/BP_GyroCopter_Possessable'
$modDroppedActorPackage = '/Game/Mods/GyroKeyboardBP/BP_GyroCopter_KeyboardCtl'
$stockDroppedActorObject = 'BP_GyroCopter_Possessable_C'
$modDroppedActorObject = 'BP_GyroCopter_KeyboardCtl_C'
if ($stockDroppedActorPackage.Length -ne $modDroppedActorPackage.Length -or
    $stockDroppedActorObject.Length -ne $modDroppedActorObject.Length) {
    throw 'The stock and mod DroppedActor package/object identities must have equal component lengths.'
}
$cooked = (Resolve-Path -LiteralPath $CookedRoot).Path
$originalDirectory = (Resolve-Path -LiteralPath $OriginalItemDirectory).Path
$scriptObjectsPath = (Resolve-Path -LiteralPath $ScriptObjects).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$retocSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $retocPath).Hash
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

function Get-ExtractionEvidence {
    param(
        [Parameter(Mandatory = $true)] [string]$InputPath,
        [Parameter(Mandatory = $true)] [string]$ExpectedFilter,
        [Parameter(Mandatory = $true)] [string]$Label
    )

    $resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
    $cursor = if (Test-Path -LiteralPath $resolvedInput -PathType Leaf) {
        Split-Path -Parent $resolvedInput
    } else {
        $resolvedInput
    }
    $manifestPath = $null
    while ($cursor) {
        $candidate = Join-Path $cursor 'extraction-manifest.json'
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $manifestPath = $candidate
            break
        }
        $parent = Split-Path -Parent $cursor
        if (-not $parent -or $parent -eq $cursor) {
            break
        }
        $cursor = $parent
    }
    if (-not $manifestPath) {
        throw "$Label is not inside an extraction root with extraction-manifest.json: $resolvedInput"
    }

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if (-not ($manifest.PSObject.Properties.Name -contains 'allowAdditionalContainers') -or
        $manifest.allowAdditionalContainers -ne $false) {
        throw "$Label extraction was not proven free of additional shadowing containers: $manifestPath"
    }
    if ([string]$manifest.filter -cne $ExpectedFilter) {
        throw "$Label extraction used filter '$($manifest.filter)'; expected '$ExpectedFilter'."
    }
    if ([string]$manifest.retocEngineVersion -cne $retocCompatibilityVersion -or
        [string]$manifest.retocSha256 -cne $retocSha256) {
        throw "$Label extraction does not match the selected retoc compatibility contract."
    }
    if ([string]$manifest.steamBuildId -cne $expectedSteamBuildId -or
        [string]$manifest.executableSha256 -cne $expectedExecutableSha256) {
        throw "$Label extraction fingerprint does not match this builder's validated game version."
    }

    return [pscustomobject]@{
        Root = Split-Path -Parent $manifestPath
        ManifestPath = $manifestPath
        Manifest = $manifest
    }
}

$itemEvidence = Get-ExtractionEvidence `
    -InputPath $originalDirectory `
    -ExpectedFilter $expectedItemFilter `
    -Label 'GyroCopter item data asset'
$scriptObjectsEvidence = Get-ExtractionEvidence `
    -InputPath $scriptObjectsPath `
    -ExpectedFilter $expectedItemFilter `
    -Label 'scriptobjects.bin'
if ($scriptObjectsEvidence.ManifestPath -cne $itemEvidence.ManifestPath) {
    throw 'scriptobjects.bin must come from the same extraction root as the Gyro item data asset.'
}

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
$itemStage = Join-Path $stage 'Voyage\Content\Data\Assets\Modules'
$childStage = Join-Path $stage 'Voyage\Content\Mods\GyroKeyboardBP'
$helperStage = Join-Path $stage 'Voyage\Content\Mods\GyroKeyboardControl'
$contextStage = Join-Path $stage 'Voyage\Content\Game\Input\Vehicle'
New-Item -ItemType Directory -Path $itemStage, $childStage, $helperStage, $contextStage, $package -Force | Out-Null

$childSource = Join-Path $cooked 'Content\Mods\GyroKeyboardBP'
$helperSource = Join-Path $cooked 'Content\Mods\GyroKeyboardControl'
foreach ($assetName in @('BP_GyroCopter_KeyboardCtl.uasset', 'BP_GyroCopter_KeyboardCtl.uexp')) {
    Copy-Item -LiteralPath (Join-Path $childSource $assetName) -Destination $childStage
}
foreach ($assetName in @('ModActor.uasset', 'ModActor.uexp')) {
    Copy-Item -LiteralPath (Join-Path $helperSource $assetName) -Destination $helperStage
}
foreach ($assetName in @('IAV_GyroPitchReset.uasset', 'IAV_GyroPitchReset.uexp')) {
    Copy-Item -LiteralPath (Join-Path $helperSource $assetName) -Destination $helperStage
}
$contextSource = Join-Path $cooked 'Content\Game\Input\Vehicle'
foreach ($assetName in @('IMC_GyroCopter_Keyboard.uasset', 'IMC_GyroCopter_Keyboard.uexp')) {
    Copy-Item -LiteralPath (Join-Path $contextSource $assetName) -Destination $contextStage
}
function Find-ByteSequenceOffsets {
    param(
        [Parameter(Mandatory = $true)] [byte[]]$Bytes,
        [Parameter(Mandatory = $true)] [byte[]]$Sequence
    )

    $matches = [Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le $Bytes.Length - $Sequence.Length; $offset++) {
        $isMatch = $true
        for ($index = 0; $index -lt $Sequence.Length; $index++) {
            if ($Bytes[$offset + $index] -ne $Sequence[$index]) {
                $isMatch = $false
                break
            }
        }
        if ($isMatch) {
            $matches.Add($offset)
            $offset += $Sequence.Length - 1
        }
    }
    return $matches
}

function Copy-PatchedAsset {
    param(
        [Parameter(Mandatory = $true)] [string]$SourceDirectory,
        [Parameter(Mandatory = $true)] [string]$AssetName,
        [Parameter(Mandatory = $true)] [string]$DestinationDirectory,
        [Parameter(Mandatory = $true)] [string[]]$SourceValues,
        [Parameter(Mandatory = $true)] [string[]]$DestinationValues
    )

    $sourceUasset = Join-Path $SourceDirectory ($AssetName + '.uasset')
    $sourceUexp = Join-Path $SourceDirectory ($AssetName + '.uexp')
    if (-not (Test-Path -LiteralPath $sourceUasset -PathType Leaf) -or
        -not (Test-Path -LiteralPath $sourceUexp -PathType Leaf)) {
        throw "Original cooked asset is incomplete: $SourceDirectory\$AssetName"
    }

    $bytes = [IO.File]::ReadAllBytes($sourceUasset)
    $exportBytes = [IO.File]::ReadAllBytes($sourceUexp)
    if ($SourceValues.Count -ne $DestinationValues.Count) {
        throw 'DroppedActor patch source/destination counts differ.'
    }
    for ($patchIndex = 0; $patchIndex -lt $SourceValues.Count; $patchIndex++) {
        $find = [Text.Encoding]::ASCII.GetBytes($SourceValues[$patchIndex])
        $replace = [Text.Encoding]::ASCII.GetBytes($DestinationValues[$patchIndex])
        if ($find.Length -ne $replace.Length) {
            throw "DroppedActor patch $patchIndex changes serialized byte length."
        }
        $preexistingDestinationCount =
            @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $replace).Count +
            @(Find-ByteSequenceOffsets -Bytes $exportBytes -Sequence $replace).Count
        if ($preexistingDestinationCount -ne 0) {
            throw "$AssetName already contains mod DroppedActor identity part $patchIndex."
        }
        $uassetMatches = @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $find)
        $uexpMatches = @(Find-ByteSequenceOffsets -Bytes $exportBytes -Sequence $find)
        if (($uassetMatches.Count + $uexpMatches.Count) -ne 1) {
            throw "Expected one stock DroppedActor identity part $patchIndex in $AssetName; found $($uassetMatches.Count + $uexpMatches.Count)."
        }
        foreach ($offset in $uassetMatches) {
            [Array]::Copy($replace, 0, $bytes, $offset, $replace.Length)
        }
        foreach ($offset in $uexpMatches) {
            [Array]::Copy($replace, 0, $exportBytes, $offset, $replace.Length)
        }
        $remainingSourceCount =
            @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $find).Count +
            @(Find-ByteSequenceOffsets -Bytes $exportBytes -Sequence $find).Count
        $writtenDestinationCount =
            @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $replace).Count +
            @(Find-ByteSequenceOffsets -Bytes $exportBytes -Sequence $replace).Count
        if ($remainingSourceCount -ne 0 -or $writtenDestinationCount -ne 1) {
            throw "DroppedActor patch postcondition failed for identity part $patchIndex in $AssetName."
        }
    }

    [IO.File]::WriteAllBytes((Join-Path $DestinationDirectory ($AssetName + '.uasset')), $bytes)
    [IO.File]::WriteAllBytes((Join-Path $DestinationDirectory ($AssetName + '.uexp')), $exportBytes)
}

Copy-PatchedAsset `
    -SourceDirectory $originalDirectory `
    -AssetName 'DA_Item_Module_GyroCopter' `
    -DestinationDirectory $itemStage `
    -SourceValues @($stockDroppedActorPackage, $stockDroppedActorObject) `
    -DestinationValues @($modDroppedActorPackage, $modDroppedActorObject)
Copy-Item -LiteralPath $scriptObjectsPath -Destination (Join-Path $stage 'scriptobjects.bin')

$utoc = Join-Path $package ($ContainerName + '.utoc')
& $retocPath to-zen --version $retocCompatibilityVersion $stage $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc verify failed with exit code $LASTEXITCODE"
}
$expectedAssetPaths = @(
    '../../../Voyage/Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset'
    '../../../Voyage/Content/Game/Input/Vehicle/IMC_GyroCopter_Keyboard.uasset'
    '../../../Voyage/Content/Mods/GyroKeyboardControl/IAV_GyroPitchReset.uasset'
    '../../../Voyage/Content/Mods/GyroKeyboardControl/ModActor.uasset'
    '../../../Voyage/Content/Mods/GyroKeyboardBP/BP_GyroCopter_KeyboardCtl.uasset'
)
$inventory = @(& $retocPath list --path --size --hash --package $utoc)
if ($LASTEXITCODE -ne 0) {
    throw "retoc list failed with exit code $LASTEXITCODE"
}
$inventoryRecords = @(
    $inventory | ForEach-Object {
        if ($_ -match '^\S+\s+(?<chunk>\S+)\s+(?<hash>\S+)\s+(?<package>\S+)\s+' +
            '\S+\s+(?<size>\d+)\s+(?<path>\.\./\.\./\.\./Voyage/Content/.+\.uasset)$') {
            [pscustomobject]@{
                Path = $Matches.path
                Size = [int64]$Matches.size
                Hash = $Matches.hash
                PackageId = $Matches.package
                ChunkId = $Matches.chunk
            }
        }
    }
)
$actualAssetPaths = @($inventoryRecords.Path | Sort-Object)
$inventoryDifference = @(Compare-Object ($expectedAssetPaths | Sort-Object) $actualAssetPaths)
if ($inventoryRecords.Count -ne $expectedAssetPaths.Count -or $inventoryDifference.Count -ne 0) {
    throw 'Packaged asset inventory does not match the exact GyroKeyboard production set.'
}
$inventoryPath = Join-Path $package ($ContainerName + '.inventory.txt')
$inventoryLines = @(
    $inventoryRecords |
        Sort-Object Path |
        ForEach-Object {
            '{0}|{1}|{2}|{3}|{4}' -f $_.Path, $_.Size, $_.Hash, $_.PackageId, $_.ChunkId
        }
)
[IO.File]::WriteAllLines($inventoryPath, $inventoryLines)

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

Write-Host 'Autonomous GyroKeyboard control package built successfully.'
Get-FileHash -Algorithm SHA256 -LiteralPath $outputs |
    Select-Object Path, Hash |
    Format-Table -AutoSize
