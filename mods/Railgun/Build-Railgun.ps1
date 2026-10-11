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
$ammoPickupPackage = '/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette'
$ammoPickupClass = $ammoPickupPackage + '.BP_RailgunAmmoCassette_C'
$defaultVersion = 'build-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
$tmpOwnerRoot = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/Railgun'))
$tmpBoundary = $tmpOwnerRoot + [IO.Path]::DirectorySeparatorChar
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $tmpOwnerRoot (
        $defaultVersion + '-' + [Guid]::NewGuid().ToString('N'))
}
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not $output.StartsWith($tmpBoundary, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot must be a fresh directory below repository Tmp/Railgun.'
}
if (Test-Path -LiteralPath $output) { throw 'OutputRoot already exists.' }
$version = $defaultVersion
$null = New-Item -ItemType Directory -Path $output
$releaseStaging = Join-Path $output 'candidate'
$boundedToolRoot = Join-Path $output 'bounded-tool-runs'
$containerCheckRoot = Join-Path $output 'container-checks'
$modInspectionRoot = Join-Path $output 'asset-inspections'
$settingsSchema = Join-Path $PSScriptRoot 'Settings/Railgun.settings.json'
$settingsDefaults = Join-Path $PSScriptRoot 'Assets/Railgun.ini'
$settingsGenerator = Join-Path $PSScriptRoot 'Build/New-RailgunSettings.ps1'
$generatedSettingsDirectory = Join-Path $PSScriptRoot 'Intermediate/GeneratedSettings'
$generatedSettingsEvidenceDirectory = Join-Path $output 'generated-settings'
$null = New-Item -ItemType Directory -Path $generatedSettingsDirectory -Force
$null = New-Item -ItemType Directory -Path $generatedSettingsEvidenceDirectory
$generatedSettingsHeader = Join-Path $generatedSettingsDirectory 'StationSettings.generated.h'
$generatedSettingsIni = Join-Path $generatedSettingsDirectory 'Railgun.ini'
& $settingsGenerator -SchemaPath $settingsSchema -DefaultIniPath $settingsDefaults -HeaderPath $generatedSettingsHeader -IniPath $generatedSettingsIni
Copy-Item -LiteralPath $generatedSettingsHeader,$generatedSettingsIni `
    -Destination $generatedSettingsEvidenceDirectory
$sourcePaths = @(
    'mods/Railgun',
    'tools/UnrealEditorGeneratorCommon/Public',
    'tools/Get-VoyageAssetJson.ps1',
    'tools/Get-VoyageBuildFingerprint.ps1',
    'tools/Get-VoyageMappings.ps1',
    'tools/Install-VoyageRelease.ps1',
    'tools/Invoke-VoyageBoundedTool.ps1',
    'tools/New-VoyageAssetRegistry.ps1',
    'tools/New-VoyageReleaseManifest.ps1',
    'tools/Restore-VoyageReleaseInstallation.ps1',
    'tools/Test-VoyageContainer.ps1',
    'tools/VoyageAssetRegistryWriter'
)
$sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
$modelDirectory = Join-Path $PSScriptRoot 'Assets/Model'
$modelPath = Join-Path $modelDirectory 'model-source.json'
$model = Get-Content -LiteralPath $modelPath -Raw | ConvertFrom-Json
$modelFields = @($model.PSObject.Properties.Name | Sort-Object)
$expectedModelFields = @('entryInteraction','fabricatorCollision','inventoryInteraction','nodes','schemaVersion')
if ($model.schemaVersion -ne 1 -or (Compare-Object $modelFields $expectedModelFields)) { throw 'Unsupported Railgun model registry.' }
$glbPath = [IO.Path]::GetFullPath((Join-Path $modelDirectory 'Railgun.glb'))
if (-not (Test-Path -LiteralPath $glbPath -PathType Leaf)) { throw "GLB not found: $glbPath" }
$ammoItemJsonPath = Join-Path $PSScriptRoot 'Assets/Fabricator/railgun-ammo-item.json'
$skillJsonPath = Join-Path $PSScriptRoot 'Assets/Skill/railgun-skill.json'
$gunJsonPath = Join-Path $PSScriptRoot 'Assets/Fabricator/railgun-item.json'
$dataAssetContractPath = Join-Path $PSScriptRoot 'Assets/data-assets-contract.json'
if (-not (Test-Path -LiteralPath $ammoItemJsonPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $skillJsonPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $gunJsonPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $dataAssetContractPath -PathType Leaf)) {
    throw 'Railgun item/skill source or the shared data-asset contract is missing.'
}
function Read-OwnedAssetJson([string]$JsonPath, $Contract,
    [string]$Label) {
    $contractFields = @($contract.PSObject.Properties.Name | Sort-Object)
    $expectedJsonPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot $contract.sourceFile))
    if ((Compare-Object $contractFields @('class','object','package','sourceFile')) -or
        $expectedJsonPath -cne [IO.Path]::GetFullPath($JsonPath)) {
        throw "$Label entry has an unsupported identity schema or source path."
    }
    $source = Get-Content -LiteralPath $JsonPath -Raw | ConvertFrom-Json
    $primaryExports = @($source.Exports | Where-Object {
        $_.ObjectName -ceq $contract.object
    })
    if ($source.FolderName -cne $contract.package -or $primaryExports.Count -ne 1 -or
        @($source.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count -ne 0) {
        throw "$Label JSON has the wrong package identity, primary export, or a RawExport."
    }
    $classIndex = [int]$primaryExports[0].ClassIndex
    $classImportIndex = -$classIndex - 1
    if ($classIndex -ge 0 -or $classImportIndex -lt 0 -or
        $classImportIndex -ge @($source.Imports).Count -or
        $source.Imports[$classImportIndex].ObjectName -cne $contract.class) {
        throw "$Label JSON primary export has the wrong native class."
    }
    $nullPackageImports = @($source.Imports | Where-Object {
        [string]::IsNullOrEmpty([string]$_.PackageName)
    })
    if ($nullPackageImports.Count -ne 0) {
        $names = @($nullPackageImports | ForEach-Object { [string]$_.ObjectName })
        throw "$Label JSON has imports with null PackageName: $($names -join ', ')."
    }
    [pscustomobject]@{contract=$contract;source=$source;primary=$primaryExports[0]}
}
$dataAssetContract = Get-Content -LiteralPath $dataAssetContractPath -Raw |
    ConvertFrom-Json
$dataAssetContractFields = @($dataAssetContract.PSObject.Properties.Name |
    Sort-Object)
$serializationFields = @($dataAssetContract.serialization.PSObject.Properties.Name |
    Sort-Object)
$expectedAssetSources = @(
    'Assets/Fabricator/railgun-ammo-item.json',
    'Assets/Fabricator/railgun-item.json',
    'Assets/Skill/railgun-skill.json'
)
$actualAssetSources = @($dataAssetContract.assets | ForEach-Object {
    [string]$_.sourceFile
} | Sort-Object)
if ($dataAssetContract.schemaVersion -ne 3 -or
    (Compare-Object $dataAssetContractFields @(
        'assets','revalidateWhen','schemaVersion','serialization')) -or
    (Compare-Object $serializationFields @(
        'engineVersion','executableSha256','mappingSha256','steamBuildId',
        'uassetApiCommit','uassetGuiCommit','uassetGuiSha256')) -or
    (Compare-Object $actualAssetSources @($expectedAssetSources | Sort-Object)) -or
    @($dataAssetContract.assets).Count -ne $expectedAssetSources.Count) {
    throw 'Railgun shared data-asset contract has an unsupported schema or asset set.'
}
$serializationContract = $dataAssetContract.serialization
function Get-OwnedAssetContract([string]$SourceFile) {
    $matches = @($dataAssetContract.assets | Where-Object {
        $_.sourceFile -ceq $SourceFile
    })
    if ($matches.Count -ne 1) {
        throw "Railgun data-asset contract entry is missing or duplicated: $SourceFile"
    }
    $matches[0]
}
function ConvertTo-ComparableExportsJson($Exports) {
    $normalized = @($Exports | ForEach-Object {
        $export = [ordered]@{}
        foreach ($property in $_.PSObject.Properties) {
            # The writer recomputes these physical package-layout values. They
            # are not authored UObject data and may change with payload size.
            if ($property.Name -cnotin @('SerialOffset', 'SerialSize')) {
                $export[$property.Name] = $property.Value
            }
        }
        [pscustomobject]$export
    })
    $normalized | ConvertTo-Json -Depth 100 -Compress
}
$ammoItemAsset = Read-OwnedAssetJson $ammoItemJsonPath `
    (Get-OwnedAssetContract 'Assets/Fabricator/railgun-ammo-item.json') `
    'Railgun ammo'
$ammoItemSource = $ammoItemAsset.source
$ammoWeightProperties = @($ammoItemAsset.primary.Data | Where-Object {
    $_.Name -ceq 'Weight'
})
if ($ammoWeightProperties.Count -ne 1 -or
    [double]$ammoWeightProperties[0].Value -le 0.0) {
    throw 'Railgun ammo JSON requires one positive Weight for the six-round magazine limit.'
}
$ammoWeightKg = [double]$ammoWeightProperties[0].Value
$ammoDropVariationProperties = @($ammoItemAsset.primary.Data | Where-Object {
    $_.Name -ceq 'DropVariations'
})
if ($ammoDropVariationProperties.Count -ne 1) {
    throw 'Railgun ammo JSON requires one DropVariations property.'
}
$ammoRenderAssets = @($ammoDropVariationProperties[0].Value.Value | Where-Object {
    $_.Name -ceq 'RenderAsset'
})
if ($ammoRenderAssets.Count -ne 1) {
    throw 'Railgun ammo JSON requires one supported RenderAsset drop variation.'
}
$ammoRenderAssetIdentity = $ammoRenderAssets[0].Value.AssetPath
$ammoPickupCarrierMesh = [string]$ammoRenderAssetIdentity.PackageName + '.' +
    [string]$ammoRenderAssetIdentity.AssetName
if ([string]::IsNullOrWhiteSpace([string]$ammoRenderAssetIdentity.PackageName) -or
    [string]::IsNullOrWhiteSpace([string]$ammoRenderAssetIdentity.AssetName)) {
    throw 'Railgun ammo JSON RenderAsset identity is incomplete.'
}
$skillAsset = Read-OwnedAssetJson $skillJsonPath `
    (Get-OwnedAssetContract 'Assets/Skill/railgun-skill.json') 'Railgun skill'
$skillSource = $skillAsset.source
$gunAsset = Read-OwnedAssetJson $gunJsonPath `
    (Get-OwnedAssetContract 'Assets/Fabricator/railgun-item.json') 'Railgun item'
$gunSource = $gunAsset.source
# The stable filename makes model replacement independent of revision names.
# Build provenance hashes the actual file and rejects edits during a build.
$sourcePaths += @($glbPath.Substring($repo.Length + 1).Replace('\','/'))
function Get-RailgunSourceStatus {
    @(& git -C $repo status --porcelain -- $sourcePaths)
}
$sourceStatus = @(Get-RailgunSourceStatus)
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
if ($Install) {
    $legacyAutoload = [IO.Path]::GetFullPath((Join-Path $fingerprint.gameRoot `
        'Voyage/Content/Paks/Railgun.autoload'))
    if (Test-Path -LiteralPath $legacyAutoload -PathType Leaf) {
        throw ('Legacy Railgun.autoload is still installed. Refusing a loader-free ' +
            'Railgun install until the reviewed one-time recoverable migration retires it.')
    }
}
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1')
if ([string]$serializationContract.steamBuildId -cne
        [string]$fingerprint.steam.buildId -or
    $serializationContract.executableSha256 -cne
        $fingerprint.executable.sha256 -or
    $serializationContract.engineVersion -cne $mapping.engineVersion -or
    $serializationContract.mappingSha256 -cne $mapping.sha256) {
    throw 'Railgun data-asset contract does not match the selected game and mapping.'
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
    # Exact owned generated tree, checked destination remains under this run's scratch.
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
$shotSound = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Railgun_Shot_Blast.wav')).Path
$scopeOverlay = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ScopeOverlay.png')).Path
$chargingStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ChargingStatusIcon.png')).Path
$offlineStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/OfflineStatusIcon.png')).Path
$readyStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ReadyStatusIcon.png')).Path
$ammoIndicator = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/HUD/RailgunAmmoIndicator.png')).Path
$ammoIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Fabricator/RailgunAmmoIcon.png')).Path
$gunIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Fabricator/RailgunIcon.png')).Path
$skillIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Skill/RailgunSkill.png')).Path
$unrealPak = Join-Path $engine 'Binaries/Win64/UnrealPak.exe'
if (-not (Test-Path -LiteralPath $unrealPak -PathType Leaf)) { throw 'UnrealPak is missing.' }
Invoke-NativeStage 'generate-inputs' $editor @($project,'-run=GenerateRailgunInputs','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-inputs-unreal.log')))
$ammoWeightText = $ammoWeightKg.ToString(
    'R', [Globalization.CultureInfo]::InvariantCulture)
Invoke-NativeStage 'generate-runtime' $editor @($project,'-run=GenerateRailgunRuntime','-DedicatedStation',('-AmmoWeightKg=' + $ammoWeightText),('-ShotSound=' + $shotSound),('-ScopeOverlay=' + $scopeOverlay),('-ChargingStatusIcon=' + $chargingStatusIcon),('-OfflineStatusIcon=' + $offlineStatusIcon),('-ReadyStatusIcon=' + $readyStatusIcon),('-AmmoIndicator=' + $ammoIndicator),('-AmmoIcon=' + $ammoIcon),('-GunIcon=' + $gunIcon),('-SkillIcon=' + $skillIcon),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-runtime-unreal.log')))
$packages += @(
    '/Game/Mods/Railgun/Inputs/IA_RailgunLookYaw',
    '/Game/Mods/Railgun/Inputs/IA_RailgunLookPitch',
    '/Game/Mods/Railgun/Inputs/IA_RailgunExit',
    '/Game/Mods/Railgun/Inputs/IA_RailgunZoom',
    '/Game/Mods/Railgun/Inputs/IA_RailgunFire',
    '/Game/Mods/Railgun/Inputs/IA_RailgunReload',
    '/Game/Mods/Railgun/Inputs/IMC_RailgunKeyboard',
    '/Game/Mods/Railgun/Inputs/DA_RailgunInputContext',
    '/Game/Mods/Railgun/Station/BP_RailgunOperator',
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
$cookPackageManifest = Join-Path $output 'cook-packages.txt'
$cookPackageManifestText = ($packages -join "`n") + "`n"
[IO.File]::WriteAllText($cookPackageManifest, $cookPackageManifestText,
    (New-Object System.Text.UTF8Encoding($false)))
$cookPackageManifestSha256 = (Get-FileHash -LiteralPath $cookPackageManifest -Algorithm SHA256).Hash
$cookPackageManifestSha1 = (Get-FileHash -LiteralPath $cookPackageManifest -Algorithm SHA1).Hash
$cookPackageManifestEvidence = [ordered]@{
    schemaVersion = 1
    packageCount = $packages.Count
    sha256 = $cookPackageManifestSha256
    sha1 = $cookPackageManifestSha1
    manifestPath = $cookPackageManifest
}
$cookPackageManifestEvidence | ConvertTo-Json -Depth 3 |
    Set-Content -LiteralPath (Join-Path $output 'cook-packages.manifest.json') -Encoding UTF8
# Keep new material shader code inline in owned packages. Do not change shared
# project config or require a game-global ShaderArchive-Voyage library override.
# Partial native mirrors serialize named property tags, never positional indices.
$cookArguments = @($project,'-run=CookPackageManifest','-RunAsCookCommandlet',('-PackageManifest=' + $cookPackageManifest),('-PackageManifestCount=' + $packages.Count),('-PackageManifestSha1=' + $cookPackageManifestSha1),'-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs','-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False','-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6,D3D11TargetedShaderFormats=PCD3D_SM5','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'cook-unreal.log')))
if (($cookArguments -join ' ').Length -gt 4096) {
    throw 'Cook launcher arguments unexpectedly exceed the bounded manifest-adapter contract.'
}
Invoke-NativeStage 'cook' $editor $cookArguments
Invoke-NativeStage 'verify-tagged' $editor @($project,'-run=GenerateRailgun','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-tagged-unreal.log')))
Invoke-NativeStage 'verify-runtime-tagged' $editor @($project,'-run=GenerateRailgunRuntime','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-runtime-tagged-unreal.log')))
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
$retocManifestPath = Join-Path $repo '.tools/bin/retoc.manifest.json'
if (-not (Test-Path -LiteralPath $retoc -PathType Leaf) -or
    -not (Test-Path -LiteralPath $retocManifestPath -PathType Leaf)) {
    throw 'Canonical retoc or its publication manifest is missing.'
}
$retocManifest = Get-Content -LiteralPath $retocManifestPath -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $retoc -Algorithm SHA256).Hash -cne
    $retocManifest.executableSha256) {
    throw 'Canonical retoc differs from its publication manifest.'
}
$loose = Join-Path $output 'loose'
$assetRelatives = @($packages | ForEach-Object { $_.Replace('/Game/', 'Voyage/Content/') })
$stockHintWidgetRelative =
    $stockHintWidgetPackage.Replace('/Game/', 'Voyage/Content/')
$stockSfxSoundClassRelative =
    $stockSfxSoundClassPackage.Replace('/Game/', 'Voyage/Content/')
if ($assetRelatives -ccontains $stockHintWidgetRelative -or
    $assetRelatives -ccontains $stockSfxSoundClassRelative) {
    throw 'Editor-only stock reference entered the loose release inventory.'
}
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
$skillRelative = 'Voyage/Content/Data/Assets/Skill/Railgun/DA_Skill_Railgun'
$skillPackage = '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun'

if (-not $newPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'Railgun ammo escaped the confirmed Item AssetManager discovery root.'
}
$cloneTarget = Join-Path $loose $cloneRelative
$uassetGui = Join-Path $repo '.tools/bin/UAssetGUI.exe'
$uassetGuiManifestPath = Join-Path $repo '.tools/bin/UAssetGUI.manifest.json'
if (-not (Test-Path -LiteralPath $uassetGui -PathType Leaf) -or
    -not (Test-Path -LiteralPath $uassetGuiManifestPath -PathType Leaf)) {
    throw 'Canonical UAssetGUI or its publication manifest is missing.'
}
$uassetGuiManifest = Get-Content -LiteralPath $uassetGuiManifestPath -Raw | ConvertFrom-Json
if ($uassetGuiManifest.guiCommit -cne $serializationContract.uassetGuiCommit -or
    $uassetGuiManifest.apiCommit -cne $serializationContract.uassetApiCommit -or
    $uassetGuiManifest.executableSha256 -cne
        $serializationContract.uassetGuiSha256 -or
    (Get-FileHash -LiteralPath $uassetGui -Algorithm SHA256).Hash -cne
        $serializationContract.uassetGuiSha256) {
    throw 'Canonical UAssetGUI/API writer differs from the Railgun data-asset contract.'
}
$authoredAmmoDirectory = Join-Path $output 'json-authored-ammo'
$null = New-Item -ItemType Directory -Path $authoredAmmoDirectory
$authoredAmmo = Join-Path $authoredAmmoDirectory 'DA_Ammo_Railgun_FullRod.uasset'
$ammoWrite = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $uassetGui -Arguments @(
        '--portable','fromjson',$ammoItemJsonPath,$authoredAmmo,
        $mapping.mappingsPath,[string]$serializationContract.engineVersion) `
    -OutputRoot $boundedToolRoot -MemoryLimitMB 2048 -TimeoutSeconds 45
$authoredAmmoExport = [IO.Path]::ChangeExtension($authoredAmmo, '.uexp')
if ($ammoWrite.status -cne 'passed' -or
    -not (Test-Path -LiteralPath $authoredAmmo -PathType Leaf) -or
    -not (Test-Path -LiteralPath $authoredAmmoExport -PathType Leaf)) {
    throw 'Direct JSON authoring did not produce the complete Railgun ammo package.'
}
Copy-Item -LiteralPath $authoredAmmo -Destination ($cloneTarget + '.uasset') -Force
Copy-Item -LiteralPath $authoredAmmoExport -Destination ($cloneTarget + '.uexp') -Force
$ammoReadbackPath = Join-Path $output 'ammo-import-readback.json'
$ammoReadback = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $uassetGui -Arguments @(
        '--portable','tojson',($cloneTarget + '.uasset'),$ammoReadbackPath,
        [string]$serializationContract.engineVersion,$mapping.mappingsPath) `
    -OutputRoot $boundedToolRoot -MemoryLimitMB 2048 -TimeoutSeconds 45
if ($ammoReadback.status -cne 'passed' -or
    -not (Test-Path -LiteralPath $ammoReadbackPath -PathType Leaf)) {
    throw 'Railgun ammo import readback failed.'
}
$ammoReadbackData = Get-Content -LiteralPath $ammoReadbackPath -Raw | ConvertFrom-Json
foreach ($field in @('ObjectVersion','ObjectVersionUE5','CustomVersionContainer','NameMap','Imports')) {
    if (($ammoItemSource.$field | ConvertTo-Json -Depth 100 -Compress) -cne
        ($ammoReadbackData.$field | ConvertTo-Json -Depth 100 -Compress)) {
        throw "Direct JSON ammo readback changed $field."
    }
}
if ((ConvertTo-ComparableExportsJson $ammoItemSource.Exports) -cne
        (ConvertTo-ComparableExportsJson $ammoReadbackData.Exports) -or
    $ammoReadbackData.LegacyFileVersion -ne $ammoItemSource.LegacyFileVersion -or
    [bool]$ammoReadbackData.IsUnversioned -ne [bool]$ammoItemSource.IsUnversioned -or
    $ammoReadbackData.FolderName -cne $ammoItemSource.FolderName -or
    @($ammoReadbackData.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count -ne 0) {
    throw 'Direct JSON ammo readback changed the authored payload or package contract.'
}
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
$authoredSkillDirectory = Join-Path $output 'json-authored-skill'
$null = New-Item -ItemType Directory -Path $authoredSkillDirectory
$authoredSkill = Join-Path $authoredSkillDirectory 'DA_Skill_Railgun.uasset'
$skillWrite = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $uassetGui -Arguments @(
        '--portable','fromjson',$skillJsonPath,$authoredSkill,
        $mapping.mappingsPath,[string]$serializationContract.engineVersion) `
    -OutputRoot $boundedToolRoot -MemoryLimitMB 2048 -TimeoutSeconds 45
$authoredSkillExport = [IO.Path]::ChangeExtension($authoredSkill, '.uexp')
if ($skillWrite.status -cne 'passed' -or
    -not (Test-Path -LiteralPath $authoredSkill -PathType Leaf) -or
    -not (Test-Path -LiteralPath $authoredSkillExport -PathType Leaf)) {
    throw 'Direct JSON authoring did not produce the complete Railgun skill package.'
}
Copy-Item -LiteralPath $authoredSkill -Destination ($skillTarget + '.uasset') -Force
Copy-Item -LiteralPath $authoredSkillExport -Destination ($skillTarget + '.uexp') -Force
$skillReadbackPath = Join-Path $output 'skill-import-readback.json'
$skillReadback = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $uassetGui -Arguments @(
        '--portable','tojson',($skillTarget + '.uasset'),$skillReadbackPath,
        [string]$serializationContract.engineVersion,$mapping.mappingsPath) `
    -OutputRoot $boundedToolRoot -MemoryLimitMB 2048 -TimeoutSeconds 45
if ($skillReadback.status -cne 'passed' -or
    -not (Test-Path -LiteralPath $skillReadbackPath -PathType Leaf)) {
    throw 'Railgun skill import readback failed.'
}
$skillReadbackData = Get-Content -LiteralPath $skillReadbackPath -Raw | ConvertFrom-Json
foreach ($field in @('ObjectVersion','ObjectVersionUE5','CustomVersionContainer','NameMap','Imports')) {
    if (($skillSource.$field | ConvertTo-Json -Depth 100 -Compress) -cne
        ($skillReadbackData.$field | ConvertTo-Json -Depth 100 -Compress)) {
        throw "Direct JSON skill readback changed $field."
    }
}
if ((ConvertTo-ComparableExportsJson $skillSource.Exports) -cne
        (ConvertTo-ComparableExportsJson $skillReadbackData.Exports) -or
    $skillReadbackData.LegacyFileVersion -ne $skillSource.LegacyFileVersion -or
    [bool]$skillReadbackData.IsUnversioned -ne [bool]$skillSource.IsUnversioned -or
    $skillReadbackData.FolderName -cne $skillSource.FolderName -or
    @($skillReadbackData.Exports | Where-Object {
        $_.'$type' -like '*RawExport*'
    }).Count -ne 0) {
    throw 'Direct JSON skill readback changed the authored payload or package contract.'
}

# Preserve the complete working four-export item graph from the editable JSON.
$gunRelative = 'Voyage/Content/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$gunTarget = Join-Path $loose $gunRelative
$null = New-Item -ItemType Directory -Path (Split-Path -Parent $gunTarget) -Force
$newGunName = 'DA_Item_Module_RailgunCannonMk01'
$newGunPackage = '/Game/Data/Assets/Modules/' + $newGunName
if (-not $newGunPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) {
    throw 'Railgun item escaped the confirmed Item AssetManager discovery root.'
}
$authoredGunDirectory = Join-Path $output 'json-authored-gun'
$null = New-Item -ItemType Directory -Path $authoredGunDirectory
$authoredGun = Join-Path $authoredGunDirectory ($newGunName + '.uasset')
$gunWrite = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $uassetGui -Arguments @(
        '--portable','fromjson',$gunJsonPath,$authoredGun,
        $mapping.mappingsPath,[string]$serializationContract.engineVersion) `
    -OutputRoot $boundedToolRoot -MemoryLimitMB 2048 -TimeoutSeconds 45
$authoredGunExport = [IO.Path]::ChangeExtension($authoredGun, '.uexp')
if ($gunWrite.status -cne 'passed' -or
    -not (Test-Path -LiteralPath $authoredGun -PathType Leaf) -or
    -not (Test-Path -LiteralPath $authoredGunExport -PathType Leaf)) {
    throw 'Direct JSON authoring did not produce the complete Railgun item package.'
}
Copy-Item -LiteralPath $authoredGun -Destination ($gunTarget + '.uasset') -Force
Copy-Item -LiteralPath $authoredGunExport -Destination ($gunTarget + '.uexp') -Force
$gunReadbackPath = Join-Path $output 'gun-import-readback.json'
$gunReadback = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $uassetGui -Arguments @(
        '--portable','tojson',($gunTarget + '.uasset'),$gunReadbackPath,
        [string]$serializationContract.engineVersion,$mapping.mappingsPath) `
    -OutputRoot $boundedToolRoot -MemoryLimitMB 2048 -TimeoutSeconds 45
if ($gunReadback.status -cne 'passed' -or
    -not (Test-Path -LiteralPath $gunReadbackPath -PathType Leaf)) {
    throw 'Railgun item import readback failed.'
}
$gunReadbackData = Get-Content -LiteralPath $gunReadbackPath -Raw | ConvertFrom-Json
foreach ($field in @('ObjectVersion','ObjectVersionUE5','CustomVersionContainer','NameMap','Imports')) {
    if (($gunSource.$field | ConvertTo-Json -Depth 100 -Compress) -cne
        ($gunReadbackData.$field | ConvertTo-Json -Depth 100 -Compress)) {
        throw "Direct JSON Railgun item readback changed $field."
    }
}
if ((ConvertTo-ComparableExportsJson $gunSource.Exports) -cne
        (ConvertTo-ComparableExportsJson $gunReadbackData.Exports) -or
    $gunReadbackData.LegacyFileVersion -ne $gunSource.LegacyFileVersion -or
    [bool]$gunReadbackData.IsUnversioned -ne [bool]$gunSource.IsUnversioned -or
    $gunReadbackData.FolderName -cne $gunSource.FolderName -or
    @($gunReadbackData.Exports | Where-Object {
        $_.'$type' -like '*RawExport*'
    }).Count -ne 0) {
    throw 'Direct JSON Railgun item readback changed the package contract.'
}
$assetRelatives += $gunRelative
if ($assetRelatives -contains
        'Voyage/Content/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New' -or
    $assetRelatives -contains
        'Voyage/Content/Data/Assets/Skill/Weapons/Ammo/DA_Skill_Railgun_Ammo_Test') {
    throw 'Release inventory still contains a stock Cyclone override or obsolete ammo skill.'
}
$globalUtoc = Join-Path $fingerprint.gameRoot 'Voyage/Content/Paks/global.utoc'
$globalUcas = [IO.Path]::ChangeExtension($globalUtoc, '.ucas')
if (-not (Test-Path -LiteralPath $globalUtoc -PathType Leaf) -or
    -not (Test-Path -LiteralPath $globalUcas -PathType Leaf)) {
    throw 'Current-game global IoStore metadata is missing.'
}
$scriptObjectsInput = @(
    [pscustomobject]@{path=$globalUtoc;sha256=(Get-FileHash -LiteralPath $globalUtoc -Algorithm SHA256).Hash},
    [pscustomobject]@{path=$globalUcas;sha256=(Get-FileHash -LiteralPath $globalUcas -Algorithm SHA256).Hash}
)
$scriptObjectsDirectory = Join-Path $output 'scriptobjects-only'
$null = New-Item -ItemType Directory -Path $scriptObjectsDirectory
$scriptObjectsRun = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $retoc -Arguments @(
        'to-legacy','--version','UE5_8','--no-assets','--no-shaders',
        $globalUtoc,$scriptObjectsDirectory) -OutputRoot $boundedToolRoot `
    -TimeoutSeconds 60 -MemoryLimitMB 1024
$scriptObjectsFiles = @(Get-ChildItem -LiteralPath $scriptObjectsDirectory -File -Recurse)
if ($scriptObjectsRun.status -cne 'passed' -or $scriptObjectsFiles.Count -ne 1 -or
    $scriptObjectsFiles[0].Name -cne 'scriptobjects.bin') {
    throw 'Metadata-only extraction did not produce exactly one scriptobjects.bin.'
}
$scriptObjectsSha256 = (Get-FileHash -LiteralPath $scriptObjectsFiles[0].FullName `
    -Algorithm SHA256).Hash
Copy-Item -LiteralPath $scriptObjectsFiles[0].FullName `
    -Destination (Join-Path $loose 'scriptobjects.bin')
$null = New-Item -ItemType Directory -Path $releaseStaging
$payload = Join-Path $releaseStaging 'payload'
$null = New-Item -ItemType Directory -Path $payload
$stem = 'Railgun'
$container = Join-Path $payload ($stem + '.utoc')
$pack = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
    -Executable $retoc `
    -Arguments @('to-zen',$loose,$container,'--version','UE5_8') `
    -OutputRoot $boundedToolRoot -TimeoutSeconds 60 -MemoryLimitMB 1024
if ($pack.status -ne 'passed') { throw 'Packaging failed.' }
$containerPak = [IO.Path]::ChangeExtension($container, '.pak')
$resolvedContainerPak = [IO.Path]::GetFullPath($containerPak)
if (-not $resolvedContainerPak.StartsWith($output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath $resolvedContainerPak -PathType Leaf)) { throw 'retoc did not create the expected owned PAK.' }
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
Move-Item -LiteralPath $containerPak -Destination (Join-Path $output ($stem + '.empty.pak'))
Move-Item -LiteralPath $registryPak -Destination $containerPak
$expected = Join-Path $output 'expected-packages.txt'
[IO.File]::WriteAllLines($expected, @($assetRelatives | ForEach-Object { $_ + '.uasset' }))
$verify = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') `
    -Container $container -ExpectedPackageList $expected `
    -OutputRoot $containerCheckRoot
if ($verify.status -ne 'passed' -or -not $verify.packageSetMatches) { throw 'Container verification failed.' }
$cloneInspection = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
    $cloneRelative -Source Mod -ModContainer $container `
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
    $pickupRelative -Source Mod -ModContainer $container `
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
    $gunRelative -Source Mod -ModContainer $container `
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
    $skillRelative -Source Mod -ModContainer $container `
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
$semantic = & (Join-Path $PSScriptRoot 'Validate-Railgun.ps1') `
    -Container $container -OutputRoot (Join-Path $output 'semantic') `
    -ModelInventory $inventoryPath
if ($semantic.status -ne 'passed') { throw 'Railgun semantic validation failed.' }
$containerReport = Get-Content -LiteralPath $verify.reportPath -Raw | ConvertFrom-Json
$bulkChunks = @($containerReport.chunkTypes | Where-Object { $_.type -ceq 'BulkData' } | ForEach-Object { $_.count } | Measure-Object -Sum).Sum
if ($null -eq $bulkChunks -or $bulkChunks -lt 1) { throw 'Cooked shot sound bulk data is absent from the container.' }
Copy-Item -LiteralPath $generatedSettingsIni -Destination $payload
$descriptorSource = Join-Path $PSScriptRoot 'Registry/RailgunCatalogue.uplugin'
$descriptorDirectory = Join-Path $payload 'Mods/RailgunCatalogue'
$null = New-Item -ItemType Directory -Path $descriptorDirectory
$descriptor = Join-Path $descriptorDirectory 'RailgunCatalogue.uplugin'
Copy-Item -LiteralPath $descriptorSource -Destination $descriptor
$archivePath = Join-Path $releaseStaging ('Railgun_' + $version + '.zip')
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archiveFiles = @(
    [pscustomobject]@{ source = Join-Path $payload 'Railgun.pak'; entry = 'Content/Paks/Railgun.pak' },
    [pscustomobject]@{ source = Join-Path $payload 'Railgun.ucas'; entry = 'Content/Paks/Railgun.ucas' },
    [pscustomobject]@{ source = Join-Path $payload 'Railgun.utoc'; entry = 'Content/Paks/Railgun.utoc' },
    [pscustomobject]@{ source = Join-Path $payload 'Railgun.ini'; entry = 'Content/Paks/Railgun.ini' },
    [pscustomobject]@{
        source = $descriptor
        entry = 'Mods/RailgunCatalogue/RailgunCatalogue.uplugin'
    }
)
$archiveStream = [IO.File]::Open(
    $archivePath,
    [IO.FileMode]::CreateNew,
    [IO.FileAccess]::ReadWrite,
    [IO.FileShare]::None)
try {
    $archive = [IO.Compression.ZipArchive]::new(
        $archiveStream,
        [IO.Compression.ZipArchiveMode]::Create,
        $false)
    try {
        foreach ($archiveFile in $archiveFiles) {
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive,
                $archiveFile.source,
                $archiveFile.entry,
                [IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    }
    finally {
        $archive.Dispose()
    }
}
finally {
    $archiveStream.Dispose()
}
$archiveReadback = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $expectedArchiveEntries = @($archiveFiles.entry | Sort-Object)
    $actualArchiveEntries = @($archiveReadback.Entries | ForEach-Object {
        $_.FullName.Replace('\', '/')
    } | Sort-Object)
    if (Compare-Object $expectedArchiveEntries $actualArchiveEntries) {
        throw 'Railgun release ZIP does not contain exactly the supported five files.'
    }
    foreach ($archiveFile in $archiveFiles) {
        $entry = @($archiveReadback.Entries | Where-Object {
            $_.FullName.Replace('\', '/') -ceq $archiveFile.entry
        })
        if ($entry.Count -ne 1 -or $entry[0].Length -ne
            (Get-Item -LiteralPath $archiveFile.source).Length) {
            throw "Railgun release ZIP entry size mismatch: $($archiveFile.entry)"
        }
        $entryStream = $entry[0].Open()
        $entryHasher = [Security.Cryptography.SHA256]::Create()
        try {
            $entrySha256 = [BitConverter]::ToString(
                $entryHasher.ComputeHash($entryStream)).Replace('-', '')
        }
        finally {
            $entryHasher.Dispose()
            $entryStream.Dispose()
        }
        if ($entrySha256 -cne
            (Get-FileHash -LiteralPath $archiveFile.source -Algorithm SHA256).Hash) {
            throw "Railgun release ZIP entry hash mismatch: $($archiveFile.entry)"
        }
    }
}
finally {
    $archiveReadback.Dispose()
}
$sourceAfter = @(Get-RailgunSourceStatus)
if (($sourceAfter -join "`n") -cne ($sourceStatus -join "`n")) { throw 'Source status changed during preparation.' }
if ((@(Get-RailgunSourceHashes) | ConvertTo-Json -Compress) -cne ($sourceHashes | ConvertTo-Json -Compress)) { throw 'Source content changed during preparation.' }
if ((& git -C $repo rev-parse HEAD).Trim() -cne $sourceCommit) { throw 'Repository HEAD changed during preparation.' }
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
                sha256 = [string]$_.sha256
            }
        })
    }
    semantic = [ordered]@{
        status = [string]$semanticReport.status
        runtime = [string]$semanticReport.runtime
        containerSha256 = [string]$semanticReport.containerSha256
        assertions = [string]$semanticReport.assertions
    }
    primaryAssetRegistry = [ordered]@{
        sha256 = (Get-FileHash -LiteralPath $pluginRegistry -Algorithm SHA256).Hash
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
    sourceCommit=$sourceCommit;dirtySource=($sourceStatus.Count -gt 0);sourceStatus=$sourceStatus;sourceHashes=$sourceHashes;
    gameEngineVersion='5.8';gameEngineVersionBasis='Reviewed mapping/parser target; game patch version not independently established';editorEngineVersion='5.8.2';retocCompatibilityVersion='UE5_8';retocSha256=$retocManifest.executableSha256;
    gameFingerprint=@{steamBuildId=[string]$fingerprint.steam.buildId;executableSha256=$fingerprint.executable.sha256};
    validation='build and static verification; gameplay validation is a separate gate';runtimeArchitecture='Single Railgun container with independent gun and ammo items, one research skill, plugin-local three-record primary-asset registry, GLB module actor with lifecycle-owned station initialization, operator, inputs, HUD and shot audio';
    mappingSha256=$mapping.sha256;validationSummary='validation-summary.json';
    packagingStatus=$pack.status;containerVerificationStatus=$verify.status;semanticStatus=$semantic.status;
    ammoSourceJson='mods/Railgun/Assets/Fabricator/railgun-ammo-item.json';ammoSourceJsonSha256=(Get-FileHash -LiteralPath $ammoItemJsonPath -Algorithm SHA256).Hash;
    ammoWriterStatus=$ammoWrite.status;ammoReadbackStatus=$ammoReadback.status;
    dataAssetContract='mods/Railgun/Assets/data-assets-contract.json';dataAssetContractSha256=(Get-FileHash -LiteralPath $dataAssetContractPath -Algorithm SHA256).Hash;
    dataAssetSerialization=$serializationContract;
    ammoWriterSha256=$serializationContract.uassetGuiSha256;
    skillSourceJson='mods/Railgun/Assets/Skill/railgun-skill.json';skillSourceJsonSha256=(Get-FileHash -LiteralPath $skillJsonPath -Algorithm SHA256).Hash;
    skillWriterStatus=$skillWrite.status;skillReadbackStatus=$skillReadback.status;
    skillWriterSha256=$serializationContract.uassetGuiSha256;
    gunSourceJson='mods/Railgun/Assets/Fabricator/railgun-item.json';gunSourceJsonSha256=(Get-FileHash -LiteralPath $gunJsonPath -Algorithm SHA256).Hash;
    gunWriterStatus=$gunWrite.status;gunReadbackStatus=$gunReadback.status;
    gunWriterSha256=$serializationContract.uassetGuiSha256;
    scriptObjectsInput=@($scriptObjectsInput | ForEach-Object { [ordered]@{name=[IO.Path]::GetFileName($_.path);sha256=$_.sha256} });scriptObjectsStatus=$scriptObjectsRun.status;
    scriptObjectsSha256=$scriptObjectsSha256;
    primaryAssetRegistrySha256=(Get-FileHash -LiteralPath $pluginRegistry -Algorithm SHA256).Hash;
    primaryAssetRegistryAssetCount=[int]$registryResult.assetCount;
    primaryAssetRegistryPackageCount=[int]$registryResult.packageCount;
    primaryAssetRegistryReopenVerified=[bool]$registryResult.reopenVerified;
    contentPluginDescriptor='mods/Railgun/Registry/RailgunCatalogue.uplugin';
    contentPluginDescriptorSha256=(Get-FileHash -LiteralPath $descriptorSource -Algorithm SHA256).Hash;
}
[IO.File]::WriteAllText(
    (Join-Path $releaseStaging 'build-provenance.json'),
    (($provenance | ConvertTo-Json -Depth 8) + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))
$releaseSourcePaths = @($sourceHashes.path | ForEach-Object {
    [IO.Path]::GetFullPath((Join-Path $repo $_))
})
$release = (& (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') `
    -ReleaseRoot $releaseStaging -Mod Railgun -Version $version -Container $container `
    -Archive $archivePath -SourcePath $releaseSourcePaths `
    -ContentPluginDescriptor $descriptor -AllowDirtySource -AsJson) |
    ConvertFrom-Json
$manifestPath = Join-Path $releaseStaging 'release-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Manifest producer did not publish the candidate.' }
$publicationValidation = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') `
    -ReleaseManifest $manifestPath -ValidateOnly -AllowDirtySource `
    -RetainArchiveOnRestore
if (-not [bool]$publicationValidation.validated) {
    throw 'Prepared Railgun release did not pass installation validation.'
}
$installation = $null
$settingsInstallation = $null
$restoreValidation = $null
$scratchCleaned = $false
if ($Install) {
    $installation = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') `
        -ReleaseManifest $manifestPath -AllowDirtySource -RetainArchiveOnRestore
    $settingsPath = Join-Path $installation.paksDirectory 'Railgun.ini'
    if (-not (Test-Path -LiteralPath $settingsPath)) {
        if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' -ErrorAction SilentlyContinue).Count -gt 0) { throw 'Game started; settings installation refused.' }
        Copy-Item -LiteralPath (Join-Path $payload 'Railgun.ini') -Destination $settingsPath
        $settingsInstallation = [pscustomobject]@{path=$settingsPath;created=$true;addedKeys=@();sha256=(Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash}
    } else {
        $addedKeys = @(Add-MissingRailgunSettings (Join-Path $payload 'Railgun.ini') $settingsPath)
        $settingsInstallation = [pscustomobject]@{path=$settingsPath;created=$false;addedKeys=$addedKeys;sha256=(Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash}
    }
    $restoreValidation = & (Join-Path $repo `
        'tools/Restore-VoyageReleaseInstallation.ps1') `
        -InstallManifest $installation.installManifestPath -ValidateOnly
    if (-not [bool]$restoreValidation.validated -or
        [int]$restoreValidation.retainedCount -ne 1) {
        throw 'Installed Railgun release did not retain its canonical archive in the restore plan.'
    }
    $installedArchivePath = [IO.Path]::GetFullPath($installation.installedArchivePath)
    if ((Get-FileHash -LiteralPath $installedArchivePath -Algorithm SHA256).Hash -cne
        (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash) {
        throw 'Installed Railgun archive hash does not match the prepared release.'
    }
    if (-not $output.StartsWith($tmpBoundary, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFullPath($output) -ceq $tmpOwnerRoot) {
        throw 'Refusing to remove an unowned Railgun scratch path.'
    }
    [IO.Directory]::Delete($output, $true)
    if (Test-Path -LiteralPath $output) {
        throw 'Railgun scratch cleanup did not complete.'
    }
    $scratchCleaned = $true
    $manifestPath = $null
    $archivePath = $installedArchivePath
    $validationSummaryPath = $null
    Write-Host 'Railgun installed successfully; container hashes verified and settings are present.'
}
[pscustomobject]@{
    status = $(if ($Install) { 'installed' } else { 'prepared-not-installed' })
    releaseManifestPath = $manifestPath
    archivePath = $archivePath
    validationSummaryPath = $validationSummaryPath
    verificationReport = $validationSummaryPath
    candidateRoot = $(if ($Install) { $null } else { $output })
    scratchCleaned = $scratchCleaned
    installation = $installation
    settingsInstallation = $settingsInstallation
    restoreValidation = $restoreValidation
} | ConvertTo-Json -Depth 8 -Compress
