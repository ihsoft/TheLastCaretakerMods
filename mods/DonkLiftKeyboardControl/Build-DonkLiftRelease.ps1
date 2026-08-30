# HAND-WRITTEN RELEASE ORCHESTRATOR: builds, generates, cooks, packages, and
# archives DonkLiftKeyboardControl. It creates only ignored local artifacts and
# never publishes externally.

param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]*$')]
    [string]$Version,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EngineRoot = 'K:\Epic Games\UE_5.7',

    [string]$Retoc = 'R:\Codex\ToolCache\rust-retoc-master\source\target\release\retoc.exe',

    [string]$OutputRoot,

    [string]$OriginalsRoot,

    [switch]$Install,

    [switch]$AllowDirtySource
)

$ErrorActionPreference = 'Stop'

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$artifactsRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'artifacts'))
$project = Join-Path $modRoot 'Voyage.uproject'
$provenancePath = Join-Path $modRoot 'GAME_DERIVED_SOURCES.md'
$buildBatch = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$unrealEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$engineVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
$fingerprinter = Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1'
$cookScript = Join-Path $modRoot 'Cook-DonkLiftAssets.ps1'
$prepareScript = Join-Path $modRoot 'Prepare-DonkLiftOriginals.ps1'
$packageScript = Join-Path $modRoot 'Build-InheritancePackage.ps1'
$containerName = 'DonkLiftKeyboardControl_P'
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
        [Parameter(Mandatory = $true)] [string]$ExpectedExecutableHash
    )

    $resolvedRoot = Resolve-RequiredPath -Path $Root -Label 'Originals root'
    $manifests = @(Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter 'extraction-manifest.json')
    if ($manifests.Count -ne 1) {
        throw "Expected exactly one extraction manifest below $resolvedRoot; found $($manifests.Count)."
    }

    $manifest = Get-Content -LiteralPath $manifests[0].FullName -Raw | ConvertFrom-Json
    if ([string]$manifest.filter -cne 'Vehicles/BP_Forklift_Possesable' -or
        [string]$manifest.steamBuildId -cne $ExpectedBuild -or
        [string]$manifest.executableSha256 -cne $ExpectedExecutableHash -or
        $manifest.allowAdditionalContainers -ne $false) {
        throw "Originals manifest is not a clean extraction for the current validated game: $($manifests[0].FullName)"
    }

    $extractionRoot = $manifests[0].Directory.FullName
    $forkliftDirectory = Join-Path $extractionRoot 'Voyage\Content\Blueprints\Vehicles'
    $scriptObjects = Join-Path $extractionRoot 'scriptobjects.bin'
    foreach ($required in @(
        (Join-Path $forkliftDirectory 'BP_Forklift_Possesable.uasset'),
        (Join-Path $forkliftDirectory 'BP_Forklift_Possesable.uexp'),
        $scriptObjects
    )) {
        Resolve-RequiredPath -Path $required -Label 'Clean original input' | Out-Null
    }

    return [pscustomobject]@{
        Manifest = $manifests[0].FullName
        ForkliftDirectory = $forkliftDirectory
        ScriptObjects = $scriptObjects
    }
}

$project = Resolve-RequiredPath -Path $project -Label 'Unreal project'
$provenancePath = Resolve-RequiredPath -Path $provenancePath -Label 'DonkLift provenance registry'
$buildBatch = Resolve-RequiredPath -Path $buildBatch -Label 'Unreal build script'
$unrealEditor = Resolve-RequiredPath -Path $unrealEditor -Label 'UnrealEditor-Cmd'
$engineVersionPath = Resolve-RequiredPath -Path $engineVersionPath -Label 'Unreal build version'
$fingerprinter = Resolve-RequiredPath -Path $fingerprinter -Label 'Voyage fingerprint tool'
$cookScript = Resolve-RequiredPath -Path $cookScript -Label 'DonkLift cook script'
$prepareScript = Resolve-RequiredPath -Path $prepareScript -Label 'DonkLift original preparer'
$packageScript = Resolve-RequiredPath -Path $packageScript -Label 'DonkLift package builder'
$Retoc = Resolve-RequiredPath -Path $Retoc -Label 'retoc'
$GameRoot = Resolve-RequiredPath -Path $GameRoot -Label 'Voyage game root'

$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$actualEngineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
if ($actualEngineVersion -cne '5.7.4') {
    throw "DonkLift requires Unreal Engine 5.7.4; selected engine is $actualEngineVersion."
}

$sourceStatus = @(& git -C $repoRoot status --porcelain -- `
    'mods/DonkLiftKeyboardControl' `
    ':(exclude)mods/DonkLiftKeyboardControl/Slideshow')
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect DonkLift source status.'
}
if ($sourceStatus.Count -gt 0 -and -not $AllowDirtySource) {
    throw "DonkLift source is dirty. Commit the validated source or use -AllowDirtySource for a non-release test build.`n$($sourceStatus -join "`n")"
}
$sourceCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to resolve the source commit.'
}

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $artifactsRoot "releases\DonkLiftKeyboardControl-$Version"
}
$releaseRoot = Assert-UnderArtifacts -Path $OutputRoot -Label 'OutputRoot'
if (Test-Path -LiteralPath $releaseRoot) {
    throw "OutputRoot already exists: $releaseRoot"
}
New-Item -ItemType Directory -Path $releaseRoot | Out-Null
$logs = Join-Path $releaseRoot 'logs'
New-Item -ItemType Directory -Path $logs | Out-Null

$totalStopwatch = [Diagnostics.Stopwatch]::StartNew()
$timings = [ordered]@{}

Write-Host '1/7 Validating game fingerprint'
$phase = [Diagnostics.Stopwatch]::StartNew()
$fingerprintPath = Join-Path $releaseRoot 'game-fingerprint.json'
& $fingerprinter -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$provenance = Get-Content -LiteralPath $provenancePath -Raw
$buildMatch = [regex]::Match($provenance, '(?m)^- Steam build: `(?<value>\d+)`\r?$')
$hashMatch = [regex]::Match($provenance, '(?m)^  `(?<value>[0-9A-F]{64})`\r?$')
if (-not $buildMatch.Success -or -not $hashMatch.Success) {
    throw "Unable to read the expected game fingerprint from $provenancePath"
}
$expectedBuild = $buildMatch.Groups['value'].Value
$expectedExecutableHash = $hashMatch.Groups['value'].Value
if ([string]$fingerprint.steam.buildId -cne $expectedBuild -or
    [string]$fingerprint.executable.sha256 -cne $expectedExecutableHash) {
    throw "Installed Voyage fingerprint does not match the validated DonkLift source. See $fingerprintPath"
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
Invoke-UnrealGenerator -Commandlet 'GenerateDonkLiftMod' -LogDirectory $logs
Invoke-UnrealGenerator -Commandlet 'GenerateDonkLiftInheritance' -LogDirectory $logs
$phase.Stop()
$timings.Generation = $phase.Elapsed.TotalSeconds

Write-Host '4/7 Cooking five production packages in one Unreal process'
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

Write-Host '5/7 Resolving a clean original forklift'
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
    -ExpectedExecutableHash $expectedExecutableHash
$phase.Stop()
$timings.Originals = $phase.Elapsed.TotalSeconds

Write-Host '6/7 Building and verifying the six-asset IoStore container'
$phase.Restart()
$containerRoot = Join-Path $releaseRoot 'container'
& $packageScript `
    -CookedRoot $cookedRoot `
    -OriginalForkliftDirectory $originalInputs.ForkliftDirectory `
    -ScriptObjects $originalInputs.ScriptObjects `
    -Retoc $Retoc `
    -OutputRoot $containerRoot
$phase.Stop()
$timings.Package = $phase.Elapsed.TotalSeconds

Write-Host '7/7 Creating the player-facing archive'
$phase.Restart()
$containerFiles = Join-Path $containerRoot 'package'
$payloadRoot = Join-Path $releaseRoot "payload\DonkLiftKeyboardControl-$Version"
New-Item -ItemType Directory -Path $payloadRoot -Force | Out-Null
foreach ($name in $payloadNames) {
    Copy-Item -LiteralPath (Join-Path $containerFiles $name) -Destination (Join-Path $payloadRoot $name)
}
Copy-Item -LiteralPath (Join-Path $modRoot 'README.txt') -Destination (Join-Path $payloadRoot 'README.txt')
$archivePath = Join-Path $releaseRoot "DonkLiftKeyboardControl-$Version.zip"
$payloadFiles = @(Get-ChildItem -LiteralPath $payloadRoot -File | Select-Object -ExpandProperty FullName)
Compress-Archive -LiteralPath $payloadFiles -DestinationPath $archivePath -CompressionLevel Optimal
$phase.Stop()
$timings.Archive = $phase.Elapsed.TotalSeconds

$installed = $false
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
    schemaVersion = 1
    mod = 'DonkLiftKeyboardControl'
    version = $Version
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    sourceCommit = $sourceCommit
    dirtySource = ($sourceStatus.Count -gt 0)
    sourceStatus = $sourceStatus
    engineVersion = $actualEngineVersion
    gameFingerprint = [ordered]@{
        steamBuildId = [string]$fingerprint.steam.buildId
        executableSha256 = [string]$fingerprint.executable.sha256
    }
    originalsManifest = $originalInputs.Manifest
    installed = $installed
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
Write-Host "DonkLift release artifact is ready: $archivePath"
Write-Host "Manifest: $manifestPath"
Write-Host ("Elapsed: {0:N1} seconds" -f $totalStopwatch.Elapsed.TotalSeconds)
if ($installed) {
    Write-Host 'The prepared container was backed up and installed with matching hashes.'
}
