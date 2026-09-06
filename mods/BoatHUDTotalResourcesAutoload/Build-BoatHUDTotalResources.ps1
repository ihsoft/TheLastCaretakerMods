# HAND-WRITTEN BUILD TOOL with game-derived contracts revalidated for Steam
# build 25056839, VoyageSteam-Win64-Shipping.exe SHA-256
# CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.

param(
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc,

    [Parameter(Mandatory = $true)]
    [string]$OriginalsRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'BoatHUDTotalResources'
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $PSScriptRoot '..\..\.tools\bin\retoc.exe'
}

$expectedSteamBuildId = '25056839'
$expectedExecutableSha256 = 'CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933'
$expectedGameEngineVersion = '5.8.1'
$expectedEditorEngineVersion = '5.8.2'
$retocCompatibilityVersion = 'UE5_7'
$cookStorage = 'LooseCookedPackageWriter'
$expectedFilter = 'UI/Game/HUD/BP_VoyageIngameBoatHud'
$stockParentPackage = '/Game/UI/Game/HUD/BP_VoyageIngameBoatHud'
$temporaryParentPackage = '/Game/Mods/Boat/BP_VoyageIngameBoatHud_O'
$childPackage = '/Game/Mods/BoatHUDTotalResources/BP_BoatHUDTotalResources'
$swapActorPackage = '/Game/Mods/BoatHUDTotalResources/ModActor'
$dieselReplacementPackage = '/Game/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalDieselValue'
$electricityReplacementPackage = '/Game/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalElectricityValue'
$childAssetName = 'BP_BoatHUDTotalResources'
$swapActorAssetName = 'ModActor'
$dieselReplacementAssetName = 'WBP_BoatHUDTotalDieselValue'
$electricityReplacementAssetName = 'WBP_BoatHUDTotalElectricityValue'
$autoloadFileName = $ContainerName + '.autoload'
if ($stockParentPackage.Length -ne $temporaryParentPackage.Length) {
    throw 'The stock and temporary Boat HUD parent paths must have identical byte lengths.'
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
$cookedChildLive = Join-Path $cookedContent 'Mods\BoatHUDTotalResources\BP_BoatHUDTotalResources'
$cookedDieselReplacementLive = Join-Path $cookedContent 'Mods\BoatHUDTotalResources\WBP_BoatHUDTotalDieselValue'
$cookedElectricityReplacementLive = Join-Path $cookedContent 'Mods\BoatHUDTotalResources\WBP_BoatHUDTotalElectricityValue'
$cookedSwapActorLive = Join-Path $cookedContent 'Mods\BoatHUDTotalResources\ModActor'
foreach ($cookedAssetBase in @(
    $cookedChildLive,
    $cookedDieselReplacementLive,
    $cookedElectricityReplacementLive,
    $cookedSwapActorLive
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
$cookedChild = Join-Path $replacementSnapshotDirectory $childAssetName
$cookedSwapActor = Join-Path $replacementSnapshotDirectory $swapActorAssetName
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
    -PackageName $childPackage `
    -LogName 'cook-child.log' `
    -CookedAssetBase $cookedChildLive `
    -SnapshotDirectory $replacementSnapshotDirectory
Invoke-ModCook `
    -PackageName $swapActorPackage `
    -LogName 'cook-swap-actor.log' `
    -CookedAssetBase $cookedSwapActorLive `
    -SnapshotDirectory $replacementSnapshotDirectory

foreach ($cookedAssetBase in @(
    $cookedChild,
    $cookedDieselReplacement,
    $cookedElectricityReplacement,
    $cookedSwapActor
)) {
    foreach ($extension in @('.uasset', '.uexp')) {
        if (-not (Test-Path -LiteralPath ($cookedAssetBase + $extension) -PathType Leaf)) {
            throw "Expected cooked mod file was not produced: $cookedAssetBase$extension"
        }
    }
}

function Replace-ExactAsciiPath {
    param(
        [Parameter(Mandatory = $true)] [string]$AssetBase,
        [Parameter(Mandatory = $true)] [string]$OldPath,
        [Parameter(Mandatory = $true)] [string]$NewPath
    )

    $oldBytes = [Text.Encoding]::ASCII.GetBytes($OldPath)
    $newBytes = [Text.Encoding]::ASCII.GetBytes($NewPath)
    if ($oldBytes.Length -ne $newBytes.Length) {
        throw "Cannot replace unequal-length package paths in $AssetBase."
    }

    $replacementCount = 0
    foreach ($extension in @('.uasset', '.uexp')) {
        $filePath = $AssetBase + $extension
        $bytes = [IO.File]::ReadAllBytes($filePath)
        for ($offset = 0; $offset -le $bytes.Length - $oldBytes.Length; $offset++) {
            $matches = $true
            for ($index = 0; $index -lt $oldBytes.Length; $index++) {
                if ($bytes[$offset + $index] -ne $oldBytes[$index]) {
                    $matches = $false
                    break
                }
            }
            if ($matches) {
                [Array]::Copy($newBytes, 0, $bytes, $offset, $newBytes.Length)
                $replacementCount++
                $offset += $oldBytes.Length - 1
            }
        }
        [IO.File]::WriteAllBytes($filePath, $bytes)
    }
    if ($replacementCount -lt 1) {
        throw "Cooked asset does not reference the temporary parent path: $AssetBase"
    }
}

Replace-ExactAsciiPath `
    -AssetBase $cookedChild `
    -OldPath $temporaryParentPackage `
    -NewPath $stockParentPackage

$cookedChildNameTable = [Text.Encoding]::ASCII.GetString(
    [IO.File]::ReadAllBytes($cookedChild + '.uasset'))
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
$replacementStage = Join-Path $stage 'Voyage\Content\Mods\BoatHUDTotalResources'
[IO.Directory]::CreateDirectory($replacementStage) | Out-Null
[IO.Directory]::CreateDirectory($package) | Out-Null
$stagedAssets = @(
    ($cookedChild + '.uasset'),
    ($cookedChild + '.uexp'),
    ($cookedSwapActor + '.uasset'),
    ($cookedSwapActor + '.uexp'),
    ($cookedDieselReplacement + '.uasset'),
    ($cookedDieselReplacement + '.uexp'),
    ($cookedElectricityReplacement + '.uasset'),
    ($cookedElectricityReplacement + '.uexp')
)
Copy-Item -LiteralPath $stagedAssets -Destination $replacementStage

$cookedSwapActorNameTable = [Text.Encoding]::ASCII.GetString(
    [IO.File]::ReadAllBytes($cookedSwapActor + '.uasset'))
if (-not $cookedSwapActorNameTable.Contains('ReceiveBeginPlay') -or
    -not $cookedSwapActorNameTable.Contains('GetPlayerController') -or
    -not $cookedSwapActorNameTable.Contains('GetHUD') -or
    -not $cookedSwapActorNameTable.Contains('CharacterWidgetClasses') -or
    -not $cookedSwapActorNameTable.Contains('Map_Add') -or
    -not $cookedSwapActorNameTable.Contains('VoyageBoatPawn')) {
    throw 'Cooked swap actor is missing the native HUD class-map replacement graph.'
}
foreach ($forbiddenSwapOperation in @('GetAllWidgetsOfClass', 'RemoveFromParent', 'AddToViewport')) {
    if ($cookedSwapActorNameTable.Contains($forbiddenSwapOperation)) {
        throw "Cooked swap actor still contains obsolete viewport operation: $forbiddenSwapOperation"
    }
}

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
    '../../../Voyage/Content/Mods/BoatHUDTotalResources/BP_BoatHUDTotalResources.uasset'
    '../../../Voyage/Content/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalDieselValue.uasset'
    '../../../Voyage/Content/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalElectricityValue.uasset'
    '../../../Voyage/Content/Mods/BoatHUDTotalResources/ModActor.uasset'
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
    mountEvent = 'AActor::BeginPlay updates VoyageHUD.CharacterWidgetClasses'
    childBlueprintPackage = $childPackage
    swapActorBlueprintPackage = $swapActorPackage
    childBlueprintAsset = $childAssetName
    swapActorAsset = $swapActorAssetName
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
    containerFiles = $containerHashes
    autoloadFile = $autoloadFileName
    autoloadEntryClass = '/Game/Mods/BoatHUDTotalResources/ModActor.ModActor_C'
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$autoloadPath = Join-Path $package $autoloadFileName
$autoloadText = 'entryClass: /Game/Mods/BoatHUDTotalResources/ModActor.ModActor_C' + [Environment]::NewLine
[IO.File]::WriteAllText($autoloadPath, $autoloadText)
[IO.File]::WriteAllText(
    (Join-Path $package 'build-manifest.json'),
    (($buildManifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine))

Write-Host 'BoatHUDTotalResources mod built successfully.'
Get-FileHash -Algorithm SHA256 -LiteralPath $containerFiles |
    Select-Object Path, Hash |
    Format-Table -AutoSize
