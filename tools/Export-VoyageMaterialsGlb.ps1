[CmdletBinding(DefaultParameterSetName = 'Inline')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Inline')][string[]]$Materials,
    [Parameter(Mandatory = $true, ParameterSetName = 'File')][string]$MaterialsFile,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [ValidateSet('PbrApproximation', 'BakeReconstructed')][string]$MaterialMode,
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
if (-not $output.StartsWith($artifacts, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($output) -ine '.glb') {
    throw 'Game-derived GLB output must be a fresh .glb path below this repository artifacts directory.'
}
$evidence = $output + '.evidence'
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $evidence)) { throw 'Output/evidence already exists; choose a new output path.' }
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
[void](New-Item -ItemType Directory -Path $evidence)
$fingerprintPath = Join-Path $evidence 'fingerprint.json'
$fingerprintText = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ($mapping.engineVersion -ne '5.8' -or $mapping.executableSha256 -cne $fingerprint.executable.sha256) { throw 'Unsupported/mismatched game mapping.' }
$requestPath = Join-Path $evidence 'request.json'
$request = [ordered]@{ output = $output; materials = @($Materials); materialMode = $MaterialMode; fingerprintPath = $fingerprintPath;
    mappingPath = $mapping.mappingsPath; mappingManifestPath = $mapping.manifestPath }
[IO.File]::WriteAllText($requestPath, ($request | ConvertTo-Json -Depth 5))
Copy-Item -LiteralPath $manifestFile -Destination (Join-Path $evidence 'tool-manifest.json')
$log = Join-Path $evidence 'export.log'
# Capture native diagnostics without PowerShell 5.1 turning stderr into an unrelated terminating error.
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = Join-Path $bundle 'VoyageMaterialLibrary.exe'
$start.Arguments = '"' + $requestPath + '"'
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
[IO.File]::WriteAllText($log, ($outputText + [Environment]::NewLine + $stderr.Result))
$process.Dispose()
if ($exitCode -ne 0) { throw "Material export failed (exit $exitCode); evidence: $log" }
$lastLine = @($outputText -split '\r?\n' | Where-Object { $_.Trim() })[-1]
$result = $lastLine | ConvertFrom-Json
if ($result.glbPath -cne $output -or (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash -cne $result.sha256) { throw 'Export result/readback mismatch.' }
$lastLine
