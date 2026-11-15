[CmdletBinding()]
param(
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$OutputRoot,
    [switch]$AllowDirtySource,
    [switch]$IgnorePinnedGameVersion
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $repo 'tools/unreal-coordination.psm1') -Force
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Build ElectrifiedBuildings'
    return
}
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $PSScriptRoot -OutputRoot $OutputRoot -Kind 'build'
$fingerprint = & (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath (Join-Path $run 'game-build.json') | ConvertFrom-Json
$pin = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'game-build.json') -Raw | ConvertFrom-Json
if (-not $IgnorePinnedGameVersion -and $fingerprint.steam.buildId -ne $pin.steam.buildId) {
    throw "Pinned Steam build $($pin.steam.buildId) differs from installed $($fingerprint.steam.buildId). Use -IgnorePinnedGameVersion for a compatibility candidate."
}
$project = New-VoyageModProjectCopy -ModRoot $PSScriptRoot -Destination (Join-Path $run 'Project')
$workspace = Join-Path $repo ('Tmp/BlueprintText/ElectrifiedBuildings-build-' + [IO.Path]::GetFileName($run))
& (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $workspace > (Join-Path $run 'prepare.log')
$packages = @(
    '/Game/Mods/ElectrifiedBuildings/ModActor'
    '/Game/Mods/ElectrifiedBuildings/BP_BuildingSocket'
)
$cooked = Invoke-VoyageModCook -ModRoot $PSScriptRoot -ProjectFile $project -EngineRoot $EngineRoot -OutputRoot $run -Packages $packages -ParserVersion 'UE5_8'
$version = 'V' + (1 + [int]([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'version.txt')).Trim()))
$archive = Join-Path $run ('ElectrifiedBuildings-' + $version + '.zip')
$null = & (Join-Path $repo 'tools/New-VoyageVortexArchive.ps1') -Container $cooked.container -Archive $archive -LogicMods
$manifestPath = Join-Path $run 'release-manifest.json'
$null = & (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $run -Mod 'ElectrifiedBuildings' -Version $version -Container $cooked.container -Archive $archive -SourcePath $PSScriptRoot -GameRoot $GameRoot -OutputPath $manifestPath -LogicMods -AllowDirtySource:$AllowDirtySource
$summaryPath = Join-Path $run 'validation-summary.json'
[ordered]@{
    status = 'passed'
    packages = $packages
    containerValidationPath = [string]$cooked.validation.reportPath
    serialization = 'unversioned'
    runtimeValidated = $false
    limitation = 'Cook and package validation do not establish in-game behavior or cable restoration.'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $run -Path (Split-Path -Parent $project)
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $run -Path $cooked.stage
[ordered]@{ status='built'; archivePath=$archive; releaseManifestPath=$manifestPath; validationSummaryPath=$summaryPath } | ConvertTo-Json
