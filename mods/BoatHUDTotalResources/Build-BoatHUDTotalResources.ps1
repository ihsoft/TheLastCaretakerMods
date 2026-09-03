# HAND-WRITTEN BUILD TOOL with game-derived contracts revalidated for Steam
# build 25056839, VoyageSteam-Win64-Shipping.exe SHA-256
# CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.

param(
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc = "$PSScriptRoot\..\..\.tools\bin\retoc.exe",

    [Parameter(Mandatory = $true)]
    [string]$OriginalsRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'BoatHUDTotalResources_P'
)

$ErrorActionPreference = 'Stop'

$expectedSteamBuildId = '25056839'
$expectedExecutableSha256 = 'CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933'
$expectedGameEngineVersion = '5.8.1'
$expectedEditorEngineVersion = '5.8.2'
$retocCompatibilityVersion = 'UE5_7'
$cookStorage = 'LooseCookedPackageWriter'
$expectedFilter = 'UI/Game/HUD/BP_VoyageIngameBoatHud'
$sourcePackage = '/Game/UI/Game/HUD/BP_VoyageIngameBoatHud'
$relocatedPackage = '/Game/Mods/Boat/BP_VoyageIngameBoatHud_O'
$dieselReplacementPackage = '/Game/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalDieselValue'
$electricityReplacementPackage = '/Game/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalElectricityValue'
$assetName = 'BP_VoyageIngameBoatHud'
$relocatedAssetName = 'BP_VoyageIngameBoatHud_O'
$dieselReplacementAssetName = 'WBP_BoatHUDTotalDieselValue'
$electricityReplacementAssetName = 'WBP_BoatHUDTotalElectricityValue'
if ($sourcePackage.Length -ne $relocatedPackage.Length) {
    throw 'The source and relocated Boat HUD package paths must have identical byte lengths.'
}

$projectRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$project = Join-Path $projectRoot 'Voyage.uproject'
$buildBatch = (Resolve-Path -LiteralPath (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat')).Path
$editor = (Resolve-Path -LiteralPath (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe')).Path
$engineBuildVersionPath = (Resolve-Path -LiteralPath (Join-Path $EngineRoot 'Engine\Build\Build.version')).Path
$engineBuildVersion = Get-Content -LiteralPath $engineBuildVersionPath -Raw | ConvertFrom-Json
$editorEngineVersion = '{0}.{1}.{2}' -f @(
    $engineBuildVersion.MajorVersion,
    $engineBuildVersion.MinorVersion,
    $engineBuildVersion.PatchVersion)
if ($editorEngineVersion -cne $expectedEditorEngineVersion) {
    throw "Unreal Editor version must be exactly $expectedEditorEngineVersion; found $editorEngineVersion."
}
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$retocSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $retocPath).Hash
$originals = (Resolve-Path -LiteralPath $OriginalsRoot).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

$fingerprintPath = Join-Path $output 'game-fingerprint.json'
$fingerprintTool = Join-Path $projectRoot '..\..\tools\Get-VoyageBuildFingerprint.ps1'
& $fingerprintTool -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne $expectedSteamBuildId -or
    [string]$fingerprint.executable.sha256 -cne $expectedExecutableSha256) {
    throw 'Installed game fingerprint does not match the BoatHUDTotalResources contracts.'
}

$manifests = @(Get-ChildItem -LiteralPath $originals -File -Recurse -Filter 'extraction-manifest.json')
if ($manifests.Count -ne 1) {
    throw "Expected exactly one extraction manifest; found $($manifests.Count)."
}
$extractionManifest = Get-Content -LiteralPath $manifests[0].FullName -Raw | ConvertFrom-Json
if ([string]$extractionManifest.filter -cne $expectedFilter -or
    $extractionManifest.allowAdditionalContainers -ne $false -or
    [string]$extractionManifest.retocEngineVersion -cne $retocCompatibilityVersion -or
    [string]$extractionManifest.retocSha256 -cne $retocSha256 -or
    [string]$extractionManifest.steamBuildId -cne $expectedSteamBuildId -or
    [string]$extractionManifest.executableSha256 -cne $expectedExecutableSha256) {
    throw 'Original Boat HUD extraction does not satisfy the clean current-build gate.'
}

$originalUassets = @(Get-ChildItem -LiteralPath $originals -File -Recurse -Filter "$assetName.uasset")
$originalUexps = @(Get-ChildItem -LiteralPath $originals -File -Recurse -Filter "$assetName.uexp")
$scriptObjectsFiles = @(Get-ChildItem -LiteralPath $originals -File -Recurse -Filter 'scriptobjects.bin')
if ($originalUassets.Count -ne 1 -or $originalUexps.Count -ne 1 -or $scriptObjectsFiles.Count -ne 1) {
    throw 'The original extraction must contain exactly one Boat HUD uasset, uexp, and scriptobjects.bin.'
}

$generatedContent = Join-Path $projectRoot 'Content'
if (Test-Path -LiteralPath $generatedContent) {
    $resolvedGeneratedContent = (Resolve-Path -LiteralPath $generatedContent).Path
    if (-not $resolvedGeneratedContent.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar)) {
        throw "Refusing to clean Content outside the BoatHUDTotalResources project: $resolvedGeneratedContent"
    }
    Remove-Item -LiteralPath $resolvedGeneratedContent -Recurse -Force
}

$buildLog = Join-Path $output 'editor-build.log'
$buildArguments = @(
    'VoyageEditor',
    'Win64',
    'Development',
    ('-Project="{0}"' -f $project),
    ('-Log="{0}"' -f (Join-Path $output 'unreal-build-tool.log')),
    '-WaitMutex',
    '-NoHotReload',
    '-NoUBA'
)
$buildProcess = Start-Process `
    -FilePath $buildBatch `
    -ArgumentList $buildArguments `
    -Wait `
    -PassThru `
    -WindowStyle Hidden `
    -RedirectStandardOutput $buildLog `
    -RedirectStandardError (Join-Path $output 'editor-build-errors.log')
if ($buildProcess.ExitCode -ne 0) {
    throw "VoyageEditor build failed with exit code $($buildProcess.ExitCode)."
}

$ddc = Join-Path $projectRoot 'DerivedDataCache'
$generateLog = Join-Path $output 'generate.log'
$generateArguments = @(
    ('"{0}"' -f $project),
    '-run=GenerateBoatHUDTotalResources',
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nullrhi',
    '-ddc=NoZenLocalFallback',
    ('-LocalDataCachePath="{0}"' -f $ddc),
    ('-abslog="{0}"' -f $generateLog)
)
$generateProcess = Start-Process `
    -FilePath $editor `
    -ArgumentList $generateArguments `
    -Wait `
    -PassThru `
    -WindowStyle Hidden
if ($generateProcess.ExitCode -ne 0) {
    throw "BoatHUDTotalResources generation failed with exit code $($generateProcess.ExitCode)."
}

$cookedPlatform = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Saved\Cooked\Windows'))
$expectedCookedPrefix = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Saved\Cooked')) + '\'
if (-not $cookedPlatform.StartsWith($expectedCookedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing unexpected cooked-output path: $cookedPlatform"
}
if (Test-Path -LiteralPath $cookedPlatform) {
    Remove-Item -LiteralPath $cookedPlatform -Recurse -Force
}
$cookedContent = Join-Path $cookedPlatform 'Voyage\Content'
$cookedBase = Join-Path $cookedContent 'UI\Game\HUD\BP_VoyageIngameBoatHud'
$cookedDieselReplacementLive = Join-Path $cookedContent 'Mods\BoatHUDTotalResources\WBP_BoatHUDTotalDieselValue'
$cookedElectricityReplacementLive = Join-Path $cookedContent 'Mods\BoatHUDTotalResources\WBP_BoatHUDTotalElectricityValue'
foreach ($cookedAssetBase in @(
    $cookedBase,
    $cookedDieselReplacementLive,
    $cookedElectricityReplacementLive
)) {
    foreach ($extension in @('.uasset', '.uexp')) {
        $staleCookedFile = $cookedAssetBase + $extension
        if (Test-Path -LiteralPath $staleCookedFile -PathType Leaf) {
            Remove-Item -LiteralPath $staleCookedFile -Force
        }
    }
}

function Invoke-ModCook {
    param(
        [Parameter(Mandatory = $true)] [string]$PackageName,
        [Parameter(Mandatory = $true)] [string]$LogName,
        [Parameter(Mandatory = $true)] [string]$CookedAssetBase,
        [string]$SnapshotDirectory
    )

    $cookLog = Join-Path $output $LogName
    $cookArguments = @(
        ('"{0}"' -f $project),
        '-run=cook',
        '-targetplatform=Windows',
        '-unversioned',
        '-SkipZenStore',
        ("-Package=$PackageName"),
        '-CookSinglePackageNoRefs',
        '-unattended',
        '-nop4',
        '-nosplash',
        '-nullrhi',
        '-ddc=NoZenLocalFallback',
        ('-LocalDataCachePath="{0}"' -f $ddc),
        ('-abslog="{0}"' -f $cookLog)
    )
    $cookProcess = Start-Process `
        -FilePath $editor `
        -ArgumentList $cookArguments `
        -Wait `
        -PassThru `
        -WindowStyle Hidden
    if ($cookProcess.ExitCode -ne 0) {
        throw "BoatHUDTotalResources cook failed for $PackageName with exit code $($cookProcess.ExitCode)."
    }

    foreach ($extension in @('.uasset', '.uexp')) {
        $cookedFile = $CookedAssetBase + $extension
        if (-not (Test-Path -LiteralPath $cookedFile -PathType Leaf)) {
            throw "Expected cooked mod file was not produced: $cookedFile"
        }
        if ($SnapshotDirectory) {
            [IO.Directory]::CreateDirectory($SnapshotDirectory) | Out-Null
            Copy-Item -LiteralPath $cookedFile -Destination $SnapshotDirectory
        }
    }
}

$replacementSnapshotDirectory = Join-Path $output 'cooked-replacement'
$cookedDieselReplacement = Join-Path $replacementSnapshotDirectory $dieselReplacementAssetName
$cookedElectricityReplacement = Join-Path $replacementSnapshotDirectory $electricityReplacementAssetName
Invoke-ModCook `
    -PackageName $dieselReplacementPackage `
    -LogName 'cook-diesel-replacement.log' `
    -CookedAssetBase $cookedDieselReplacementLive `
    -SnapshotDirectory $replacementSnapshotDirectory
Invoke-ModCook `
    -PackageName $electricityReplacementPackage `
    -LogName 'cook-electricity-replacement.log' `
    -CookedAssetBase $cookedElectricityReplacementLive `
    -SnapshotDirectory $replacementSnapshotDirectory
Invoke-ModCook `
    -PackageName $sourcePackage `
    -LogName 'cook-child.log' `
    -CookedAssetBase $cookedBase

foreach ($cookedAssetBase in @(
    $cookedBase,
    $cookedDieselReplacement,
    $cookedElectricityReplacement
)) {
    foreach ($extension in @('.uasset', '.uexp')) {
        if (-not (Test-Path -LiteralPath ($cookedAssetBase + $extension) -PathType Leaf)) {
            throw "Expected cooked mod file was not produced: $cookedAssetBase$extension"
        }
    }
}
$cookedChildNameTable = [Text.Encoding]::ASCII.GetString(
    [IO.File]::ReadAllBytes($cookedBase + '.uasset'))
if (-not $cookedChildNameTable.Contains('PreConstruct') -or
    -not $cookedChildNameTable.Contains($dieselReplacementAssetName) -or
    -not $cookedChildNameTable.Contains($electricityReplacementAssetName) -or
    -not $cookedChildNameTable.Contains('SetVisibility') -or
    -not $cookedChildNameTable.Contains('SetFont') -or
    -not $cookedChildNameTable.Contains('SetColorAndOpacity') -or
    -not $cookedChildNameTable.Contains('SetTextTransformPolicy') -or
    -not $cookedChildNameTable.Contains('AddChild') -or
    $cookedChildNameTable.Contains('K2Node_Event_1')) {
    throw 'Cooked child does not contain the checked replacement-slot PreConstruct graph.'
}
$replacementContracts = @(
    [pscustomobject]@{
        Path = $cookedDieselReplacement
        TextName = 'DieselValueText'
        ResourceName = 'Diesel'
    },
    [pscustomobject]@{
        Path = $cookedElectricityReplacement
        TextName = 'ElectricityValueText'
        ResourceName = 'Electricity'
    }
)
foreach ($contract in $replacementContracts) {
    $replacementNameTable = [Text.Encoding]::ASCII.GetString(
        [IO.File]::ReadAllBytes($contract.Path + '.uasset'))
    foreach ($requiredName in @(
        $contract.TextName,
        'Tick',
        'GetModulesInSameGrid',
        'GetResourceAmount',
        'SetText',
        $contract.ResourceName
    )) {
        if (-not $replacementNameTable.Contains($requiredName)) {
            throw "Cooked replacement widget is missing aggregation contract: $requiredName"
        }
    }
}

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
$childStage = Join-Path $stage 'Voyage\Content\UI\Game\HUD'
$parentStage = Join-Path $stage 'Voyage\Content\Mods\Boat'
$replacementStage = Join-Path $stage 'Voyage\Content\Mods\BoatHUDTotalResources'
[IO.Directory]::CreateDirectory($childStage) | Out-Null
[IO.Directory]::CreateDirectory($parentStage) | Out-Null
[IO.Directory]::CreateDirectory($replacementStage) | Out-Null
[IO.Directory]::CreateDirectory($package) | Out-Null
Copy-Item -LiteralPath ($cookedBase + '.uasset'), ($cookedBase + '.uexp') -Destination $childStage
Copy-Item -LiteralPath `
    ($cookedDieselReplacement + '.uasset'), `
    ($cookedDieselReplacement + '.uexp'), `
    ($cookedElectricityReplacement + '.uasset'), `
    ($cookedElectricityReplacement + '.uexp') `
    -Destination $replacementStage
Copy-Item -LiteralPath $scriptObjectsFiles[0].FullName -Destination (Join-Path $stage 'scriptobjects.bin')

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

$originalBytes = [IO.File]::ReadAllBytes($originalUassets[0].FullName)
$originalExportBytes = [IO.File]::ReadAllBytes($originalUexps[0].FullName)
$sourceBytes = [Text.Encoding]::ASCII.GetBytes($sourcePackage)
$relocatedBytes = [Text.Encoding]::ASCII.GetBytes($relocatedPackage)
$sourceMatches = @(Find-ByteSequenceOffsets -Bytes $originalBytes -Sequence $sourceBytes)
$sourceExportMatches = @(Find-ByteSequenceOffsets -Bytes $originalExportBytes -Sequence $sourceBytes)
$preexistingRelocatedMatches = @(Find-ByteSequenceOffsets -Bytes $originalBytes -Sequence $relocatedBytes)
$preexistingRelocatedExportMatches = @(
    Find-ByteSequenceOffsets -Bytes $originalExportBytes -Sequence $relocatedBytes
)
if ($sourceMatches.Count -ne 2 -or $sourceExportMatches.Count -ne 0 -or
    $preexistingRelocatedMatches.Count -ne 0 -or $preexistingRelocatedExportMatches.Count -ne 0) {
    throw 'Original Boat HUD relocation assertions no longer match the validated package.'
}
foreach ($offset in $sourceMatches) {
    [Array]::Copy($relocatedBytes, 0, $originalBytes, $offset, $relocatedBytes.Length)
}
if (@(Find-ByteSequenceOffsets -Bytes $originalBytes -Sequence $sourceBytes).Count -ne 0 -or
    @(Find-ByteSequenceOffsets -Bytes $originalBytes -Sequence $relocatedBytes).Count -ne 2) {
    throw 'Boat HUD relocation postcondition failed.'
}
[IO.File]::WriteAllBytes(
    (Join-Path $parentStage "$relocatedAssetName.uasset"),
    $originalBytes)
Copy-Item -LiteralPath $originalUexps[0].FullName -Destination (
    Join-Path $parentStage "$relocatedAssetName.uexp")

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
    '../../../Voyage/Content/Mods/Boat/BP_VoyageIngameBoatHud_O.uasset'
    '../../../Voyage/Content/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalDieselValue.uasset'
    '../../../Voyage/Content/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalElectricityValue.uasset'
    '../../../Voyage/Content/UI/Game/HUD/BP_VoyageIngameBoatHud.uasset'
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
    throw 'Packaged asset inventory does not match the exact four-asset mod set.'
}
$inventoryRecords | Sort-Object Path | ForEach-Object {
    '{0}|{1}|{2}|{3}|{4}' -f $_.Path, $_.Size, $_.Hash, $_.PackageId, $_.ChunkId
} | Set-Content -LiteralPath (Join-Path $package ($ContainerName + '.inventory.txt'))

$containerFiles = @(
    Join-Path $package ($ContainerName + '.pak')
    Join-Path $package ($ContainerName + '.ucas')
    $utoc
)
foreach ($containerFile in $containerFiles) {
    if (-not (Test-Path -LiteralPath $containerFile -PathType Leaf)) {
        throw "Expected container file was not produced: $containerFile"
    }
}
$containerHashes = @(
    Get-FileHash -Algorithm SHA256 -LiteralPath $containerFiles |
        ForEach-Object { [ordered]@{ path = $_.Path; sha256 = $_.Hash } }
)
$buildManifest = [ordered]@{
    kind = 'BoatHUDTotalResources mod'
    mountEvent = 'UUserWidget::PreConstruct override'
    stockPetrolVisibility = 'Collapsed'
    stockBatteryVisibility = 'Collapsed'
    dieselReplacementPackage = $dieselReplacementPackage
    electricityReplacementPackage = $electricityReplacementPackage
    aggregationResources = @(
        [ordered]@{
            name = 'Diesel'
            value = 16
            divisor = 1
            fractionalDigits = 0
            suffix = ' L'
        },
        [ordered]@{
            name = 'Electricity'
            value = 0
            divisor = 1000
            fractionalDigits = 1
            suffix = ' KWH'
        }
    )
    styleSource = 'Corresponding stock TextBlock at PreConstruct'
    styleProperties = @('Font', 'ColorAndOpacity', 'TextTransformPolicy')
    engineVersion = $expectedGameEngineVersion
    editorEngineVersion = $editorEngineVersion
    retocCompatibilityVersion = $retocCompatibilityVersion
    retocSha256 = $retocSha256
    legacyImportPackageNameLayout = 'SerializedForFilteredCookedPackages'
    cookStorage = $cookStorage
    steamBuildId = $expectedSteamBuildId
    executableSha256 = $expectedExecutableSha256
    originalExtractionManifest = $manifests[0].FullName
    originalAssetSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $originalUassets[0].FullName).Hash
    containerFiles = $containerHashes
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $package 'build-manifest.json'),
    (($buildManifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine))

Write-Host 'BoatHUDTotalResources mod built successfully.'
Get-FileHash -Algorithm SHA256 -LiteralPath $containerFiles |
    Select-Object Path, Hash |
    Format-Table -AutoSize
