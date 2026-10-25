[CmdletBinding(DefaultParameterSetName = 'Inline')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Inline')][string[]]$Materials,
    [Parameter(Mandatory = $true, ParameterSetName = 'File')][string]$MaterialsFile,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [ValidateSet('PbrApproximation', 'BakeReconstructed')][string]$MaterialMode,
    [string]$BlenderPath = 'K:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ([string]::IsNullOrWhiteSpace($MaterialMode)) {
    throw 'Choose -MaterialMode PbrApproximation or BakeReconstructed. The caller must ask which material representation is wanted when it was not specified.'
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($PSCmdlet.ParameterSetName -eq 'File') {
    $raw = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $MaterialsFile).Path)
    if (-not $raw.TrimStart().StartsWith('[')) { throw 'MaterialsFile must contain a JSON array of exact material package strings.' }
    $items = ConvertFrom-Json -InputObject $raw
    $items = @($items)
    if (@($items | Where-Object { $_ -isnot [string] }).Count -ne 0) { throw 'Material identities must be strings.' }
    $Materials = [string[]]$items
}
if ($Materials.Count -eq 0 -or $Materials.Count -gt 128 -or
    @($Materials | Sort-Object -Unique).Count -ne $Materials.Count -or
    @($Materials | Where-Object { $_ -notmatch '^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$' -or $_.Trim() -cne $_ }).Count -ne 0) {
    throw 'Supply 1..128 unique exact material package paths, not fragments, suffixes or wildcards.'
}
$output = [IO.Path]::GetFullPath($OutputPath)
$artifacts = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifacts, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($output) -ine '.png') {
    throw 'Game-derived preview output must be a fresh .png path below this repository artifacts directory.'
}
$evidence = $output + '.evidence'
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $evidence)) { throw 'Output/evidence already exists; choose a new output path.' }
if (-not (Test-Path -LiteralPath $BlenderPath -PathType Leaf)) { throw "Blender executable not found: $BlenderPath" }
$bundle = Join-Path $repo '.tools\bin\VoyageMaterialLibrary'
$manifestFile = Join-Path $bundle 'publish-manifest.json'
if (-not (Test-Path -LiteralPath $manifestFile)) { throw 'Publish once with tools/VoyageMaterialLibrary/Publish.ps1; normal preview never builds tools.' }
$manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
if ($manifest.kind -cne 'VoyageMaterialLibrary') { throw 'Unexpected exporter interface.' }
foreach ($file in $manifest.files) {
    if (-not (Test-Path -LiteralPath (Join-Path $bundle $file.path) -PathType Leaf)) { throw "Exporter bundle file is missing: $($file.path)" }
}
[void](New-Item -ItemType Directory -Path $evidence)
$fingerprintPath = Join-Path $evidence 'fingerprint.json'
$fingerprintText = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ($mapping.engineVersion -ne '5.8' -or $mapping.steamBuildId -cne $fingerprint.steam.buildId) { throw 'Unsupported/mismatched game mapping.' }
$previewGlb = Join-Path $evidence 'preview-source.glb'
$requestPath = Join-Path $evidence 'request.json'
$request = [ordered]@{ operation = 'MaterialGlb'; output = $previewGlb; materials = @($Materials); materialMode = $MaterialMode;
    sourceArtifactPolicy = 'metadata-only'; fingerprintPath = $fingerprintPath; mappingPath = $mapping.mappingsPath; mappingManifestPath = $mapping.manifestPath }
[IO.File]::WriteAllText($requestPath, ($request | ConvertTo-Json -Depth 5))
Copy-Item -LiteralPath $manifestFile -Destination (Join-Path $evidence 'tool-manifest.json')
$exportLog = Join-Path $evidence 'export.log'
$exportStart = New-Object Diagnostics.ProcessStartInfo
$exportStart.FileName = Join-Path $bundle 'VoyageMaterialLibrary.exe'
$exportStart.Arguments = '"' + $requestPath + '"'
$exportStart.UseShellExecute = $false
$exportStart.CreateNoWindow = $true
$exportStart.RedirectStandardOutput = $true
$exportStart.RedirectStandardError = $true
$exportProcess = [Diagnostics.Process]::Start($exportStart)
$exportStdout = $exportProcess.StandardOutput.ReadToEndAsync()
$exportStderr = $exportProcess.StandardError.ReadToEndAsync()
$exportProcess.WaitForExit()
$exportExitCode = $exportProcess.ExitCode
$exportText = $exportStdout.Result
[IO.File]::WriteAllText($exportLog, ($exportText + [Environment]::NewLine + $exportStderr.Result))
$exportProcess.Dispose()
if ($exportExitCode -ne 0) { throw "Material preview source export failed (exit $exportExitCode); evidence: $exportLog" }
$exportResult = @($exportText -split '\r?\n' | Where-Object { $_.Trim() })[-1] | ConvertFrom-Json
if ($exportResult.glbPath -cne $previewGlb -or -not (Test-Path -LiteralPath $previewGlb -PathType Leaf) -or
    (Get-Item -LiteralPath $previewGlb).Length -lt 1) {
    throw 'Material preview source/readback mismatch.'
}
$renderLog = Join-Path $evidence 'render.log'
$renderStart = New-Object Diagnostics.ProcessStartInfo
$renderStart.FileName = [IO.Path]::GetFullPath($BlenderPath)
$renderScript = Join-Path $PSScriptRoot 'glb\preview_material_glb.py'
$renderStart.Arguments = '--background --factory-startup --python-exit-code 1 --python "' + $renderScript + '" -- --input "' + $previewGlb + '" --output "' + $output + '"'
$renderStart.UseShellExecute = $false
$renderStart.CreateNoWindow = $true
$renderStart.RedirectStandardOutput = $true
$renderStart.RedirectStandardError = $true
$renderProcess = [Diagnostics.Process]::Start($renderStart)
$renderStdout = $renderProcess.StandardOutput.ReadToEndAsync()
$renderStderr = $renderProcess.StandardError.ReadToEndAsync()
$renderProcess.WaitForExit()
$renderExitCode = $renderProcess.ExitCode
$renderText = $renderStdout.Result
[IO.File]::WriteAllText($renderLog, ($renderText + [Environment]::NewLine + $renderStderr.Result))
$renderProcess.Dispose()
if ($renderExitCode -ne 0 -or -not (Test-Path -LiteralPath $output)) { throw "Material preview render failed (exit $renderExitCode); evidence: $renderLog" }
$renderLine = @($renderText -split '\r?\n' | Where-Object { $_ -like 'MATERIAL_PREVIEW_OK *' })[-1]
if (-not $renderLine) { throw 'Blender preview completion marker missing.' }
$renderResult = $renderLine.Substring('MATERIAL_PREVIEW_OK '.Length) | ConvertFrom-Json
$expectedFitAxis = if ($renderResult.columns -ge ($renderResult.rows * $renderResult.tileHeight / [double]$renderResult.tileWidth)) { 'horizontal' } else { 'vertical' }
$expectedOrthoScale = if ($expectedFitAxis -ceq 'horizontal') { [double]$renderResult.columns } else { $renderResult.rows * $renderResult.tileHeight / [double]$renderResult.tileWidth }
if (-not $renderResult.sourceUnchanged -or
    -not $renderResult.isolatedRenders -or $renderResult.columns -lt 1 -or $renderResult.columns -gt 6 -or
    $renderResult.rows -ne [Math]::Ceiling($Materials.Count / [double]$renderResult.columns) -or
    $renderResult.contactSheetFitAxis -cne $expectedFitAxis -or
    [Math]::Abs([double]$renderResult.contactSheetOrthoScale - $expectedOrthoScale) -gt 0.0001) {
    throw 'Blender preview source/layout verification failed.'
}
$png = [IO.File]::ReadAllBytes($output)
if ($png.Length -lt 24) { throw 'Preview PNG is truncated.' }
$width = [Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($png, 16))
$height = [Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($png, 20))
$reportPath = Join-Path $evidence 'preview-report.json'
$result = [ordered]@{ schema = 'voyage.material-preview/1'; status = 'preview'; materialMode = $MaterialMode;
    previewPath = $output; width = $width; height = $height;
    materialCount = $Materials.Count; sourceGlbPath = $previewGlb;
    isolatedRenders = $true; lightingRecipe = $renderResult.lightingRecipe; columns = $renderResult.columns; rows = $renderResult.rows;
    tileWidth = $renderResult.tileWidth; tileHeight = $renderResult.tileHeight; contactSheetFitAxis = $renderResult.contactSheetFitAxis;
    contactSheetOrthoScale = $renderResult.contactSheetOrthoScale;
    sourceArtifactsEmbedded = $false; originalsWritten = $false; exportReportPath = (Join-Path $evidence 'export-report.json'); materials = @($Materials) }
[IO.File]::WriteAllText($reportPath, ($result | ConvertTo-Json -Depth 5))
$result['reportPath'] = $reportPath
$result | ConvertTo-Json -Compress
