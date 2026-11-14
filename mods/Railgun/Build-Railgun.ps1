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
$editor = Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
$itemDiscoveryRoot = '/Game/Data/Assets'
$skillDiscoveryRoot = '/Game/Data/Assets/Skill'
$ammoPickupPackage = '/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette'
$ammoPickupClass = $ammoPickupPackage + '.BP_RailgunAmmoCassette_C'
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
$modInspectionRoot = Join-Path $output 'asset-inspections'
$settingsSchema = Join-Path $PSScriptRoot 'Settings/Railgun.settings.json'
$settingsDefaults = Join-Path $PSScriptRoot 'Assets/Railgun.ini'
Import-Module (Join-Path $repo 'tools/VoyageModSettings/VoyageModSettings.psm1') -Force
$null = Read-VoyageModSettingsSchema -SchemaPath $settingsSchema -DefaultIniPath $settingsDefaults
$preparedSettingsDirectory = Join-Path $output 'settings'
$null = New-Item -ItemType Directory -Path $preparedSettingsDirectory
$preparedSettingsIni = Join-Path $preparedSettingsDirectory 'Railgun.ini'
Copy-Item -LiteralPath $settingsDefaults -Destination $preparedSettingsIni
$sourcePaths = @(
    'mods/Railgun',
    'tools/Get-VoyageAssetJson.ps1',
    'tools/VoyageModPipeline.psm1',
    'tools/UnrealCookSupport',
    'tools/Get-VoyageBuildFingerprint.ps1',
    'tools/Get-VoyageMappings.ps1',
    'tools/Install-VoyageRelease.ps1',
    'tools/Merge-VoyageModIni.ps1',
    'tools/VoyageModSettings',
    'tools/Invoke-VoyageBoundedTool.ps1',
    'tools/New-VoyageAssetRegistry.ps1',
    'tools/New-VoyageReleaseManifest.ps1',
    'tools/Restore-VoyageReleaseInstallation.ps1',
    'tools/Test-VoyageContainer.ps1',
    'tools/VoyageAssetRegistryWriter'
)
$modelDirectory = Join-Path $PSScriptRoot 'Assets/Model'
$modelPath = Join-Path $modelDirectory 'model-source.json'
$model = Get-Content -LiteralPath $modelPath -Raw | ConvertFrom-Json
$modelFields = @($model.PSObject.Properties.Name | Sort-Object)
$expectedModelFields = @('entryInteraction','fabricatorCollision','inventoryInteraction','nodes','schemaVersion')
if ($model.schemaVersion -ne 1 -or (Compare-Object $modelFields $expectedModelFields)) { throw 'Unsupported Railgun model registry.' }
$glbPath = [IO.Path]::GetFullPath((Join-Path $modelDirectory 'Railgun.glb'))
if (-not (Test-Path -LiteralPath $glbPath -PathType Leaf)) { throw "GLB not found: $glbPath" }
$dataAssetContractPath = Join-Path $PSScriptRoot 'Build/data-assets-contract.json'
$dataAssetContract = Get-Content -LiteralPath $dataAssetContractPath -Raw | ConvertFrom-Json
if ($dataAssetContract.schemaVersion -ne 5 -or @($dataAssetContract.assets).Count -ne 3 -or
    (Compare-Object @($dataAssetContract.assets.kind | Sort-Object) @('ammo','gun','skill'))) {
    throw 'Expected the three maintained Railgun primary DataAssets.'
}
foreach ($entry in $dataAssetContract.assets) {
    if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $entry.assetFile) -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $entry.serializationFile) -PathType Leaf)) {
        throw "Maintained DataAsset or serialization contract missing: $($entry.kind)"
    }
}
$serializationContract = $dataAssetContract.serialization
# Retain the external authoring input in the release source inventory.
$sourcePaths += @($glbPath.Substring($repo.Length + 1).Replace('\','/'))
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

function Invoke-NativeStage([string]$Name, [string]$Executable, [string[]]$NativeArguments) {
    if ($Executable -eq $editor) { $NativeArguments += ('-ZenDataPath=' + (Join-Path $ddc 'Zen')) }
    $log = Join-Path $output ($Name + '.log')
    & $Executable @NativeArguments *> $log
    if ($LASTEXITCODE -ne 0) { throw "$Name failed ($LASTEXITCODE); log: $log" }
    Write-Host "$Name passed; log: $log"
}
$content = Join-Path $projectRoot 'Content'
if (-not (Test-Path -LiteralPath $content -PathType Container)) {
    throw 'Maintained Railgun Content is missing. Restore project assets before building.'
}
$ddc = [IO.Path]::GetFullPath($CacheRoot)
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $ddc, 'Process')
$null = New-Item -ItemType Directory -Path $ddc -Force
& (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare `
    -ProjectFile $project -WorkspaceRoot $bptWorkspace -EngineRoot (Split-Path -Parent $engine) > (Join-Path $output 'prepare.log')
$settingsValidationPath = Join-Path $output 'settings-validation.json'
Invoke-NativeStage 'test-settings' $editor @($project,'-run=TestRailgunSettings',('-Schema=' + $settingsSchema),('-Output=' + $settingsValidationPath),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'test-settings-unreal.log')))
$ammoNativePath = Join-Path $output 'ammo-project.json'
Invoke-NativeStage 'read-ammo-project' $editor @($project,'-run=ReadRailgunDataAssets',('-Output=' + $ammoNativePath),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'read-ammo-project-unreal.log')))
$ammoNative = Get-Content -LiteralPath $ammoNativePath -Raw | ConvertFrom-Json
$ammoWeightKg = [double]$ammoNative.fields.Weight.value
if ($ammoWeightKg -le 0.0) { throw 'Maintained ammo mass must be positive.' }
$ammoDrops = @($ammoNative.fields.DropVariations.array)
if ($ammoDrops.Count -ne 1) { throw 'Maintained ammo must have one supported drop variation.' }
$ammoRenderIdentity = $ammoDrops[0].fields.RenderAsset.value.AssetPath
$ammoPickupCarrierMesh = [string]$ammoRenderIdentity.PackageName + '.' + [string]$ammoRenderIdentity.AssetName
$primaryNativePaths = @{ammo=$ammoNativePath}
foreach ($kind in @('gun','skill')) {
    $nativePath = Join-Path $output ($kind + '-project.json')
    Invoke-NativeStage ('read-' + $kind + '-project') $editor @($project,'-run=ReadRailgunDataAssets',('-Asset=' + $kind),'-Graph',('-Output=' + $nativePath),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output ('read-' + $kind + '-project-unreal.log'))))
    $primaryNativePaths[$kind] = $nativePath
}
$inventoryPath = Join-Path $output 'model-inventory.json'
Invoke-NativeStage 'inspect-project' $editor @($project,'-run=InspectRailgunProject',('-ModelRoles=' + $modelPath),('-Output=' + $inventoryPath),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'inspect-project-unreal.log')))
$inventory = Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
$packages = @($inventory.packages)
if ($packages.Count -lt 2 -or @($packages | Where-Object {
    -not $_.StartsWith('/Game/Mods/Railgun/Visual/') -and
    -not $_.StartsWith('/Game/Mods/Railgun/Fabricator/AmmoCassette/') -and
    $_ -cne '/Game/Mods/Railgun/Module/BP_Module_Railgun'
}).Count) { throw 'GLB cook inventory escaped owned packages.' }
$ammoPickupInventory = $inventory.ammoPickup
$ammoPickupCollisionSize = @($ammoPickupInventory.collisionSizeCm)
$ammoPickupCollisionCenter = @($ammoPickupInventory.collisionCenterCm)
if ($null -eq $ammoPickupInventory -or
    $ammoPickupInventory.root -cne [string]$model.nodes.ammoPickupRoot -or
    $ammoPickupInventory.carrier -cne [string]$model.nodes.ammoPickupCarrier -or
    $ammoPickupInventory.package -cne $ammoPickupPackage -or
    $ammoPickupInventory.classObjectPath -cne $ammoPickupClass -or
    $ammoPickupInventory.carrierMesh -cne $ammoPickupCarrierMesh -or
    @($ammoPickupInventory.parts).Count -lt 1 -or
    $ammoPickupCollisionSize.Count -ne 3 -or
    $ammoPickupCollisionCenter.Count -ne 3) {
    throw 'Shared composite Railgun ammo pickup lost its functional role contract.'
}
foreach ($extent in $ammoPickupCollisionSize) {
    $value = [double]$extent
    if ($value -le 0.0 -or [double]::IsNaN($value) -or
        [double]::IsInfinity($value)) {
        throw 'Shared ammo pickup collision size is not finite and nondegenerate.'
    }
}
foreach ($center in $ammoPickupCollisionCenter) {
    $value = [double]$center
    if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) {
        throw 'Shared ammo pickup collision center is not finite.'
    }
}
if ($packages -cnotcontains $ammoPickupPackage -or
    $packages -cnotcontains $ammoPickupCarrierMesh.Substring(0,
        $ammoPickupCarrierMesh.LastIndexOf('.'))) {
    throw 'Shared ammo pickup package or carrier mesh is absent from cook inventory.'
}
$pickupAssetPaths = @($ammoPickupInventory.carrierMesh) +
    @($ammoPickupInventory.carrierMaterials) +
    @($ammoPickupInventory.parts | ForEach-Object {
        @($_.mesh) + @($_.materials)
    })
foreach ($pickupAssetPath in $pickupAssetPaths) {
    if ([string]::IsNullOrWhiteSpace([string]$pickupAssetPath)) { continue }
    $separator = ([string]$pickupAssetPath).LastIndexOf('.')
    if ($separator -le 0 -or $packages -cnotcontains
        ([string]$pickupAssetPath).Substring(0, $separator)) {
        throw "Shared ammo pickup dependency is absent from cook inventory: $pickupAssetPath"
    }
}
$unrealPak = Join-Path $engine 'Binaries/Win64/UnrealPak.exe'
if (-not (Test-Path -LiteralPath $unrealPak -PathType Leaf)) { throw 'UnrealPak is missing.' }
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
$stockHintWidgetPackage =
    '/Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget'
$stockSfxSoundClassPackage = '/Game/Audio/Shares/SoundClasses/SC_SFX'
if ($packages -cnotcontains $ammoPickupPackage -or
    $packages -ccontains '/Game/Blueprints/BP_DynamicMeshActor' -or
    $packages -ccontains $stockHintWidgetPackage -or
    $packages -ccontains $stockSfxSoundClassPackage) {
    throw 'Package set lost an owned-child/editor-only stock-reference boundary.'
}
# Preserve named properties for the partial native mirrors and keep shader code
# in the owned packages. The shared cooker owns manifest transport and staging.
$cook = Invoke-VoyageModCookAssets -ModRoot $PSScriptRoot -ProjectFile $project -EngineRoot $EngineRoot `
    -OutputRoot $output -Packages $packages -TaggedPackages $packages -AdditionalCookArguments @(
        '-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False',
        '-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6,D3D11TargetedShaderFormats=PCD3D_SM5',
        ('-LocalDataCachePath=' + $ddc), ('-ZenDataPath=' + (Join-Path $ddc 'Zen')))
$loose = $cook.stage
$assetRelatives = @($packages | ForEach-Object { $_.Replace('/Game/', 'Voyage/Content/') })
$cloneRelative = 'Voyage/Content/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod'
$newName = 'DA_Ammo_Railgun_FullRod'
$newPackage = '/Game/Data/Assets/Ammo/' + $newName
$skillRelative = 'Voyage/Content/Data/Assets/Skill/Railgun/DA_Skill_Railgun'
$skillPackage = '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun'

if (-not $newPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'Railgun ammo escaped the confirmed Item AssetManager discovery root.'
}
$cloneTarget = Join-Path $loose $cloneRelative
$uassetGui = Join-Path $repo '.tools/bin/UAssetGUI.exe'
if (-not (Test-Path -LiteralPath $uassetGui -PathType Leaf)) {
    throw 'UAssetGUI is missing.'
}
$ammoConversion = & (Join-Path $PSScriptRoot 'Build/Convert-RailgunDataAsset.ps1') `
    -CookedAsset ($cloneTarget + '.uasset') -NativeReadback $ammoNativePath `
    -MappingsPath $mapping.mappingsPath -OutputRoot (Join-Path $output 'project-ammo')
$ammoWrite = $ammoConversion.write
$ammoReadback = $ammoConversion.readback
$ammoReadbackPath = $ammoConversion.readbackPath
$ammoReadbackData = Get-Content -LiteralPath $ammoReadbackPath -Raw | ConvertFrom-Json
$ammoImportNames = @($ammoReadbackData.Imports | ForEach-Object { [string]$_.ObjectName })
$removedImpactSoundPackage = `
    '/Game/Audio/SFX/Player/Pla_Weapons/Pla_Wpn_Boltgun/CUE_Pla_Wpn_BoltGun_Impact_Hard'
$removedImpactSoundAsset = 'CUE_Pla_Wpn_BoltGun_Impact_Hard'
if ($ammoImportNames -ccontains $removedImpactSoundPackage -or
    $ammoImportNames -ccontains $removedImpactSoundAsset) {
    throw 'Removed Railgun ammo ImpactSound remains in the package import table.'
}
$removedImpactMetaSoundPackage = `
    '/Game/Audio/SFX/Player/Pla_Weapons/Pla_Wpn_Boltgun/MTS_Pla_Wpn_BoltGun_Ricochet'
$removedImpactMetaSoundAsset = 'MTS_Pla_Wpn_BoltGun_Ricochet'
if ($ammoImportNames -ccontains $removedImpactMetaSoundPackage -or
    $ammoImportNames -ccontains $removedImpactMetaSoundAsset) {
    throw 'Removed Railgun ammo ImpactMetaSound remains in the package import table.'
}
$removedActivationSoundPackage = `
    '/Game/Audio/SFX/Player/Pla_Weapons/Pla_Wpn_Boltgun/Pla_Wpn_BoltGun_TheRod/MTS_Pla_Wpn_BoltGun_Rod_Shoot'
$removedActivationSoundAsset = 'MTS_Pla_Wpn_BoltGun_Rod_Shoot'
if ($ammoImportNames -ccontains $removedActivationSoundPackage -or
    $ammoImportNames -ccontains $removedActivationSoundAsset) {
    throw 'Removed Railgun ammo AbilityActivationSound remains in the package import table.'
}
$removedActivationSystemPackage = `
    '/Game/VFX/Weapons/Shared/NiagaraSystem/NS_Weapon_MuzzleFlash_Sniper_Rod'
$removedActivationSystemAsset = 'NS_Weapon_MuzzleFlash_Sniper_Rod'
if ($ammoImportNames -ccontains $removedActivationSystemPackage -or
    $ammoImportNames -ccontains $removedActivationSystemAsset) {
    throw 'Removed Railgun ammo ActivationSystem remains in the package import table.'
}
$removedImpactSystemPackage = '/Game/VFX/Weapons/ImpactEffects/NS_Impact_Metal_Voyage'
$removedImpactSystemAsset = 'NS_Impact_Metal_Voyage'
if ($ammoImportNames -ccontains $removedImpactSystemPackage -or
    $ammoImportNames -ccontains $removedImpactSystemAsset) {
    throw 'Removed Railgun ammo ImpactSystem remains in the package import table.'
}
$removedSecondaryIconPackage = '/Game/UI/Icons/Weapons/Bullet_Stencil_127'
$removedSecondaryIconAsset = 'Bullet_Stencil_127'
if ($ammoImportNames -ccontains $removedSecondaryIconPackage -or
    $ammoImportNames -ccontains $removedSecondaryIconAsset) {
    throw 'Removed Railgun ammo SecondaryIcon remains in the package import table.'
}
$remainingDamageTypeImports = @($ammoImportNames | Where-Object {
    $_ -like '*BP_DamageTypePhysicalForce*'
})
if ($remainingDamageTypeImports.Count -ne 0) {
    throw 'Removed Railgun ammo DamageTypeClass remains in the package import table.'
}

$skillTarget = Join-Path $loose $skillRelative
$skillConversion = & (Join-Path $PSScriptRoot 'Build/Convert-RailgunDataAsset.ps1') `
    -Asset skill -CookedAsset ($skillTarget + '.uasset') -NativeReadback $primaryNativePaths.skill `
    -MappingsPath $mapping.mappingsPath -OutputRoot (Join-Path $output 'project-skill')
$skillWrite = $skillConversion.write
$skillReadback = $skillConversion.readback
$skillReadbackPath = $skillConversion.readbackPath
$gunRelative = 'Voyage/Content/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$gunTarget = Join-Path $loose $gunRelative
$newGunName = 'DA_Item_Module_RailgunCannonMk01'
$newGunPackage = '/Game/Data/Assets/Modules/' + $newGunName
$gunConversion = & (Join-Path $PSScriptRoot 'Build/Convert-RailgunDataAsset.ps1') `
    -Asset gun -CookedAsset ($gunTarget + '.uasset') -NativeReadback $primaryNativePaths.gun `
    -MappingsPath $mapping.mappingsPath -OutputRoot (Join-Path $output 'project-gun')
$gunWrite = $gunConversion.write
$gunReadback = $gunConversion.readback
$gunReadbackPath = $gunConversion.readbackPath
if ($assetRelatives -contains
        'Voyage/Content/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New' -or
    $assetRelatives -contains
        'Voyage/Content/Data/Assets/Skill/Weapons/Ammo/DA_Skill_Railgun_Ammo_Test') {
    throw 'Release inventory still contains a stock Cyclone override or obsolete ammo skill.'
}
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
$registryResponse = Join-Path $output 'registry-response.txt'
$registryPak = Join-Path $output ($stem + '.registry.pak')
[IO.File]::WriteAllText($registryResponse,
    ('"' + [IO.Path]::GetFullPath($pluginRegistry) +
        '" "../../../Voyage/Mods/RailgunCatalogue/AssetRegistry.bin"' +
        [Environment]::NewLine),
    [Text.Encoding]::ASCII)
Invoke-NativeStage 'pack-registry' $unrealPak @($registryPak,('-Create=' + $registryResponse),'-Dest=../../../')
$registryListLog = Join-Path $output 'list-registry.log'
& $unrealPak -List $registryPak *> $registryListLog
if ($LASTEXITCODE -ne 0) { throw "Registry PAK listing failed; log: $registryListLog" }
$registryList = @(Get-Content -LiteralPath $registryListLog)
$registryEntries = @($registryList | Where-Object { $_ -match 'AssetRegistry\.bin' })
if ($registryEntries.Count -ne 1 -or
    @($registryList | Where-Object {
        $_.Contains('mount point "../../../Voyage/Mods/RailgunCatalogue/"')
    }).Count -ne 1 -or -not $registryEntries[0].Contains('"AssetRegistry.bin"')) {
    throw "Registry PAK mount/file list invalid; log: $registryListLog"
}
$registryReadback = Join-Path $output 'registry-readback'
$null = New-Item -ItemType Directory -Path $registryReadback
Invoke-NativeStage 'readback-registry' $unrealPak @($registryPak,'-Extract',$registryReadback,'-Filter=AssetRegistry.bin')
$readbackFiles = @(Get-ChildItem -LiteralPath $registryReadback -File -Recurse)
if ($readbackFiles.Count -ne 1 -or $readbackFiles[0].Name -cne 'AssetRegistry.bin' -or
    (Get-FileHash -LiteralPath $readbackFiles[0].FullName -Algorithm SHA256).Hash -cne
        (Get-FileHash -LiteralPath $pluginRegistry -Algorithm SHA256).Hash) {
    throw 'Packaged plugin registry readback hash mismatch.'
}
$payload = Join-Path $releaseStaging 'payload'
$pack = New-VoyageModContainer -ModRoot $PSScriptRoot -Stage $loose -OutputRoot $payload `
    -Packages $packages -ParserVersion UE5_8 -GameRoot $GameRoot -ContainerName $stem -CompanionPak $registryPak
$container = $pack.container
$verify = $pack.validation
$cloneInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
    $cloneRelative -Source Mod -ModContainer $container -GameRoot $GameRoot `
    -ModInspectionRoot $modInspectionRoot -AsJson) | ConvertFrom-Json
$cloneExports = Get-Content -LiteralPath $cloneInspection.jsonPath -Raw | ConvertFrom-Json
$cloneItems = @($cloneExports | Where-Object { $_.Type -ceq 'VoyageItemAmmo' -and $_.Name -ceq $newName })
if ($cloneItems.Count -ne 1) {
    throw 'Railgun ammo primary export is missing or duplicated.'
}
if ($cloneItems[0].Package -cne $newPackage -or
    $cloneItems[0].Properties.Category -cne 'EVoyageItemCategory::Ammo' -or
    [Math]::Abs([double]$cloneItems[0].Properties.Weight - $ammoWeightKg) -gt 0.00001 -or
    @($cloneItems[0].Properties.DropVariations | Where-Object {
        $_.RenderAsset.AssetPathName -ceq
            $ammoPickupCarrierMesh
    }).Count -lt 1 -or
    $cloneItems[0].Properties.DroppedActor.AssetPathName -cne $ammoPickupClass -or
    $null -eq $cloneItems[0].Properties.Components) {
    throw 'Railgun ammo lost a runtime identity, inventory, fabrication, or pickup contract.'
}
$pickupRelative = $ammoPickupPackage.Replace('/Game/', 'Voyage/Content/')
$pickupInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
    $pickupRelative -Source Mod -ModContainer $container -GameRoot $GameRoot `
    -ModInspectionRoot $modInspectionRoot -AsJson) | ConvertFrom-Json
$pickupDocument = Get-Content -LiteralPath $pickupInspection.jsonPath -Raw |
    ConvertFrom-Json
$pickupExports = @()
foreach ($pickupExport in $pickupDocument) { $pickupExports += $pickupExport }
function Get-PickupPropertyValue($Object, [string]$Name) {
    if ($null -eq $Object) { return $null }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    $property.Value
}
$pickupClasses = @($pickupExports | Where-Object {
    (Get-PickupPropertyValue $_ 'Type') -ceq 'BlueprintGeneratedClass' -and
        (Get-PickupPropertyValue $_ 'Name') -ceq
            'BP_RailgunAmmoCassette_C'
})
$pickupNodes = @($pickupExports | Where-Object {
    if ((Get-PickupPropertyValue $_ 'Type') -cne 'SCS_Node') { return $false }
    $properties = Get-PickupPropertyValue $_ 'Properties'
    (Get-PickupPropertyValue $properties 'ParentComponentOrVariableName') -ceq
        'MeshComponent' -and
        (Get-PickupPropertyValue $properties 'bIsParentComponentNative') -eq $true
})
$pickupMeshes = @($pickupExports | Where-Object {
    (Get-PickupPropertyValue $_ 'Type') -ceq 'StaticMeshComponent'
})
$pickupSuperPath = ''
$pickupClass = $pickupClasses | Select-Object -First 1
if ($pickupClasses.Count -eq 1) {
    $pickupSuper = Get-PickupPropertyValue $pickupClass 'Super'
    $pickupSuperPath = [string](Get-PickupPropertyValue $pickupSuper 'ObjectPath')
}
$expectedPickupParts = @()
foreach ($pickupPart in $ammoPickupInventory.parts) {
    $expectedPickupParts += $pickupPart
}
if ($pickupClasses.Count -ne 1 -or
    $pickupSuperPath -cne '/Game/Blueprints/BP_DynamicMeshActor.0' -or
    $pickupNodes.Count -ne $expectedPickupParts.Count -or
    $pickupMeshes.Count -ne $expectedPickupParts.Count) {
    throw 'Ammo pickup lost its exact stock parent or complete native-parent child set.'
}
foreach ($pickupPart in $expectedPickupParts) {
    $componentName = [string]$pickupPart.componentName
    $matchingNodes = @($pickupNodes | Where-Object {
        $properties = Get-PickupPropertyValue $_ 'Properties'
        (Get-PickupPropertyValue $properties 'InternalVariableName') -ceq
            $componentName
    })
    $matchingMeshes = @($pickupMeshes | Where-Object {
        (Get-PickupPropertyValue $_ 'Name') -ceq
            ($componentName + '_GEN_VARIABLE')
    })
    if ($matchingNodes.Count -ne 1 -or $matchingMeshes.Count -ne 1) {
        throw "Ammo pickup child is missing or duplicated: $componentName"
    }
    $meshProperties = Get-PickupPropertyValue $matchingMeshes[0] 'Properties'
    $staticMesh = Get-PickupPropertyValue $meshProperties 'StaticMesh'
    $bodyInstance = Get-PickupPropertyValue $meshProperties 'BodyInstance'
    $meshObjectPath = [string](Get-PickupPropertyValue $staticMesh 'ObjectPath')
    $collisionEnabled = [string](Get-PickupPropertyValue $bodyInstance 'CollisionEnabled')
    $expectedMeshPackage = ([string]$pickupPart.mesh).Substring(
        0, ([string]$pickupPart.mesh).LastIndexOf('.'))
    if (-not $meshObjectPath.StartsWith($expectedMeshPackage + '.',
            [StringComparison]::Ordinal) -or
        $collisionEnabled -cne 'ECollisionEnabled::NoCollision') {
        throw "Ammo pickup child lost shared mesh or no-collision identity: $componentName"
    }
}
$gunInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
    $gunRelative -Source Mod -ModContainer $container -GameRoot $GameRoot `
    -ModInspectionRoot $modInspectionRoot -AsJson) | ConvertFrom-Json
$gunExports = Get-Content -LiteralPath $gunInspection.jsonPath -Raw | ConvertFrom-Json
$gunItems = @($gunExports | Where-Object { $_.Type -ceq 'VoyageItem' -and $_.Name -ceq $newGunName })
if ($gunItems.Count -ne 1 -or
    $gunItems[0].Package -cne $newGunPackage -or
    $gunItems[0].Properties.Category -cne 'EVoyageItemCategory::Module' -or
    $gunItems[0].Properties.DroppedActor.AssetPathName -cne
        '/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun_C' -or
    $null -eq $gunItems[0].Properties.Components) {
    throw 'Railgun item lost its primary identity, module actor, or fabrication contract.'
}
if (-not $skillPackage.StartsWith($skillDiscoveryRoot + '/', [StringComparison]::Ordinal) -or
    -not $newGunPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal) -or
    -not $newPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'A Railgun primary asset escaped its confirmed AssetManager discovery root.'
}
$skillInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
    $skillRelative -Source Mod -ModContainer $container -GameRoot $GameRoot `
    -ModInspectionRoot $modInspectionRoot -AsJson) | ConvertFrom-Json
$skillExports = @(Get-Content -LiteralPath $skillInspection.jsonPath -Raw | ConvertFrom-Json)
$skillAssets = @($skillExports | Where-Object { $_.Type -ceq 'VoyageSkill' -and $_.Name -ceq 'DA_Skill_Railgun' })
if ($skillAssets.Count -ne 1) { throw 'Railgun research skill asset is missing or duplicated.' }
$skillItems = @($skillAssets[0].Properties.Items | ForEach-Object { [string]$_.ObjectPath })
if ($skillAssets[0].Package -cne $skillPackage -or
    $skillAssets[0].Properties.Type.Name -cne 'Skill' -or
    $skillItems.Count -ne 2 -or
    @($skillItems | Where-Object { $_.StartsWith($newGunPackage + '.', [StringComparison]::Ordinal) }).Count -ne 1 -or
    @($skillItems | Where-Object { $_.StartsWith($newPackage + '.', [StringComparison]::Ordinal) }).Count -ne 1) {
    throw 'Railgun research skill lost its primary type or gun/ammo unlock references.'
}
$semantic = & (Join-Path $PSScriptRoot 'Build/Test-RailgunPackage.ps1') `
    -Container $container -OutputRoot (Join-Path $output 'semantic') `
    -ModelInventory $inventoryPath -AmmoSourceJson $ammoReadbackPath -GameRoot $GameRoot
if ($semantic.status -ne 'passed') { throw 'Railgun semantic validation failed.' }
$containerReport = Get-Content -LiteralPath $verify.reportPath -Raw | ConvertFrom-Json
$bulkChunks = @($containerReport.chunkTypes | Where-Object { $_.type -ceq 'BulkData' } | ForEach-Object { $_.count } | Measure-Object -Sum).Sum
if ($null -eq $bulkChunks -or $bulkChunks -lt 1) { throw 'Cooked shot sound bulk data is absent from the container.' }
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
$semanticReport = Get-Content -LiteralPath $semantic.reportPath -Raw |
    ConvertFrom-Json
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
    semantic = [ordered]@{
        status = [string]$semanticReport.status
        runtime = [string]$semanticReport.runtime
        assertions = [string]$semanticReport.assertions
    }
    primaryAssetRegistry = [ordered]@{
        assetCount = [int]$registryResult.assetCount
        packageCount = [int]$registryResult.packageCount
        reopenVerified = [bool]$registryResult.reopenVerified
        primaryAssetIds = @($registryResult.primaryAssetIds)
    }
    settingsLoader = (Get-Content -LiteralPath $settingsValidationPath -Raw | ConvertFrom-Json)
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
    validation='build and static verification; gameplay validation is a separate gate';runtimeArchitecture='Single Railgun container with independent gun and ammo items, one research skill, plugin-local three-record primary-asset registry, GLB module actor with lifecycle-owned station initialization, operator, inputs, HUD and shot audio';
    mappingPath=$mapping.mappingsPath;validationSummary='validation-summary.json';
    packagingStatus=$pack.status;containerVerificationStatus=$verify.status;semanticStatus=$semantic.status;
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
$publicationValidation = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') `
    -ReleaseManifest $manifestPath -GameRoot $GameRoot -ValidateOnly -AllowDirtySource:$AllowDirtySource
if (-not [bool]$publicationValidation.validated) {
    throw 'Prepared Railgun release did not pass installation validation.'
}
$installation = $null
$settingsInstallation = $null
$restoreValidation = $null
if ($Install) {
    $installed = & (Join-Path $PSScriptRoot 'Install-Railgun.ps1') -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource
    $installation = $installed.installation
    $settingsInstallation = $installed.settingsInstallation
    $restoreValidation = $installed.restoreValidation
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
    settingsInstallation = $settingsInstallation
    restoreValidation = $restoreValidation
} | ConvertTo-Json -Depth 8 -Compress
