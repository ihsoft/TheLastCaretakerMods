[CmdletBinding()]
param([string]$OutputRoot = '', [switch]$Install,
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [switch]$AllowDirtySource,
    [switch]$IgnorePinnedGameVersion,
    [string]$ArtifactVersion = '',
    [string]$CacheRoot = 'P:\UnrealCache\TheLastCaretakerMods\UE5.8')
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Build Railgun'
    return
}
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($Install -and @(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' `
        -ErrorAction SilentlyContinue).Count -gt 0) {
    throw 'Voyage must be closed before a Railgun build that installs its candidate.'
}
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1')
$engine = Join-Path $EngineRoot 'Engine'
$ammoPickupPackage = '/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette'
$publishedVersionText = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'version.txt') -Raw).Trim()
if ($publishedVersionText -cnotmatch '^(0|[1-9][0-9]*)$') {
    throw 'Railgun version.txt must contain a non-negative integer.'
}
$releaseVersion = 'V' + ([int]$publishedVersionText + 1)
$defaultVersion = if ($ArtifactVersion) {
    if ($ArtifactVersion -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,79}$') {
        throw 'ArtifactVersion must be a safe 1-80 character identity.'
    }
    $ArtifactVersion
} else {
    'build-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
}
$output = New-VoyageModRun -ModRoot $PSScriptRoot -OutputRoot $OutputRoot -Kind $defaultVersion
$version = $releaseVersion
$project = & (Join-Path $PSScriptRoot 'New-RailgunProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$projectRoot = Split-Path -Parent $project
$bptWorkspace = Join-Path $repo ('Tmp/BlueprintText/Railgun-build-' + [Guid]::NewGuid().ToString('N'))
$releaseStaging = Join-Path $output 'candidate'
$settingsDefaults = Join-Path $PSScriptRoot 'Assets/Railgun.ini'
$preparedSettingsDirectory = Join-Path $output 'settings'
$null = New-Item -ItemType Directory -Path $preparedSettingsDirectory
$preparedSettingsIni = Join-Path $preparedSettingsDirectory 'Railgun.ini'
Copy-Item -LiteralPath $settingsDefaults -Destination $preparedSettingsIni
$sourcePaths = @(
    'mods/Railgun',
    'tools/VoyageModPipeline.psm1',
    'tools/UnrealCookSupport',
    'tools/Get-VoyageBuildFingerprint.ps1',
    'tools/Get-VoyageMappings.ps1',
    'tools/Install-VoyageRelease.ps1',
    'tools/VoyageModSettings',
    'tools/Invoke-VoyageBoundedTool.ps1',
    'tools/New-VoyageAssetRegistry.ps1',
    'tools/Convert-VoyageDataAsset.ps1',
    'tools/Read-VoyageDataAsset.ps1',
    'tools/New-VoyagePak.ps1',
    'tools/AssetPackaging',
    'tools/New-VoyageReleaseManifest.ps1',
    'tools/Restore-VoyageReleaseInstallation.ps1',
    'tools/Test-VoyageContainer.ps1',
    'tools/VoyageAssetRegistryWriter'
)
$dataAssetContractPath = Join-Path $PSScriptRoot 'Build/data-assets-contract.json'
$dataAssetContract = Get-Content -LiteralPath $dataAssetContractPath -Raw | ConvertFrom-Json
if ($dataAssetContract.schemaVersion -ne 5 -or @($dataAssetContract.assets).Count -ne 3 -or
    (Compare-Object @($dataAssetContract.assets.kind | Sort-Object) @('ammo','gun','skill'))) {
    throw 'Expected the three maintained Railgun primary DataAssets.'
}
$dataAssetsByKind = @{}
foreach ($entry in $dataAssetContract.assets) {
    $dataAssetsByKind[$entry.kind] = $entry
    if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $entry.assetFile) -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $entry.serializationFile) -PathType Leaf)) {
        throw "Maintained DataAsset or serialization contract missing: $($entry.kind)"
    }
}
$serializationContract = $dataAssetContract.serialization
function Get-RailgunSourceStatus {
    @(& git -C $repo status --porcelain -- $sourcePaths)
}
$sourceStatus = @(Get-RailgunSourceStatus)
$fingerprint = (& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath (Join-Path $output 'fingerprint.json')) | ConvertFrom-Json
$gameBuild = & (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') `
    -ModRoot $PSScriptRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) `
    -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ([string]$mapping.steamBuildId -cne [string]$fingerprint.steam.buildId -or
    $serializationContract.engineVersion -cne $mapping.engineVersion) {
    throw 'Railgun selected mapping does not match the installed game or parser profile.'
}
$engineVersion = Get-Content -LiteralPath (Join-Path $engine 'Build/Build.version') -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or $engineVersion.PatchVersion -ne 2) { throw 'Railgun requires reviewed editor 5.8.2.' }

$content = Join-Path $projectRoot 'Content'
if (-not (Test-Path -LiteralPath $content -PathType Container)) {
    throw 'Maintained Railgun Content is missing. Restore project assets before building.'
}
$ddc = [IO.Path]::GetFullPath($CacheRoot)
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $ddc, 'Process')
$null = New-Item -ItemType Directory -Path $ddc -Force
& (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare `
    -ProjectFile $project -WorkspaceRoot $bptWorkspace -EngineRoot $EngineRoot > (Join-Path $output 'prepare.log')
$primaryNativePaths = @{}
foreach ($entry in $dataAssetContract.assets) {
    $native = & (Join-Path $repo 'tools/Read-VoyageDataAsset.ps1') -ProjectFile $project `
        -Package $entry.package -ExpectedClass ('/Script/Voyage.' + $entry.class) `
        -EngineRoot $EngineRoot -OutputRoot (Join-Path $output ('native-' + $entry.kind)) | ConvertFrom-Json
    $primaryNativePaths[$entry.kind] = $native.readbackPath
}
# This directory owns its release resources; enumerate saved packages without loading Unreal.
$visualRoot = Join-Path $content 'Mods/Railgun/Visual'
$visualPackages = @(Get-ChildItem -LiteralPath $visualRoot -Filter '*.uasset' -File -Recurse | ForEach-Object {
    $relative = $_.FullName.Substring($content.Length + 1).Replace('\','/')
    '/Game/' + $relative.Substring(0, $relative.Length - $_.Extension.Length)
})
if ($visualPackages.Count -eq 0) { throw 'Maintained visual assets are absent.' }
$packages = $visualPackages + @('/Game/Mods/Railgun/Module/BP_Module_Railgun', $ammoPickupPackage)
$packages += @(
    '/Game/Mods/Railgun/Inputs/DA_RailgunCharacterContext',
    '/Game/Mods/Railgun/Inputs/DA_RailgunExitContext',
    '/Game/Mods/Railgun/Inputs/DA_RailgunADSServiceEquipment',
    '/Game/Mods/Railgun/Inputs/BP_RailgunADSServiceEquipment',
    '/Game/Mods/Railgun/Station/BP_RailgunOperator',
    '/Game/Mods/Railgun/Settings/BP_IniFileParser',
    '/Game/Mods/Railgun/Station/WBP_RailgunHUD',
    '/Game/Mods/Railgun/Station/T_RailgunOpticalMask',
    '/Game/Mods/Railgun/Station/T_RailgunStatusCharging',
    '/Game/Mods/Railgun/Station/T_RailgunStatusOffline',
    '/Game/Mods/Railgun/Station/T_RailgunStatusReady',
    '/Game/Mods/Railgun/Station/T_RailgunAmmoIndicator',
    '/Game/Mods/Railgun/Station/BP_RailgunTestShot',
    '/Game/Mods/Railgun/Station/BP_RailgunWaterWakeController',
    '/Game/Mods/Railgun/Station/BP_RailgunTransientVfx',
    '/Game/Mods/Railgun/Station/S_RailgunShotBlast',
    '/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod',
    '/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01',
    '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun',
    '/Game/Mods/Railgun/Research/T_RailgunSkill',
    '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon',
    '/Game/Mods/Railgun/Fabricator/T_RailgunIcon'
)
$packages = @($packages | Sort-Object -Unique)
# Preserve named properties for the partial native mirrors and keep shader code
# in the owned packages. The shared cooker owns manifest transport and staging.
$cook = Invoke-VoyageModCookAssets -ModRoot $PSScriptRoot -ProjectFile $project -EngineRoot $EngineRoot `
    -OutputRoot $output -Packages $packages -TaggedPackages $packages -AdditionalCookArguments @(
        '-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False',
        '-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6,D3D11TargetedShaderFormats=PCD3D_SM5',
        ('-LocalDataCachePath=' + $ddc), ('-ZenDataPath=' + (Join-Path $ddc 'Zen')))
$loose = $cook.stage
$cloneRelative = 'Voyage/Content/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod'
$skillRelative = 'Voyage/Content/Data/Assets/Skill/Railgun/DA_Skill_Railgun'

$cloneTarget = Join-Path $loose $cloneRelative
$conversionTool = Join-Path $repo 'tools/Convert-VoyageDataAsset.ps1'
$conversionOptions = @{MappingsPath=$mapping.mappingsPath; EngineVersion=$serializationContract.engineVersion}
$ammoConversion = & $conversionTool @conversionOptions `
    -Package $dataAssetsByKind.ammo.package -ExpectedClass ('/Script/Voyage.' + $dataAssetsByKind.ammo.class) `
    -SerializationContract (Join-Path $PSScriptRoot $dataAssetsByKind.ammo.serializationFile) `
    -CookedAsset ($cloneTarget + '.uasset') -NativeReadback $primaryNativePaths.ammo `
    -OutputRoot (Join-Path $output 'project-ammo')
$ammoWrite = $ammoConversion.write
$ammoReadback = $ammoConversion.readback
$ammoReadbackPath = $ammoConversion.readbackPath
$skillTarget = Join-Path $loose $skillRelative
$skillConversion = & $conversionTool @conversionOptions `
    -Package $dataAssetsByKind.skill.package -ExpectedClass ('/Script/Voyage.' + $dataAssetsByKind.skill.class) `
    -SerializationContract (Join-Path $PSScriptRoot $dataAssetsByKind.skill.serializationFile) `
    -CookedAsset ($skillTarget + '.uasset') -NativeReadback $primaryNativePaths.skill `
    -OutputRoot (Join-Path $output 'project-skill')
$skillWrite = $skillConversion.write
$skillReadback = $skillConversion.readback
$skillReadbackPath = $skillConversion.readbackPath
$gunRelative = 'Voyage/Content/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$gunTarget = Join-Path $loose $gunRelative
$gunConversion = & $conversionTool @conversionOptions `
    -Package $dataAssetsByKind.gun.package -ExpectedClass ('/Script/Voyage.' + $dataAssetsByKind.gun.class) `
    -SerializationContract (Join-Path $PSScriptRoot $dataAssetsByKind.gun.serializationFile) `
    -CookedAsset ($gunTarget + '.uasset') -NativeReadback $primaryNativePaths.gun `
    -OutputRoot (Join-Path $output 'project-gun')
$gunWrite = $gunConversion.write
$gunReadback = $gunConversion.readback
$gunReadbackPath = $gunConversion.readbackPath
$stem = 'Railgun'
# Write only the three Railgun primary records from their authored JSON readbacks.
$pluginRegistry = Join-Path $output 'RailgunCatalogue-AssetRegistry.bin'
$registryResult = & (Join-Path $repo 'tools/New-VoyageAssetRegistry.ps1') `
    -AssetJsonPath @($ammoReadbackPath,$gunReadbackPath,$skillReadbackPath) `
    -OutputPath $pluginRegistry `
    -EngineRoot $engine `
    -EvidenceRoot (Join-Path $output 'asset-registry-writer')
if ($registryResult.status -cne 'passed' -or
    $registryResult.assetCount -ne 3 -or $registryResult.packageCount -ne 3 -or
    -not [bool]$registryResult.reopenVerified -or
    -not (Test-Path -LiteralPath $pluginRegistry -PathType Leaf) -or
    (Get-Item -LiteralPath $pluginRegistry).Length -le 0) {
    throw 'Owned-entry plugin registry is missing or empty.'
}
$registryManifest = Join-Path $output 'registry-files.json'
[ordered]@{schemaVersion=1;files=@([ordered]@{
    source=$pluginRegistry;mountPath='../../../Voyage/Mods/RailgunCatalogue/AssetRegistry.bin'
})} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $registryManifest -Encoding UTF8
$registryArchive = & (Join-Path $repo 'tools/New-VoyagePak.ps1') -Manifest $registryManifest `
    -OutputRoot (Join-Path $output 'registry-pak') -EngineRoot $EngineRoot
$registryPak = $registryArchive.pakPath
$payload = Join-Path $releaseStaging 'payload'
$pack = New-VoyageModContainer -ModRoot $PSScriptRoot -Stage $loose -OutputRoot $payload `
    -Packages $packages -ParserVersion UE5_8 -GameRoot $GameRoot -ContainerName $stem -CompanionPak $registryPak
$container = $pack.container
$verify = $pack.validation
$containerReport = Get-Content -LiteralPath $verify.reportPath -Raw | ConvertFrom-Json
Copy-Item -LiteralPath $preparedSettingsIni -Destination $payload
$descriptorSource = Join-Path $PSScriptRoot 'Build/RailgunCatalogue.uplugin'
$descriptorDirectory = Join-Path $payload 'Mods/RailgunCatalogue'
$null = New-Item -ItemType Directory -Path $descriptorDirectory
$descriptor = Join-Path $descriptorDirectory 'RailgunCatalogue.uplugin'
Copy-Item -LiteralPath $descriptorSource -Destination $descriptor
$archivePath = Join-Path $releaseStaging ('Railgun-' + $version + '.zip')
& (Join-Path $repo 'tools/New-VoyageVortexArchive.ps1') `
    -Container (Join-Path $payload 'Railgun.utoc') -Archive $archivePath `
    -AdditionalFile (Join-Path $payload 'Railgun.ini') `
    -ContentPluginDescriptor $descriptor | Out-Null
$sourceAfter = @(Get-RailgunSourceStatus)
if (($sourceAfter -join "`n") -cne ($sourceStatus -join "`n")) { throw 'Source status changed during preparation.' }
$validationSummary = [ordered]@{
    schemaVersion = 1
    status = 'passed'
    runtime = 'pending'
    container = [ordered]@{
        packageCount = [int]$containerReport.packageCount
        packageSetMatches = [bool]$containerReport.packageSetMatches
        chunkCount = [int]$containerReport.chunkCount
        chunkTypes = @($containerReport.chunkTypes)
        files = @($containerReport.files | ForEach-Object {
            [ordered]@{
                name = [string]$_.name
                size = [long]$_.size
            }
        })
    }
    dataAssetConversions = [ordered]@{
        ammo = $ammoConversion.status
        gun = $gunConversion.status
        skill = $skillConversion.status
    }
    primaryAssetRegistry = [ordered]@{
        assetCount = [int]$registryResult.assetCount
        packageCount = [int]$registryResult.packageCount
        reopenVerified = [bool]$registryResult.reopenVerified
        primaryAssetIds = @($registryResult.primaryAssetIds)
    }
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$validationSummaryPath = Join-Path $releaseStaging 'validation-summary.json'
[IO.File]::WriteAllText(
    $validationSummaryPath,
    (($validationSummary | ConvertTo-Json -Depth 8) + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))
$provenance = [ordered]@{
    schemaVersion=1;mod='Railgun';version=$version;createdAtUtc=[DateTime]::UtcNow.ToString('o');
    dirtySource=($sourceStatus.Count -gt 0);sourceStatus=$sourceStatus;
    gameEngineVersion='5.8';gameEngineVersionBasis='Reviewed mapping/parser target; game patch version not independently established';editorEngineVersion='5.8.2';retocCompatibilityVersion='UE5_8';
    gameFingerprint=@{steamBuildId=[string]$fingerprint.steam.buildId};ignoredPinnedGameVersion=[bool]$gameBuild.ignoredPinnedGameVersion;
    validation='build and static verification; gameplay validation is a separate gate';runtimeArchitecture='Single Railgun container with independent gun and ammo items, one research skill, plugin-local three-record primary-asset registry, project-owned module actor with lifecycle-owned station initialization, operator, inputs, HUD and shot audio';
    mappingPath=$mapping.mappingsPath;validationSummary='validation-summary.json';
    packagingStatus=$pack.status;containerVerificationStatus=$verify.status;
    ammoSourceAsset='mods/Railgun/Project/Content/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod.uasset';
    ammoSerializationContract='mods/Railgun/Build/ammo-serialization.json';
    ammoWriterStatus=$ammoWrite.status;ammoReadbackStatus=$ammoReadback.status;
    dataAssetContract='mods/Railgun/Build/data-assets-contract.json';
    dataAssetSerialization=$serializationContract;
    skillSourceAsset='mods/Railgun/Project/Content/Data/Assets/Skill/Railgun/DA_Skill_Railgun.uasset';
    skillWriterStatus=$skillWrite.status;skillReadbackStatus=$skillReadback.status;
    gunSourceAsset='mods/Railgun/Project/Content/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01.uasset';
    gunWriterStatus=$gunWrite.status;gunReadbackStatus=$gunReadback.status;
    scriptObjectsInput=@($pack.scriptObjects.inputs | ForEach-Object { [IO.Path]::GetFileName($_) });scriptObjectsStatus=$pack.scriptObjects.status;
    primaryAssetRegistryAssetCount=[int]$registryResult.assetCount;
    primaryAssetRegistryPackageCount=[int]$registryResult.packageCount;
    primaryAssetRegistryReopenVerified=[bool]$registryResult.reopenVerified;
    contentPluginDescriptor='mods/Railgun/Build/RailgunCatalogue.uplugin';
}
[IO.File]::WriteAllText(
    (Join-Path $releaseStaging 'build-provenance.json'),
    (($provenance | ConvertTo-Json -Depth 8) + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))
$releaseSourcePaths = @($sourcePaths | ForEach-Object {
    [IO.Path]::GetFullPath((Join-Path $repo $_))
})
$release = (& (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') `
    -ReleaseRoot $releaseStaging -Mod Railgun -Version $version -Container $container `
    -Archive $archivePath -SourcePath $releaseSourcePaths `
    -ContentPluginDescriptor $descriptor -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource -AsJson) |
    ConvertFrom-Json
$manifestPath = Join-Path $releaseStaging 'release-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Manifest producer did not publish the candidate.' }
$installation = $null
if ($Install) {
    $installed = & (Join-Path $PSScriptRoot 'Install-Railgun.ps1') -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource
    $installation = $installed
}
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $output -Path $loose
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $output -Path $projectRoot
[pscustomobject]@{
    status = $(if ($Install) { 'installed' } else { 'built' })
    releaseManifestPath = $manifestPath
    archivePath = $archivePath
    validationSummaryPath = $validationSummaryPath
    verificationReport = $validationSummaryPath
    candidateRoot = $(if ($Install) { $null } else { $output })
    installation = $installation
} | ConvertTo-Json -Depth 8 -Compress
