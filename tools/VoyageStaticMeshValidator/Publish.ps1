[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source = Join-Path $repo '.tools\CUE4Parse'
$gitSource = $source.Replace('\', '/')
if (@(& git -c "safe.directory=$gitSource" -C $source status --porcelain --untracked-files=no).Count -ne 0 -or $LASTEXITCODE -ne 0) {
    throw 'Refusing a dirty CUE4Parse source dependency.'
}
$run = Join-Path $repo ('Tmp\tools\VoyageStaticMeshValidator\publish-' + [Guid]::NewGuid().ToString('N'))
$candidate = Join-Path $run 'bundle'
$log = Join-Path $run 'publish.log'
$parser = Join-Path $repo '.tools\bin\CUE4Parse'
$parserManifestPath = Join-Path $parser 'publish-manifest.json'
$parserManifest = Get-Content -LiteralPath $parserManifestPath -Raw | ConvertFrom-Json
if ($parserManifest.kind -cne 'Voyage canonical CUE4Parse bundle' -or
    $parserManifest.sourceRepository -cne 'https://github.com/ihsoft/CUE4Parse.git') {
    throw 'Canonical CUE4Parse fork provenance mismatch.'
}
foreach ($file in $parserManifest.files) {
    if (-not (Test-Path -LiteralPath (Join-Path $parser $file.path) -PathType Leaf)) {
        throw "Canonical CUE4Parse file is missing: $($file.path)"
    }
}
$exporter = Join-Path $repo '.tools\bin\VoyageModelExporter'
$exporterManifestPath = Join-Path $exporter 'publish-manifest.json'
$exporterManifest = Get-Content -LiteralPath $exporterManifestPath -Raw | ConvertFrom-Json
if ($exporterManifest.kind -cne 'VoyageModelExporter') { throw 'Canonical VoyageModelExporter manifest is invalid.' }
foreach ($file in $exporterManifest.files) {
    if (-not (Test-Path -LiteralPath (Join-Path $exporter $file.path) -PathType Leaf)) { throw "Canonical VoyageModelExporter file is missing: $($file.path)" }
}
$toolsRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceFiles = @(Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object {
    $_.Extension -in '.cs','.csproj','.json'
}) + @(Get-Item -LiteralPath (Join-Path $toolsRoot 'Test-VoyageStaticMesh.ps1'))
$sourceHashesBefore = @{}
foreach ($file in $sourceFiles) { $sourceHashesBefore[$file.FullName] = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash }
[void](New-Item -ItemType Directory -Path $run)
$runRoot = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp\tools\VoyageStaticMeshValidator')) + [IO.Path]::DirectorySeparatorChar
if (-not ([IO.Path]::GetFullPath($run) + [IO.Path]::DirectorySeparatorChar).StartsWith($runRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe publish workspace path.' }
& dotnet publish (Join-Path $PSScriptRoot 'VoyageStaticMeshValidator.csproj') -c Release -o $candidate `
    "-p:CanonicalCue4ParseDir=$parser" "-p:CanonicalExporterDir=$exporter" -p:NuGetAudit=false --nologo -v quiet *> $log
if ($LASTEXITCODE -ne 0) { throw "Validator publish failed: $log" }
if ((Get-FileHash -LiteralPath (Join-Path $candidate 'CUE4Parse.dll') -Algorithm SHA256).Hash -cne
    (Get-FileHash -LiteralPath (Join-Path $parser 'CUE4Parse.dll') -Algorithm SHA256).Hash) {
    throw 'Validator parser copy differs from the canonical CUE4Parse bundle.'
}
$selfTest = (& (Join-Path $candidate 'VoyageStaticMeshValidator.exe') --self-test) -join [Environment]::NewLine
if ($LASTEXITCODE -ne 0 -or ($selfTest | ConvertFrom-Json).retainedRayTracingFields -ne 3) { throw 'Private CUE4Parse runtime field smoke check failed.' }
if (@(& git -c "safe.directory=$gitSource" -C $source status --porcelain --untracked-files=no).Count -ne 0) {
    throw 'CUE4Parse source changed during publish.'
}
$files = @(Get-ChildItem -LiteralPath $candidate -File -Recurse | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($candidate.Length + 1); length = $_.Length }
})
$sources = @($sourceFiles | ForEach-Object {
    $afterHash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    if ($afterHash -cne $sourceHashesBefore[$_.FullName]) { throw "Validator source changed during publish: $($_.FullName)" }
    [ordered]@{ path = $_.FullName.Substring($toolsRoot.Length + 1); length = $_.Length }
})
$manifest = [ordered]@{ kind = 'VoyageStaticMeshValidator'; files = $files; sources = $sources; publishedAtUtc = [DateTime]::UtcNow.ToString('o') }
[IO.File]::WriteAllText((Join-Path $candidate 'publish-manifest.json'), ($manifest | ConvertTo-Json -Depth 6))
$destination = Join-Path $repo '.tools\bin\VoyageStaticMeshValidator'
$backup = $destination + '.old-' + [Guid]::NewGuid().ToString('N')
$binRoot = [IO.Path]::GetFullPath((Join-Path $repo '.tools\bin')) + [IO.Path]::DirectorySeparatorChar
foreach ($path in @($candidate, $destination, $backup)) {
    $full = [IO.Path]::GetFullPath($path)
    $allowed = $full.StartsWith($runRoot, [StringComparison]::OrdinalIgnoreCase) -or $full.StartsWith($binRoot, [StringComparison]::OrdinalIgnoreCase)
    if (-not $allowed) { throw "Unsafe publish move/remove path: $full" }
}
if (Test-Path -LiteralPath $destination) { Move-Item -LiteralPath $destination -Destination $backup }
try { Move-Item -LiteralPath $candidate -Destination $destination }
catch { if (Test-Path -LiteralPath $backup) { Move-Item -LiteralPath $backup -Destination $destination }; throw }
if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup -Recurse -Force }
[ordered]@{ status = 'published'; binary = (Join-Path $destination 'VoyageStaticMeshValidator.exe'); manifest = (Join-Path $destination 'publish-manifest.json'); logPath = $log } | ConvertTo-Json -Compress
