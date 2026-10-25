[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source = Join-Path $repo '.tools\CUE4Parse'
$gitSource = $source.Replace('\', '/')
if (@(& git -c "safe.directory=$gitSource" -C $source status --porcelain --untracked-files=no).Count -ne 0 -or $LASTEXITCODE -ne 0) {
    throw 'Refusing a dirty conversion dependency.'
}
$parser = Join-Path $repo '.tools\bin\CUE4Parse'
$canonical = Get-Content -LiteralPath (Join-Path $parser 'publish-manifest.json') -Raw | ConvertFrom-Json
foreach ($file in $canonical.files) {
    if (-not (Test-Path -LiteralPath (Join-Path $parser $file.path) -PathType Leaf)) {
        throw "Canonical parser file is missing: $($file.path)"
    }
}
$output = Join-Path $repo '.tools\bin\VoyageMaterialLibrary'
$logs = Join-Path $repo ('Tmp\tools\VoyageMaterialLibrary\publish-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $logs)
$log = Join-Path $logs 'build.log'
& dotnet publish (Join-Path $PSScriptRoot 'VoyageMaterialLibrary.csproj') -c Release -o $output -p:RestoreLockedMode=true --nologo -v quiet *> $log
if ($LASTEXITCODE -ne 0) { throw "Material exporter publish failed: $log" }
$files = @(Get-ChildItem -LiteralPath $output -File -Recurse | Where-Object { $_.Name -ne 'publish-manifest.json' } | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($output.Length + 1); length = $_.Length }
})
$manifest = [ordered]@{ kind = 'VoyageMaterialLibrary'; files = $files; publishedAtUtc = [DateTime]::UtcNow.ToString('o') }
[IO.File]::WriteAllText((Join-Path $output 'publish-manifest.json'), ($manifest | ConvertTo-Json -Depth 6))
[ordered]@{ status = 'published'; binary = (Join-Path $output 'VoyageMaterialLibrary.exe'); logPath = $log } | ConvertTo-Json -Compress
