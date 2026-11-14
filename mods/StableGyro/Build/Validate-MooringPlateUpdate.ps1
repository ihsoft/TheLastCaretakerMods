# INTERNAL UPDATE STAGE: validate the freshly extracted and imported plate.
# Called only by Update-StableGyroMooringPlate.ps1, never by ordinary builds.
param(
    [Parameter(Mandatory)][string]$ContainerUtoc,
    [Parameter(Mandatory)][string]$OutputRoot,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][ValidateNotNullOrEmpty()][string]$SourceGlb
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$releaseRoot = $OutputRoot
$staticMeshValidator = Join-Path $repoRoot 'tools/Test-VoyageStaticMesh.ps1'
$assetJsonReader = Join-Path $repoRoot 'tools/Get-VoyageAssetJson.ps1'
function Resolve-RequiredPath {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "$Label missing: $Path" }
    return (Resolve-Path -LiteralPath $Path).Path
}
$staticMeshValidationRequestPath = Join-Path $releaseRoot 'static-mesh-validation-request.json'
$staticMeshValidationOutput = Join-Path $releaseRoot 'static-mesh-validation'
$staticMeshValidationAssets = @()
foreach ($side in @('PositiveY')) {
    $assetName = "StableGyroMooringPlate$side"
    $staticMeshValidationAssets += [ordered]@{
        id = $side
        package = "/Game/Mods/StableGyro/$assetName/StaticMeshes/$assetName"
        decodeNanite = $true
        expectations = @(
            [ordered]@{ fact = '/nanite/present'; equals = $true },
            [ordered]@{ fact = '/nanite/decodedTriangles'; equals = 6852 },
            [ordered]@{ fact = '/nanite/inputTriangles'; equals = 6852 },
            [ordered]@{ fact = '/nanite/positionPrecision'; equals = 4 },
            [ordered]@{ fact = '/nanite/normalPrecision'; equals = 8 },
            [ordered]@{ fact = '/renderingLods/0/triangles'; min = 1500 },
            [ordered]@{ fact = '/renderingLods/0/triangles'; max = 1900 },
            [ordered]@{ fact = '/renderingLods/0/uvChannels'; equals = 4 },
            [ordered]@{ fact = '/rayTracing/hasProxyFlag'; equals = $true },
            [ordered]@{ fact = '/rayTracing/proxyPresent'; equals = $true },
            [ordered]@{ fact = '/rayTracing/usingRenderingLods'; equals = $true },
            [ordered]@{ fact = '/rayTracing/proxyLodCount'; equals = 1 }
        )
    }
}
$comparison = [ordered]@{
    path = $SourceGlb
    positionMatchToleranceMeters = 0.00033
    attributes = [ordered]@{
        POSITION = 0.00033
        NORMAL = 0.012
        TEXCOORD_0 = 0.0002
        TEXCOORD_1 = 0.0002
        TEXCOORD_2 = 0.0002
        TEXCOORD_3 = 0.0002
        COLOR_0 = (0.5 / 255.0)
    }
    topology = [ordered]@{
        maxExtraFaces = 0
        maxMissingNondegenerateFaces = 0
        maxMissingFaces = 43
    }
}
$staticMeshValidationAssets[0].sourceGlb = $comparison
$staticMeshValidationRequest = [ordered]@{
    schemaVersion = 1
    source = [ordered]@{ kind = 'utoc'; utoc = $containerUtoc }
    assets = $staticMeshValidationAssets
}
$staticMeshValidationRequest | ConvertTo-Json -Depth 12 |
    Set-Content -LiteralPath $staticMeshValidationRequestPath -Encoding utf8
& $staticMeshValidator -RequestPath $staticMeshValidationRequestPath `
    -OutputDirectory $staticMeshValidationOutput -GameRoot $GameRoot | Out-Null
$staticMeshInspectionPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshValidationOutput 'inspection.json') `
    -Label 'Final-container StaticMesh inspection'
$staticMeshValidationPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshValidationOutput 'validation.json') `
    -Label 'Final-container StaticMesh validation'
$staticMeshValidationResult = Get-Content -LiteralPath $staticMeshValidationPath -Raw |
    ConvertFrom-Json
if ([string]$staticMeshValidationResult.status -cne 'passed') {
    throw 'Final-container StaticMesh validation did not report passed.'
}

$materialInspectionRoot = Join-Path $releaseRoot 'final-container-material-inspection'
$materialValidation = [ordered]@{}
foreach ($side in @('PositiveY')) {
    $assetName = "StableGyroMooringPlate$side"
    $virtualPath = "Voyage/Content/Mods/StableGyro/$assetName/StaticMeshes/$assetName.uasset"
    $inspectionLine = @(& pwsh -NoProfile `
        -File $assetJsonReader $virtualPath -AsJson -Source Mod `
        -ModContainer $containerUtoc -ModInspectionRoot $materialInspectionRoot `
        -GameRoot $GameRoot) |
        Select-Object -Last 1
    if ($LASTEXITCODE -ne 0 -or @($inspectionLine).Count -ne 1) {
        throw "Final-container material inspection failed for $assetName."
    }
    $inspection = $inspectionLine | ConvertFrom-Json
    $assetJson = Get-Content -LiteralPath ([string]$inspection.jsonPath) -Raw | ConvertFrom-Json
    $meshExports = @($assetJson | Where-Object {
        [string]$_.Type -ceq 'StaticMesh' -and [string]$_.Name -ceq $assetName
    })
    if ($meshExports.Count -ne 1) {
        throw "Expected one final-container StaticMesh export for $assetName."
    }
    $mesh = $meshExports[0]
    $materials = @($mesh.Properties.StaticMaterials)
    if ($materials.Count -ne 1 -or
        [string]$materials[0].MaterialSlotName -cne 'Gyrocopter_Seat_p1' -or
        [string]$materials[0].ImportedMaterialSlotName -cne 'Gyrocopter_Seat_p1' -or
        [string]$materials[0].MaterialInterface.ObjectName -cne
            "MaterialInstanceConstant'Gyrocopter_Seat_p1'" -or
        -not ([string]$materials[0].MaterialInterface.ObjectPath).StartsWith(
            '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.',
            [StringComparison]::Ordinal)) {
        throw "Final-container material binding contract failed for $assetName."
    }
    $materialValidation[$side] = [ordered]@{
        virtualPath = $virtualPath
        jsonPath = [string]$inspection.jsonPath
        materialObjectName = [string]$materials[0].MaterialInterface.ObjectName
        materialObjectPath = [string]$materials[0].MaterialInterface.ObjectPath
        materialSlotName = [string]$materials[0].MaterialSlotName
        importedMaterialSlotName = [string]$materials[0].ImportedMaterialSlotName
    }
}
$materialValidationPath = Join-Path $releaseRoot 'final-container-material-validation.json'
$materialValidation | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $materialValidationPath -Encoding utf8

[pscustomobject]@{
    request = $staticMeshValidationRequestPath
    inspection = $staticMeshInspectionPath
    validation = $staticMeshValidationPath
    materialValidation = $materialValidationPath
    comparedSourceGeometry = $true
}
