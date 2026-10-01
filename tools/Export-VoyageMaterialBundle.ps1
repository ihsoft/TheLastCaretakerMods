[CmdletBinding(DefaultParameterSetName = 'Inline')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Inline')][string[]]$Materials,
    [Parameter(Mandatory = $true, ParameterSetName = 'File')][string]$MaterialsFile,
    [Parameter(Mandatory = $true)][string]$BundleName,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [ValidateSet('PbrApproximation', 'BakeReconstructed')][string]$MaterialMode,
    [ValidateSet('AnalysisCompact')][string]$Profile = 'AnalysisCompact',
    [string]$BlenderPath = 'K:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ([string]::IsNullOrWhiteSpace($MaterialMode)) {
    throw 'Choose -MaterialMode PbrApproximation or BakeReconstructed.'
}
if ($PSCmdlet.ParameterSetName -ceq 'File') {
    $loadedMaterials = Get-Content -LiteralPath $MaterialsFile -Raw | ConvertFrom-Json
    $Materials = @()
    foreach ($loadedMaterial in $loadedMaterials) {
        $Materials += [string]$loadedMaterial
    }
}
$Materials = @($Materials)
if ($Materials.Count -lt 1 -or $Materials.Count -gt 128 -or @($Materials | Select-Object -Unique).Count -ne $Materials.Count) {
    throw 'Supply 1..128 unique exact material package paths.'
}
foreach ($materialPath in $Materials) {
    if ($materialPath -notmatch '^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$' -or $materialPath.Trim() -cne $materialPath) {
        throw "Invalid exact material package path: $materialPath"
    }
}
if ($BundleName -notmatch '^[A-Za-z0-9_.-]+$' -or $BundleName -in '.', '..') {
    throw 'BundleName may contain only letters, digits, dot, underscore or hyphen.'
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputPath)
$artifacts = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
$expectedName = $BundleName + '.materialbundle.zip'
if (-not $output.StartsWith($artifacts, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($output) -cne $expectedName) {
    throw "Game-derived material bundle must be a fresh artifacts/.../$expectedName path."
}
$evidence = $output + '.evidence'
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $evidence)) { throw 'Output/evidence already exists; choose a new output path.' }
[void](New-Item -ItemType Directory -Path $evidence)
$packsDirectory = Join-Path $evidence 'packs'
[void](New-Item -ItemType Directory -Path $packsDirectory)
$packPaths = @()
$packResults = @()
foreach ($materialPath in $Materials) {
    $name = $materialPath.Split('/')[-1]
    $packPath = Join-Path $packsDirectory ($name + '.materialpack.zip')
    $packText = (& (Join-Path $PSScriptRoot 'Export-VoyageMaterialPack.ps1') -Material $materialPath -MaterialMode $MaterialMode `
        -Profile $Profile -SourceTextures MetadataOnly -OutputPath $packPath -BlenderPath $BlenderPath -GameRoot $GameRoot) -join [Environment]::NewLine
    $packResult = $packText | ConvertFrom-Json
    if ($packResult.status -cne 'exported' -or $packResult.profile -cne 'AnalysisCompact' -or
        $packResult.sourceTexturePolicy -cne 'MetadataOnly' -or $packResult.includedSourceTextureCount -ne 0) {
        throw "AnalysisCompact material pack failed contract verification: $materialPath"
    }
    $packPaths += $packPath
    $packResults += $packResult
}

$bundleRequest = Join-Path $evidence 'bundle-request.json'
[IO.File]::WriteAllText($bundleRequest, ([ordered]@{ bundleName = $BundleName; outputPath = $output; materialPacks = $packPaths } |
    ConvertTo-Json -Depth 5))
$exporter = Join-Path $repo '.tools\bin\VoyageMaterialLibrary\VoyageMaterialLibrary.exe'
$finalizeText = (& $exporter --finalize-material-bundle $bundleRequest) -join [Environment]::NewLine
if ($LASTEXITCODE -ne 0) { throw "Material-bundle assembly failed; evidence: $evidence" }
$finalized = @($finalizeText -split '\r?\n' | Where-Object { $_.Trim() })[-1] | ConvertFrom-Json
$verifyText = (& $exporter --verify-material-bundle $output) -join [Environment]::NewLine
if ($LASTEXITCODE -ne 0) { throw "Material-bundle validation failed; evidence: $evidence" }
$verified = @($verifyText -split '\r?\n' | Where-Object { $_.Trim() })[-1] | ConvertFrom-Json
if ($verified.status -cne 'verified' -or $verified.bundleName -cne $BundleName -or
    $verified.materials -ne $Materials.Count -or (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash -cne $verified.sha256) {
    throw 'Material-bundle validation/readback mismatch.'
}
[IO.File]::WriteAllText((Join-Path $evidence 'finalize.log'), $finalizeText)
[IO.File]::WriteAllText((Join-Path $evidence 'verify.log'), $verifyText)
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($output)
try {
    $entry = $archive.GetEntry('bundle.json')
    $reader = New-Object IO.StreamReader($entry.Open(), [Text.Encoding]::UTF8)
    try { [IO.File]::WriteAllText((Join-Path $evidence 'bundle.json'), $reader.ReadToEnd()) } finally { $reader.Dispose() }
} finally { $archive.Dispose() }
foreach ($packPath in $packPaths) { Remove-Item -LiteralPath $packPath -Force }
$result = [ordered]@{ schema = 'voyage.material-bundle-export/1'; status = 'exported'; profile = $Profile;
    materialMode = $MaterialMode; sourceTexturePolicy = 'MetadataOnly'; bundleName = $BundleName;
    materialBundlePath = $output; sha256 = $verified.sha256; bytes = $verified.bytes; materialCount = $verified.materials;
    textureCount = $verified.textures; textureReferences = $verified.textureReferences;
    deduplicatedTextureReferences = $verified.deduplicatedTextureReferences; materials = $Materials;
    bundleManifestPath = (Join-Path $evidence 'bundle.json'); reportPath = (Join-Path $evidence 'bundle-report.json') }
[IO.File]::WriteAllText($result.reportPath, ($result | ConvertTo-Json -Depth 5))
$result | ConvertTo-Json -Compress
