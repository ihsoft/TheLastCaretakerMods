# Build a candidate from maintained project assets through the shared pipeline.
# Optional installation uses the same public installer as an existing candidate.

[CmdletBinding()]
param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EngineRoot = 'P:\UnrealEngine\UE_5.8',

    [string]$Retoc,

    [string]$OutputRoot,

    [switch]$Install,

    [switch]$AllowDirtySource,

    [switch]$IgnorePinnedGameVersion
)

if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'StableGyro source/build operation'
    return
}
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$expectedEditorEngineVersion = '5.8.2'
$retocCompatibilityVersion = 'UE5_8'

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$publishedVersionText = (Get-Content -LiteralPath (Join-Path $modRoot 'version.txt') -Raw).Trim()
if ($publishedVersionText -cnotmatch '^(0|[1-9][0-9]*)$') {
    throw 'StableGyro published version must be a non-negative integer in version.txt.'
}
$candidateModVersion = 'V' + ([int]$publishedVersionText + 1)
$Version = $candidateModVersion
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}
$projectRoot = Join-Path $modRoot 'Project'
$project = Join-Path $projectRoot 'Voyage.uproject'
Import-Module (Join-Path $repoRoot 'tools/VoyageModPipeline.psm1') -Force
$engineVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
$fingerprinter = Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1'
$settingsDefaults = Join-Path $modRoot 'Assets\StableGyro.ini'
$releaseManifestBuilder = Join-Path $repoRoot 'tools\New-VoyageReleaseManifest.ps1'
$meshSourceStatePath = Join-Path $modRoot 'Build\MooringMeshes.source.json'
$itemSourceStatePath = Join-Path $modRoot 'Build\Item.source.json'
$containerName = 'StableGyro_P'
function Resolve-RequiredPath {
    param([Parameter(Mandatory = $true)] [string]$Path, [Parameter(Mandatory = $true)] [string]$Label)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Label was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

$project = Resolve-RequiredPath -Path $project -Label 'Unreal project'
$engineVersionPath = Resolve-RequiredPath -Path $engineVersionPath -Label 'Unreal build version'
$fingerprinter = Resolve-RequiredPath -Path $fingerprinter -Label 'Voyage fingerprint tool'
$settingsDefaults = Resolve-RequiredPath -Path $settingsDefaults -Label 'StableGyro canonical settings INI'
$releaseManifestBuilder = Resolve-RequiredPath -Path $releaseManifestBuilder -Label 'Voyage release manifest builder'
$Retoc = Resolve-RequiredPath -Path $Retoc -Label 'retoc'
$GameRoot = Resolve-RequiredPath -Path $GameRoot -Label 'Voyage game root'

$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$actualEngineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
if ($actualEngineVersion -cne $expectedEditorEngineVersion) {
    throw "StableGyro requires Unreal Engine $expectedEditorEngineVersion; selected engine is $actualEngineVersion."
}

$sourceStatus = @(& git -C $repoRoot status --porcelain -- `
    'mods/StableGyro' `
    'tools/VoyageModPipeline.psm1' `
    'tools/UnrealCookSupport' `
    'tools/VoyageModSettings' `
    ':(exclude)mods/StableGyro/Slideshow')
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect StableGyro source status.'
}
if ($sourceStatus.Count -gt 0 -and -not $AllowDirtySource) {
    throw 'StableGyro source is dirty. Pass -AllowDirtySource for an intentional development candidate.'
}
$releaseRoot = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot

$totalStopwatch = [Diagnostics.Stopwatch]::StartNew()
$timings = [ordered]@{}

Write-Host '1/4 Validating game fingerprint'
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

Assert-VoyageModAssetSourceBuild -StatePath $meshSourceStatePath -SteamBuildId $expectedBuild `
    -Label 'StableGyro mooring plate' -RefreshCommand 'mods/StableGyro/Build/Update-StableGyroMooringPlate.ps1'
Assert-VoyageModAssetSourceBuild -StatePath $itemSourceStatePath -SteamBuildId $expectedBuild `
    -Label 'StableGyro item assets' -RefreshCommand 'mods/StableGyro/Build/Update-StableGyroItemAsset.ps1'
foreach ($relativePath in @('Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset',
        'Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset')) {
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot $relativePath) -PathType Leaf)) {
        throw "Maintained item asset is missing: $relativePath. Restore it from Git or run Build/Update-StableGyroItemAsset.ps1."
    }
}
Write-Host '2/4 Copying maintained project inputs'
$phase.Restart()
$project = & (Join-Path $modRoot 'New-StableGyroProjectCopy.ps1') -Destination (Join-Path $releaseRoot 'Project')
$projectRoot = Split-Path -Parent $project
$phase.Stop()
$timings.ProjectCopy = $phase.Elapsed.TotalSeconds

Write-Host '3/4 Cooking and packaging the release inventory'
$phase.Restart()
$packages = @(
    '/Game/Data/Assets/Modules/DA_Item_Module_GyroCopter',
    '/Game/Mods/StableGyro/ModActor',
    '/Game/Mods/StableGyro/PitchInputActor',
    '/Game/Mods/StableGyro/Settings/BP_IniFileParser',
    '/Game/Mods/StableGyro/IAV_GyroPitchReset',
    '/Game/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCompassHUD',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroMapHints',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCameraCompass',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCompassDisplay',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroMinimap',
    '/Game/Mods/StableGyro/UI/M_StableGyroMinimapComposite',
    '/Game/Mods/StableGyro/UI/DA_StableGyroMap',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroMapScreen',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroMinimapDisplay',
    '/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard',
    '/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY'
)
# These UI assets use partial native editor mirrors and need tagged properties.
$taggedPackages = @(
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCompassDisplay',
    '/Game/Mods/StableGyro/UI/DA_StableGyroMap',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroMapScreen',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroMinimapDisplay'
)
$cook = Invoke-VoyageModCook -ModRoot $modRoot -ProjectFile $project -EngineRoot $EngineRoot `
    -OutputRoot $releaseRoot -Packages $packages -TaggedPackages $taggedPackages `
    -ParserVersion $retocCompatibilityVersion -GameRoot $GameRoot -Retoc $Retoc `
    -ContainerName $containerName -AdditionalCookArguments @('-LocalDataCachePath=' + (Join-Path $releaseRoot 'DerivedDataCache'))
$containerUtoc = $cook.container
$phase.Stop()
$timings.CookAndPackage = $phase.Elapsed.TotalSeconds
Write-Host '4/4 Creating the player-facing archive'
$phase.Restart()
$packagedIni = Join-Path $releaseRoot 'StableGyro.ini'
Copy-Item -LiteralPath $settingsDefaults -Destination $packagedIni
$archivePath = Join-Path $releaseRoot "StableGyro-$Version.zip"
& (Join-Path $repoRoot 'tools/New-VoyageVortexArchive.ps1') `
    -Container $containerUtoc -Archive $archivePath `
    -AdditionalFile $packagedIni | Out-Null
$manifestPath = Join-Path $releaseRoot 'release-manifest.json'
& $releaseManifestBuilder `
    -ReleaseRoot $releaseRoot `
    -Mod 'StableGyro' `
    -Version $Version `
    -ModVersion $candidateModVersion `
    -Container $containerUtoc `
    -Archive $archivePath `
    -SourcePath @($modRoot,(Join-Path $repoRoot 'tools/VoyageModPipeline.psm1'),(Join-Path $repoRoot 'tools/UnrealCookSupport')) `
    -GameRoot $GameRoot `
    -OutputPath $manifestPath `
    -AllowDirtySource:$AllowDirtySource | Out-Null
$phase.Stop()
$timings.Archive = $phase.Elapsed.TotalSeconds

$settingsInstallation = $null
if ($Install) {
    $settingsInstallation = & (Join-Path $modRoot 'Install-StableGyro.ps1') `
        -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource
}

$totalStopwatch.Stop()
$summaryPath = Join-Path $releaseRoot 'validation-summary.json'
[ordered]@{
    status = 'passed'
    runtime = 'pending'
    container = $cook.validation
    timingsSeconds = $timings
    totalSeconds = $totalStopwatch.Elapsed.TotalSeconds
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding utf8

foreach ($scratch in @('Project','DerivedDataCache','editor-support','stage')) {
    $path = Join-Path $releaseRoot $scratch
    if (Test-Path -LiteralPath $path) { Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $releaseRoot -Path $path }
}
[pscustomobject]@{
    status = 'built'
    releaseManifestPath = $manifestPath
    validationSummaryPath = $summaryPath
    archivePath = $archivePath
    settingsInstallation = $settingsInstallation
} | ConvertTo-Json -Depth 6 -Compress
