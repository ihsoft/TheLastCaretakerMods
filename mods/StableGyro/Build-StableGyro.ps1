# HAND-WRITTEN BUILD ORCHESTRATOR: the only public producer. It builds,
# refreshes project meshes, cooks, packages, verifies, archives, and optionally installs
# StableGyro. It refreshes the two maintained project meshes; build output stays
# in ignored local storage. It never publishes externally.

[CmdletBinding()]
param(
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]*$')]
    [string]$Version,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EngineRoot = 'P:\UnrealEngine\UE_5.8',

    [string]$BlenderPath = 'K:\Program Files\Blender Foundation\Blender 5.2\blender.exe',

    [string]$Retoc,

    [string]$OutputRoot,

    [string]$OriginalsRoot,

    [switch]$Install,

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
$modelExporter = Join-Path $repoRoot 'tools\Export-VoyageModelGlb.ps1'
$staticMeshBuilder = Join-Path $repoRoot 'tools\New-VoyageStaticMeshAssets.ps1'
$staticMeshValidator = Join-Path $repoRoot 'tools\Test-VoyageStaticMesh.ps1'
$assetJsonReader = Join-Path $repoRoot 'tools\Get-VoyageAssetJson.ps1'
$releaseManifestBuilder = Join-Path $repoRoot 'tools\New-VoyageReleaseManifest.ps1'
$releaseInstaller = Join-Path $repoRoot 'tools\Install-VoyageRelease.ps1'
$mooringSplitter = Join-Path $modRoot 'Build\Split-MooringPlateGlb.py'
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
$modelExporter = Resolve-RequiredPath -Path $modelExporter -Label 'Voyage model exporter'
$staticMeshBuilder = Resolve-RequiredPath -Path $staticMeshBuilder -Label 'Voyage StaticMesh builder'
$staticMeshValidator = Resolve-RequiredPath -Path $staticMeshValidator -Label 'Voyage StaticMesh validator'
$assetJsonReader = Resolve-RequiredPath -Path $assetJsonReader -Label 'Voyage asset JSON reader'
$releaseManifestBuilder = Resolve-RequiredPath -Path $releaseManifestBuilder -Label 'Voyage release manifest builder'
$releaseInstaller = Resolve-RequiredPath -Path $releaseInstaller -Label 'Voyage release installer'
$mooringSplitter = Resolve-RequiredPath -Path $mooringSplitter -Label 'Mooring plate splitter'
$BlenderPath = Resolve-RequiredPath -Path $BlenderPath -Label 'Blender 5 executable'
$Retoc = Resolve-RequiredPath -Path $Retoc -Label 'retoc'
$GameRoot = Resolve-RequiredPath -Path $GameRoot -Label 'Voyage game root'

$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$actualEngineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
if ($actualEngineVersion -cne $expectedEditorEngineVersion) {
    throw "StableGyro requires Unreal Engine $expectedEditorEngineVersion; selected engine is $actualEngineVersion."
}

if ([string]::IsNullOrWhiteSpace($OriginalsRoot) -or $Install) {
    Assert-GameClosed
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

Write-Host '1/8 Validating game fingerprint'
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

Write-Host '2/8 Exporting and splitting the current stock mooring plate'
$phase.Restart()
Assert-GameClosed
$mooringGeometryRoot = Join-Path $releaseRoot 'mooring-geometry'
New-Item -ItemType Directory -Path $mooringGeometryRoot | Out-Null
$stockMooringGlb = Join-Path $mooringGeometryRoot 'Gyro_MooringPlate.stock.glb'
$positiveMooringGlb = Join-Path $mooringGeometryRoot 'StableGyroMooringPlatePositiveY.glb'
$negativeMooringGlb = Join-Path $mooringGeometryRoot 'StableGyroMooringPlateNegativeY.glb'
$mooringSplitReport = Join-Path $mooringGeometryRoot 'split-report.json'
& $modelExporter `
    -Asset '/Game/AssetSets/Vehicles/Gyrocopter/Mesh/Gyro_MooringPlate' `
    -OutputPath $stockMooringGlb `
    -MaterialMode PbrApproximation `
    -GeometryMode Nanite `
    -GameRoot $GameRoot | Out-Null
$stockExportEvidence = Get-Content -LiteralPath ($stockMooringGlb + '.evidence\export-report.json') -Raw |
    ConvertFrom-Json
$stockNanite = @($stockExportEvidence.meshComponents)[0].Nanite
if ($stockExportEvidence.geometryMode -cne 'Nanite' -or $stockNanite.Pages -ne 4 -or
    $stockNanite.LoadedPages -ne 4 -or $stockNanite.DecodedClusters -ne 223 -or
    $stockNanite.LeafClusters -ne 111 -or $stockNanite.LeafTriangles -ne 13790 -or
    $stockNanite.UvChannels -ne 4 -or $stockNanite.ExactZeroAreaTriangles -ne 114 -or
    $stockNanite.HasTangents) {
    throw 'Stock mooring plate Nanite decode evidence differs from the reviewed source contract.'
}
$splitLog = Join-Path $logs 'mooring-split.log'
$splitOutput = @(& $BlenderPath --background --factory-startup --python-exit-code 1 `
    --python $mooringSplitter -- `
    --source $stockMooringGlb `
    --positive-y $positiveMooringGlb `
    --negative-y $negativeMooringGlb `
    --report $mooringSplitReport 2>&1)
$splitExitCode = $LASTEXITCODE
$splitOutput | Set-Content -LiteralPath $splitLog -Encoding utf8
if ($splitExitCode -ne 0) {
    $splitOutput | Select-Object -Last 60 | Write-Host
    throw "Mooring plate split failed with exit code $splitExitCode."
}
$splitEvidence = Get-Content -LiteralPath $mooringSplitReport -Raw | ConvertFrom-Json
if ($splitEvidence.status -cne 'passed' -or $splitEvidence.crossingTriangles -ne 0 -or
    $splitEvidence.sourceMesh.triangles -ne 13790 -or
    $splitEvidence.sourceMesh.exactZeroAreaTriangles -ne 88 -or
    $splitEvidence.positiveY.triangles -ne 6895 -or
    $splitEvidence.negativeY.triangles -ne 6895 -or
    $splitEvidence.positiveY.materials -ne 1 -or
    $splitEvidence.negativeY.materials -ne 1 -or
    $splitEvidence.rawSourceNormalTrianglesRestored -ne 13790 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.positiveY.triangles -ne 6895 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.positiveY.nonNormalBytesChanged -ne 0 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.negativeY.triangles -ne 6895 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.negativeY.nonNormalBytesChanged -ne 0 -or
    [string]$splitEvidence.materialBindings.positiveY.slot -cne 'Gyrocopter_Seat_p1' -or
    $splitEvidence.materialBindings.positiveY.primitiveMaterialIndex -ne 0 -or
    [string]$splitEvidence.materialBindings.negativeY.slot -cne 'Gyrocopter_Seat_p1' -or
    $splitEvidence.materialBindings.negativeY.primitiveMaterialIndex -ne 0 -or
    $splitEvidence.materialBindings.positiveY.preservedNonJsonChunks -ne $true -or
    $splitEvidence.materialBindings.negativeY.preservedNonJsonChunks -ne $true -or
    $splitEvidence.manualFacesRemoved -ne 0) {
    throw 'Mooring plate split evidence does not match the reviewed stock geometry.'
}
$phase.Stop()
$timings.MooringGeometry = $phase.Elapsed.TotalSeconds

Write-Host '3/8 Building the two editor mooring meshes through the common producer'
$phase.Restart()
$staticMeshRequestPath = Join-Path $releaseRoot 'static-mesh-request.json'
$staticMeshOutputRoot = Join-Path $releaseRoot 'static-mesh-assets'
$expectedStaticMeshObjectPaths = @(
    '/Game/Mods/StableGyro/StableGyroMooringPlateNegativeY/StaticMeshes/StableGyroMooringPlateNegativeY.StableGyroMooringPlateNegativeY',
    '/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY.StableGyroMooringPlatePositiveY'
)
$staticMeshRequest = [ordered]@{
    schemaVersion = 1
    assets = @(
        [ordered]@{
            id = 'PositiveY'
            source = $positiveMooringGlb
            destination = [ordered]@{
                packagePath = '/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY'
                objectName = 'StableGyroMooringPlatePositiveY'
            }
            materialMode = 'ExternalReferences'
            materialBindings = @([ordered]@{
                slot = 'Gyrocopter_Seat_p1'
                objectPath = '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.Gyrocopter_Seat_p1'
                classPath = '/Script/Engine.MaterialInstanceConstant'
            })
            import = [ordered]@{
                generateLightmapUVs = $false
                removeDegenerates = $false
                recomputeNormals = $false
                recomputeTangents = $true
                useFullPrecisionUVs = $false
                useHighPrecisionTangentBasis = $false
            }
            nanite = [ordered]@{
                enabled = $true
                explicitTangents = $false
                positionPrecision = 4
                normalPrecision = 8
                keepPercentTriangles = 1.0
                trimRelativeError = 0.0
                generateFallback = 'Enabled'
                fallbackTarget = 'PercentTriangles'
                fallbackPercentTriangles = 0.25
                fallbackRelativeError = 0.0
            }
            rayTracing = [ordered]@{ support = $true }
            assertions = [ordered]@{
                sourceTriangles = [ordered]@{ exact = 6895 }
                naniteInputTriangles = [ordered]@{ exact = 6852 }
                fallbackTriangles = [ordered]@{ min = 1500; max = 1900 }
            }
        },
        [ordered]@{
            id = 'NegativeY'
            source = $negativeMooringGlb
            destination = [ordered]@{
                packagePath = '/Game/Mods/StableGyro/StableGyroMooringPlateNegativeY/StaticMeshes/StableGyroMooringPlateNegativeY'
                objectName = 'StableGyroMooringPlateNegativeY'
            }
            materialMode = 'ExternalReferences'
            materialBindings = @([ordered]@{
                slot = 'Gyrocopter_Seat_p1'
                objectPath = '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.Gyrocopter_Seat_p1'
                classPath = '/Script/Engine.MaterialInstanceConstant'
            })
            import = [ordered]@{
                generateLightmapUVs = $false
                removeDegenerates = $false
                recomputeNormals = $false
                recomputeTangents = $true
                useFullPrecisionUVs = $false
                useHighPrecisionTangentBasis = $false
            }
            nanite = [ordered]@{
                enabled = $true
                explicitTangents = $false
                positionPrecision = 4
                normalPrecision = 8
                keepPercentTriangles = 1.0
                trimRelativeError = 0.0
                generateFallback = 'Enabled'
                fallbackTarget = 'PercentTriangles'
                fallbackPercentTriangles = 0.25
                fallbackRelativeError = 0.0
            }
            rayTracing = [ordered]@{ support = $true }
            assertions = [ordered]@{
                sourceTriangles = [ordered]@{ exact = 6895 }
                naniteInputTriangles = [ordered]@{ exact = 6852 }
                fallbackTriangles = [ordered]@{ min = 1500; max = 1900 }
            }
        }
    )
}
$staticMeshRequest | ConvertTo-Json -Depth 12 |
    Set-Content -LiteralPath $staticMeshRequestPath -Encoding utf8
& $staticMeshBuilder -Manifest $staticMeshRequestPath -OutputRoot $staticMeshOutputRoot `
    -EngineRoot (Join-Path $EngineRoot 'Engine') -OutputKind Uncooked | Out-Null
$staticMeshBuildResultPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshOutputRoot 'build-result.json') `
    -Label 'Common StaticMesh build result'
$staticMeshOutputManifestPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshOutputRoot 'output-manifest.json') `
    -Label 'Common StaticMesh output manifest'
$staticMeshBuildResult = Get-Content -LiteralPath $staticMeshBuildResultPath -Raw | ConvertFrom-Json
if ([string]$staticMeshBuildResult.status -cne 'passed' -or
    @($staticMeshBuildResult.assets).Count -ne 2 -or
    ((@($staticMeshBuildResult.assets.objectPath | Sort-Object) -join "`n") -cne
        (($expectedStaticMeshObjectPaths | Sort-Object) -join "`n")) -or
    @($staticMeshBuildResult.assets | Where-Object {
        $_.sourceTriangles -ne 6895 -or $_.naniteInputTriangles -ne 6852 -or
        $_.fallbackTriangles -lt 1500 -or $_.fallbackTriangles -gt 1900 -or
        $_.uvChannels -ne 4 -or $_.positionPrecision -ne 4 -or
        $_.normalPrecision -ne 8 -or -not $_.supportRayTracing -or
        $_.rayTracingProxyLods -ne 1 -or -not $_.rayTracingUsesRenderingLods -or
        @($_.importedMaterialSlots).Count -ne 1 -or
        [string](@($_.importedMaterialSlots)[0]) -cne 'Gyrocopter_Seat_p1' -or
        @($_.resolvedExternalMaterialReferences).Count -ne 1 -or
        [string](@($_.resolvedExternalMaterialReferences)[0]) -cne
            '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.Gyrocopter_Seat_p1'
    }).Count -ne 0) {
    throw 'Common StaticMesh build result differs from StableGyro consumer policy.'
}
$phase.Stop()
$timings.StaticMeshBuild = $phase.Elapsed.TotalSeconds

Write-Host '4/8 Updating project mesh assets; preserving authored Blueprints'
$phase.Restart()
$meshManifest = Get-Content -LiteralPath $staticMeshOutputManifestPath -Raw | ConvertFrom-Json
$expectedMeshPaths = @($expectedStaticMeshObjectPaths | ForEach-Object {
    'Content/' + (($_ -split '\.')[0]).Substring('/Game/'.Length) + '.uasset'
})
if ($meshManifest.kind -cne 'Voyage uncooked StaticMesh assets' -or
    (@($meshManifest.files.path | Sort-Object) -join "`n") -cne
    (($expectedMeshPaths | Sort-Object) -join "`n")) {
    throw 'Unexpected editor mesh output; project was not changed.'
}
$meshBackup = Join-Path $releaseRoot 'project-mesh-backup'
foreach ($file in $meshManifest.files) {
    $source = Join-Path $staticMeshOutputRoot $file.path
    $destination = Resolve-RequiredPath -Path (Join-Path $modRoot $file.path) -Label 'Maintained project mesh'
    if ((Get-Item -LiteralPath $source).Length -ne $file.length) { throw "Mesh output size mismatch: $source" }
    $backup = Join-Path $meshBackup $file.path
    New-Item -ItemType Directory -Path (Split-Path $backup -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $destination -Destination $backup
}
try {
    foreach ($file in $meshManifest.files) {
        $source = Join-Path $staticMeshOutputRoot $file.path
        $destination = Join-Path $modRoot $file.path
        Copy-Item -LiteralPath $source -Destination $destination -Force
        if ([Convert]::ToBase64String([IO.File]::ReadAllBytes($source)) -cne
            [Convert]::ToBase64String([IO.File]::ReadAllBytes($destination))) {
            throw "Project mesh copy mismatch: $destination"
        }
    }
} catch {
    foreach ($file in $meshManifest.files) {
        Copy-Item -LiteralPath (Join-Path $meshBackup $file.path) -Destination (Join-Path $modRoot $file.path) -Force
    }
    throw
}
$phase.Stop()
$timings.ProjectMeshUpdate = $phase.Elapsed.TotalSeconds

Write-Host '5/8 Refreshing the maintained project item from clean stock data'
$phase.Restart()
if ([string]::IsNullOrWhiteSpace($OriginalsRoot)) {
    Assert-GameClosed
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
& (Join-Path $repoRoot 'tools/Update-VoyageItemAsset.ps1') `
    -Project $project -Recipe (Join-Path $modRoot 'Build/Item.patch.json') `
    -EngineRoot $EngineRoot -GameRoot $GameRoot `
    -OutputRoot (Join-Path $releaseRoot 'item-update')
$phase.Stop()
$timings.ItemUpdate = $phase.Elapsed.TotalSeconds

Write-Host '6/8 Cooking nine maintained project packages'
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

Write-Host '7/8 Building and verifying the nine-asset IoStore container'
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
$staticMeshValidationRequestPath = Join-Path $releaseRoot 'static-mesh-validation-request.json'
$staticMeshValidationOutput = Join-Path $releaseRoot 'static-mesh-validation'
$staticMeshValidationAssets = @()
foreach ($side in @('PositiveY', 'NegativeY')) {
    $assetName = "StableGyroMooringPlate$side"
    $sourceGlb = if ($side -ceq 'PositiveY') { $positiveMooringGlb } else { $negativeMooringGlb }
    $staticMeshValidationAssets += [ordered]@{
        id = $side
        package = "/Game/Mods/StableGyro/$assetName/StaticMeshes/$assetName"
        decodeNanite = $true
        sourceGlb = [ordered]@{
            path = $sourceGlb
            positionMatchToleranceMeters = 0.00033
            attributes = [ordered]@{
                POSITION = 0.00033
                NORMAL = 0.012
                TEXCOORD_0 = 0.0002
                TEXCOORD_1 = 0.0002
                TEXCOORD_2 = 0.0002
                TEXCOORD_3 = 0.0002
                COLOR_0 = (0.5 / 255.0)
            }
            topology = [ordered]@{
                maxExtraFaces = 0
                maxMissingNondegenerateFaces = 0
                maxMissingFaces = 43
            }
        }
        expectations = @(
            [ordered]@{ fact = '/nanite/present'; equals = $true },
            [ordered]@{ fact = '/nanite/decodedTriangles'; equals = 6852 },
            [ordered]@{ fact = '/nanite/inputTriangles'; equals = 6852 },
            [ordered]@{ fact = '/nanite/positionPrecision'; equals = 4 },
            [ordered]@{ fact = '/nanite/normalPrecision'; equals = 8 },
            [ordered]@{ fact = '/renderingLods/0/triangles'; min = 1500 },
            [ordered]@{ fact = '/renderingLods/0/triangles'; max = 1900 },
            [ordered]@{ fact = '/renderingLods/0/uvChannels'; equals = 4 },
            [ordered]@{ fact = '/rayTracing/hasProxyFlag'; equals = $true },
            [ordered]@{ fact = '/rayTracing/proxyPresent'; equals = $true },
            [ordered]@{ fact = '/rayTracing/usingRenderingLods'; equals = $true },
            [ordered]@{ fact = '/rayTracing/proxyLodCount'; equals = 1 }
        )
    }
}
$staticMeshValidationRequest = [ordered]@{
    schemaVersion = 1
    source = [ordered]@{ kind = 'utoc'; utoc = $containerUtoc }
    assets = $staticMeshValidationAssets
}
$staticMeshValidationRequest | ConvertTo-Json -Depth 12 |
    Set-Content -LiteralPath $staticMeshValidationRequestPath -Encoding utf8
& $staticMeshValidator -RequestPath $staticMeshValidationRequestPath `
    -OutputDirectory $staticMeshValidationOutput -GameRoot $GameRoot | Out-Null
$staticMeshInspectionPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshValidationOutput 'inspection.json') `
    -Label 'Final-container StaticMesh inspection'
$staticMeshValidationPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshValidationOutput 'validation.json') `
    -Label 'Final-container StaticMesh validation'
$staticMeshValidationResult = Get-Content -LiteralPath $staticMeshValidationPath -Raw |
    ConvertFrom-Json
if ([string]$staticMeshValidationResult.status -cne 'passed') {
    throw 'Final-container StaticMesh validation did not report passed.'
}

$materialInspectionRoot = Join-Path $releaseRoot 'final-container-material-inspection'
$materialValidation = [ordered]@{}
foreach ($side in @('PositiveY', 'NegativeY')) {
    $assetName = "StableGyroMooringPlate$side"
    $virtualPath = "Voyage/Content/Mods/StableGyro/$assetName/StaticMeshes/$assetName.uasset"
    $inspectionLine = @(& powershell.exe -NoProfile -ExecutionPolicy Bypass `
        -File $assetJsonReader $virtualPath -AsJson -Source Mod `
        -ModContainer $containerUtoc -ModInspectionRoot $materialInspectionRoot `
        -GameRoot $GameRoot) |
        Select-Object -Last 1
    if ($LASTEXITCODE -ne 0 -or @($inspectionLine).Count -ne 1) {
        throw "Final-container material inspection failed for $assetName."
    }
    $inspection = $inspectionLine | ConvertFrom-Json
    $assetJson = Get-Content -LiteralPath ([string]$inspection.jsonPath) -Raw | ConvertFrom-Json
    $meshExports = @($assetJson | Where-Object {
        [string]$_.Type -ceq 'StaticMesh' -and [string]$_.Name -ceq $assetName
    })
    if ($meshExports.Count -ne 1) {
        throw "Expected one final-container StaticMesh export for $assetName."
    }
    $mesh = $meshExports[0]
    $materials = @($mesh.Properties.StaticMaterials)
    if ($materials.Count -ne 1 -or
        [string]$materials[0].MaterialSlotName -cne 'Gyrocopter_Seat_p1' -or
        [string]$materials[0].ImportedMaterialSlotName -cne 'Gyrocopter_Seat_p1' -or
        [string]$materials[0].MaterialInterface.ObjectName -cne
            "MaterialInstanceConstant'Gyrocopter_Seat_p1'" -or
        -not ([string]$materials[0].MaterialInterface.ObjectPath).StartsWith(
            '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.',
            [StringComparison]::Ordinal)) {
        throw "Final-container material binding contract failed for $assetName."
    }
    $materialValidation[$side] = [ordered]@{
        virtualPath = $virtualPath
        jsonPath = [string]$inspection.jsonPath
        materialObjectName = [string]$materials[0].MaterialInterface.ObjectName
        materialObjectPath = [string]$materials[0].MaterialInterface.ObjectPath
        materialSlotName = [string]$materials[0].MaterialSlotName
        importedMaterialSlotName = [string]$materials[0].ImportedMaterialSlotName
    }
}
$materialValidationPath = Join-Path $releaseRoot 'final-container-material-validation.json'
$materialValidation | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $materialValidationPath -Encoding utf8
$phase.Stop()
$timings.Package = $phase.Elapsed.TotalSeconds

Write-Host '8/8 Creating the player-facing archive'
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
    staticMeshes = [ordered]@{
        builderRequest = [ordered]@{
            path = $staticMeshRequestPath
        }
        builderResult = [ordered]@{
            path = $staticMeshBuildResultPath
        }
        builderOutputManifest = [ordered]@{
            path = $staticMeshOutputManifestPath
        }
        stagingReport = [ordered]@{
            path = $staticMeshStagingReportPath
        }
        validationRequest = [ordered]@{
            path = $staticMeshValidationRequestPath
        }
        inspection = [ordered]@{
            path = $staticMeshInspectionPath
        }
        validation = [ordered]@{
            path = $staticMeshValidationPath
        }
        materialValidation = [ordered]@{
            path = $materialValidationPath
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
