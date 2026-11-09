# HAND-WRITTEN RELEASE PRODUCER: uses the maintained plate without extraction.
# Refreshes the item when needed, cooks, packages, verifies, archives, and
# optionally installs StableGyro. Output stays
# in ignored local storage. It never publishes externally.

[CmdletBinding()]
param(
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]*$')]
    [string]$Version,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EngineRoot = 'P:\UnrealEngine\UE_5.8',

    [string]$Retoc,

    [string]$OutputRoot,

    [string]$OriginalsRoot,

    [switch]$Install,

    [switch]$ReimportItemAsset,

    [switch]$IgnorePinnedGameVersion
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$expectedGameEngineVersion = '5.8'
$expectedEditorEngineVersion = '5.8.2'
$retocCompatibilityVersion = 'UE5_8'
$cookStorage = 'LooseCookedPackageWriter'

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$publishedVersionText = (Get-Content -LiteralPath (Join-Path $modRoot 'version.txt') -Raw).Trim()
if ($publishedVersionText -cnotmatch '^(0|[1-9][0-9]*)$') {
    throw 'StableGyro published version must be a non-negative integer in version.txt.'
}
$candidateModVersion = 'V' + ([int]$publishedVersionText + 1)
if (-not [string]::IsNullOrWhiteSpace($Version) -and $Version -cne $candidateModVersion) {
    throw "Version is derived from version.txt and must be $candidateModVersion. Use OutputRoot to label a temporary run."
}
$Version = $candidateModVersion
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}
$artifactsRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'artifacts'))
$tmpRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'Tmp'))
$tmpOwnerRoot = Join-Path $tmpRoot 'StableGyro'
$project = Join-Path $modRoot 'Voyage.uproject'
$provenancePath = Join-Path $modRoot 'GAME_DERIVED_SOURCES.md'
$unrealEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$engineVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
$fingerprinter = Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1'
$cookScript = Join-Path $modRoot 'Cook-StableGyroAssets.ps1'
$prepareScript = Join-Path $modRoot 'Prepare-StableGyroOriginals.ps1'
$packageScript = Join-Path $modRoot 'Build-InheritancePackage.ps1'
$settingsSchema = Join-Path $modRoot 'Settings\StableGyro.settings.json'
$settingsDefaults = Join-Path $modRoot 'Assets\StableGyro.ini'
$settingsModule = Join-Path $repoRoot 'tools\VoyageModSettings\VoyageModSettings.psm1'
$releaseManifestBuilder = Join-Path $repoRoot 'tools\New-VoyageReleaseManifest.ps1'
$releaseInstaller = Join-Path $repoRoot 'tools\Install-VoyageRelease.ps1'
$meshSourceStatePath = Join-Path $modRoot 'Build\MooringMeshes.source.json'
Import-Module (Join-Path $modRoot 'Build\MooringMeshRefresh.psm1') -Force
$itemSourceStatePath = Join-Path $modRoot 'Build\Item.source.json'
$itemRecipePath = Join-Path $modRoot 'Build\Item.patch.json'
Import-Module (Join-Path $modRoot 'Build\ItemAssetRefresh.psm1') -Force
$containerName = 'StableGyro_P'
$payloadNames = @(
    "$containerName.pak",
    "$containerName.ucas",
    "$containerName.utoc"
)
function Resolve-RequiredPath {
    param([Parameter(Mandatory = $true)] [string]$Path, [Parameter(Mandatory = $true)] [string]$Label)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Label was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Assert-UnderIgnoredBuildStorage {
    param([Parameter(Mandatory = $true)] [string]$Path, [Parameter(Mandatory = $true)] [string]$Label)

    $fullPath = [IO.Path]::GetFullPath($Path)
    $allowedRoots = @($tmpRoot, $artifactsRoot)
    $isAllowed = $false
    foreach ($allowedRoot in $allowedRoots) {
        $prefix = $allowedRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) +
            [IO.Path]::DirectorySeparatorChar
        if ($fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            $isAllowed = $true
            break
        }
    }
    if (-not $isAllowed) {
        throw "$Label must stay below the repository Tmp or artifacts directory: $fullPath"
    }
    return $fullPath
}

function Assert-GameClosed {
    $running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
    if ($running.Count -gt 0) {
        $names = ($running | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ', '
        throw "Voyage must be closed for this phase: $names"
    }
}

function Update-StableGyroSettings {
    param([string]$TemplatePath, [string]$SettingsPath)

    $retiredKeys = @(
        'AltitudeStabilizationDelaySeconds',
        'CompensateTiltLift',
        'TiltLiftCompensationMultiplier',
        'StabilizationMinimumRotorSpeedPercent',
        'EnableFuelDiagnostics',
        'FuelConsumptionPerMinute'
    )
    $retiredComments = @(
        '# Absolute delay after releasing Space at full throttle before exact altitude hold activates.',
        '# A release below full throttle is ignored. Set to 0 for immediate activation. Clamped to 0..60 seconds.',
        '# Keep vertical rotor lift while pitch is tilted forward/back.',
        '# Scale the separate world-up force: 0=none, 1=lost vertical component. Clamped to 0..5.',
        '# Percent of the stock rotor''s physical maximum speed (0 disables the gate).',
        '# Percent of the stock rotor''s normal 100% operating speed (0 disables the gate).',
        '# Temporary compact fuel/throttle diagnostics; set false to hide on the next Gyro entry.',
        '# Valid range is 0.5..2.5; missing or invalid values fall back to 1.0.',
        '# Base Petrol request at 100% engine throttle, in native resource units per minute.'
    )
    $temporary = $SettingsPath + '.stablegyro-' + [guid]::NewGuid().ToString('N') + '.tmp'
    $backup = $SettingsPath + '.stablegyro-' + [guid]::NewGuid().ToString('N') + '.bak'
    $keepBackup = $false
    try {
        $result = & (Join-Path $repoRoot 'tools/Merge-VoyageModIni.ps1') `
            -TemplatePath $TemplatePath -CurrentPath $SettingsPath `
            -OutputPath $temporary -RetiredKeys $retiredKeys `
            -RetiredComments $retiredComments `
            -DefaultsComment '# Defaults added by a newer StableGyro build.'
        if ($result.Changed) {
            Assert-GameClosed
            [IO.File]::Replace($temporary, $SettingsPath, $backup, $true)
        }
        return [pscustomobject]@{
            AddedKeys=@($result.AddedKeys); RemovedKeys=@($result.RemovedKeys)
        }
    } catch {
        $failure = $_.Exception.Message
        if (Test-Path -LiteralPath $backup) {
            $keepBackup = $true
            Assert-GameClosed
            Copy-Item -LiteralPath $backup -Destination $SettingsPath -Force
            if ((Get-FileHash -LiteralPath $SettingsPath -Algorithm SHA256).Hash -cne
                (Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash) {
                throw "StableGyro settings recovery hash mismatch; backup retained: $backup"
            }
            $keepBackup = $false
        }
        throw "StableGyro settings update failed: $failure"
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
        if (-not $keepBackup -and (Test-Path -LiteralPath $backup)) {
            Remove-Item -LiteralPath $backup -Force
        }
    }
}
function Get-CleanOriginalInputs {
    param(
        [Parameter(Mandatory = $true)] [string]$Root,
        [Parameter(Mandatory = $true)] [string]$ExpectedBuild,
        [Parameter(Mandatory = $true)] [string]$ExpectedRetocCompatibilityVersion
    )

    $resolvedRoot = Resolve-RequiredPath -Path $Root -Label 'Originals root'
    $manifests = @(Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter 'extraction-manifest.json')
    if ($manifests.Count -ne 1) {
        throw "Expected exactly one extraction manifest below $resolvedRoot; found $($manifests.Count)."
    }

    $manifest = Get-Content -LiteralPath $manifests[0].FullName -Raw | ConvertFrom-Json
    if ([string]$manifest.filter -cne 'Data/Assets/Modules/DA_Item_Module_GyroCopter' -or
        [string]$manifest.steamBuildId -cne $ExpectedBuild -or
        [string]$manifest.retocEngineVersion -cne $ExpectedRetocCompatibilityVersion -or
        $manifest.allowAdditionalContainers -ne $false) {
        throw "Originals manifest is not a clean extraction for the current validated game: $($manifests[0].FullName)"
    }

    $extractionRoot = $manifests[0].Directory.FullName
    $itemDirectory = Join-Path $extractionRoot 'Voyage\Content\Data\Assets\Modules'
    $scriptObjects = Join-Path $extractionRoot 'scriptobjects.bin'
    foreach ($required in @(
        (Join-Path $itemDirectory 'DA_Item_Module_GyroCopter.uasset'),
        (Join-Path $itemDirectory 'DA_Item_Module_GyroCopter.uexp'),
        $scriptObjects
    )) {
        Resolve-RequiredPath -Path $required -Label 'Clean original input' | Out-Null
    }

    return [pscustomobject]@{
        Manifest = $manifests[0].FullName
        ItemDirectory = $itemDirectory
        ScriptObjects = $scriptObjects
    }
}

$project = Resolve-RequiredPath -Path $project -Label 'Unreal project'
$provenancePath = Resolve-RequiredPath -Path $provenancePath -Label 'StableGyro provenance registry'
$unrealEditor = Resolve-RequiredPath -Path $unrealEditor -Label 'UnrealEditor-Cmd'
$engineVersionPath = Resolve-RequiredPath -Path $engineVersionPath -Label 'Unreal build version'
$fingerprinter = Resolve-RequiredPath -Path $fingerprinter -Label 'Voyage fingerprint tool'
$cookScript = Resolve-RequiredPath -Path $cookScript -Label 'StableGyro cook script'
$prepareScript = Resolve-RequiredPath -Path $prepareScript -Label 'StableGyro original preparer'
$packageScript = Resolve-RequiredPath -Path $packageScript -Label 'StableGyro package builder'
$settingsSchema = Resolve-RequiredPath -Path $settingsSchema -Label 'StableGyro settings schema'
$settingsDefaults = Resolve-RequiredPath -Path $settingsDefaults -Label 'StableGyro canonical settings INI'
$settingsModule = Resolve-RequiredPath -Path $settingsModule -Label 'Settings validation module'
$releaseManifestBuilder = Resolve-RequiredPath -Path $releaseManifestBuilder -Label 'Voyage release manifest builder'
$releaseInstaller = Resolve-RequiredPath -Path $releaseInstaller -Label 'Voyage release installer'
$Retoc = Resolve-RequiredPath -Path $Retoc -Label 'retoc'
$GameRoot = Resolve-RequiredPath -Path $GameRoot -Label 'Voyage game root'

$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$actualEngineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
if ($actualEngineVersion -cne $expectedEditorEngineVersion) {
    throw "StableGyro requires Unreal Engine $expectedEditorEngineVersion; selected engine is $actualEngineVersion."
}

$sourceStatus = @(& git -C $repoRoot status --porcelain -- `
    'mods/StableGyro' `
    'tools/New-VoyageModSettings.ps1' `
    'tools/Merge-VoyageModIni.ps1' `
    'tools/VoyageModSettings' `
    ':(exclude)mods/StableGyro/Slideshow')
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect StableGyro source status.'
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $buildIdentity = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
    $OutputRoot = Join-Path $tmpOwnerRoot "build-$buildIdentity"
}
$releaseRoot = Assert-UnderIgnoredBuildStorage -Path $OutputRoot -Label 'OutputRoot'
if (-not $releaseRoot.StartsWith($tmpOwnerRoot.TrimEnd('\') + '\',
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot must be below Tmp/StableGyro.'
}
if (Test-Path -LiteralPath $releaseRoot) {
    throw "OutputRoot already exists: $releaseRoot"
}
New-Item -ItemType Directory -Path $releaseRoot | Out-Null
$logs = Join-Path $releaseRoot 'logs'
New-Item -ItemType Directory -Path $logs | Out-Null
Import-Module $settingsModule -Force
$validatedSettings = @(Read-VoyageModSettingsSchema -SchemaPath $settingsSchema -DefaultIniPath $settingsDefaults)
$releaseSettingsDirectory = Join-Path $releaseRoot 'validated-settings'
New-Item -ItemType Directory -Path $releaseSettingsDirectory | Out-Null
$generatedSettingsIni = Join-Path $releaseSettingsDirectory 'StableGyro.ini'
Copy-Item -LiteralPath $settingsDefaults -Destination $generatedSettingsIni

$totalStopwatch = [Diagnostics.Stopwatch]::StartNew()
$timings = [ordered]@{}

Write-Host '1/5 Validating game fingerprint'
$phase = [Diagnostics.Stopwatch]::StartNew()
$fingerprintPath = Join-Path $releaseRoot 'game-fingerprint.json'
& $fingerprinter -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$gameBuild = & (Join-Path $repoRoot 'tools/Assert-VoyageModGameBuild.ps1') `
    -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) `
    -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
$expectedBuild = [string]$fingerprint.steam.buildId
$mapping = & (Join-Path $repoRoot 'tools\Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ([string]$mapping.steamBuildId -cne $expectedBuild -or
    [string]$mapping.uassetApiEngineVersion -cne 'VER_UE5_8') {
    throw 'Resolved Voyage mapping does not match the current StableGyro build contract.'
}
$phase.Stop()
$timings.Fingerprint = $phase.Elapsed.TotalSeconds

$meshPlan = Get-StableGyroMooringMeshPlan -StatePath $meshSourceStatePath `
    -SteamBuildId $expectedBuild
$itemPlan = Get-StableGyroItemRefreshPlan -StatePath $itemSourceStatePath `
    -RecipePath $itemRecipePath -SteamBuildId $expectedBuild -Reimport:$ReimportItemAsset
if (-not $itemPlan.refresh) {
    foreach ($relativePath in @('Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset',
            'Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset')) {
        if (-not (Test-Path -LiteralPath (Join-Path $modRoot $relativePath) -PathType Leaf)) {
            throw "Maintained item asset is missing: $relativePath. Restore it from Git or request -ReimportItemAsset."
        }
    }
}
$expectedStaticMeshObjectPaths = @(
    '/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY.StableGyroMooringPlatePositiveY'
)
$expectedMeshPaths = @($expectedStaticMeshObjectPaths | ForEach-Object {
    'Content/' + (($_ -split '\.')[0]).Substring('/Game/'.Length) + '.uasset'
})
foreach ($relativePath in $expectedMeshPaths) {
    if (-not (Test-Path -LiteralPath (Join-Path $modRoot $relativePath) -PathType Leaf)) {
        throw "Maintained mesh is missing: $relativePath. Restore it from Git or run Update-StableGyroMooringPlate.ps1."
    }
}

Write-Host 'Reusing the maintained plate; source geometry is checked by the explicit updater'

Write-Host '2/5 Preparing current stock item validation and maintained project inputs'
$phase.Restart()
if ([string]::IsNullOrWhiteSpace($OriginalsRoot)) {
    $OriginalsRoot = Join-Path $releaseRoot 'originals'
    & $prepareScript -GameRoot $GameRoot -Retoc $Retoc -OutputRoot $OriginalsRoot
} else {
    $OriginalsRoot = Assert-UnderIgnoredBuildStorage -Path $OriginalsRoot -Label 'OriginalsRoot'
}
$originalInputs = Get-CleanOriginalInputs `
    -Root $OriginalsRoot `
    -ExpectedBuild $expectedBuild `
    -ExpectedRetocCompatibilityVersion $retocCompatibilityVersion
$phase.Stop()
$timings.Originals = $phase.Elapsed.TotalSeconds

$phase.Restart()
if ($itemPlan.refresh) {
    Write-Host "Refreshing item and crafting-generator stub: $($itemPlan.reason)"
    Set-StableGyroItemSourceState -StatePath $itemSourceStatePath -SteamBuildId $expectedBuild `
        -Recipe $itemPlan.recipe -Status pending
    & (Join-Path $repoRoot 'tools/Update-VoyageItemAsset.ps1') `
        -Project $project -Recipe $itemRecipePath `
        -EngineRoot $EngineRoot -GameRoot $GameRoot `
        -OutputRoot (Join-Path $releaseRoot 'item-update')
} else {
    Write-Host 'Reusing item and crafting-generator stub for the same Steam build and recipe'
    # The updater normally prepares these dependencies. Reuse must also work in
    # a checkout without installed native support, without recreating assets.
    $support = & (Join-Path $repoRoot 'tools/Prepare-UnrealEditorSupport.ps1') `
        -ProjectFile $project -EngineRoot $EngineRoot `
        -OutputRoot (Join-Path $releaseRoot 'editor-support') | ConvertFrom-Json
    $loadedModules = @($support.providers | ForEach-Object loadedModules)
    if ('Voyage' -notin $loadedModules -or 'VoyageItemPatchEditor' -notin $loadedModules) {
        throw 'Project support must provide Voyage and VoyageItemPatchEditor.'
    }
}
$phase.Stop()
$timings.ItemUpdate = $phase.Elapsed.TotalSeconds

Write-Host '3/5 Cooking maintained project packages'
$phase.Restart()
$cookedRoot = Join-Path $releaseRoot 'cooked'
& $cookScript `
    -UnrealEditor $unrealEditor `
    -Project $project `
    -LocalDataCachePath (Join-Path $modRoot 'DerivedDataCache') `
    -LogPath (Join-Path $logs 'cook.log') `
    -OutputRoot $cookedRoot
$stagedStaticMeshFiles = @()
foreach ($objectPath in $expectedStaticMeshObjectPaths) {
    $relativeBase = 'Content/' + (($objectPath -split '\.')[0]).Substring('/Game/'.Length)
    foreach ($extension in @('.uasset','.uexp','.ubulk','.uptnl','.m.ubulk')) {
        $relativePath = $relativeBase + $extension
        $candidate = Join-Path $cookedRoot $relativePath
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $stagedStaticMeshFiles += [ordered]@{path=$relativePath;length=(Get-Item -LiteralPath $candidate).Length}
        } elseif ($extension -in @('.uasset','.uexp')) {
            throw "Missing project-cooked mesh companion: $candidate"
        }
    }
}
$staticMeshStagingReportPath = Join-Path $releaseRoot 'static-mesh-staging.json'
([ordered]@{ files = $stagedStaticMeshFiles }) | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $staticMeshStagingReportPath -Encoding utf8
$phase.Stop()
$timings.Cook = $phase.Elapsed.TotalSeconds

Write-Host '4/5 Building and verifying the IoStore container'
$phase.Restart()
$containerRoot = Join-Path $releaseRoot 'container'
if (@($originalInputs).Count -ne 1) {
    throw "Expected one clean-original input record; found $(@($originalInputs).Count)."
}
$packageArguments = @{
    CookedRoot = [string]$cookedRoot
    OriginalItemDirectory = [string]$originalInputs.ItemDirectory
    ScriptObjects = [string]$originalInputs.ScriptObjects
    Retoc = [string]$Retoc
    OutputRoot = [string]$containerRoot
    ActualSteamBuildId = $expectedBuild
    MappingsPath = [string]$mapping.mappingsPath
}
& $packageScript @packageArguments
$containerFiles = Join-Path $containerRoot 'package'
$containerUtoc = Join-Path $containerFiles ($containerName + '.utoc')
$meshValidation = & (Join-Path $modRoot 'Build/Test-MooringPlateContainer.ps1') `
    -ContainerUtoc $containerUtoc -OutputRoot $releaseRoot -GameRoot $GameRoot
$phase.Stop()
$timings.Package = $phase.Elapsed.TotalSeconds
if ($itemPlan.refresh) {
    Set-StableGyroItemSourceState -StatePath $itemSourceStatePath -SteamBuildId $expectedBuild `
        -Recipe $itemPlan.recipe -Status ready
}

Write-Host '5/5 Creating the player-facing archive'
$phase.Restart()
$payloadRoot = Join-Path $releaseRoot "payload\StableGyro-$Version"
New-Item -ItemType Directory -Path $payloadRoot -Force | Out-Null
foreach ($name in $payloadNames) {
    Copy-Item -LiteralPath (Join-Path $containerFiles $name) -Destination (Join-Path $payloadRoot $name)
}
Copy-Item -LiteralPath (Join-Path $modRoot 'README.md') -Destination (Join-Path $payloadRoot 'README.md')
Copy-Item -LiteralPath $generatedSettingsIni -Destination (Join-Path $payloadRoot 'StableGyro.ini')
$archivePath = Join-Path $releaseRoot "StableGyro-$Version.zip"
& (Join-Path $repoRoot 'tools/New-VoyageVortexArchive.ps1') `
    -Container (Join-Path $payloadRoot ($containerName + '.utoc')) -Archive $archivePath `
    -AdditionalFile (Join-Path $payloadRoot 'StableGyro.ini') | Out-Null
$commonReleaseManifestPath = Join-Path $releaseRoot 'common-release-manifest.json'
& $releaseManifestBuilder `
    -ReleaseRoot $releaseRoot `
    -Mod 'StableGyro' `
    -Version $Version `
    -ModVersion $candidateModVersion `
    -Container $containerUtoc `
    -Archive $archivePath `
    -SourcePath $modRoot `
    -GameRoot $GameRoot `
    -OutputPath $commonReleaseManifestPath `
    -AllowDirtySource | Out-Null
$commonReleaseManifestPath = Resolve-RequiredPath `
    -Path $commonReleaseManifestPath -Label 'Validated common release manifest'
$installValidationEvidenceRoot = Join-Path $releaseRoot 'install-validation'
$installValidationPlanPath = Join-Path $releaseRoot 'install-validation-plan.json'
$installValidationPlan = & $releaseInstaller `
    -ReleaseManifest $commonReleaseManifestPath `
    -GameRoot $GameRoot `
    -EvidenceRoot $installValidationEvidenceRoot `
    -AllowDirtySource `
    -ValidateOnly
$installValidationPlan | ConvertTo-Json -Depth 10 |
    Set-Content -LiteralPath $installValidationPlanPath -Encoding utf8
$phase.Stop()
$timings.Archive = $phase.Elapsed.TotalSeconds

$installed = $false
$settingsInstallation = $null
if ($Install) {
    Assert-GameClosed
    $paks = Resolve-RequiredPath -Path (Join-Path $GameRoot 'Voyage\Content\Paks') -Label 'Voyage Paks directory'
    $backup = Join-Path $releaseRoot 'installed-backup'
    New-Item -ItemType Directory -Path $backup | Out-Null
    foreach ($name in $payloadNames) {
        $source = Join-Path $containerFiles $name
        $target = Join-Path $paks $name
        if (Test-Path -LiteralPath $target -PathType Leaf) {
            Copy-Item -LiteralPath $target -Destination (Join-Path $backup $name)
        }
        Copy-Item -LiteralPath $source -Destination $target -Force
        $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
        $targetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash
        if ($sourceHash -cne $targetHash) {
            throw "Installed file hash mismatch: $name"
        }
    }
    $settingsTemplate = $generatedSettingsIni
    $settingsPath = Join-Path $paks 'StableGyro.ini'
    if (-not (Test-Path -LiteralPath $settingsPath -PathType Leaf)) {
        $temporarySettings = $settingsPath + '.stablegyro-' +
            [guid]::NewGuid().ToString('N') + '.tmp'
        try {
            $settingsCreate = & (Join-Path $repoRoot 'tools/Merge-VoyageModIni.ps1') `
                -TemplatePath $settingsTemplate -OutputPath $temporarySettings
            Assert-GameClosed
            [IO.File]::Move($temporarySettings, $settingsPath)
        } finally {
            if (Test-Path -LiteralPath $temporarySettings) {
                Remove-Item -LiteralPath $temporarySettings -Force
            }
        }
        $addedKeys = @()
        $removedKeys = @()
        $settingsCreated = $true
    } else {
        $settingsUpdate = Update-StableGyroSettings $settingsTemplate $settingsPath
        $addedKeys = @($settingsUpdate.AddedKeys)
        $removedKeys = @($settingsUpdate.RemovedKeys)
        $settingsCreated = $false
    }
    $settingsInstallation = [ordered]@{
        path = $settingsPath
        created = $settingsCreated
        addedKeys = $addedKeys
        removedKeys = $removedKeys
    }
    $installed = $true
}

$totalStopwatch.Stop()
$payloadEvidence = @(
    Get-ChildItem -LiteralPath $payloadRoot -File |
        Sort-Object Name |
        ForEach-Object {
            [pscustomobject]@{
                name = $_.Name
                size = $_.Length
            }
        }
)
$manifest = [ordered]@{
    schemaVersion = 2
    mod = 'StableGyro'
    version = $Version
    modVersion = $candidateModVersion
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    dirtySource = ($sourceStatus.Count -gt 0)
    sourceStatus = $sourceStatus
    gameEngineVersion = $expectedGameEngineVersion
    editorEngineVersion = $actualEngineVersion
    retocCompatibilityVersion = $retocCompatibilityVersion
    cookStorage = $cookStorage
    itemRefresh = $itemPlan
    staticMeshes = [ordered]@{
        refresh = $meshPlan
        stagingReport = [ordered]@{
            path = $staticMeshStagingReportPath
        }
        validationRequest = [ordered]@{
            path = $meshValidation.request
        }
        inspection = [ordered]@{
            path = $meshValidation.inspection
        }
        validation = [ordered]@{
            path = $meshValidation.validation
        }
        materialValidation = [ordered]@{
            path = $meshValidation.materialValidation
        }
    }
    commonReleaseManifest = [ordered]@{
        path = $commonReleaseManifestPath
    }
    installValidationPlan = [ordered]@{
        path = $installValidationPlanPath
    }
    gameFingerprint = [ordered]@{
        steamBuildId = [string]$fingerprint.steam.buildId
    }
    ignoredPinnedGameVersion = [bool]$gameBuild.ignoredPinnedGameVersion
    originalsManifest = $originalInputs.Manifest
    installed = $installed
    settingsInstallation = $settingsInstallation
    payload = $payloadEvidence
    archive = [ordered]@{
        name = (Split-Path -Leaf $archivePath)
        size = (Get-Item -LiteralPath $archivePath).Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash
    }
    timingsSeconds = $timings
    totalSeconds = $totalStopwatch.Elapsed.TotalSeconds
}
$manifestPath = Join-Path $releaseRoot 'release-manifest.json'
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding utf8

Write-Host ''
Write-Host "StableGyro release artifact is ready: $archivePath"
Write-Host "Manifest: $manifestPath"
Write-Host ("Elapsed: {0:N1} seconds" -f $totalStopwatch.Elapsed.TotalSeconds)
if ($installed) {
    Write-Host 'The prepared container was backed up, installed, and read back successfully.'
}

[pscustomobject]@{
    status = if ($installed) { 'installed' } else { 'prepared-not-installed' }
    releaseManifestPath = $manifestPath
    archivePath = $archivePath
    settingsInstallation = $settingsInstallation
} | ConvertTo-Json -Depth 6 -Compress
