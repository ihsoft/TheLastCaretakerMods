# HAND-WRITTEN BUILD TOOL with game-derived relocation contracts validated for
# Steam build 25056839.
# Revalidate per GAME_DERIVED_SOURCES.md after every game update.

param(
    [string]$CookedRoot = "$PSScriptRoot\Saved\Cooked\Windows\Voyage",

    [Parameter(Mandatory = $true)]
    [string]$OriginalForkliftDirectory,

    [Parameter(Mandatory = $true)]
    [string]$ScriptObjects,

    [Parameter(Mandatory = $true)]
    [string]$Mappings,

    [string]$Retoc,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'DonkLiftKeyboardControl_P',

    [Parameter(Mandatory = $true)][string]$ActualSteamBuildId
)

$ErrorActionPreference = 'Stop'

$sourceRetocEngineVersion = 'UE5_8'
$targetRetocEngineVersion = 'UE5_7'
$expectedForkliftFilter = 'Vehicles/BP_Forklift_Possesable'
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}

$originalPackage = '/Game/Blueprints/Vehicles/BP_Forklift_Possesable'
$renamedPackage = '/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original'
if ($originalPackage.Length -ne $renamedPackage.Length) {
    throw 'The original and renamed package paths must have identical byte lengths.'
}
$cooked = (Resolve-Path -LiteralPath $CookedRoot).Path
$originalDirectory = (Resolve-Path -LiteralPath $OriginalForkliftDirectory).Path
$scriptObjectsPath = (Resolve-Path -LiteralPath $ScriptObjects).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
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
    if ([string]$manifest.retocEngineVersion -cne $sourceRetocEngineVersion) {
        throw "$Label extraction does not match the selected retoc compatibility contract."
    }
    if ([string]$manifest.steamBuildId -cne $ActualSteamBuildId) {
        throw "$Label extraction fingerprint does not match the installed game."
    }

    return [pscustomobject]@{
        Root = Split-Path -Parent $manifestPath
        ManifestPath = $manifestPath
        Manifest = $manifest
    }
}

$forkliftEvidence = Get-ExtractionEvidence `
    -InputPath $originalDirectory `
    -ExpectedFilter $expectedForkliftFilter `
    -Label 'Forklift original'
$scriptObjectsEvidence = Get-ExtractionEvidence `
    -InputPath $scriptObjectsPath `
    -ExpectedFilter $expectedForkliftFilter `
    -Label 'scriptobjects.bin'
if ($scriptObjectsEvidence.ManifestPath -cne $forkliftEvidence.ManifestPath) {
    throw 'scriptobjects.bin must come from the same extraction root as the forklift original.'
}

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
$childStage = Join-Path $stage 'Voyage\Content\Blueprints\Vehicles'
$helperStage = Join-Path $stage 'Voyage\Content\Mods\DonkLiftKeyboardControl'
$contextStage = Join-Path $stage 'Voyage\Content\Game\Input\Vehicle'
New-Item -ItemType Directory -Path $childStage, $helperStage, $contextStage, $package -Force | Out-Null

$childSource = Join-Path $cooked 'Content\Blueprints\Vehicles'
$helperSource = Join-Path $cooked 'Content\Mods\DonkLiftKeyboardControl'
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
    $contextSource = Join-Path (Join-Path $cooked 'Content\Game\Input\Vehicle') $assetName
    Copy-Item -LiteralPath $contextSource -Destination $contextStage
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
    $exportBytes = [IO.File]::ReadAllBytes($sourceUexp)
    $find = [Text.Encoding]::ASCII.GetBytes($SourcePackageName)
    $replace = [Text.Encoding]::ASCII.GetBytes($DestinationPackageName)
    $preexistingDestinationMatches = @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $replace)
    $preexistingExportDestinationMatches = @(Find-ByteSequenceOffsets -Bytes $exportBytes -Sequence $replace)
    if ($preexistingDestinationMatches.Count -ne 0 -or
        $preexistingExportDestinationMatches.Count -ne 0) {
        throw "$SourceAssetName already references its relocated parent path " +
            "($DestinationPackageName). It is probably a generated replacement " +
            'shadowing the original game asset; relocating it would create a self-parent cycle.'
    }
    $matches = @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $find)
    if ($matches.Count -ne 2) {
        throw "Expected exactly two embedded package paths in $SourceAssetName; found $($matches.Count)."
    }
    foreach ($offset in $matches) {
        [Array]::Copy($replace, 0, $bytes, $offset, $replace.Length)
    }
    if (@(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $find).Count -ne 0 -or
        @(Find-ByteSequenceOffsets -Bytes $bytes -Sequence $replace).Count -ne 2) {
        throw "Relocation postcondition failed for $SourceAssetName."
    }

    [IO.File]::WriteAllBytes((Join-Path $DestinationDirectory ($DestinationAssetName + '.uasset')), $bytes)
    Copy-Item -LiteralPath $sourceUexp -Destination (Join-Path $DestinationDirectory ($DestinationAssetName + '.uexp'))
}

$assemblerInput = Join-Path $output 'assembler-input'
$donorParentDirectory = Join-Path $assemblerInput 'Voyage\Content\Mods\DonkLiftKeyboard'
New-Item -ItemType Directory -Path $donorParentDirectory -Force | Out-Null
Copy-RelocatedAsset `
    -SourceDirectory $originalDirectory `
    -SourceAssetName 'BP_Forklift_Possesable' `
    -DestinationDirectory $donorParentDirectory `
    -DestinationAssetName 'BP_Forklift_Original' `
    -SourcePackageName $originalPackage `
    -DestinationPackageName $renamedPackage

$assemblerProject = Join-Path $modRoot 'Tools\DonkLiftPackageAssembler\DonkLiftPackageAssembler.csproj'
$assemblerTool = Join-Path $output 'assembler-tool'
$assemblyReport = Join-Path $output 'assembly-report.json'
$stockAsset = Join-Path $originalDirectory 'BP_Forklift_Possesable.uasset'
$donorAsset = Join-Path $childSource 'BP_Forklift_Possesable.uasset'
$donorParentAsset = Join-Path $donorParentDirectory 'BP_Forklift_Original.uasset'
$mappingsPath = (Resolve-Path -LiteralPath $Mappings).Path
& dotnet build $assemblerProject --configuration Release --output $assemblerTool
if ($LASTEXITCODE -ne 0) {
    throw "DonkLift package assembler build failed with exit code $LASTEXITCODE."
}
& dotnet (Join-Path $assemblerTool 'DonkLiftPackageAssembler.dll') `
    --stock $stockAsset `
    --donor $donorAsset `
    --donor-parent $donorParentAsset `
    --mappings $mappingsPath `
    --output (Join-Path $childStage 'BP_Forklift_Possesable.uasset') `
    --report $assemblyReport
if ($LASTEXITCODE -ne 0) {
    throw "DonkLift package assembly failed with exit code $LASTEXITCODE."
}
Remove-Item -LiteralPath $assemblerInput, $assemblerTool -Recurse -Force
Copy-Item -LiteralPath $scriptObjectsPath -Destination (Join-Path $stage 'scriptobjects.bin')

$utoc = Join-Path $package ($ContainerName + '.utoc')
& $retocPath to-zen --version $targetRetocEngineVersion $stage $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc verify failed with exit code $LASTEXITCODE"
}
$expectedAssetPaths = @(
    '../../../Voyage/Content/Blueprints/Vehicles/BP_Forklift_Possesable.uasset'
    '../../../Voyage/Content/Game/Input/Vehicle/IMC_Forklift_Keyboard.uasset'
    '../../../Voyage/Content/Mods/DonkLiftKeyboardControl/IAV_DonkLiftBrake.uasset'
    '../../../Voyage/Content/Mods/DonkLiftKeyboardControl/IAV_DonkLiftCenterSteering.uasset'
    '../../../Voyage/Content/Mods/DonkLiftKeyboardControl/ModActor.uasset'
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
    throw 'Packaged asset inventory does not match the exact DonkLift production set.'
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

Write-Host 'Autonomous DonkLift control package built successfully.'
$outputs | ForEach-Object { Get-Item -LiteralPath $_ } |
    Select-Object FullName, Length | Format-Table -AutoSize
