[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Asset,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($Asset -notmatch '^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$' -or $Asset.Trim() -cne $Asset) {
    throw 'Asset must be one exact virtual StaticMesh or Blueprint package path, without .uasset, object suffix, wildcard or traversal.'
}
$output = [IO.Path]::GetFullPath($OutputPath)
$artifacts = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifacts, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($output) -ine '.glb') {
    throw 'Game-derived GLB output must be a fresh .glb path below this repository artifacts directory.'
}
$evidence = $output + '.evidence'
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $evidence)) { throw 'Output/evidence already exists; choose a new output path.' }
$bundle = Join-Path $repo '.tools\bin\VoyageModelExporter'
$manifestFile = Join-Path $bundle 'publish-manifest.json'
if (-not (Test-Path -LiteralPath $manifestFile)) { throw 'Publish once with tools/VoyageModelExporter/Publish.ps1; normal export never builds tools.' }
$manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
if ($manifest.kind -cne 'VoyageModelExporter' -or $manifest.conversionCommit -cne 'ec6595e46448a817ac21ea9bde01caa48f80a420') {
    throw 'Unexpected model exporter provenance.'
}
foreach ($file in $manifest.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $bundle $file.path) -Algorithm SHA256).Hash -cne $file.sha256) { throw "Exporter bundle drift: $($file.path)" }
}
foreach ($file in $manifest.sources) {
    if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot ('VoyageModelExporter\' + $file.path)) -Algorithm SHA256).Hash -cne $file.sha256) {
        throw 'Exporter source changed; republish explicitly.'
    }
}
[void](New-Item -ItemType Directory -Path $evidence)
$fingerprintPath = Join-Path $evidence 'fingerprint.json'
$fingerprintText = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ($mapping.engineVersion -ne '5.8' -or $mapping.executableSha256 -cne $fingerprint.executable.sha256) { throw 'Unsupported/mismatched game mapping.' }
$requestPath = Join-Path $evidence 'request.json'
$request = [ordered]@{
    output = $output
    asset = $Asset
    fingerprintPath = $fingerprintPath
    mappingPath = $mapping.mappingsPath
    mappingManifestPath = $mapping.manifestPath
}
[IO.File]::WriteAllText($requestPath, ($request | ConvertTo-Json -Depth 5))
Copy-Item -LiteralPath $manifestFile -Destination (Join-Path $evidence 'tool-manifest.json')
$log = Join-Path $evidence 'export.log'
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = Join-Path $bundle 'VoyageModelExporter.exe'
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
if ($exitCode -ne 0) { throw "Model export failed (exit $exitCode); evidence: $log" }
$lastLine = @($outputText -split '\r?\n' | Where-Object { $_.Trim() })[-1]
$result = $lastLine | ConvertFrom-Json
if ($result.glbPath -cne $output -or (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash -cne $result.sha256) {
    throw 'Export result/readback mismatch.'
}
$lastLine
