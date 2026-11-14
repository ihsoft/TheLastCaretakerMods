# INTERNAL BUILD STAGE: package validated project-cooked assets.
# The stock extraction supplies the script-object table for packaging.

param(
    [Parameter(Mandatory = $true)][string]$CookedRoot,
    [Parameter(Mandatory = $true)][string]$ProjectRoot,

    [Parameter(Mandatory = $true)]
    [string]$ScriptObjects,

    [string]$Retoc,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'StableGyro_P',

    [Parameter(Mandatory = $true)][string]$ActualSteamBuildId,

    [Parameter(Mandatory = $true)][string]$MappingsPath
)

$ErrorActionPreference = 'Stop'

$retocCompatibilityVersion = 'UE5_8'
$expectedExtractionFilter = 'Data/Assets/Modules/DA_Item_Module_GyroCopter'
$modRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}

$cooked = (Resolve-Path -LiteralPath $CookedRoot).Path
$scriptObjectsPath = (Resolve-Path -LiteralPath $ScriptObjects).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

function Get-ExtractionEvidence {
    param(
        [Parameter(Mandatory = $true)] [string]$InputPath,
        [Parameter(Mandatory = $true)] [string]$ExpectedFilter,
        [Parameter(Mandatory = $true)] [string]$Label
    )

    $resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
    $cursor = if (Test-Path -LiteralPath $resolvedInput -PathType Leaf) {
        Split-Path -Parent $resolvedInput
    } else {
        $resolvedInput
    }
    $manifestPath = $null
    while ($cursor) {
        $candidate = Join-Path $cursor 'extraction-manifest.json'
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $manifestPath = $candidate
            break
        }
        $parent = Split-Path -Parent $cursor
        if (-not $parent -or $parent -eq $cursor) {
            break
        }
        $cursor = $parent
    }
    if (-not $manifestPath) {
        throw "$Label is not inside an extraction root with extraction-manifest.json: $resolvedInput"
    }

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if (-not ($manifest.PSObject.Properties.Name -contains 'allowAdditionalContainers') -or
        $manifest.allowAdditionalContainers -ne $false) {
        throw "$Label extraction was not proven free of additional shadowing containers: $manifestPath"
    }
    if ([string]$manifest.filter -cne $ExpectedFilter) {
        throw "$Label extraction used filter '$($manifest.filter)'; expected '$ExpectedFilter'."
    }
    if ([string]$manifest.retocEngineVersion -cne $retocCompatibilityVersion) {
        throw "$Label extraction does not match the selected retoc compatibility contract."
    }
    if ([string]$manifest.steamBuildId -cne $ActualSteamBuildId) {
        throw "$Label extraction fingerprint does not match the installed game."
    }

    return [pscustomobject]@{
        Root = Split-Path -Parent $manifestPath
        ManifestPath = $manifestPath
        Manifest = $manifest
    }
}

$scriptObjectsEvidence = Get-ExtractionEvidence `
    -InputPath $scriptObjectsPath `
    -ExpectedFilter $expectedExtractionFilter `
    -Label 'scriptobjects.bin'

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
$itemStage = Join-Path $stage 'Voyage\Content\Data\Assets\Modules'
$childStage = Join-Path $stage 'Voyage\Content\Mods\StableGyroCtrl'
$helperStage = Join-Path $stage 'Voyage\Content\Mods\StableGyro'
$contextStage = Join-Path $stage 'Voyage\Content\Game\Input\Vehicle'
New-Item -ItemType Directory -Path $itemStage, $childStage, $helperStage, $contextStage, $package -Force | Out-Null

$childSource = Join-Path $cooked 'Content\Mods\StableGyroCtrl'
$helperSource = Join-Path $cooked 'Content\Mods\StableGyro'
foreach ($assetName in @('BP_GyroCopter_StablePilot.uasset', 'BP_GyroCopter_StablePilot.uexp')) {
    Copy-Item -LiteralPath (Join-Path $childSource $assetName) -Destination $childStage
}
foreach ($assetName in @(
    'ModActor.uasset',
    'ModActor.uexp',
    'PitchInputActor.uasset',
    'PitchInputActor.uexp'
)) {
    Copy-Item -LiteralPath (Join-Path $helperSource $assetName) -Destination $helperStage
}
$settingsStage = Join-Path $helperStage 'Settings'
$hudStage = Join-Path $helperStage 'UI'
New-Item -ItemType Directory -Path $hudStage -Force | Out-Null
foreach ($extension in @('.uasset', '.uexp')) {
    Copy-Item -LiteralPath (Join-Path $helperSource ('UI\WBP_StableGyroCompassHUD' + $extension)) -Destination $hudStage
    Copy-Item -LiteralPath (Join-Path $helperSource ('UI\WBP_StableGyroCameraCompass' + $extension)) -Destination $hudStage
    Copy-Item -LiteralPath (Join-Path $helperSource ('UI\WBP_StableGyroCompassDisplay' + $extension)) -Destination $hudStage
}
New-Item -ItemType Directory -Path $settingsStage -Force | Out-Null
foreach ($extension in @('.uasset', '.uexp')) {
    Copy-Item -LiteralPath (Join-Path $helperSource ('Settings\BP_IniFileParser' + $extension)) -Destination $settingsStage
}
foreach ($meshName in @('StableGyroMooringPlatePositiveY')) {
    $meshSource = Join-Path $helperSource ($meshName + '\StaticMeshes')
    $meshStage = Join-Path $helperStage ($meshName + '\StaticMeshes')
    New-Item -ItemType Directory -Path $meshStage -Force | Out-Null
    $meshFiles = @(Get-ChildItem -LiteralPath $meshSource -File)
    foreach ($requiredExtension in @('.uasset', '.uexp')) {
        $requiredName = $meshName + $requiredExtension
        if (@($meshFiles | Where-Object { $_.Name -ceq $requiredName }).Count -ne 1) {
            throw "Expected one common-builder companion named $requiredName in $meshSource."
        }
    }
    if ($meshFiles.Count -lt 2 -or
        @($meshFiles | Where-Object { -not $_.Name.StartsWith($meshName + '.', [StringComparison]::Ordinal) }).Count -ne 0) {
        throw "Unexpected common-builder mesh companion set in $meshSource."
    }
    foreach ($source in $meshFiles) {
        Copy-Item -LiteralPath $source.FullName -Destination $meshStage
    }
}
$validatorArguments = @(
    '-NoProfile',
    '-File', (Join-Path $PSScriptRoot 'Validate-StableGyroMooringTemplates.ps1'),
    '-RepositoryRoot', $repoRoot,
    '-AssetPath', (Join-Path $childStage 'BP_GyroCopter_StablePilot.uasset'),
    '-GeneratedParentAssetPath', (Join-Path $ProjectRoot 'Content\Blueprints\Vehicles\BP_GyroCopter_Possessable.uasset'),
    '-PitchHelperAssetPath', (Join-Path $helperStage 'PitchInputActor.uasset'),
    '-StabilizationHelperAssetPath', (Join-Path $helperStage 'ModActor.uasset'),
    '-MappingsPath', $MappingsPath,
    '-EvidencePath', (Join-Path $output 'mooring-template-validation.json')
)
& pwsh @validatorArguments | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "StableGyro mooring template validation failed with exit code $LASTEXITCODE."
}
foreach ($assetName in @('IAV_GyroPitchReset.uasset', 'IAV_GyroPitchReset.uexp')) {
    Copy-Item -LiteralPath (Join-Path $helperSource $assetName) -Destination $helperStage
}
$contextSource = Join-Path $cooked 'Content\Game\Input\Vehicle'
foreach ($assetName in @('IMC_GyroCopter_Keyboard.uasset', 'IMC_GyroCopter_Keyboard.uexp')) {
    Copy-Item -LiteralPath (Join-Path $contextSource $assetName) -Destination $contextStage
}
$itemSource = Join-Path $cooked 'Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset'
foreach ($extension in @('.uasset','.uexp')) {
    Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($itemSource,$extension)) -Destination $itemStage
}
Copy-Item -LiteralPath $scriptObjectsPath -Destination (Join-Path $stage 'scriptobjects.bin')

$utoc = Join-Path $package ($ContainerName + '.utoc')
& $retocPath to-zen --version $retocCompatibilityVersion $stage $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc verify failed with exit code $LASTEXITCODE"
}
$expectedAssetPaths = @(
    '../../../Voyage/Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset'
    '../../../Voyage/Content/Game/Input/Vehicle/IMC_GyroCopter_Keyboard.uasset'
    '../../../Voyage/Content/Mods/StableGyro/IAV_GyroPitchReset.uasset'
    '../../../Voyage/Content/Mods/StableGyro/ModActor.uasset'
    '../../../Voyage/Content/Mods/StableGyro/PitchInputActor.uasset'
    '../../../Voyage/Content/Mods/StableGyro/Settings/BP_IniFileParser.uasset'
    '../../../Voyage/Content/Mods/StableGyro/UI/WBP_StableGyroCompassHUD.uasset'
    '../../../Voyage/Content/Mods/StableGyro/UI/WBP_StableGyroCameraCompass.uasset'
    '../../../Voyage/Content/Mods/StableGyro/UI/WBP_StableGyroCompassDisplay.uasset'
    '../../../Voyage/Content/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY.uasset'
    '../../../Voyage/Content/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot.uasset'
)
$expectedList = Join-Path $output 'expected-packages.txt'
$expectedAssetPaths | Set-Content -LiteralPath $expectedList -Encoding ASCII
& (Join-Path $repoRoot 'tools/Test-VoyageContainer.ps1') -Container $utoc `
    -ExpectedPackageList $expectedList -Retoc $retocPath `
    -OutputRoot (Join-Path $output 'container-check') | Out-Null

$outputs = @(
    Join-Path $package ($ContainerName + '.pak')
    Join-Path $package ($ContainerName + '.ucas')
    $utoc
)
foreach ($path in $outputs) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Expected package file was not produced: $path"
    }
}

Write-Host 'Autonomous StableGyro control package built successfully.'
$outputs | ForEach-Object { Get-Item -LiteralPath $_ } |
    Select-Object FullName, Length | Format-Table -AutoSize
