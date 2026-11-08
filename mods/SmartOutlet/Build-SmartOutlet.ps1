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
$tmpOwner = Join-Path $repoRoot 'Tmp/SmartOutlet'
if (-not $OutputRoot) { $OutputRoot = Join-Path $tmpOwner ('build-' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not $output.StartsWith($tmpOwner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw "OutputRoot must be below $tmpOwner" }
if (Test-Path -LiteralPath $output) { throw "Use a fresh output directory: $output" }
[IO.Directory]::CreateDirectory($output) | Out-Null
$versionText = (Get-Content -LiteralPath (Join-Path $modRoot 'version.txt') -Raw).Trim()
if ($versionText -cnotmatch '^(0|[1-9][0-9]*)$') { throw 'Invalid published version.txt' }
$candidateVersion = 'V' + ([int]$versionText + 1)
$project = Join-Path $modRoot 'Voyage.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$fingerprintPath = Join-Path $output 'game-fingerprint.json'
& (Join-Path $repoRoot 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$buildGate = & (Join-Path $repoRoot 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
$mappings = & (Join-Path $repoRoot 'tools/Get-VoyageMappings.ps1') -GameRoot $GameRoot
Import-Module (Join-Path $repoRoot 'tools/VoyageModSettings/VoyageModSettings.psm1') -Force
$settingsTemplate = Join-Path $modRoot 'Assets/SmartOutlet.ini'
$settings = @(Read-VoyageModSettingsSchema -SchemaPath (Join-Path $modRoot 'Settings/SmartOutlet.settings.json') -DefaultIniPath $settingsTemplate)
$settingsByKey = @{}
foreach ($setting in $settings) { $settingsByKey[$setting.Key] = $setting }
$ruleCount = [double]$settingsByKey['RuleCount'].IniValue
if ($ruleCount -ne [math]::Truncate($ruleCount)) { throw 'RuleCount must be an integer' }
for ($ruleIndex = 1; $ruleIndex -le $ruleCount; $ruleIndex++) {
    $enable = [double]$settingsByKey["Rule${ruleIndex}EnablePortAtPercent"].IniValue
    $disable = [double]$settingsByKey["Rule${ruleIndex}DisablePortAtPercent"].IniValue
    $mode = [string]$settingsByKey["Rule${ruleIndex}Mode"].IniValue
    $slot = [double]$settingsByKey["Rule${ruleIndex}GlowMaterialSlot"].IniValue
    $invalidThresholds = if ($mode -ceq 'input') { $enable -ge $disable } else { $enable -le $disable }
    if ($invalidThresholds -or $slot -ne [math]::Truncate($slot) -or $mode -cnotin @('input','output')) { throw "Invalid thresholds, mode or slot in rule $ruleIndex" }
}
& (Join-Path $repoRoot 'tools/Prepare-UnrealEditorSupport.ps1') -ProjectFile $project -EngineRoot $EngineRoot | Out-Null
$ownedNames = @('BP_OutletController', 'ModActor', 'M_PortGlow', 'Settings/BP_IniFileParser', 'Settings/BP_OutletRule')
$packages = @($ownedNames | ForEach-Object { '/Game/Mods/SmartOutlet/' + $_ })
$cookedRoot = [IO.Path]::GetFullPath((Join-Path $modRoot 'Saved/Cooked/Windows/Voyage/Content/Mods/SmartOutlet'))
foreach ($name in $ownedNames) {
    $source = Join-Path $modRoot ('Content/Mods/SmartOutlet/' + $name + '.uasset')
    if (-not (Test-Path -LiteralPath $source)) { throw "Maintained asset missing: $source; apply BPT before building." }
    foreach ($ext in @('.uasset','.uexp')) {
        $stale = Join-Path $cookedRoot ($name + $ext)
        if (Test-Path -LiteralPath $stale) { Remove-Item -LiteralPath $stale -Force }
    }
}
$cookLog = Join-Path $output 'cook.log'
$cookArgs = @($project,'-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore',('-Package=' + ($packages -join '+')),
    '-CookSinglePackageNoRefs','-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',
    ('-LocalDataCachePath=' + (Join-Path $modRoot 'DerivedDataCache')),('-abslog=' + $cookLog))
& (Join-Path $repoRoot 'tools/Invoke-UnrealQueued.ps1') -Operation 'Cook SmartOutlet' -FilePath $editor -ArgumentList $cookArgs > (Join-Path $output 'cook-console.log')
if (Select-String -LiteralPath $cookLog -Pattern 'LogBlueprint: Error:|LogLinker: Error:|LogCook: Error:' -Quiet) { throw "Cook reported an error: $cookLog" }
$stage = Join-Path $output 'stage'
$stageMod = Join-Path $stage 'Voyage/Content/Mods/SmartOutlet'
[IO.Directory]::CreateDirectory($stageMod) | Out-Null
foreach ($name in $ownedNames) {
    foreach ($ext in @('.uasset','.uexp')) {
        $source = Join-Path $cookedRoot ($name + $ext)
        if (-not (Test-Path -LiteralPath $source)) { throw "Cooked asset missing: $source" }
        $staged = Join-Path $stageMod ($name + $ext)
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($staged)) | Out-Null
        Copy-Item -LiteralPath $source -Destination $staged
    }
}
$package = Join-Path $output 'package'
[IO.Directory]::CreateDirectory($package) | Out-Null
$utoc = Join-Path $package 'SmartOutlet.utoc'
$retoc = Join-Path $repoRoot '.tools/bin/retoc.exe'
& $retoc to-zen --version UE5_8 $stage $utoc > (Join-Path $output 'retoc.log')
if ($LASTEXITCODE -ne 0) { throw 'retoc conversion failed; see retoc.log' }
$expected = Join-Path $output 'expected-packages.txt'
@($ownedNames | ForEach-Object { '../../../Voyage/Content/Mods/SmartOutlet/' + $_ + '.uasset' }) | Set-Content -LiteralPath $expected -Encoding ASCII
$validation = & (Join-Path $repoRoot 'tools/Test-VoyageContainer.ps1') -Container $utoc -ExpectedPackageList $expected
$zip = Join-Path $output ('SmartOutlet-' + $candidateVersion + '.zip')
$packagedIni = Join-Path $output 'SmartOutlet.ini'
Copy-Item -LiteralPath $settingsTemplate -Destination $packagedIni
& (Join-Path $repoRoot 'tools/New-VoyageVortexArchive.ps1') -Container $utoc -Archive $zip -LogicMods -AdditionalFile $packagedIni -AdditionalDestination 'Voyage/Content/Paks/LogicMods/SmartOutlet.ini' | Out-Null
$manifest = & (Join-Path $repoRoot 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod SmartOutlet -Version $candidateVersion -Container $utoc -Archive $zip -SourcePath $modRoot -GameRoot $GameRoot -LogicMods -AllowDirtySource:$AllowDirtySource
$result = [ordered]@{status='built';archivePath=$zip;releaseManifestPath=$manifest.manifestPath;entryClass='/Game/Mods/SmartOutlet/ModActor.ModActor_C';steamBuildId=[string]$fingerprint.steam.buildId;ownedPackages=$packages;gameplayValidated=$false;saveReloadValidated=$false;containerValidation=$validation}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-result.json') -Encoding UTF8
$stagePath = (Resolve-Path -LiteralPath $stage).Path
if (-not $stagePath.StartsWith($output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected scratch directory' }
Remove-Item -LiteralPath $stagePath -Recurse -Force
$result | ConvertTo-Json -Depth 8
