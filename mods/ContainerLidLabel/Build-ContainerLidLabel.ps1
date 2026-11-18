# Cooks the maintained Blueprint project and packages its four additive assets.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$OutputRoot,
    [switch]$AllowDirtySource,
    [switch]$IgnorePinnedGameVersion
)
$ErrorActionPreference = 'Stop'
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Build ContainerLidLabel'
    return
}
Set-StrictMode -Version Latest
$modRoot = $PSScriptRoot
$repo = (Resolve-Path (Join-Path $modRoot '../..')).Path
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1')
$output = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot
$project = & (Join-Path $modRoot 'New-ContainerLidLabelProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$projectRoot = Split-Path -Parent $project
$fingerprintPath = Join-Path $output 'game-fingerprint.json'
& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content $fingerprintPath -Raw | ConvertFrom-Json
& (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) -IgnorePinnedGameVersion:$IgnorePinnedGameVersion | Out-Null
$versionText = (Get-Content (Join-Path $modRoot 'version.txt') -Raw).Trim()
if ($versionText -cnotmatch '^(0|[1-9][0-9]*)$') { throw 'Invalid published version' }
$version = 'V' + ([int]$versionText + 1)
$names = @('ModActor', 'BP_ContainerLidDisplay', 'BFL_ContainerTypes', 'Settings/BP_IniFileParser')
$packages = @($names | ForEach-Object { '/Game/Mods/ContainerLidLabel/' + $_ })
$cook = Invoke-VoyageModCook -ModRoot $modRoot -ProjectFile $project -EngineRoot $EngineRoot -OutputRoot $output -Packages $packages -ParserVersion UE5_8
$utoc = $cook.container
$check = $cook.validation
$ini = Join-Path $output 'ContainerLidLabel.ini'
Copy-Item -LiteralPath (Join-Path $modRoot 'Assets/ContainerLidLabel.ini') -Destination $ini
$zip = Join-Path $output ('ContainerLidLabel-' + $version + '.zip')
& (Join-Path $repo 'tools/New-VoyageVortexArchive.ps1') -Container $utoc -Archive $zip -LogicMods -AdditionalFile $ini -AdditionalDestination 'Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini' | Out-Null
$manifest = & (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod ContainerLidLabel -Version $version -Container $utoc -Archive $zip -SourcePath @($modRoot,(Join-Path $repo 'tools/VoyageModPipeline.psm1'),(Join-Path $repo 'tools/UnrealCookSupport')) -GameRoot $GameRoot -LogicMods -AllowDirtySource:$AllowDirtySource
$summaryPath = Join-Path $output 'validation-summary.json'
[ordered]@{status='passed';container=$check;runtime='pending'} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
$result = [ordered]@{status='built';archivePath=$zip;releaseManifestPath=$manifest.manifestPath;validationSummaryPath=$summaryPath;entryClass='/Game/Mods/ContainerLidLabel/ModActor.ModActor_C';steamBuildId=[string]$fingerprint.steam.buildId;packages=$packages;containerCheck=$check;runtimeIni='Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini';gameplayValidated=$false}
$result | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $output 'build-result.json') -Encoding utf8
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $output -Path $cook.stage
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $output -Path $projectRoot
$result | ConvertTo-Json -Depth 8
