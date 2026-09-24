[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source = Join-Path $repo '.tools\CUE4Parse'
$gitSource = $source.Replace('\', '/')
$expectedCommit = 'ec6595e46448a817ac21ea9bde01caa48f80a420'
$sourceCommit = & git -c "safe.directory=$gitSource" -C $source rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or [string]$sourceCommit -cne $expectedCommit) { throw 'CUE4Parse conversion checkout must match the reviewed parser commit.' }
if (@(& git -c "safe.directory=$gitSource" -C $source status --porcelain --untracked-files=no).Count -ne 0 -or $LASTEXITCODE -ne 0) {
    throw 'Refusing a dirty conversion dependency.'
}
$parser = Join-Path $repo '.tools\bin\CUE4Parse'
$canonical = Get-Content -LiteralPath (Join-Path $parser 'publish-manifest.json') -Raw | ConvertFrom-Json
if ($canonical.sourceCommit -cne $expectedCommit) { throw 'Canonical parser commit mismatch.' }
foreach ($file in $canonical.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $parser $file.path) -Algorithm SHA256).Hash -cne $file.sha256) {
        throw "Canonical parser hash mismatch: $($file.path)"
    }
}
$output = Join-Path $repo '.tools\bin\VoyageModelExporter'
$logs = Join-Path $repo ('artifacts\model-export\publish-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $logs)
$log = Join-Path $logs 'build.log'
& dotnet publish (Join-Path $PSScriptRoot 'VoyageModelExporter.csproj') -c Release -o $output -p:RestoreLockedMode=true --nologo -v quiet *> $log
if ($LASTEXITCODE -ne 0) { throw "Model exporter publish failed: $log" }
foreach ($file in $canonical.files) {
    if ($file.path -notlike '*.dll') { continue }
    if ((Get-FileHash -LiteralPath (Join-Path $output $file.path) -Algorithm SHA256).Hash -cne $file.sha256) {
        throw "Private parser copy differs from canonical bundle: $($file.path)"
    }
}
$files = @(Get-ChildItem -LiteralPath $output -File -Recurse | Where-Object { $_.Name -ne 'publish-manifest.json' } | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($output.Length + 1); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
})
$sources = @(Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object { $_.Extension -in '.cs','.csproj' -or $_.Name -eq 'packages.lock.json' } | ForEach-Object {
    [ordered]@{ path = $_.Name; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
})
$manifest = [ordered]@{ kind = 'VoyageModelExporter'; conversionCommit = $expectedCommit; files = $files; sources = $sources }
[IO.File]::WriteAllText((Join-Path $output 'publish-manifest.json'), ($manifest | ConvertTo-Json -Depth 6))
[ordered]@{ status = 'published'; binary = (Join-Path $output 'VoyageModelExporter.exe'); logPath = $log } | ConvertTo-Json -Compress
