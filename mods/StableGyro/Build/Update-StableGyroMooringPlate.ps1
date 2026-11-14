# Internal implementation; invoke the public command at the mod root.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][string]$BlenderPath,
    [string]$Retoc,
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion
)
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    throw 'Use the root Update-StableGyroMooringPlate.ps1 command to acquire the Unreal queue.'
}
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$modRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = [IO.Path]::GetFullPath((Join-Path $modRoot '../..'))
if (-not $Retoc) { $Retoc = Join-Path $repoRoot '.tools/bin/retoc.exe' }
Import-Module (Join-Path $repoRoot 'tools/VoyageModPipeline.psm1') -Force
Import-Module (Join-Path $modRoot 'Build/SourceUpdate.psm1') -Force
$releaseRoot = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot -Kind update-plate
$project = & (Join-Path $modRoot 'New-StableGyroProjectCopy.ps1') -Destination (Join-Path $releaseRoot 'Project')
$projectRoot = Split-Path -Parent $project
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
$expectedStaticMeshObjectPaths = @('/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY.StableGyroMooringPlatePositiveY')
$relativeMesh = 'Content/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY.uasset'
$acceptPaths = @(('Project/' + $relativeMesh), 'Build/MooringMeshes.source.json')
$snapshot = Get-StableGyroSourceSnapshot -ModRoot $modRoot -Paths $acceptPaths
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
$stockExportEvidence = Get-Content -LiteralPath ($stockMooringGlb + '.evidence\export-report.json') -Raw |
    ConvertFrom-Json
$stockNanite = @($stockExportEvidence.meshComponents)[0].Nanite
if ($stockExportEvidence.geometryMode -cne 'Nanite' -or $stockNanite.Pages -ne 4 -or
    $stockNanite.LoadedPages -ne 4 -or $stockNanite.DecodedClusters -ne 223 -or
    $stockNanite.LeafClusters -ne 111 -or $stockNanite.LeafTriangles -ne 13790 -or
    $stockNanite.UvChannels -ne 4 -or $stockNanite.ExactZeroAreaTriangles -ne 114 -or
    $stockNanite.HasTangents) {
    throw 'Stock mooring plate Nanite decode evidence differs from the reviewed source contract.'
}
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
if ($splitEvidence.status -cne 'passed' -or $splitEvidence.crossingTriangles -ne 0 -or
    $splitEvidence.sourceMesh.triangles -ne 13790 -or
    $splitEvidence.sourceMesh.exactZeroAreaTriangles -ne 88 -or
    $splitEvidence.positiveY.triangles -ne 6895 -or
    $splitEvidence.negativeY.triangles -ne 6895 -or
    $splitEvidence.positiveY.materials -ne 1 -or
    $splitEvidence.negativeY.materials -ne 1 -or
    $splitEvidence.rawSourceNormalTrianglesRestored -ne 13790 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.positiveY.triangles -ne 6895 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.positiveY.nonNormalBytesChanged -ne 0 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.negativeY.triangles -ne 6895 -or
    $splitEvidence.exportedRawNormalTrianglesRestored.negativeY.nonNormalBytesChanged -ne 0 -or
    [string]$splitEvidence.materialBindings.positiveY.slot -cne 'Gyrocopter_Seat_p1' -or
    $splitEvidence.materialBindings.positiveY.primitiveMaterialIndex -ne 0 -or
    [string]$splitEvidence.materialBindings.negativeY.slot -cne 'Gyrocopter_Seat_p1' -or
    $splitEvidence.materialBindings.negativeY.primitiveMaterialIndex -ne 0 -or
    $splitEvidence.materialBindings.positiveY.preservedNonJsonChunks -ne $true -or
    $splitEvidence.materialBindings.negativeY.preservedNonJsonChunks -ne $true -or
    $splitEvidence.manualFacesRemoved -ne 0) {
    throw 'Mooring plate split evidence does not match the reviewed stock geometry.'
}
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
            assertions = [ordered]@{
                sourceTriangles = [ordered]@{ exact = 6895 }
                naniteInputTriangles = [ordered]@{ exact = 6852 }
                fallbackTriangles = [ordered]@{ min = 1500; max = 1900 }
            }
        }
    )
}
$staticMeshRequest | ConvertTo-Json -Depth 12 |
    Set-Content -LiteralPath $staticMeshRequestPath -Encoding utf8
& $staticMeshBuilder -Manifest $staticMeshRequestPath -OutputRoot $staticMeshOutputRoot `
    -EngineRoot (Join-Path $EngineRoot 'Engine') -OutputKind Uncooked | Out-Null
$staticMeshBuildResultPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshOutputRoot 'build-result.json') `
    -Label 'Common StaticMesh build result'
$staticMeshOutputManifestPath = Resolve-RequiredPath `
    -Path (Join-Path $staticMeshOutputRoot 'output-manifest.json') `
    -Label 'Common StaticMesh output manifest'
$staticMeshBuildResult = Get-Content -LiteralPath $staticMeshBuildResultPath -Raw | ConvertFrom-Json
if ([string]$staticMeshBuildResult.status -cne 'passed' -or
    @($staticMeshBuildResult.assets).Count -ne 1 -or
    ((@($staticMeshBuildResult.assets.objectPath | Sort-Object) -join "`n") -cne
        (($expectedStaticMeshObjectPaths | Sort-Object) -join "`n")) -or
    @($staticMeshBuildResult.assets | Where-Object {
        $_.sourceTriangles -ne 6895 -or $_.naniteInputTriangles -ne 6852 -or
        $_.fallbackTriangles -lt 1500 -or $_.fallbackTriangles -gt 1900 -or
        $_.uvChannels -ne 4 -or $_.positionPrecision -ne 4 -or
        $_.normalPrecision -ne 8 -or -not $_.supportRayTracing -or
        $_.rayTracingProxyLods -ne 1 -or -not $_.rayTracingUsesRenderingLods -or
        @($_.importedMaterialSlots).Count -ne 1 -or
        [string](@($_.importedMaterialSlots)[0]) -cne 'Gyrocopter_Seat_p1' -or
        @($_.resolvedExternalMaterialReferences).Count -ne 1 -or
        [string](@($_.resolvedExternalMaterialReferences)[0]) -cne
            '/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.Gyrocopter_Seat_p1'
    }).Count -ne 0) {
    throw 'Common StaticMesh build result differs from StableGyro consumer policy.'
}
$phase.Stop()
$timings.StaticMeshBuild = $phase.Elapsed.TotalSeconds


$meshManifest = Get-Content -LiteralPath $staticMeshOutputManifestPath -Raw | ConvertFrom-Json
if ($meshManifest.kind -cne 'Voyage uncooked StaticMesh assets' -or
    @($meshManifest.files).Count -ne 1 -or $meshManifest.files[0].path -cne $relativeMesh) {
    throw 'Unexpected editor mesh output; project was not changed.'
}
$source = Join-Path $staticMeshOutputRoot $relativeMesh
if ((Get-Item -LiteralPath $source).Length -ne $meshManifest.files[0].length) { throw 'Mesh size mismatch.' }
$destination = Join-Path $projectRoot $relativeMesh
Set-StableGyroMooringMeshState -StatePath $meshSourceStatePath -SteamBuildId $expectedBuild -Status pending
try {

    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes($source)) -cne
        [Convert]::ToBase64String([IO.File]::ReadAllBytes($destination))) { throw 'Project mesh copy mismatch.' }

    & (Join-Path $repoRoot 'tools/Prepare-UnrealEditorSupport.ps1') `
        -ProjectFile $project -EngineRoot $EngineRoot `
        -OutputRoot (Join-Path $releaseRoot 'editor-support') | Out-Null
    $cookedRoot = Join-Path $releaseRoot 'cooked'
    & (Join-Path $modRoot 'Build/Cook-StableGyroAssets.ps1') -MooringPlateOnly `
        -UnrealEditor (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
        -Project $project -LocalDataCachePath (Join-Path $releaseRoot 'DerivedDataCache') `
        -OutputRoot $cookedRoot -LogPath (Join-Path $logs 'cook.log')

    # Reuse the clean stock extractor to obtain this game's script-object table.
    $originals = Join-Path $releaseRoot 'originals'
    & (Join-Path $modRoot 'Build/Prepare-StableGyroOriginals.ps1') -GameRoot $GameRoot -Retoc $Retoc -OutputRoot $originals
    $scriptObjects = @(Get-ChildItem -LiteralPath $originals -Recurse -File -Filter 'scriptobjects.bin')
    if ($scriptObjects.Count -ne 1) { throw 'Expected one current stock script-object table.' }
    $stage = Join-Path $releaseRoot 'stage'
    New-Item -ItemType Directory -Path (Join-Path $stage 'Voyage') -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $cookedRoot 'Content') -Destination (Join-Path $stage 'Voyage/Content') -Recurse
    Copy-Item -LiteralPath $scriptObjects[0].FullName -Destination (Join-Path $stage 'scriptobjects.bin')
    $containerRoot = Join-Path $releaseRoot 'container'
    New-Item -ItemType Directory -Path $containerRoot | Out-Null
    $utoc = Join-Path $containerRoot 'StableGyroPlate.utoc'
    & $Retoc to-zen --version UE5_8 $stage $utoc
    if ($LASTEXITCODE -ne 0) { throw 'Plate validation packaging failed.' }
    & $Retoc verify $utoc
    if ($LASTEXITCODE -ne 0) { throw 'Plate validation container is invalid.' }
    $validation = & (Join-Path $modRoot 'Build/Validate-MooringPlateUpdate.ps1') `
        -ContainerUtoc $utoc -OutputRoot $releaseRoot -GameRoot $GameRoot -SourceGlb $positiveMooringGlb
    Set-StableGyroMooringMeshState -StatePath $meshSourceStatePath -SteamBuildId $expectedBuild -Status ready
} catch {
    throw "Plate update failed; maintained source was not changed. Inspect $releaseRoot : $_"
}
Save-StableGyroSourceUpdate -ModRoot $modRoot -RunRoot $releaseRoot -Paths $acceptPaths -Snapshot $snapshot
$result = [ordered]@{
    status = 'updated-and-validated'; steamBuildId = $expectedBuild
    mesh = (Join-Path $modRoot ('Project/' + $relativeMesh)); sourceState = (Join-Path $modRoot 'Build/MooringMeshes.source.json')
    validation = $validation; outputRoot = $releaseRoot; installed = $false
}
$result | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $releaseRoot 'update-result.json') -Encoding utf8
# Retain comparison evidence and logs, discard successful generation/cook scratch.
foreach ($name in @('Project','DerivedDataCache','static-mesh-assets','editor-support','cooked','stage','originals')) {
    $scratch = Join-Path $releaseRoot $name
    if (Test-Path -LiteralPath $scratch) { Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $releaseRoot -Path $scratch }
}
$result | ConvertTo-Json -Depth 6 -Compress
