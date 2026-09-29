[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Material,
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
if ($Material -notmatch '^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$' -or $Material.Trim() -cne $Material) {
    throw 'Supply one exact material package path, not an object suffix, wildcard or fragment.'
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputPath)
$expectedName = $Material.Split('/')[-1] + '.materialpack.zip'
$artifacts = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifacts, [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetFileName($output) -cne $expectedName) {
    throw "Game-derived material pack must be a fresh artifacts/.../$expectedName path."
}
$evidence = $output + '.evidence'
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $evidence)) { throw 'Output/evidence already exists; choose a new output path.' }
if (-not (Test-Path -LiteralPath $BlenderPath -PathType Leaf)) { throw "Blender executable not found: $BlenderPath" }
$bundle = Join-Path $repo '.tools\bin\VoyageMaterialLibrary'
$manifestFile = Join-Path $bundle 'publish-manifest.json'
if (-not (Test-Path -LiteralPath $manifestFile)) { throw 'Publish once with tools/VoyageMaterialLibrary/Publish.ps1; normal export never builds tools.' }
$manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
if ($manifest.kind -cne 'VoyageMaterialLibrary' -or $manifest.conversionCommit -cne 'ec6595e46448a817ac21ea9bde01caa48f80a420') { throw 'Unexpected exporter provenance.' }
foreach ($file in $manifest.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $bundle $file.path) -Algorithm SHA256).Hash -cne $file.sha256) { throw "Exporter bundle drift: $($file.path)" }
}
foreach ($file in $manifest.sources) {
    if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot ('VoyageMaterialLibrary\' + $file.path)) -Algorithm SHA256).Hash -cne $file.sha256) { throw 'Exporter source changed; republish explicitly.' }
}

function Invoke-CapturedProcess([string]$FileName, [string]$Arguments, [string]$LogPath) {
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $FileName
    $start.Arguments = $Arguments
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $exitCode = $process.ExitCode
    $outputText = $stdout.Result
    [IO.File]::WriteAllText($LogPath, ($outputText + [Environment]::NewLine + $stderr.Result))
    $process.Dispose()
    return [pscustomobject]@{ ExitCode = $exitCode; Output = $outputText }
}

[void](New-Item -ItemType Directory -Path $evidence)
$fingerprintPath = Join-Path $evidence 'fingerprint.json'
$fingerprintText = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ($mapping.engineVersion -ne '5.8' -or $mapping.executableSha256 -cne $fingerprint.executable.sha256) { throw 'Unsupported/mismatched game mapping.' }
$stage = Join-Path $evidence 'stage'
$requestPath = Join-Path $evidence 'request.json'
$request = [ordered]@{ operation = 'MaterialPackStage'; output = $stage; materials = @($Material); materialMode = $MaterialMode;
    fingerprintPath = $fingerprintPath; mappingPath = $mapping.mappingsPath; mappingManifestPath = $mapping.manifestPath }
[IO.File]::WriteAllText($requestPath, ($request | ConvertTo-Json -Depth 5))
Copy-Item -LiteralPath $manifestFile -Destination (Join-Path $evidence 'tool-manifest.json')

$export = Invoke-CapturedProcess (Join-Path $bundle 'VoyageMaterialLibrary.exe') ('"' + $requestPath + '"') (Join-Path $evidence 'export.log')
if ($export.ExitCode -ne 0) { throw "Material-pack staging failed (exit $($export.ExitCode)); evidence: $evidence" }
$exportLine = @($export.Output -split '\r?\n' | Where-Object { $_.Trim() })[-1]
$exportResult = $exportLine | ConvertFrom-Json
if ($exportResult.stagePath -cne $stage -or $exportResult.material -cne $Material -or -not (Test-Path -LiteralPath $exportResult.previewSourceGlb)) {
    throw 'Material-pack stage result mismatch.'
}

$previewPath = Join-Path $stage 'preview.webp'
$previewScript = Join-Path $PSScriptRoot 'materialpack\render_material_pack_preview.py'
$renderArguments = '--background --factory-startup --python-exit-code 1 --python "' + $previewScript + '" -- --input "' + $exportResult.previewSourceGlb + '" --output "' + $previewPath + '"'
$render = Invoke-CapturedProcess ([IO.Path]::GetFullPath($BlenderPath)) $renderArguments (Join-Path $evidence 'render.log')
if ($render.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $previewPath)) { throw "Material-pack preview failed (exit $($render.ExitCode)); evidence: $evidence" }
$renderLine = @($render.Output -split '\r?\n' | Where-Object { $_ -like 'MATERIAL_PACK_PREVIEW_OK *' })[-1]
if (-not $renderLine) { throw 'Material-pack preview completion marker missing.' }
$renderResult = $renderLine.Substring('MATERIAL_PACK_PREVIEW_OK '.Length) | ConvertFrom-Json
if (-not $renderResult.sourceUnchanged -or $renderResult.width -ne 768 -or $renderResult.height -ne 768) { throw 'Material-pack preview verification failed.' }

$exporter = Join-Path $bundle 'VoyageMaterialLibrary.exe'
$build = Invoke-CapturedProcess $exporter ('--finalize-material-pack "' + $stage + '" "' + $output + '"') (Join-Path $evidence 'pack.log')
if ($build.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $output)) { throw "Material-pack assembly failed (exit $($build.ExitCode)); evidence: $evidence" }
$verify = Invoke-CapturedProcess $exporter ('--verify-material-pack "' + $output + '"') (Join-Path $evidence 'verify.log')
if ($verify.ExitCode -ne 0) { throw "Material-pack validation failed (exit $($verify.ExitCode)); evidence: $evidence" }
$verified = @($verify.Output -split '\r?\n' | Where-Object { $_.Trim() })[-1] | ConvertFrom-Json
if ($verified.status -cne 'verified' -or $verified.material -cne $Material -or
    (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash -cne $verified.sha256) { throw 'Material-pack validation/readback mismatch.' }

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($output)
try {
    $manifestEntry = $archive.GetEntry('manifest.json')
    $reader = New-Object IO.StreamReader($manifestEntry.Open(), [Text.Encoding]::UTF8)
    try { [IO.File]::WriteAllText((Join-Path $evidence 'materialpack-manifest.json'), $reader.ReadToEnd()) } finally { $reader.Dispose() }
} finally { $archive.Dispose() }
$stageFull = [IO.Path]::GetFullPath($stage)
$evidencePrefix = [IO.Path]::GetFullPath($evidence) + [IO.Path]::DirectorySeparatorChar
if (-not $stageFull.StartsWith($evidencePrefix, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($stageFull) -cne 'stage') {
    throw 'Refusing unsafe stage cleanup.'
}
Remove-Item -LiteralPath $stageFull -Recurse -Force
$result = [ordered]@{ schema = 'voyage.material-pack-export/1'; status = 'exported'; materialMode = $MaterialMode;
    material = $Material; materialPackPath = $output; sha256 = $verified.sha256; bytes = $verified.bytes;
    pbrMaps = @($verified.pbrMaps); sourceTextureCount = $verified.sourceTextures;
    includedSourceTextureCount = $verified.includedSourceTextures; previewRecipe = 'voyage.material-sphere/1';
    blenderVersion = $renderResult.blenderVersion; manifestPath = (Join-Path $evidence 'materialpack-manifest.json');
    reportPath = (Join-Path $evidence 'export-report.json') }
[IO.File]::WriteAllText((Join-Path $evidence 'materialpack-report.json'), ($result | ConvertTo-Json -Depth 5))
$result | ConvertTo-Json -Compress
