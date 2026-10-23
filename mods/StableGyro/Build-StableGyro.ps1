# HAND-WRITTEN BUILD ORCHESTRATOR: the only public producer. It builds,
# generates, cooks, packages, verifies, archives, and optionally installs
# StableGyro. It creates only ignored local artifacts and
# never publishes externally.

[CmdletBinding()]
param(
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]*$')]
    [string]$Version,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',

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
$candidateModVersion = 'v' + ([int]$publishedVersionText + 1)
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = $candidateModVersion + '-candidate'
}
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}
$artifactsRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'artifacts'))
$tmpRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'Tmp'))
$tmpOwnerRoot = Join-Path $tmpRoot 'StableGyro'
$project = Join-Path $modRoot 'Voyage.uproject'
$provenancePath = Join-Path $modRoot 'GAME_DERIVED_SOURCES.md'
$buildBatch = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$unrealEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$engineVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
$fingerprinter = Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1'
$cookScript = Join-Path $modRoot 'Cook-StableGyroAssets.ps1'
$prepareScript = Join-Path $modRoot 'Prepare-StableGyroOriginals.ps1'
$packageScript = Join-Path $modRoot 'Build-InheritancePackage.ps1'
$settingsSchema = Join-Path $modRoot 'Settings\StableGyro.settings.json'
$settingsDefaults = Join-Path $modRoot 'Assets\StableGyro.ini'
$settingsGenerator = Join-Path $modRoot 'Build\New-StableGyroSettings.ps1'
$modelExporter = Join-Path $repoRoot 'tools\Export-VoyageModelGlb.ps1'
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
        'EnableFuelDiagnostics'
    )
    $retiredComments = @(
        '# Absolute delay after releasing Space at full throttle before exact altitude hold activates.',
        '# A release below full throttle is ignored. Set to 0 for immediate activation. Clamped to 0..60 seconds.',
        '# Keep vertical rotor lift while pitch is tilted forward/back.',
        '# Scale the separate world-up force: 0=none, 1=lost vertical component. Clamped to 0..5.',
        '# Percent of the stock rotor''s physical maximum speed (0 disables the gate).',
        '# Percent of the stock rotor''s normal 100% operating speed (0 disables the gate).',
        '# Temporary compact fuel/throttle diagnostics; set false to hide on the next Gyro entry.',
        '# Valid range is 0.5..2.5; missing or invalid values fall back to 1.0.'
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
            if ((Get-FileHash -LiteralPath $SettingsPath -Algorithm SHA256).Hash -cne
                $result.Sha256) { throw 'StableGyro settings readback hash mismatch.' }
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
function Invoke-UnrealGenerator {
    param(
        [Parameter(Mandatory = $true)] [string]$Commandlet,
        [Parameter(Mandatory = $true)] [string]$LogDirectory,
        [string[]]$ExtraArguments = @()
    )

    $stdout = Join-Path $LogDirectory "$Commandlet.stdout.log"
    $stderr = Join-Path $LogDirectory "$Commandlet.stderr.log"
    $arguments = @(
        ('"{0}"' -f $project),
        "-run=$Commandlet",
        '-unattended',
        '-nop4',
        '-nosplash',
        '-nullrhi',
        '-ddc=NoZenLocalFallback',
        ('-LocalDataCachePath="{0}"' -f (Join-Path $modRoot 'DerivedDataCache'))
    )
    $arguments += $ExtraArguments
    $process = Start-Process `
        -FilePath $unrealEditor `
        -ArgumentList $arguments `
        -Wait `
        -PassThru `
        -WindowStyle Hidden `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr
    if ($process.ExitCode -ne 0) {
        Write-Host "Last output from $Commandlet"
        Get-Content -LiteralPath $stdout -Tail 40 -ErrorAction SilentlyContinue
        Get-Content -LiteralPath $stderr -Tail 40 -ErrorAction SilentlyContinue
        throw "$Commandlet failed with exit code $($process.ExitCode). Full logs: $LogDirectory"
    }
}

function Get-CleanOriginalInputs {
    param(
        [Parameter(Mandatory = $true)] [string]$Root,
        [Parameter(Mandatory = $true)] [string]$ExpectedBuild,
        [Parameter(Mandatory = $true)] [string]$ExpectedRetocCompatibilityVersion,
        [Parameter(Mandatory = $true)] [string]$ExpectedRetocHash
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
        [string]$manifest.retocSha256 -cne $ExpectedRetocHash -or
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
$buildBatch = Resolve-RequiredPath -Path $buildBatch -Label 'Unreal build script'
$unrealEditor = Resolve-RequiredPath -Path $unrealEditor -Label 'UnrealEditor-Cmd'
$engineVersionPath = Resolve-RequiredPath -Path $engineVersionPath -Label 'Unreal build version'
$fingerprinter = Resolve-RequiredPath -Path $fingerprinter -Label 'Voyage fingerprint tool'
$cookScript = Resolve-RequiredPath -Path $cookScript -Label 'StableGyro cook script'
$prepareScript = Resolve-RequiredPath -Path $prepareScript -Label 'StableGyro original preparer'
$packageScript = Resolve-RequiredPath -Path $packageScript -Label 'StableGyro package builder'
$settingsSchema = Resolve-RequiredPath -Path $settingsSchema -Label 'StableGyro settings schema'
$settingsDefaults = Resolve-RequiredPath -Path $settingsDefaults -Label 'StableGyro canonical settings INI'
$settingsGenerator = Resolve-RequiredPath -Path $settingsGenerator -Label 'StableGyro settings generator'
$modelExporter = Resolve-RequiredPath -Path $modelExporter -Label 'Voyage model exporter'
$mooringSplitter = Resolve-RequiredPath -Path $mooringSplitter -Label 'Mooring plate splitter'
$BlenderPath = Resolve-RequiredPath -Path $BlenderPath -Label 'Blender 5 executable'
$Retoc = Resolve-RequiredPath -Path $Retoc -Label 'retoc'
$retocSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Retoc).Hash
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
$sourceCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to resolve the source commit.'
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
$generatedSettingsDirectory = Join-Path $modRoot 'Intermediate\GeneratedSettings'
$releaseGeneratedSettingsDirectory = Join-Path $releaseRoot 'generated-settings'
$generatedSettingsHeader = Join-Path $generatedSettingsDirectory 'StableGyroSettings.generated.h'
$generatedSettingsIni = Join-Path $generatedSettingsDirectory 'StableGyro.ini'
& $settingsGenerator `
    -SchemaPath $settingsSchema `
    -DefaultIniPath $settingsDefaults `
    -HeaderPath $generatedSettingsHeader `
    -IniPath $generatedSettingsIni
if ((Get-FileHash -LiteralPath $settingsDefaults -Algorithm SHA256).Hash -cne
    (Get-FileHash -LiteralPath $generatedSettingsIni -Algorithm SHA256).Hash) {
    throw 'Generated settings INI differs from the canonical source INI.'
}
New-Item -ItemType Directory -Path $releaseGeneratedSettingsDirectory | Out-Null
Copy-Item -LiteralPath $generatedSettingsHeader -Destination $releaseGeneratedSettingsDirectory
Copy-Item -LiteralPath $generatedSettingsIni -Destination $releaseGeneratedSettingsDirectory

$totalStopwatch = [Diagnostics.Stopwatch]::StartNew()
$timings = [ordered]@{}

Write-Host '1/7 Validating game fingerprint'
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
    $splitEvidence.rawSourceNormalTrianglesRestored -ne 13790 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.positiveY.triangles -ne 6895 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.positiveY.nonNormalBytesChanged -ne 0 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.negativeY.triangles -ne 6895 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.negativeY.nonNormalBytesChanged -ne 0 -or
    $splitEvidence.manualFacesRemoved -ne 0) {
    throw 'Mooring plate split evidence does not match the reviewed stock geometry.'
}
$phase.Stop()
$timings.MooringGeometry = $phase.Elapsed.TotalSeconds

Write-Host '3/8 Building editor modules incrementally'
$phase.Restart()
$buildOutput = @(& $buildBatch VoyageEditor Win64 Development $project -NoUBA 2>&1)
$buildExitCode = $LASTEXITCODE
$buildOutput | Set-Content -LiteralPath (Join-Path $logs 'editor-build.log') -Encoding utf8
if ($buildExitCode -ne 0) {
    $buildOutput | Select-Object -Last 60 | Write-Host
    throw "VoyageEditor build failed with exit code $buildExitCode."
}
$phase.Stop()
$timings.EditorBuild = $phase.Elapsed.TotalSeconds

Write-Host '4/8 Generating Blueprint source assets'
$phase.Restart()
$content = [IO.Path]::GetFullPath((Join-Path $modRoot 'Content'))
$expectedContent = [IO.Path]::GetFullPath((Join-Path $modRoot 'Content'))
if ($content -cne $expectedContent -or (Split-Path -Leaf $content) -cne 'Content') {
    throw "Refusing to clear an unexpected generated directory: $content"
}
if (Test-Path -LiteralPath $content) {
    $contentItem = Get-Item -LiteralPath $content -Force
    if ($contentItem.Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw "Refusing to clear reparse-point Content directory: $content"
    }
    Remove-Item -LiteralPath $content -Recurse -Force
}
New-Item -ItemType Directory -Path $content | Out-Null
Invoke-UnrealGenerator -Commandlet 'GenerateStableGyroMod' -LogDirectory $logs
Invoke-UnrealGenerator -Commandlet 'GenerateStableGyroInheritance' -LogDirectory $logs `
    -ExtraArguments @(
        ('-MooringPositiveYGlb="{0}"' -f $positiveMooringGlb),
        ('-MooringNegativeYGlb="{0}"' -f $negativeMooringGlb)
    )
$phase.Stop()
$timings.Generation = $phase.Elapsed.TotalSeconds

Write-Host '5/8 Cooking seven generated production packages in one Unreal process'
$phase.Restart()
$cookedRoot = Join-Path $releaseRoot 'cooked'
& $cookScript `
    -UnrealEditor $unrealEditor `
    -Project $project `
    -LocalDataCachePath (Join-Path $modRoot 'DerivedDataCache') `
    -LogPath (Join-Path $logs 'cook.log') `
    -OutputRoot $cookedRoot
$phase.Stop()
$timings.Cook = $phase.Elapsed.TotalSeconds

Write-Host '6/8 Resolving the clean stock Gyro item data asset'
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
    -ExpectedRetocCompatibilityVersion $retocCompatibilityVersion `
    -ExpectedRetocHash $retocSha256
$phase.Stop()
$timings.Originals = $phase.Elapsed.TotalSeconds

Write-Host '7/8 Building and verifying the eight-asset IoStore container'
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
$phase.Stop()
$timings.Package = $phase.Elapsed.TotalSeconds

Write-Host '8/8 Creating the player-facing archive'
$phase.Restart()
$containerFiles = Join-Path $containerRoot 'package'
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
            if ((Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash -cne
                $settingsCreate.Sha256) {
                throw 'Created StableGyro settings readback hash mismatch.'
            }
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
        sha256 = (Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash
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
                sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
            }
        }
)
$manifest = [ordered]@{
    schemaVersion = 2
    mod = 'StableGyro'
    version = $Version
    modVersion = $candidateModVersion
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    sourceCommit = $sourceCommit
    dirtySource = ($sourceStatus.Count -gt 0)
    sourceStatus = $sourceStatus
    gameEngineVersion = $expectedGameEngineVersion
    editorEngineVersion = $actualEngineVersion
    retocCompatibilityVersion = $retocCompatibilityVersion
    retocSha256 = $retocSha256
    cookStorage = $cookStorage
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
    Write-Host 'The prepared container was backed up and installed with matching hashes.'
}

[pscustomobject]@{
    status = if ($installed) { 'installed' } else { 'prepared-not-installed' }
    releaseManifestPath = $manifestPath
    archivePath = $archivePath
    settingsInstallation = $settingsInstallation
} | ConvertTo-Json -Depth 6 -Compress
