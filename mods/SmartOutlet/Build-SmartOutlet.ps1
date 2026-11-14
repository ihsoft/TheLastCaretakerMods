# Cook the maintained project assets, then package the exact additive DML set.
# Does not author/rebuild Blueprint graphs, install, or publish.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion,
    [switch]$AllowDirtySource
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '../..')).Path
Import-Module (Join-Path $repoRoot 'tools/VoyageModPipeline.psm1')
$output = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot
$versionText = (Get-Content -LiteralPath (Join-Path $modRoot 'version.txt') -Raw).Trim()
if ($versionText -cnotmatch '^(0|[1-9][0-9]*)$') { throw 'Invalid published version.txt' }
$candidateVersion = 'V' + ([int]$versionText + 1)
$projectRoot = Join-Path $output 'Project'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$fingerprintPath = Join-Path $output 'game-fingerprint.json'
& (Join-Path $repoRoot 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$buildGate = & (Join-Path $repoRoot 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
$mappings = & (Join-Path $repoRoot 'tools/Get-VoyageMappings.ps1') -GameRoot $GameRoot
$settingsTemplate = Join-Path $modRoot 'Assets/SmartOutlet.ini'
& (Join-Path $modRoot 'Settings/Test-SettingsTemplate.ps1') -Path $settingsTemplate | Out-Null
$project = & (Join-Path $modRoot 'New-SmartOutletProjectCopy.ps1') -Destination $projectRoot
$bptWorkspace = Join-Path $repoRoot ('Tmp/BlueprintText/SmartOutlet-build-' + [guid]::NewGuid().ToString('N'))
& (Join-Path $repoRoot 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $bptWorkspace > (Join-Path $output 'prepare.log')
$ownedNames = @('BP_OutletController', 'ModActor', 'WBP_PortModeAction', 'M_PortGlow', 'Settings/BP_SectionedIniFileParser', 'Settings/BP_OutletRule', 'Settings/BP_StorageProfile')
$packages = @($ownedNames | ForEach-Object { '/Game/Mods/SmartOutlet/' + $_ })
$cook = Invoke-VoyageModCook -ModRoot $modRoot -ProjectFile $project -EngineRoot $EngineRoot -OutputRoot $output -Packages $packages -ParserVersion UE5_8 `
    -AdditionalCookArguments @('-LocalDataCachePath=' + (Join-Path $projectRoot 'DerivedDataCache'))
$utoc = $cook.container
$validation = $cook.validation
$zip = Join-Path $output ('SmartOutlet-' + $candidateVersion + '.zip')
$packagedIni = Join-Path $output 'SmartOutlet.ini'
Copy-Item -LiteralPath $settingsTemplate -Destination $packagedIni
& (Join-Path $repoRoot 'tools/New-VoyageVortexArchive.ps1') -Container $utoc -Archive $zip -LogicMods -AdditionalFile $packagedIni -AdditionalDestination 'Voyage/Content/Paks/LogicMods/SmartOutlet.ini' | Out-Null
$manifest = & (Join-Path $repoRoot 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod SmartOutlet -Version $candidateVersion -Container $utoc -Archive $zip -SourcePath @($modRoot,(Join-Path $repoRoot 'tools/VoyageModPipeline.psm1'),(Join-Path $repoRoot 'tools/UnrealCookSupport')) -GameRoot $GameRoot -LogicMods -AllowDirtySource:$AllowDirtySource
$summaryPath = Join-Path $output 'validation-summary.json'
[ordered]@{status='passed';container=$validation;runtime='pending'} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
$result = [ordered]@{status='built';archivePath=$zip;releaseManifestPath=$manifest.manifestPath;validationSummaryPath=$summaryPath;entryClass='/Game/Mods/SmartOutlet/ModActor.ModActor_C';steamBuildId=[string]$fingerprint.steam.buildId;ownedPackages=$packages;gameplayValidated=$false;saveReloadValidated=$false;containerValidation=$validation}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-result.json') -Encoding UTF8
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $output -Path $cook.stage
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $output -Path $projectRoot
$result | ConvertTo-Json -Depth 8
