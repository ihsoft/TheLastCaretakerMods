[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source = Join-Path $repo '.tools\CUE4Parse'
$expectedCommit = 'c38cf24676514773ac106d7b51640f09f4590c61'
$upstreamBaseCommit = 'ec6595e46448a817ac21ea9bde01caa48f80a420'
$gitSource = $source.Replace('\', '/')
$actualCommit = (& git -c "safe.directory=$gitSource" -C $source rev-parse HEAD) -join ''
if ($LASTEXITCODE -ne 0 -or $actualCommit -cne $expectedCommit) { throw 'CUE4Parse source must match the reviewed commit.' }
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
    $parserManifest.sourceCommit -cne $expectedCommit -or
    $parserManifest.upstreamBaseCommit -cne $upstreamBaseCommit -or
    $parserManifest.sourceRepository -cne 'https://github.com/ihsoft/CUE4Parse.git') {
    throw 'Canonical CUE4Parse fork provenance mismatch.'
}
foreach ($file in $parserManifest.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $parser $file.path) -Algorithm SHA256).Hash -cne $file.sha256) {
        throw "Canonical CUE4Parse drift: $($file.path)"
    }
}
$exporter = Join-Path $repo '.tools\bin\VoyageModelExporter'
$exporterManifestPath = Join-Path $exporter 'publish-manifest.json'
$exporterManifest = Get-Content -LiteralPath $exporterManifestPath -Raw | ConvertFrom-Json
if ($exporterManifest.kind -cne 'VoyageModelExporter' -or $exporterManifest.conversionCommit -cne $expectedCommit) { throw 'Canonical VoyageModelExporter provenance mismatch.' }
foreach ($file in $exporterManifest.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $exporter $file.path) -Algorithm SHA256).Hash -cne $file.sha256) { throw "Canonical VoyageModelExporter drift: $($file.path)" }
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
$afterCommit = (& git -c "safe.directory=$gitSource" -C $source rev-parse HEAD) -join ''
if ($LASTEXITCODE -ne 0 -or $afterCommit -cne $expectedCommit -or @(& git -c "safe.directory=$gitSource" -C $source status --porcelain --untracked-files=no).Count -ne 0) {
    throw 'CUE4Parse source identity changed during publish.'
}
$files = @(Get-ChildItem -LiteralPath $candidate -File -Recurse | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($candidate.Length + 1); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
})
$sources = @($sourceFiles | ForEach-Object {
    $afterHash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    if ($afterHash -cne $sourceHashesBefore[$_.FullName]) { throw "Validator source changed during publish: $($_.FullName)" }
    [ordered]@{ path = $_.FullName.Substring($toolsRoot.Length + 1); sha256 = $afterHash }
})
$manifest = [ordered]@{
    kind = 'VoyageStaticMeshValidator'; cue4ParseSourceCommit = $expectedCommit
    cue4ParseUpstreamBaseCommit = $upstreamBaseCommit
    cue4ParseManifestSha256 = (Get-FileHash -LiteralPath $parserManifestPath -Algorithm SHA256).Hash
    modelExporterManifestSha256 = (Get-FileHash -LiteralPath $exporterManifestPath -Algorithm SHA256).Hash
    files = $files; sources = $sources
}
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
