# Explicit source refresh; never called by the release build.
[CmdletBinding()]
param(
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$BlenderPath = 'K:/Program Files/Blender Foundation/Blender 5.2/blender.exe',
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion
)
$ErrorActionPreference = 'Stop'
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Update StableGyro mooring plate'
    return
}
Set-StrictMode -Version Latest
$modRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = [IO.Path]::GetFullPath((Join-Path $modRoot '../..'))
Import-Module (Join-Path $repoRoot 'tools/VoyageModPipeline.psm1') -Force
$releaseRoot = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot -Kind update-plate
$project = & (Join-Path $modRoot 'New-StableGyroProjectCopy.ps1') -Destination (Join-Path $releaseRoot 'Project')
function Resolve-RequiredPath {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path -LiteralPath $Path)) { throw "$Label missing: $Path" }
    return (Resolve-Path -LiteralPath $Path).Path
}
$engineVersion = Get-Content (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
if ("$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)" -cne '5.8.2') {
    throw 'StableGyro plate update requires Unreal Engine 5.8.2.'
}
$logs = Join-Path $releaseRoot 'logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$fingerprintPath = Join-Path $releaseRoot 'game-fingerprint.json'
& (Join-Path $repoRoot 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content $fingerprintPath -Raw | ConvertFrom-Json
$expectedBuild = [string]$fingerprint.steam.buildId
& (Join-Path $repoRoot 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot `
    -ActualSteamBuildId $expectedBuild -IgnorePinnedGameVersion:$IgnorePinnedGameVersion | Out-Null
$meshSourceStatePath = Join-Path $releaseRoot 'Build/MooringMeshes.source.json'
$null = New-Item -ItemType Directory -Path (Split-Path -Parent $meshSourceStatePath)
Import-Module (Join-Path $modRoot 'Build/MooringMeshRefresh.psm1') -Force
$modelExporter = Join-Path $repoRoot 'tools/Export-VoyageModelGlb.ps1'
$staticMeshBuilder = Join-Path $repoRoot 'tools/New-VoyageStaticMeshAssets.ps1'
$mooringSplitter = Join-Path $modRoot 'Build/Split-MooringPlateGlb.py'
$relativeMesh = 'Content/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY.uasset'
$acceptPaths = @(('Project/' + $relativeMesh), 'Build/MooringMeshes.source.json')
$snapshot = Get-VoyageModSourceSnapshot -ModRoot $modRoot -Paths $acceptPaths
$phase = [Diagnostics.Stopwatch]::StartNew()
$timings = [ordered]@{}
Write-Host 'Exporting and splitting the current stock mooring plate'
$phase.Restart()
$mooringGeometryRoot = Join-Path $releaseRoot 'mooring-geometry'
New-Item -ItemType Directory -Path $mooringGeometryRoot | Out-Null
$stockMooringGlb = Join-Path $mooringGeometryRoot 'Gyro_MooringPlate.stock.glb'
$positiveMooringGlb = Join-Path $mooringGeometryRoot 'StableGyroMooringPlatePositiveY.glb'
$negativeMooringGlb = Join-Path $mooringGeometryRoot 'StableGyroMooringPlateNegativeY.glb'
$mooringSplitReport = Join-Path $mooringGeometryRoot 'split-report.json'
& $modelExporter `
    -Asset '/Game/AssetSets/Vehicles/Gyrocopter/Mesh/Gyro_MooringPlate' `
    -OutputPath $stockMooringGlb `
    -MaterialMode PbrApproximation `
    -GeometryMode Nanite `
    -GameRoot $GameRoot | Out-Null
$splitLog = Join-Path $logs 'mooring-split.log'
$splitOutput = @(& $BlenderPath --background --factory-startup --python-exit-code 1 `
    --python $mooringSplitter -- `
    --source $stockMooringGlb `
    --positive-y $positiveMooringGlb `
    --negative-y $negativeMooringGlb `
    --report $mooringSplitReport 2>&1)
$splitExitCode = $LASTEXITCODE
$splitOutput | Set-Content -LiteralPath $splitLog -Encoding utf8
if ($splitExitCode -ne 0) {
    $splitOutput | Select-Object -Last 60 | Write-Host
    throw "Mooring plate split failed with exit code $splitExitCode."
}
$splitEvidence = Get-Content -LiteralPath $mooringSplitReport -Raw | ConvertFrom-Json
if ($splitEvidence.status -cne 'passed') { throw "Mooring plate split failed: $mooringSplitReport" }
$phase.Stop()
$timings.MooringGeometry = $phase.Elapsed.TotalSeconds

Write-Host 'Building the common editor plate'
$phase.Restart()
$staticMeshBuilder = Resolve-RequiredPath -Path $staticMeshBuilder -Label 'Voyage StaticMesh builder'
$staticMeshRequestPath = Join-Path $releaseRoot 'static-mesh-request.json'
$staticMeshOutputRoot = Join-Path $releaseRoot 'static-mesh-assets'
$staticMeshRequest = [ordered]@{
    schemaVersion = 1
    assets = @(
        [ordered]@{
            id = 'PositiveY'
            source = $positiveMooringGlb
            destination = [ordered]@{
                packagePath = '/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY'
                objectName = 'StableGyroMooringPlatePositiveY'
            }
            materialMode = 'ExternalReferences'
            materialBindings = @([ordered]@{
                slot = 'Gyrocopter_Seat_p1'
                objectPath = '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.Gyrocopter_Seat_p1'
                classPath = '/Script/Engine.MaterialInstanceConstant'
            })
            import = [ordered]@{
                generateLightmapUVs = $false
                removeDegenerates = $false
                recomputeNormals = $false
                recomputeTangents = $true
                useFullPrecisionUVs = $false
                useHighPrecisionTangentBasis = $false
            }
            nanite = [ordered]@{
                enabled = $true
                explicitTangents = $false
                positionPrecision = 4
                normalPrecision = 8
                keepPercentTriangles = 1.0
                trimRelativeError = 0.0
                generateFallback = 'Enabled'
                fallbackTarget = 'PercentTriangles'
                fallbackPercentTriangles = 0.25
                fallbackRelativeError = 0.0
            }
            rayTracing = [ordered]@{ support = $true }
        }
    )
}
$staticMeshRequest | ConvertTo-Json -Depth 12 |
    Set-Content -LiteralPath $staticMeshRequestPath -Encoding utf8
& $staticMeshBuilder -Manifest $staticMeshRequestPath -OutputRoot $staticMeshOutputRoot `
    -EngineRoot (Join-Path $EngineRoot 'Engine') -OutputKind Uncooked -ProjectFile $project | Out-Null
$staticMeshBuildResultPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshOutputRoot 'build-result.json') `
    -Label 'Common StaticMesh build result'
$phase.Stop()
$timings.StaticMeshBuild = $phase.Elapsed.TotalSeconds

Set-StableGyroMooringMeshState -StatePath $meshSourceStatePath -SteamBuildId $expectedBuild
Save-VoyageModSourceUpdate -ModRoot $modRoot -RunRoot $releaseRoot -Paths $acceptPaths -Snapshot $snapshot
$result = [ordered]@{
    status = 'updated-and-validated'; steamBuildId = $expectedBuild
    mesh = (Join-Path $modRoot ('Project/' + $relativeMesh)); sourceState = (Join-Path $modRoot 'Build/MooringMeshes.source.json')
    validation = $staticMeshBuildResultPath; outputRoot = $releaseRoot; installed = $false
}
$result | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $releaseRoot 'update-result.json') -Encoding utf8
# Retain import evidence and logs; discard the successful disposable project.
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $releaseRoot -Path (Join-Path $releaseRoot 'Project')
$result | ConvertTo-Json -Depth 6 -Compress
