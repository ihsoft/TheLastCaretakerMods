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
Set-StrictMode -Version Latest
$modRoot = $PSScriptRoot
$repo = (Resolve-Path (Join-Path $modRoot '../..')).Path
$owner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/ContainerLidLabel'))
if (-not $OutputRoot) { $OutputRoot = Join-Path $owner ('build-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not $output.StartsWith($owner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'OutputRoot must stay under Tmp/ContainerLidLabel' }
if (Test-Path -LiteralPath $output) { throw 'Use a new output directory' }
[IO.Directory]::CreateDirectory($output) | Out-Null
$project = & (Join-Path $modRoot 'New-ContainerLidLabelProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$projectRoot = Split-Path -Parent $project
$bptWorkspace = Join-Path $repo ('Tmp/BlueprintText/ContainerLidLabel-build-' + [guid]::NewGuid().ToString('N'))
& (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $bptWorkspace
$fingerprintPath = Join-Path $output 'game-fingerprint.json'
& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content $fingerprintPath -Raw | ConvertFrom-Json
& (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) -IgnorePinnedGameVersion:$IgnorePinnedGameVersion | Out-Null
$versionText = (Get-Content (Join-Path $modRoot 'version.txt') -Raw).Trim()
if ($versionText -cnotmatch '^(0|[1-9][0-9]*)$') { throw 'Invalid published version' }
$version = 'V' + ([int]$versionText + 1)
$names = @('ModActor', 'BP_ContainerLidDisplay', 'BFL_ContainerTypes', 'Settings/BP_IniFileParser')
$packages = @($names | ForEach-Object { '/Game/Mods/ContainerLidLabel/' + $_ })
$cooked = Join-Path $projectRoot 'Saved/Cooked/Windows/Voyage/Content/Mods/ContainerLidLabel'
foreach ($name in $names) {
    if (-not (Test-Path (Join-Path $projectRoot "Content/Mods/ContainerLidLabel/$name.uasset"))) { throw "Apply the maintained BPT first: $name" }
}
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$cookLog = Join-Path $output 'cook.log'
& (Join-Path $repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Cook ContainerLidLabel' -FilePath $editor -ArgumentList @($project,'-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore',('-Package=' + ($packages -join '+')),'-CookSinglePackageNoRefs','-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',('-abslog=' + $cookLog)) > (Join-Path $output 'cook-console.log')
if (Select-String -LiteralPath $cookLog -Pattern 'LogBlueprint: Error:|LogLinker: Error:|LogCook: Error:' -Quiet) { throw "Cook errors: $cookLog" }
$stage = Join-Path $output 'stage'
$stageAssets = Join-Path $stage 'Voyage/Content/Mods/ContainerLidLabel'
[IO.Directory]::CreateDirectory($stageAssets) | Out-Null
foreach ($name in $names) {
    foreach ($ext in @('.uasset','.uexp')) {
        $destination = Join-Path $stageAssets ($name + $ext)
        [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
        Copy-Item -LiteralPath (Join-Path $cooked ($name + $ext)) -Destination $destination
    }
}
$package = Join-Path $output 'package'
[IO.Directory]::CreateDirectory($package) | Out-Null
$ini = Join-Path $package 'ContainerLidLabel.ini'
Import-Module (Join-Path $repo 'tools/VoyageModSettings/VoyageModSettings.psm1') -Force
$validatedSettings = @(Read-VoyageModSettingsSchema -SchemaPath (Join-Path $modRoot 'Settings/ContainerLidLabel.settings.json') -DefaultIniPath (Join-Path $modRoot 'Assets/ContainerLidLabel.ini'))
Copy-Item -LiteralPath (Join-Path $modRoot 'Assets/ContainerLidLabel.ini') -Destination $ini
$utoc = Join-Path $package 'ContainerLidLabel.utoc'
& (Join-Path $repo '.tools/bin/retoc.exe') to-zen --version UE5_8 $stage $utoc > (Join-Path $output 'retoc.log')
if ($LASTEXITCODE -ne 0) { throw 'retoc conversion failed' }
$expected = Join-Path $output 'expected-packages.txt'
@($names | ForEach-Object { '../../../Voyage/Content/Mods/ContainerLidLabel/' + $_ + '.uasset' }) | Set-Content $expected -Encoding ascii
$check = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') -Container $utoc -ExpectedPackageList $expected -OutputRoot (Join-Path $output 'container-check')
$zip = Join-Path $output ('ContainerLidLabel-' + $version + '.zip')
& (Join-Path $repo 'tools/New-VoyageVortexArchive.ps1') -Container $utoc -Archive $zip -LogicMods -AdditionalFile $ini -AdditionalDestination 'Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini' | Out-Null
$manifest = & (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod ContainerLidLabel -Version $version -Container $utoc -Archive $zip -SourcePath $modRoot -GameRoot $GameRoot -LogicMods -AllowDirtySource:$AllowDirtySource
$result = [ordered]@{status='built';archive=$zip;releaseManifest=$manifest.manifestPath;entryClass='/Game/Mods/ContainerLidLabel/ModActor.ModActor_C';steamBuildId=[string]$fingerprint.steam.buildId;packages=$packages;containerCheck=$check;runtimeIni='Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini';gameplayValidated=$false}
$result | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $output 'build-result.json') -Encoding utf8
$stageAbsolute = (Resolve-Path -LiteralPath $stage).Path
if (-not $stageAbsolute.StartsWith($output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe scratch path' }
Remove-Item -LiteralPath $stageAbsolute -Recurse -Force
$projectAbsolute = (Resolve-Path -LiteralPath $projectRoot).Path
if ($projectAbsolute -ne (Join-Path $output 'Project')) { throw 'Unsafe build scratch path' }
Remove-Item -LiteralPath $projectAbsolute -Recurse -Force
$result | ConvertTo-Json -Depth 8
