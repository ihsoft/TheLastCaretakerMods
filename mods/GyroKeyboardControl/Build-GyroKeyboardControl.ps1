# HAND-WRITTEN BUILD ORCHESTRATOR: the only public producer. It builds,
# generates, cooks, packages, verifies, archives, and optionally installs
# GyroKeyboardControl. It creates only ignored local artifacts and
# never publishes externally.

[CmdletBinding()]
param(
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]*$')]
    [string]$Version,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',

    [string]$Retoc,

    [string]$OutputRoot,

    [string]$OriginalsRoot,

    [switch]$Install
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$expectedGameEngineVersion = '5.8'
$expectedEditorEngineVersion = '5.8.2'
$retocCompatibilityVersion = 'UE5_8'
$cookStorage = 'LooseCookedPackageWriter'

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$versionMetadataPath = Join-Path $modRoot 'VERSION.json'
if (-not (Test-Path -LiteralPath $versionMetadataPath -PathType Leaf)) {
    throw "GyroKeyboard version metadata was not found: $versionMetadataPath"
}
$versionMetadata = Get-Content -LiteralPath $versionMetadataPath -Raw | ConvertFrom-Json
$modVersion = [string]$versionMetadata.current.modVersion
$testedGameVersion = $versionMetadata.current.testedGame.gameVersion
if ($modVersion -notmatch '^v[1-9][0-9]*$') {
    throw "GyroKeyboard version metadata is invalid: $versionMetadataPath"
}
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = $modVersion
}
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}
$artifactsRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'artifacts'))
$project = Join-Path $modRoot 'Voyage.uproject'
$provenancePath = Join-Path $modRoot 'GAME_DERIVED_SOURCES.md'
$buildBatch = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$unrealEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$engineVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
$fingerprinter = Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1'
$cookScript = Join-Path $modRoot 'Cook-GyroKeyboardAssets.ps1'
$prepareScript = Join-Path $modRoot 'Prepare-GyroKeyboardOriginals.ps1'
$packageScript = Join-Path $modRoot 'Build-InheritancePackage.ps1'
$settingsSchema = Join-Path $modRoot 'Settings\GyroKeyboardControl.settings.json'
$settingsDefaults = Join-Path $modRoot 'Assets\GyroKeyboardControl.ini'
$settingsGenerator = Join-Path $modRoot 'Build\New-GyroKeyboardControlSettings.ps1'
$containerName = 'GyroKeyboardControl_P'
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

function Assert-UnderArtifacts {
    param([Parameter(Mandatory = $true)] [string]$Path, [Parameter(Mandatory = $true)] [string]$Label)

    $fullPath = [IO.Path]::GetFullPath($Path)
    $prefix = $artifactsRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label must stay below the ignored artifacts directory: $fullPath"
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

function Add-MissingGyroKeyboardSettings {
    param([string]$TemplatePath, [string]$SettingsPath)

    $existingKeys = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($line in [IO.File]::ReadAllLines($SettingsPath)) {
        if ($line -match '^\s*([^#;][^=]*?)\s*=') {
            $null = $existingKeys.Add($matches[1].Trim())
        }
    }
    $missing = @(
        [IO.File]::ReadAllLines($TemplatePath) | Where-Object {
            $_ -match '^\s*([^#;][^=]*?)\s*=' -and
            -not $existingKeys.Contains($matches[1].Trim())
        }
    )
    if ($missing.Count -eq 0) { return @() }
    $newline = [Environment]::NewLine
    $current = [IO.File]::ReadAllText($SettingsPath)
    $prefix = if ($current.EndsWith($newline)) { '' } else { $newline }
    $addition = $prefix + $newline +
        '# Defaults added by a newer GyroKeyboardControl build.' + $newline +
        (($missing -join $newline) + $newline)
    [IO.File]::AppendAllText(
        $SettingsPath, $addition, (New-Object System.Text.UTF8Encoding($false)))
    return @($missing | ForEach-Object { ($_ -split '=', 2)[0].Trim() })
}

function Invoke-UnrealGenerator {
    param(
        [Parameter(Mandatory = $true)] [string]$Commandlet,
        [Parameter(Mandatory = $true)] [string]$LogDirectory
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
        [Parameter(Mandatory = $true)] [string]$ExpectedExecutableHash,
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
        [string]$manifest.executableSha256 -cne $ExpectedExecutableHash -or
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
$provenancePath = Resolve-RequiredPath -Path $provenancePath -Label 'GyroKeyboard provenance registry'
$buildBatch = Resolve-RequiredPath -Path $buildBatch -Label 'Unreal build script'
$unrealEditor = Resolve-RequiredPath -Path $unrealEditor -Label 'UnrealEditor-Cmd'
$engineVersionPath = Resolve-RequiredPath -Path $engineVersionPath -Label 'Unreal build version'
$fingerprinter = Resolve-RequiredPath -Path $fingerprinter -Label 'Voyage fingerprint tool'
$cookScript = Resolve-RequiredPath -Path $cookScript -Label 'GyroKeyboard cook script'
$prepareScript = Resolve-RequiredPath -Path $prepareScript -Label 'GyroKeyboard original preparer'
$packageScript = Resolve-RequiredPath -Path $packageScript -Label 'GyroKeyboard package builder'
$settingsSchema = Resolve-RequiredPath -Path $settingsSchema -Label 'GyroKeyboard settings schema'
$settingsDefaults = Resolve-RequiredPath -Path $settingsDefaults -Label 'GyroKeyboard canonical settings INI'
$settingsGenerator = Resolve-RequiredPath -Path $settingsGenerator -Label 'GyroKeyboard settings generator'
$Retoc = Resolve-RequiredPath -Path $Retoc -Label 'retoc'
$retocSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Retoc).Hash
$GameRoot = Resolve-RequiredPath -Path $GameRoot -Label 'Voyage game root'

$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$actualEngineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
if ($actualEngineVersion -cne $expectedEditorEngineVersion) {
    throw "GyroKeyboard requires Unreal Engine $expectedEditorEngineVersion; selected engine is $actualEngineVersion."
}

$sourceStatus = @(& git -C $repoRoot status --porcelain -- `
    'mods/GyroKeyboardControl' `
    ':(exclude)mods/GyroKeyboardControl/Slideshow')
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect GyroKeyboard source status.'
}
$sourceCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to resolve the source commit.'
}

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $buildIdentity = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
    $OutputRoot = Join-Path $artifactsRoot "gyro-keyboard\build-$buildIdentity"
}
$releaseRoot = Assert-UnderArtifacts -Path $OutputRoot -Label 'OutputRoot'
if (Test-Path -LiteralPath $releaseRoot) {
    throw "OutputRoot already exists: $releaseRoot"
}
New-Item -ItemType Directory -Path $releaseRoot | Out-Null
$logs = Join-Path $releaseRoot 'logs'
New-Item -ItemType Directory -Path $logs | Out-Null
$generatedSettingsDirectory = Join-Path $releaseRoot 'generated-settings'
$generatedSettingsHeader = Join-Path $generatedSettingsDirectory 'GyroKeyboardControlSettings.generated.h'
$generatedSettingsIni = Join-Path $generatedSettingsDirectory 'GyroKeyboardControl.ini'
& $settingsGenerator `
    -SchemaPath $settingsSchema `
    -DefaultIniPath $settingsDefaults `
    -HeaderPath $generatedSettingsHeader `
    -IniPath $generatedSettingsIni
if ((Get-FileHash -LiteralPath $settingsDefaults -Algorithm SHA256).Hash -cne
    (Get-FileHash -LiteralPath $generatedSettingsIni -Algorithm SHA256).Hash) {
    throw 'Generated settings INI differs from the canonical source INI.'
}
[Environment]::SetEnvironmentVariable(
    'GYRO_KEYBOARD_GENERATED_SETTINGS_DIR', $generatedSettingsDirectory, 'Process')

$totalStopwatch = [Diagnostics.Stopwatch]::StartNew()
$timings = [ordered]@{}

Write-Host '1/7 Validating game fingerprint'
$phase = [Diagnostics.Stopwatch]::StartNew()
$fingerprintPath = Join-Path $releaseRoot 'game-fingerprint.json'
& $fingerprinter -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$provenance = Get-Content -LiteralPath $provenancePath -Raw
$buildMatch = [regex]::Match(
    $provenance,
    '(?m)^- Steam build:\s+(?:\x60)?(?<value>\d+)(?:\x60)?\r?$')
$hashMatch = [regex]::Match(
    $provenance,
    '(?m)^  (?:\x60)?(?<value>[0-9A-F]{64})(?:\x60)?\r?$')
if (-not $buildMatch.Success -or -not $hashMatch.Success) {
    throw "Unable to read the expected game fingerprint from $provenancePath"
}
$expectedBuild = $buildMatch.Groups['value'].Value
$expectedExecutableHash = $hashMatch.Groups['value'].Value
if ([string]$fingerprint.steam.buildId -cne $expectedBuild -or
    [string]$fingerprint.executable.sha256 -cne $expectedExecutableHash) {
    throw "Installed Voyage fingerprint does not match the validated GyroKeyboard source. See $fingerprintPath"
}
$phase.Stop()
$timings.Fingerprint = $phase.Elapsed.TotalSeconds

Write-Host '2/7 Building editor modules incrementally'
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

Write-Host '3/7 Generating Blueprint source assets'
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
Invoke-UnrealGenerator -Commandlet 'GenerateGyroKeyboardMod' -LogDirectory $logs
Invoke-UnrealGenerator -Commandlet 'GenerateGyroKeyboardInheritance' -LogDirectory $logs
$phase.Stop()
$timings.Generation = $phase.Elapsed.TotalSeconds

Write-Host '4/7 Cooking four generated production packages in one Unreal process'
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

Write-Host '5/7 Resolving the clean stock Gyro item data asset'
$phase.Restart()
if ([string]::IsNullOrWhiteSpace($OriginalsRoot)) {
    Assert-GameClosed
    $OriginalsRoot = Join-Path $releaseRoot 'originals'
    & $prepareScript -GameRoot $GameRoot -Retoc $Retoc -OutputRoot $OriginalsRoot
} else {
    $OriginalsRoot = Assert-UnderArtifacts -Path $OriginalsRoot -Label 'OriginalsRoot'
}
$originalInputs = Get-CleanOriginalInputs `
    -Root $OriginalsRoot `
    -ExpectedBuild $expectedBuild `
    -ExpectedExecutableHash $expectedExecutableHash `
    -ExpectedRetocCompatibilityVersion $retocCompatibilityVersion `
    -ExpectedRetocHash $retocSha256
$phase.Stop()
$timings.Originals = $phase.Elapsed.TotalSeconds

Write-Host '6/7 Building and verifying the five-asset IoStore container'
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
}
& $packageScript @packageArguments
$phase.Stop()
$timings.Package = $phase.Elapsed.TotalSeconds

Write-Host '7/7 Creating the player-facing archive'
$phase.Restart()
$containerFiles = Join-Path $containerRoot 'package'
$payloadRoot = Join-Path $releaseRoot "payload\GyroKeyboardControl-$Version"
New-Item -ItemType Directory -Path $payloadRoot -Force | Out-Null
foreach ($name in $payloadNames) {
    Copy-Item -LiteralPath (Join-Path $containerFiles $name) -Destination (Join-Path $payloadRoot $name)
}
Copy-Item -LiteralPath (Join-Path $modRoot 'README.txt') -Destination (Join-Path $payloadRoot 'README.txt')
Copy-Item -LiteralPath $generatedSettingsIni -Destination (Join-Path $payloadRoot 'GyroKeyboardControl.ini')
$archivePath = Join-Path $releaseRoot "GyroKeyboardControl-$Version.zip"
$installedArchiveName = "GyroKeyboardControl_$Version.zip"
$payloadFiles = @(Get-ChildItem -LiteralPath $payloadRoot -File | Select-Object -ExpandProperty FullName)
Compress-Archive -LiteralPath $payloadFiles -DestinationPath $archivePath -CompressionLevel Optimal
$phase.Stop()
$timings.Archive = $phase.Elapsed.TotalSeconds

$installed = $false
$installedArchive = $null
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
    $archiveTarget = Join-Path $paks $installedArchiveName
    if (Test-Path -LiteralPath $archiveTarget -PathType Leaf) {
        Copy-Item -LiteralPath $archiveTarget -Destination (Join-Path $backup $installedArchiveName)
    }
    Copy-Item -LiteralPath $archivePath -Destination $archiveTarget -Force
    $archiveSourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash
    $archiveTargetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archiveTarget).Hash
    if ($archiveSourceHash -cne $archiveTargetHash) {
        throw "Installed archive hash mismatch: $installedArchiveName"
    }
    $installedArchive = [ordered]@{
        name = $installedArchiveName
        size = (Get-Item -LiteralPath $archiveTarget).Length
        sha256 = $archiveTargetHash
    }
    $settingsTemplate = $generatedSettingsIni
    $settingsPath = Join-Path $paks 'GyroKeyboardControl.ini'
    if (-not (Test-Path -LiteralPath $settingsPath -PathType Leaf)) {
        Copy-Item -LiteralPath $settingsTemplate -Destination $settingsPath
        $addedKeys = @()
        $settingsCreated = $true
    } else {
        $addedKeys = @(Add-MissingGyroKeyboardSettings $settingsTemplate $settingsPath)
        $settingsCreated = $false
    }
    $settingsInstallation = [ordered]@{
        path = $settingsPath
        created = $settingsCreated
        addedKeys = $addedKeys
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
    mod = 'GyroKeyboardControl'
    version = $Version
    modVersion = $modVersion
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    sourceCommit = $sourceCommit
    dirtySource = ($sourceStatus.Count -gt 0)
    sourceStatus = $sourceStatus
    gameEngineVersion = $expectedGameEngineVersion
    testedGameVersion = $testedGameVersion
    compatibilityPolicy = 'tested-evidence-not-runtime-allowlist'
    editorEngineVersion = $actualEngineVersion
    retocCompatibilityVersion = $retocCompatibilityVersion
    retocSha256 = $retocSha256
    cookStorage = $cookStorage
    gameFingerprint = [ordered]@{
        steamBuildId = [string]$fingerprint.steam.buildId
        executableSha256 = [string]$fingerprint.executable.sha256
    }
    originalsManifest = $originalInputs.Manifest
    installed = $installed
    installedArchive = $installedArchive
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
Write-Host "GyroKeyboard release artifact is ready: $archivePath"
Write-Host "Manifest: $manifestPath"
Write-Host ("Elapsed: {0:N1} seconds" -f $totalStopwatch.Elapsed.TotalSeconds)
if ($installed) {
    Write-Host 'The prepared container was backed up and installed with matching hashes.'
}

[pscustomobject]@{
    status = if ($installed) { 'installed' } else { 'prepared-not-installed' }
    releaseManifestPath = $manifestPath
    archivePath = $archivePath
    installedArchive = $installedArchive
    settingsInstallation = $settingsInstallation
} | ConvertTo-Json -Depth 6 -Compress
