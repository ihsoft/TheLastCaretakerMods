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
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Build PearFix candidate'
    return
}
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1')
$output = New-VoyageModRun -ModRoot $PSScriptRoot -OutputRoot $OutputRoot
$project = & (Join-Path $PSScriptRoot 'New-PearFixProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$workspace = Join-Path $repo ('Tmp/BlueprintText/PearFix-build-' + [guid]::NewGuid().ToString('N'))
& (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -WorkspaceRoot $workspace -EngineRoot $EngineRoot > (Join-Path $output 'prepare.log')
$fingerprint = & (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot | ConvertFrom-Json
& (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $PSScriptRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) -IgnorePinnedGameVersion:$IgnorePinnedGameVersion | Out-Null
$versionText = (Get-Content (Join-Path $PSScriptRoot 'version.txt') -Raw).Trim()
if ($versionText -cnotmatch '^(0|[1-9][0-9]*)$') { throw 'Invalid published version.' }
$version = 'V' + ([int]$versionText + 1)
# Only the additive mod actor ships. Stock-path compilation stubs are excluded.
$packages = @('/Game/Mods/PearFix/ModActor')
$cook = Invoke-VoyageModCook -ModRoot $PSScriptRoot -ProjectFile $project -EngineRoot $EngineRoot -OutputRoot $output -Packages $packages -ParserVersion UE5_8 -GameRoot $GameRoot
$zip = Join-Path $output ('PearFix-' + $version + '.zip')
& (Join-Path $repo 'tools/New-VoyageVortexArchive.ps1') -Container $cook.container -Archive $zip -LogicMods | Out-Null
$manifest = & (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod PearFix -Version $version -Container $cook.container -Archive $zip -SourcePath $PSScriptRoot -GameRoot $GameRoot -LogicMods -AllowDirtySource:$AllowDirtySource
$result = [ordered]@{status='built';archivePath=$zip;releaseManifestPath=$manifest.manifestPath;steamBuildId=[string]$fingerprint.steam.buildId;packages=$packages;containerCheck=$cook.validation;gameplayValidated=$false;purpose='automatic pear registration and stale removal-marker repair on persistence events, one startup snapshot, and a brief successful-fruit count notice'}
$result | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $output 'build-result.json') -Encoding UTF8
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $output -Path $cook.stage
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $output -Path (Split-Path $project -Parent)
$result | ConvertTo-Json -Depth 8
