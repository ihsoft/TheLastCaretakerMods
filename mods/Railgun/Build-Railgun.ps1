[CmdletBinding()]
param([string]$OutputRoot = '', [switch]$SkipBuild, [switch]$Install, [string]$CacheRoot = 'P:\UnrealCache\TheLastCaretakerMods\UE5.8')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $PSScriptRoot 'Voyage.uproject'
$engine = 'K:\Epic Games\UE_5.8\Engine'
$editor = Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
$itemDiscoveryRoot = '/Game/Data/Assets'
$skillDiscoveryRoot = '/Game/Data/Assets/Skill'
if (-not $OutputRoot) { $OutputRoot = Join-Path $repo ('artifacts/railgun/build-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
$artifactBoundary = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifactBoundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be under repository artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Output already exists; use a new build identity.' }
$null = New-Item -ItemType Directory -Path $output
$settingsSchema = Join-Path $PSScriptRoot 'Settings/Railgun.settings.json'
$settingsDefaults = Join-Path $PSScriptRoot 'Assets/Railgun.ini'
$settingsGenerator = Join-Path $PSScriptRoot 'Build/New-RailgunSettings.ps1'
$generatedSettingsDirectory = Join-Path $output 'generated-settings'
$generatedSettingsHeader = Join-Path $generatedSettingsDirectory 'StationSettings.generated.h'
$generatedSettingsIni = Join-Path $generatedSettingsDirectory 'Railgun.ini'
& $settingsGenerator -SchemaPath $settingsSchema -DefaultIniPath $settingsDefaults -HeaderPath $generatedSettingsHeader -IniPath $generatedSettingsIni
[Environment]::SetEnvironmentVariable('RAILGUN_GENERATED_SETTINGS_DIR', $generatedSettingsDirectory, 'Process')
$sourcePaths = @('mods/Railgun','tools/UnrealEditorGeneratorCommon/Public')
$sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
$modelDirectory = Join-Path $PSScriptRoot 'Assets/Model'
$modelPath = Join-Path $modelDirectory 'model-source.json'
$model = Get-Content -LiteralPath $modelPath -Raw | ConvertFrom-Json
$modelFields = @($model.PSObject.Properties.Name | Sort-Object)
$expectedModelFields = @('entryInteraction','fabricatorCollision','nodes','schemaVersion')
if ($model.schemaVersion -ne 1 -or (Compare-Object $modelFields $expectedModelFields)) { throw 'Unsupported Railgun model registry.' }
$glbPath = [IO.Path]::GetFullPath((Join-Path $modelDirectory 'Railgun.glb'))
if (-not (Test-Path -LiteralPath $glbPath -PathType Leaf)) { throw "GLB not found: $glbPath" }
# The stable filename makes model replacement independent of revision names.
# Build provenance hashes the actual file and rejects edits during a build.
$sourcePaths += @($glbPath.Substring($repo.Length + 1).Replace('\','/'))
$sourceStatus = @(& git -C $repo status --porcelain -- $sourcePaths)
function Get-RailgunSourceHashes {
    @(& git -C $repo ls-files --cached --others --exclude-standard -- $sourcePaths | Sort-Object -Unique | Where-Object {
        Test-Path -LiteralPath (Join-Path $repo $_) -PathType Leaf
    } | ForEach-Object {
        [pscustomobject]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_) -Algorithm SHA256).Hash}
    })
}
$sourceHashes = @(Get-RailgunSourceHashes)
$fingerprint = (& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -OutputPath (Join-Path $output 'fingerprint.json')) | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne '25191271' -or $fingerprint.executable.sha256 -cne '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B') { throw 'Railgun game provenance mismatch.' }
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1')
$engineVersion = Get-Content -LiteralPath (Join-Path $engine 'Build/Build.version') -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or $engineVersion.PatchVersion -ne 2) { throw 'Railgun requires reviewed editor 5.8.2.' }

function Invoke-NativeStage([string]$Name, [string]$Executable, [string[]]$NativeArguments) {
    if ($Executable -eq $editor) { $NativeArguments += ('-ZenDataPath=' + (Join-Path $ddc 'Zen')) }
    $log = Join-Path $output ($Name + '.log')
    & $Executable @NativeArguments *> $log
    if ($LASTEXITCODE -ne 0) { throw "$Name failed ($LASTEXITCODE); log: $log" }
    Write-Host "$Name passed; log: $log"
}
function Add-MissingRailgunSettings([string]$TemplatePath, [string]$SettingsPath) {
    $templateLines = @([IO.File]::ReadAllLines($TemplatePath))
    $lines = @([IO.File]::ReadAllLines($SettingsPath))
    $newEnergyKey = 'FullChargeEnergyKWh'
    $newEnergyPattern = '^\s*' + [regex]::Escape($newEnergyKey) + '\s*='
    $legacyEnergyPattern = '^\s*FullChargeEnergyKJ\s*=\s*(.*?)\s*$'
    $defaultEnergyValue = 0.0
    $defaultEnergyText = @($templateLines | Where-Object { $_ -match $newEnergyPattern })
    if ($defaultEnergyText.Count -ne 1 -or
        -not [double]::TryParse(($defaultEnergyText[0] -split '=', 2)[1].Trim(), [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture, [ref]$defaultEnergyValue)) {
        throw 'Generated settings template has no valid FullChargeEnergyKWh default.'
    }
    $hasNewEnergy = @($lines | Where-Object { $_ -match $newEnergyPattern }).Count -gt 0
    $rewritten = New-Object 'System.Collections.Generic.List[string]'
    $migratedEnergy = $false
    foreach ($line in $lines) {
        if ($line -match $legacyEnergyPattern) {
            $migratedEnergy = $true
            if (-not $hasNewEnergy) {
                $legacyValue = 0.0
                if (-not [double]::TryParse($matches[1].Trim(), [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$legacyValue)) {
                    $legacyValue = 500.0
                }
                # Do not preserve the superseded 500-unit default over the new
                # Railgun balance. Non-default legacy values remain user tuning
                # and are converted to the game's displayed kWh scale.
                $gameValueNumber = if ([Math]::Abs($legacyValue - 500.0) -lt 0.000001) { $defaultEnergyValue } else { $legacyValue / 1000.0 }
                $gameValue = $gameValueNumber.ToString('0.###############', [Globalization.CultureInfo]::InvariantCulture)
                $rewritten.Add($newEnergyKey + '=' + $gameValue)
                $hasNewEnergy = $true
            }
            continue
        }
        $rewritten.Add($line)
    }
    if ($migratedEnergy) {
        if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' -ErrorAction SilentlyContinue).Count -gt 0) {
            throw 'Game started; settings migration refused.'
        }
        [IO.File]::WriteAllLines($SettingsPath, $rewritten, (New-Object System.Text.UTF8Encoding($false)))
        $lines = @([IO.File]::ReadAllLines($SettingsPath))
    }
    $existingKeys = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($line in $lines) {
        if ($line -match '^\s*([^#;][^=]*?)\s*=') { $null = $existingKeys.Add($matches[1].Trim()) }
    }
    $missing = New-Object 'System.Collections.Generic.List[string]'
    foreach ($line in $templateLines) {
        if ($line -match '^\s*([^#;][^=]*?)\s*=' -and -not $existingKeys.Contains($matches[1].Trim())) {
            $missing.Add($line)
        }
    }
    if ($missing.Count -eq 0) {
        if ($migratedEnergy) { return @($newEnergyKey) }
        return @()
    }
    if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' -ErrorAction SilentlyContinue).Count -gt 0) {
        throw 'Game started; settings update refused.'
    }
    $current = [IO.File]::ReadAllText($SettingsPath)
    $prefix = if ($current.EndsWith("`n")) { '' } else { "`r`n" }
    $addition = $prefix + "`r`n# Defaults added by a newer Railgun build.`r`n" + (($missing -join "`r`n") + "`r`n")
    [IO.File]::AppendAllText($SettingsPath, $addition, (New-Object System.Text.UTF8Encoding($false)))
    $reported = @($missing | ForEach-Object { ($_ -split '=', 2)[0].Trim() })
    if ($migratedEnergy) { $reported = @($newEnergyKey) + $reported }
    return $reported
}
if (-not $SkipBuild) {
    Invoke-NativeStage 'build' (Join-Path $engine 'Build/BatchFiles/Build.bat') @('VoyageEditor','Win64','Development',('-Project=' + $project),'-WaitMutex','-NoHotReloadFromIDE',('-Log=' + (Join-Path $output 'ubt.log')))
}
$content = Join-Path $PSScriptRoot 'Content'
if (Test-Path -LiteralPath $content) {
    # Exact owned generated tree, checked destination remains under artifacts.
    $resolvedContent = (Resolve-Path -LiteralPath $content).Path
    if ($resolvedContent -cne [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'Content'))) { throw 'Unexpected generated Content target.' }
    Move-Item -LiteralPath $resolvedContent -Destination (Join-Path $output 'previous-generated')
}
$ddc = [IO.Path]::GetFullPath($CacheRoot)
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $ddc, 'Process')
$null = New-Item -ItemType Directory -Path $ddc -Force
Invoke-NativeStage 'generate' $editor @($project,'-run=GenerateRailgun','-ShellOnly','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-unreal.log')))
$inventoryPath = Join-Path $output 'model-inventory.json'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Saved/RailgunGlbInventory.json') -Destination $inventoryPath
$inventory = Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
$packages = @($inventory.packages)
if ($packages.Count -lt 2 -or @($packages | Where-Object {
    -not $_.StartsWith('/Game/Mods/Railgun/Visual/') -and
    $_ -cne '/Game/Mods/Railgun/Module/BP_Module_Railgun'
}).Count) { throw 'GLB cook inventory escaped owned packages.' }
$shotSound = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Railgun_Shot_Blast.wav')).Path
$scopeOverlay = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ScopeOverlay.png')).Path
$chargingStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ChargingStatusIcon.png')).Path
$offlineStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/OfflineStatusIcon.png')).Path
$readyStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ReadyStatusIcon.png')).Path
$ammoIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Fabricator/RailgunAmmoIcon.png')).Path
$gunIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Fabricator/RailgunIcon.png')).Path
$skillIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Skill/RailgunSkill.png')).Path
$unrealPak = Join-Path $engine 'Binaries/Win64/UnrealPak.exe'
if (-not (Test-Path -LiteralPath $unrealPak -PathType Leaf)) { throw 'UnrealPak is missing.' }
$stockRegistry = & (Join-Path $repo 'tools/Extract-VoyageAssetRegistry.ps1') `
    -GameRoot $fingerprint.gameRoot -ExpectedExecutableSha256 $fingerprint.executable.sha256 `
    -UnrealPak $unrealPak -OutputRoot (Join-Path $output 'stock-registry')
if ($stockRegistry.status -cne 'passed') { throw 'Stock registry extraction failed.' }
Invoke-NativeStage 'generate-inputs' $editor @($project,'-run=GenerateRailgunInputs','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-inputs-unreal.log')))
Invoke-NativeStage 'generate-runtime' $editor @($project,'-run=GenerateRailgunRuntime','-DedicatedStation',('-StockRegistry=' + $stockRegistry.registryPath),('-ShotSound=' + $shotSound),('-ScopeOverlay=' + $scopeOverlay),('-ChargingStatusIcon=' + $chargingStatusIcon),('-OfflineStatusIcon=' + $offlineStatusIcon),('-ReadyStatusIcon=' + $readyStatusIcon),('-AmmoIcon=' + $ammoIcon),('-GunIcon=' + $gunIcon),('-SkillIcon=' + $skillIcon),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-runtime-unreal.log')))
$packages += @(
    '/Game/Mods/Railgun/Inputs/IA_RailgunLookYaw',
    '/Game/Mods/Railgun/Inputs/IA_RailgunLookPitch',
    '/Game/Mods/Railgun/Inputs/IA_RailgunExit',
    '/Game/Mods/Railgun/Inputs/IA_RailgunZoom',
    '/Game/Mods/Railgun/Inputs/IA_RailgunFire',
    '/Game/Mods/Railgun/Inputs/IMC_RailgunKeyboard',
    '/Game/Mods/Railgun/Inputs/DA_RailgunInputContext',
    '/Game/Mods/Railgun/Station/BP_RailgunOperator',
    '/Game/Mods/Railgun/Station/WBP_RailgunHUD',
    '/Game/Mods/Railgun/Station/T_RailgunOpticalMask',
    '/Game/Mods/Railgun/Station/T_RailgunStatusCharging',
    '/Game/Mods/Railgun/Station/T_RailgunStatusOffline',
    '/Game/Mods/Railgun/Station/T_RailgunStatusReady',
    '/Game/Mods/Railgun/Station/BP_RailgunTestShot',
    '/Game/Mods/Railgun/Station/S_RailgunShotBlast',
    '/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod',
    '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun',
    '/Game/Mods/Railgun/Research/T_RailgunSkill',
    '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon',
    '/Game/Mods/Railgun/Fabricator/T_RailgunIcon',
    '/Game/Mods/Railgun/Runtime/BP_RailgunCoordinator'
)
$packages = @($packages | Sort-Object -Unique)
# Keep new material shader code inline in owned packages. Do not change shared
# project config or require a game-global ShaderArchive-Voyage library override.
# Partial native mirrors serialize named property tags, never positional indices.
Invoke-NativeStage 'cook' $editor @($project,'-run=cook','-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs',('-Package=' + ($packages -join '+')),'-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False','-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6,D3D11TargetedShaderFormats=PCD3D_SM5','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'cook-unreal.log')))
Invoke-NativeStage 'verify-tagged' $editor @($project,'-run=GenerateRailgun','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-tagged-unreal.log')))
Invoke-NativeStage 'verify-runtime-tagged' $editor @($project,'-run=GenerateRailgunRuntime','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-runtime-tagged-unreal.log')))
$gunDonor = & (Join-Path $repo 'tools/Extract-VoyagePackage.ps1') -Filter 'Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.uasset' -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $output 'gun-item-original')
$gunDonorEvidence = Get-Content -LiteralPath $gunDonor.manifestPath -Raw | ConvertFrom-Json
if ($gunDonorEvidence.source -cne 'Game' -or
    $gunDonorEvidence.steamBuildId -cne [string]$fingerprint.steam.buildId -or
    $gunDonorEvidence.executableSha256 -cne $fingerprint.executable.sha256) {
    throw 'Gun item donor source provenance mismatch.'
}
$rodDonor = & (Join-Path $repo 'tools/Extract-VoyagePackage.ps1') `
    -Filter 'Data/Assets/Ammo/DA_Ammo_Bolt_Sniper_Rod.uasset' `
    -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $output 'rod-original')
$rodDonorEvidence = Get-Content -LiteralPath $rodDonor.manifestPath -Raw | ConvertFrom-Json
if ($rodDonorEvidence.source -cne 'Game' -or
    $rodDonorEvidence.steamBuildId -cne [string]$fingerprint.steam.buildId -or
    $rodDonorEvidence.executableSha256 -cne $fingerprint.executable.sha256) {
    throw 'Sniper Rod serialization donor source provenance mismatch.'
}
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
if ((Get-FileHash -LiteralPath $retoc -Algorithm SHA256).Hash -cne $gunDonorEvidence.retocSha256 -or
    $rodDonorEvidence.retocSha256 -cne $gunDonorEvidence.retocSha256) {
    throw 'retoc identity changed between donor extractions.'
}
$loose = Join-Path $output 'loose'
$assetRelatives = @($packages | ForEach-Object { $_.Replace('/Game/', 'Voyage/Content/') })
foreach ($assetRelative in $assetRelatives) {
    $looseAsset = Join-Path $loose $assetRelative
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $looseAsset) -Force
    foreach ($extension in @('.uasset','.uexp')) {
        $cooked = Join-Path (Join-Path $PSScriptRoot 'Saved/Cooked/Windows') ($assetRelative + $extension)
        Copy-Item -LiteralPath $cooked -Destination ($looseAsset + $extension)
    }
    foreach ($extension in @('.ubulk','.uptnl')) {
        $cooked = Join-Path (Join-Path $PSScriptRoot 'Saved/Cooked/Windows') ($assetRelative + $extension)
        if (Test-Path -LiteralPath $cooked -PathType Leaf) { Copy-Item -LiteralPath $cooked -Destination ($looseAsset + $extension) }
    }
}
$cloneRelative = 'Voyage/Content/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod'
$newName = 'DA_Ammo_Railgun_FullRod'
$newPackage = '/Game/Data/Assets/Ammo/' + $newName
$latin1 = [Text.Encoding]::GetEncoding(28591)

if (-not $newPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'Railgun ammo escaped the confirmed Item AssetManager discovery root.'
}
$cloneTarget = Join-Path $loose $cloneRelative
$rodSource = Join-Path $rodDonor.outputPath `
    'Voyage/Content/Data/Assets/Ammo/DA_Ammo_Bolt_Sniper_Rod.uasset'
$serializedAmmo = Join-Path $output 'serialized-ammo/DA_Ammo_Railgun_FullRod.uasset'
$ammoSerialization = & (Join-Path $repo 'tools/Invoke-VoyageAssetPatcher.ps1') `
    -Operation match-package-serialization -InputAsset ($cloneTarget + '.uasset') `
    -DonorAsset $rodSource -OutputAsset $serializedAmmo `
    -Mappings $mapping.mappingsPath -EngineVersion UE5_8 `
    -EvidenceRoot (Join-Path $output 'patcher-evidence')
if ($ammoSerialization.status -cne 'completed' -or
    -not (Test-Path -LiteralPath ([IO.Path]::ChangeExtension($serializedAmmo, '.uexp')) `
        -PathType Leaf)) {
    throw 'Railgun ammo did not adopt the Sniper Rod package serialization contract.'
}
Copy-Item -LiteralPath $serializedAmmo -Destination ($cloneTarget + '.uasset') -Force
Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($serializedAmmo, '.uexp')) `
    -Destination ($cloneTarget + '.uexp') -Force
$ammoReadbackPath = Join-Path $output 'ammo-import-readback.json'
$ammoReadback = & (Join-Path $repo 'tools/Invoke-VoyageAssetPatcher.ps1') `
    -Operation export-json -InputAsset ($cloneTarget + '.uasset') `
    -OutputAsset $ammoReadbackPath -Mappings $mapping.mappingsPath `
    -EngineVersion UE5_8 -EvidenceRoot (Join-Path $output 'patcher-evidence')
if ($ammoReadback.status -cne 'completed') {
    throw 'Railgun ammo import readback failed.'
}
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

# Preserve the complete four-export stock module-item layout while moving its
# primary identity to a mod-unique name inside the game's Item discovery root.
$gunSource = Join-Path $gunDonor.outputPath 'Voyage/Content/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium'
$gunRelative = 'Voyage/Content/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$gunTarget = Join-Path $loose $gunRelative
$null = New-Item -ItemType Directory -Path (Split-Path -Parent $gunTarget) -Force
$oldGunName = 'DA_Item_Module_WindTurbineMedium'
$newGunName = 'DA_Item_Module_RailgunCannonMk01'
$oldGunPackage = '/Game/Data/Assets/Modules/' + $oldGunName
$newGunPackage = '/Game/Data/Assets/Modules/' + $newGunName
if (-not $newGunPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'Railgun item escaped the confirmed Item AssetManager discovery root.'
}
if ($oldGunName.Length -ne $newGunName.Length -or
    $oldGunPackage.Length -ne $newGunPackage.Length) {
    throw 'Railgun item clone requires equal-length identities.'
}
$gunHeaderBytes = [IO.File]::ReadAllBytes($gunSource + '.uasset')
$gunExportBytes = [IO.File]::ReadAllBytes($gunSource + '.uexp')
$gunHeaderText = $latin1.GetString($gunHeaderBytes)
$gunExportText = $latin1.GetString($gunExportBytes)
$gunNameMatches = [regex]::Matches($gunHeaderText, [regex]::Escape($oldGunName))
$gunPackageMatches = [regex]::Matches($gunHeaderText, [regex]::Escape($oldGunPackage))
if ($gunNameMatches.Count -ne 3 -or $gunPackageMatches.Count -ne 2 -or
    $gunExportText.Contains($oldGunName) -or $gunHeaderText.Contains($newGunName) -or
    $gunExportText.Contains($newGunName)) {
    throw 'Railgun item clone identity layout changed; no byte patch applied.'
}
$gunPackageReplacement = [Text.Encoding]::ASCII.GetBytes($newGunPackage)
foreach ($match in $gunPackageMatches) {
    [Array]::Copy($gunPackageReplacement, 0, $gunHeaderBytes, $match.Index,
        $gunPackageReplacement.Length)
}
$gunHeaderText = $latin1.GetString($gunHeaderBytes)
$gunNameMatches = [regex]::Matches($gunHeaderText, [regex]::Escape($oldGunName))
if ($gunNameMatches.Count -ne 1) {
    throw 'Railgun item clone standalone identity layout changed.'
}
$gunReplacement = [Text.Encoding]::ASCII.GetBytes($newGunName)
foreach ($match in $gunNameMatches) {
    [Array]::Copy($gunReplacement, 0, $gunHeaderBytes, $match.Index, $gunReplacement.Length)
}
$patchedGunText = $latin1.GetString($gunHeaderBytes)
if ($patchedGunText.Contains($oldGunName) -or
    [regex]::Matches($patchedGunText, [regex]::Escape($newGunName)).Count -ne 3 -or
    [regex]::Matches($patchedGunText, [regex]::Escape($newGunPackage)).Count -ne 2) {
    throw 'Railgun item clone identity postcondition failed.'
}
[IO.File]::WriteAllBytes($gunTarget + '.uasset', $gunHeaderBytes)
Copy-Item -LiteralPath ($gunSource + '.uexp') -Destination ($gunTarget + '.uexp')
$patchedGun = Join-Path $output 'patched-gun-item/DA_Item_Module_RailgunCannonMk01.uasset'
$gunPatchSpecification = Join-Path $PSScriptRoot 'Build/railgun-item-patch.json'
$gunPatch = & (Join-Path $repo 'tools/Invoke-VoyageAssetPatcher.ps1') `
    -Operation patch-item-data-asset -InputAsset ($gunTarget + '.uasset') `
    -OutputAsset $patchedGun -Mappings $mapping.mappingsPath -EngineVersion UE5_8 `
    -Specification $gunPatchSpecification `
    -EvidenceRoot (Join-Path $output 'patcher-evidence')
if ($gunPatch.status -cne 'completed' -or
    -not (Test-Path -LiteralPath ([IO.Path]::ChangeExtension($patchedGun, '.uexp')) -PathType Leaf)) {
    throw 'The complete Railgun module-item clone was not patched and reopened successfully.'
}
Copy-Item -LiteralPath $patchedGun -Destination ($gunTarget + '.uasset') -Force
Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($patchedGun, '.uexp')) `
    -Destination ($gunTarget + '.uexp') -Force
$assetRelatives += $gunRelative
if ($assetRelatives -contains
        'Voyage/Content/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New' -or
    $assetRelatives -contains
        'Voyage/Content/Data/Assets/Skill/Weapons/Ammo/DA_Skill_Railgun_Ammo_Test') {
    throw 'Release inventory still contains a stock Cyclone override or obsolete ammo skill.'
}
Copy-Item -LiteralPath (Join-Path $gunDonor.outputPath 'scriptobjects.bin') -Destination (Join-Path $loose 'scriptobjects.bin')
$payload = Join-Path $output 'payload'
$null = New-Item -ItemType Directory -Path $payload
$stem = 'Railgun_P'
$container = Join-Path $payload ($stem + '.utoc')
$pack = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') -Executable $retoc -Arguments @('to-zen',$loose,$container,'--version','UE5_8') -TimeoutSeconds 60 -MemoryLimitMB 1024
if ($pack.status -ne 'passed') { throw 'Packaging failed.' }
$containerPak = [IO.Path]::ChangeExtension($container, '.pak')
$resolvedContainerPak = [IO.Path]::GetFullPath($containerPak)
if (-not $resolvedContainerPak.StartsWith($output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath $resolvedContainerPak -PathType Leaf)) { throw 'retoc did not create the expected owned PAK.' }
# Merge the gun, ammo and shared research skill into the complete stock registry.
$patchedRegistry = Join-Path $output 'AssetRegistry.bin'
Invoke-NativeStage 'patch-registry' $editor @($project,'-run=GenerateRailgunRuntime','-PatchStockRegistry',
    ('-StockRegistry=' + $stockRegistry.registryPath),('-OutputRegistry=' + $patchedRegistry),
    '-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'patch-registry-unreal.log')))
if (-not (Test-Path -LiteralPath $patchedRegistry -PathType Leaf) -or
    (Get-Item -LiteralPath $patchedRegistry).Length -lt $stockRegistry.registryLength) {
    throw 'Patched complete registry is missing or unexpectedly small.'
}
$registryResponse = Join-Path $output 'registry-response.txt'
$registryPak = Join-Path $output 'Railgun_P.registry.pak'
[IO.File]::WriteAllText($registryResponse,
    ('"' + [IO.Path]::GetFullPath($patchedRegistry) + '" "../../../Voyage/AssetRegistry.bin"' + [Environment]::NewLine),
    [Text.Encoding]::ASCII)
Invoke-NativeStage 'pack-registry' $unrealPak @($registryPak,('-Create=' + $registryResponse),'-Dest=../../../')
$registryListLog = Join-Path $output 'list-registry.log'
& $unrealPak -List $registryPak *> $registryListLog
if ($LASTEXITCODE -ne 0) { throw "Registry PAK listing failed; log: $registryListLog" }
$registryList = Get-Content -LiteralPath $registryListLog -Raw
if (-not $registryList.Contains('mount point "../../../Voyage/"') -or
    @($registryList -split "`n" | Where-Object { $_ -match 'AssetRegistry\.bin' }).Count -ne 1) {
    throw "Registry PAK mount/file list invalid; log: $registryListLog"
}
$registryReadback = Join-Path $output 'registry-readback'
$null = New-Item -ItemType Directory -Path $registryReadback
Invoke-NativeStage 'readback-registry' $unrealPak @($registryPak,'-Extract',$registryReadback,'-Filter=AssetRegistry.bin')
$readbackFiles = @(Get-ChildItem -LiteralPath $registryReadback -File -Recurse)
if ($readbackFiles.Count -ne 1 -or $readbackFiles[0].Name -cne 'AssetRegistry.bin' -or
    (Get-FileHash -LiteralPath $readbackFiles[0].FullName -Algorithm SHA256).Hash -cne
        (Get-FileHash -LiteralPath $patchedRegistry -Algorithm SHA256).Hash) {
    throw 'Packaged complete registry readback hash mismatch.'
}
Move-Item -LiteralPath $containerPak -Destination (Join-Path $output 'Railgun_P.empty.pak')
Move-Item -LiteralPath $registryPak -Destination $containerPak
$expected = Join-Path $output 'expected-packages.txt'
[IO.File]::WriteAllLines($expected, @($assetRelatives | ForEach-Object { $_ + '.uasset' }))
$verify = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') -Container $container -ExpectedPackageList $expected
if ($verify.status -ne 'passed' -or -not $verify.packageSetMatches) { throw 'Container verification failed.' }
$cloneInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') $cloneRelative -Source Mod -ModContainer $container -AsJson) | ConvertFrom-Json
$cloneExports = Get-Content -LiteralPath $cloneInspection.jsonPath -Raw | ConvertFrom-Json
$cloneItems = @($cloneExports | Where-Object { $_.Type -ceq 'VoyageItemAmmo' -and $_.Name -ceq $newName })
if ($cloneExports.Count -ne 1 -or $cloneItems.Count -ne 1) {
    throw 'Railgun ammo must contain exactly one complete VoyageItemAmmo export.'
}
$clonePropertyNames = @($cloneItems[0].Properties.PSObject.Properties.Name)
$expectedClonePropertyNames = @(
    'Caliber','Icon','Category','CategoryAsset','Quality','Weight','CraftTime',
    'CraftElectricityCost','CraftAmount','CraftFilter','Components','DropVariations',
    'DroppedActor','MaxDropCount','Name','Description'
)
$clonePropertyDifferences = @(Compare-Object -ReferenceObject $expectedClonePropertyNames `
    -DifferenceObject $clonePropertyNames -CaseSensitive)
if ($cloneItems[0].Package -cne $newPackage -or
    $clonePropertyDifferences.Count -ne 0 -or
    $clonePropertyNames -ccontains 'WeaponData' -or
    $clonePropertyNames -ccontains 'ScalePerItem' -or
    [int]$cloneItems[0].Properties.MaxDropCount -ne 50 -or
    [double]$cloneItems[0].Properties.Caliber -ne 45.0 -or
    $cloneItems[0].Properties.Icon.ObjectPath -cne '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon.0' -or
    $clonePropertyNames -ccontains 'SecondaryIcon' -or
    [double]$cloneItems[0].Properties.Weight -ne 3.9 -or
    $cloneItems[0].Properties.Quality -cne 'EVoyageItemQuality::Common' -or
    [double]$cloneItems[0].Properties.CraftTime -ne 6.0 -or
    [int]$cloneItems[0].Properties.CraftAmount -ne 6 -or
    @($cloneItems[0].Properties.DropVariations).Count -ne 1 -or
    $cloneItems[0].Properties.DroppedActor.AssetPathName -cne '/Game/Blueprints/BP_DynamicMeshActor.BP_DynamicMeshActor_C' -or
    $cloneItems[0].Properties.Name.SourceString -cne 'Railgun Kinetic Rounds' -or
    $cloneItems[0].Properties.Description.SourceString -cne 'Armor-piercing kinetic rounds. No explosives, just mass and velocity.') {
    throw 'Complete one-export Railgun ammo did not survive strict inspection.'
}
$gunInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') $gunRelative -Source Mod -ModContainer $container -AsJson) | ConvertFrom-Json
$gunExports = Get-Content -LiteralPath $gunInspection.jsonPath -Raw | ConvertFrom-Json
$gunItems = @($gunExports | Where-Object { $_.Type -ceq 'VoyageItem' -and $_.Name -ceq $newGunName })
if ($gunExports.Count -ne 4 -or $gunItems.Count -ne 1 -or
    $gunItems[0].Package -cne $newGunPackage -or
    $gunItems[0].Properties.Icon.ObjectPath -cne '/Game/Mods/Railgun/Fabricator/T_RailgunIcon.0' -or
    @($gunItems[0].Properties.Components).Count -ne 1 -or
    $gunItems[0].Properties.Components[0].Key -notmatch 'DA_Part_AlloyFrame' -or
    [int]$gunItems[0].Properties.Components[0].Value -ne 1 -or
    @($gunItems[0].Properties.properties).Count -ne 0 -or
    $gunItems[0].Properties.DroppedActor.AssetPathName -cne
        '/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun_C' -or
    [double]$gunItems[0].Properties.CraftTime -ne 10.0 -or
    [double]$gunItems[0].Properties.CraftElectricityCost -ne 5.0 -or
    [double]$gunItems[0].Properties.Weight -ne 89.7 -or
    -not [bool]$gunItems[0].Properties.bIsDismantlable -or
    -not [bool]$gunItems[0].Properties.bIsRepairable) {
    throw 'Railgun item did not survive independent complete-clone inspection.'
}
$skillPackage = '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun'
$skillRelative = 'Voyage/Content/Data/Assets/Skill/Railgun/DA_Skill_Railgun'
if (-not $skillPackage.StartsWith($skillDiscoveryRoot + '/', [StringComparison]::Ordinal) -or
    -not $newGunPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal) -or
    -not $newPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'A Railgun primary asset escaped its confirmed AssetManager discovery root.'
}
$skillInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') $skillRelative -Source Mod -ModContainer $container -AsJson) | ConvertFrom-Json
$skillExports = @(Get-Content -LiteralPath $skillInspection.jsonPath -Raw | ConvertFrom-Json)
$skillAssets = @($skillExports | Where-Object { $_.Type -ceq 'VoyageSkill' -and $_.Name -ceq 'DA_Skill_Railgun' })
if ($skillAssets.Count -ne 1) { throw 'Railgun research skill asset is missing or duplicated.' }
$skillItems = @($skillAssets[0].Properties.Items | ForEach-Object { [string]$_.ObjectPath })
if ($skillAssets[0].Package -cne $skillPackage -or
    $skillAssets[0].Properties.Unlock.UnlockMethod -cne 'EVoyageSkillUnlockMethod::Tier' -or
    [int]$skillAssets[0].Properties.Unlock.Requirement -ne 19 -or
    (($skillAssets[0].Properties.Unlock.PSObject.Properties.Name -contains 'Cost') -and
        [int]$skillAssets[0].Properties.Unlock.Cost -ne 0) -or
    $skillAssets[0].Properties.Icon.ObjectPath -cne '/Game/Mods/Railgun/Research/T_RailgunSkill.0' -or
    $skillItems.Count -ne 2 -or
    @($skillItems | Where-Object { $_.StartsWith($newGunPackage + '.', [StringComparison]::Ordinal) }).Count -ne 1 -or
    @($skillItems | Where-Object { $_.StartsWith($newPackage + '.', [StringComparison]::Ordinal) }).Count -ne 1) {
    throw 'Railgun research skill is not the exact Tier-19 unlock for gun and ammo.'
}
$semantic = & (Join-Path $PSScriptRoot 'Validate-Railgun.ps1') -Container $container -OutputRoot (Join-Path $output 'semantic') -ModelInventory $inventoryPath
if ($semantic.status -ne 'passed') { throw 'Railgun semantic validation failed.' }
$containerReport = Get-Content -LiteralPath $verify.reportPath -Raw | ConvertFrom-Json
$bulkChunks = @($containerReport.chunkTypes | Where-Object { $_.type -ceq 'BulkData' } | ForEach-Object { $_.count } | Measure-Object -Sum).Sum
if ($null -eq $bulkChunks -or $bulkChunks -lt 1) { throw 'Cooked shot sound bulk data is absent from the container.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Railgun_P.autoload') -Destination $payload
Copy-Item -LiteralPath $generatedSettingsIni -Destination $payload
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'README.txt') -Destination $payload
$version = Split-Path -Leaf $output
$archivePath = Join-Path $output ('Railgun_' + $version + '.zip')
Compress-Archive -Path (Join-Path $payload '*') -DestinationPath $archivePath
$sourceAfter = @(& git -C $repo status --porcelain -- $sourcePaths)
if (($sourceAfter -join "`n") -cne ($sourceStatus -join "`n")) { throw 'Source status changed during preparation.' }
if ((@(Get-RailgunSourceHashes) | ConvertTo-Json -Compress) -cne ($sourceHashes | ConvertTo-Json -Compress)) { throw 'Source content changed during preparation.' }
if ((& git -C $repo rev-parse HEAD).Trim() -cne $sourceCommit) { throw 'Repository HEAD changed during preparation.' }
$provenance = [ordered]@{
    schemaVersion=1;mod='Railgun';version=$version;createdAtUtc=[DateTime]::UtcNow.ToString('o');
    sourceCommit=$sourceCommit;dirtySource=($sourceStatus.Count -gt 0);sourceStatus=$sourceStatus;sourceHashes=$sourceHashes;
    gameEngineVersion='5.8';gameEngineVersionBasis='Reviewed mapping/parser target; game patch version not independently established';editorEngineVersion='5.8.2';retocCompatibilityVersion='UE5_8';retocSha256=$gunDonorEvidence.retocSha256;
    gameFingerprint=@{steamBuildId=[string]$fingerprint.steam.buildId;executableSha256=$fingerprint.executable.sha256};
    validation='build and static verification; gameplay validation is a separate gate';runtimeArchitecture='Single Railgun container with independent gun and ammo items, one research skill, GLB module actor, operator, inputs, HUD, shot audio and autoload coordinator';
    mappingSha256=$mapping.sha256;verificationReport=$verify.reportPath;packagingReport=$pack.reportPath;semanticReport=$semantic.reportPath;
    ammoSerializationReport=$ammoSerialization.logPath;ammoSerializationPatcherSha256=$ammoSerialization.patcherBinarySha256;
}
$provenance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-provenance.json') -Encoding UTF8
$releaseSourcePaths = @($sourcePaths | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $repo $_)) })
$release = (& (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod Railgun -Version $version -Container $container -Archive $archivePath -SourcePath $releaseSourcePaths -AllowDirtySource -AsJson) | ConvertFrom-Json
$manifestPath = Join-Path $output 'release-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Manifest producer did not publish the candidate.' }
$installation = $null
$settingsInstallation = $null
if ($Install) {
    $installation = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') -ReleaseManifest $manifestPath -AllowDirtySource
    $settingsPath = Join-Path $installation.paksDirectory 'Railgun.ini'
    if (-not (Test-Path -LiteralPath $settingsPath)) {
        if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' -ErrorAction SilentlyContinue).Count -gt 0) { throw 'Game started; settings installation refused.' }
        Copy-Item -LiteralPath (Join-Path $payload 'Railgun.ini') -Destination $settingsPath
        $settingsInstallation = [pscustomobject]@{path=$settingsPath;created=$true;addedKeys=@();sha256=(Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash}
    } else {
        $addedKeys = @(Add-MissingRailgunSettings (Join-Path $payload 'Railgun.ini') $settingsPath)
        $settingsInstallation = [pscustomobject]@{path=$settingsPath;created=$false;addedKeys=$addedKeys;sha256=(Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash}
    }
    Write-Host 'Railgun installed successfully; container hashes verified and settings are present.'
}
[pscustomobject]@{status=$(if ($Install) { 'installed' } else { 'prepared-not-installed' });releaseManifestPath=$manifestPath;archivePath=$archivePath;verificationReport=$verify.reportPath;installation=$installation;settingsInstallation=$settingsInstallation} | ConvertTo-Json -Depth 8 -Compress
