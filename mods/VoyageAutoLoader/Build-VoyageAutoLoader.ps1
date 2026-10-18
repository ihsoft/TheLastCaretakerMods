[CmdletBinding()]
param(
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [string]$Retoc,
    [string]$OutputRoot,
    [string]$Version,
    [switch]$AllowDirtySource,
    [switch]$IgnorePinnedGameVersion
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$ownerTmp = [IO.Path]::GetFullPath((Join-Path $repoRoot 'Tmp\VoyageAutoLoader'))
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $ownerTmp ('build-' + [Guid]::NewGuid().ToString('N'))
}
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not ($output + '\').StartsWith(($ownerTmp + '\'),
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must be a fresh directory below $ownerTmp"
}
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$project = (Resolve-Path -LiteralPath (Join-Path $modRoot 'Voyage.uproject')).Path
$buildBatch = (Resolve-Path -LiteralPath (
    Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat')).Path
$editor = (Resolve-Path -LiteralPath (
    Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe')).Path
$engineBuildVersion = Get-Content -LiteralPath (
    Join-Path $EngineRoot 'Engine\Build\Build.version') -Raw | ConvertFrom-Json
$editorVersion = '{0}.{1}.{2}' -f @(
    $engineBuildVersion.MajorVersion,
    $engineBuildVersion.MinorVersion,
    $engineBuildVersion.PatchVersion)
if ($editorVersion -cne '5.8.2') {
    throw "VoyageAutoLoader requires UE editor 5.8.2; found $editorVersion"
}

$fingerprintPath = Join-Path $output 'game-fingerprint.json'
& (Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1') `
    -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$gameBuild = & (Join-Path $repoRoot 'tools\Assert-VoyageModGameBuild.ps1') `
    -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) `
    -IgnorePinnedGameVersion:$IgnorePinnedGameVersion

$defaultEngineIni = Join-Path $modRoot 'Config\DefaultEngine.ini'
$defaultEngineIniBefore = [IO.File]::ReadAllBytes($defaultEngineIni)
try {
$generatedContent = Join-Path $modRoot 'Content'
if (Test-Path -LiteralPath $generatedContent) {
    $resolvedContent = (Resolve-Path -LiteralPath $generatedContent).Path
    if (-not $resolvedContent.StartsWith(
            $modRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean Content outside the mod: $resolvedContent"
    }
    Remove-Item -LiteralPath $resolvedContent -Recurse -Force
}

$buildLog = Join-Path $output 'editor-build.log'
$buildArguments = @(
    'VoyageEditor', 'Win64', 'Development',
    ('-Project="{0}"' -f $project),
    ('-Log="{0}"' -f (Join-Path $output 'unreal-build-tool.log')),
    '-WaitMutex', '-NoHotReload', '-NoUBA'
)
$buildProcess = Start-Process -FilePath $buildBatch -ArgumentList $buildArguments `
    -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput $buildLog `
    -RedirectStandardError (Join-Path $output 'editor-build-errors.log')
if ($buildProcess.ExitCode -ne 0) {
    throw "VoyageEditor build failed with exit code $($buildProcess.ExitCode); see $buildLog"
}

$ddc = Join-Path $modRoot 'DerivedDataCache'
$generateLog = Join-Path $output 'generate.log'
$generateArguments = @(
    ('"{0}"' -f $project), '-run=GenerateVoyageAutoLoader',
    '-unattended', '-nop4', '-nosplash', '-nullrhi',
    '-ddc=NoZenLocalFallback',
    ('-LocalDataCachePath="{0}"' -f $ddc),
    ('-abslog="{0}"' -f $generateLog)
)
$generateProcess = Start-Process -FilePath $editor -ArgumentList $generateArguments `
    -Wait -PassThru -WindowStyle Hidden
if ($generateProcess.ExitCode -ne 0) {
    throw "VoyageAutoLoader generation failed with exit code $($generateProcess.ExitCode); see $generateLog"
}

# The generated stock GameMode mirror is an editor parent, not release content.
$packages = @(
    [pscustomobject]@{ name = '/Game/Maps/Empty'; relative = 'Maps/Empty'; extension = '.umap' },
    [pscustomobject]@{ name = '/Game/VoyageAutoLoader/BP_AutoLoaderMenuGameMode'; relative = 'VoyageAutoLoader/BP_AutoLoaderMenuGameMode'; extension = '.uasset' },
    [pscustomobject]@{ name = '/Game/VoyageAutoLoader/WBP_AutoLoaderControl'; relative = 'VoyageAutoLoader/WBP_AutoLoaderControl'; extension = '.uasset' },
    [pscustomobject]@{ name = '/Game/VoyageAutoLoader/BP_AutoLoaderWorker'; relative = 'VoyageAutoLoader/BP_AutoLoaderWorker'; extension = '.uasset' }
)
$cookedPlatform = Join-Path $modRoot 'Saved\Cooked\Windows\Voyage\Content'
$cookedSnapshot = Join-Path $output 'cooked'
foreach ($entry in $packages) {
    $liveBase = Join-Path $cookedPlatform $entry.relative
    foreach ($extension in @($entry.extension, '.uexp')) {
        $stale = $liveBase + $extension
        if (Test-Path -LiteralPath $stale -PathType Leaf) {
            Remove-Item -LiteralPath $stale -Force
        }
    }
    $cookLog = Join-Path $output (
        'cook-' + [IO.Path]::GetFileName($entry.relative) + '.log')
    $cookArguments = @(
        ('"{0}"' -f $project), '-run=cook', '-targetplatform=Windows',
        '-unversioned', '-SkipZenStore', ('-Package={0}' -f $entry.name),
        '-CookSinglePackageNoRefs', '-unattended', '-nop4', '-nosplash',
        '-nullrhi', '-ddc=NoZenLocalFallback',
        ('-LocalDataCachePath="{0}"' -f $ddc),
        ('-abslog="{0}"' -f $cookLog)
    )
    $cookProcess = Start-Process -FilePath $editor -ArgumentList $cookArguments `
        -Wait -PassThru -WindowStyle Hidden
    if ($cookProcess.ExitCode -ne 0) {
        throw "Cook failed for $($entry.name) with exit code $($cookProcess.ExitCode); see $cookLog"
    }
    $destinationBase = Join-Path $cookedSnapshot $entry.relative
    [IO.Directory]::CreateDirectory(
        [IO.Path]::GetDirectoryName($destinationBase)) | Out-Null
    foreach ($extension in @($entry.extension, '.uexp')) {
        $source = $liveBase + $extension
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Expected cooked file was not produced: $source"
        }
        Copy-Item -LiteralPath $source -Destination ($destinationBase + $extension)
    }
}

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
[IO.Directory]::CreateDirectory($stage) | Out-Null
[IO.Directory]::CreateDirectory($package) | Out-Null
$stock = & (Join-Path $repoRoot 'tools\Extract-VoyagePackage.ps1') `
    -Filter 'Game/BP_VoyageGameModeMenu' -GameRoot $GameRoot `
    -Retoc $retocPath -RetocEngineVersion UE5_8 `
    -OutputRoot (Join-Path $output 'stock')
$stockManifest = Get-Content -LiteralPath $stock.manifestPath -Raw | ConvertFrom-Json
if ([string]$stockManifest.steamBuildId -cne [string]$fingerprint.steam.buildId -or
    $stockManifest.source -cne 'Game' -or
    $stockManifest.allowAdditionalContainers -ne $false) {
    throw 'The stock extraction does not match the installed game.'
}
$scriptObjects = @(Get-ChildItem -LiteralPath $stock.outputPath -File -Recurse `
    -Filter 'scriptobjects.bin')
if ($scriptObjects.Count -ne 1) {
    throw 'Expected one current-game scriptobjects.bin in the stock extraction.'
}
Copy-Item -LiteralPath $scriptObjects[0].FullName `
    -Destination (Join-Path $stage 'scriptobjects.bin')
foreach ($entry in $packages) {
    $sourceBase = Join-Path $cookedSnapshot $entry.relative
    $destinationBase = Join-Path (Join-Path $stage 'Voyage\Content') $entry.relative
    [IO.Directory]::CreateDirectory(
        [IO.Path]::GetDirectoryName($destinationBase)) | Out-Null
    foreach ($extension in @($entry.extension, '.uexp')) {
        Copy-Item -LiteralPath ($sourceBase + $extension) `
            -Destination ($destinationBase + $extension)
    }
}

$containerName = 'VoyageAutoLoader_P'
$utoc = Join-Path $package ($containerName + '.utoc')
& $retocPath to-zen --version UE5_8 $stage $utoc
if ($LASTEXITCODE -ne 0) { throw "retoc to-zen failed: $LASTEXITCODE" }
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) { throw "retoc verify failed: $LASTEXITCODE" }
$inventory = @(& $retocPath list --path --size --hash --package $utoc)
if ($LASTEXITCODE -ne 0) { throw "retoc list failed: $LASTEXITCODE" }
$actualPaths = @($inventory | ForEach-Object {
    if ($_ -match '(?<path>\.\./\.\./\.\./Voyage/Content/\S+\.(?:uasset|umap))$') {
        $Matches.path
    }
} | Sort-Object)
$expectedPaths = @($packages | ForEach-Object {
    '../../../Voyage/Content/' + $($_.relative.Replace('\', '/')) + $_.extension
} | Sort-Object)
if ($actualPaths.Count -ne $expectedPaths.Count -or
    (@(Compare-Object $expectedPaths $actualPaths)).Count -ne 0) {
    throw "Packaged inventory is not the exact four-package loader: $($actualPaths -join ', ')"
}

$archive = Join-Path $output 'VoyageAutoLoader-candidate.zip'
& (Join-Path $repoRoot 'tools\New-VoyageVortexArchive.ps1') `
    -Container $utoc -Archive $archive | Out-Null
if ([string]::IsNullOrWhiteSpace($Version)) {
    $publishedVersion = [int](Get-Content -LiteralPath (
        Join-Path $modRoot 'version.txt') -Raw)
    $Version = 'v' + ($publishedVersion + 1) + '-candidate'
}
$release = & (Join-Path $repoRoot 'tools\New-VoyageReleaseManifest.ps1') `
    -ReleaseRoot $output -Mod VoyageAutoLoader -Version $Version `
    -Container $utoc -Archive $archive -SourcePath $modRoot `
    -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource -AsJson | ConvertFrom-Json
if ($release.status -cne 'created-and-validated') {
    throw 'VoyageAutoLoader release manifest validation failed.'
}
[pscustomobject]@{
    status = 'candidate-validated'
    outputRoot = $output
    archivePath = $archive
    releaseManifest = $release.manifestPath
    steamBuildId = [string]$fingerprint.steam.buildId
    ignoredPinnedGameVersion = [bool]$gameBuild.ignoredPinnedGameVersion
    runtimeValidated = $false
}
}
finally {
    # Unreal may append AndroidFileServerEditor settings while running the
    # editor. They are run-only state, not authored mod configuration.
    [IO.File]::WriteAllBytes($defaultEngineIni, $defaultEngineIniBefore)
}
