param(
    [Parameter(Mandatory = $true)]
    [string]$ManifestPath
)

$ErrorActionPreference = 'Stop'

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    throw 'Voyage must be closed before removing an unchanged-roundtrip probe.'
}

$manifest = Get-Content -Raw -LiteralPath (Resolve-Path -LiteralPath $ManifestPath) | ConvertFrom-Json
if ([string]$manifest.kind -cne 'Voyage unchanged roundtrip probe installation' -or
    [string]$manifest.containerName -cnotmatch '^Voyage[A-Za-z0-9]+Roundtrip_P$') {
    throw 'The supplied file is not an unchanged-roundtrip probe installation manifest.'
}
$paks = (Resolve-Path -LiteralPath ([string]$manifest.paksDirectory)).Path
$records = @($manifest.files)
if ($records.Count -ne 3) {
    throw 'The installation manifest must contain exactly three container files.'
}
$expectedNames = @('.pak', '.ucas', '.utoc') | ForEach-Object {
    [string]$manifest.containerName + $_
}
$actualNames = @($records | ForEach-Object { [string]$_.name })
if (@(Compare-Object $expectedNames $actualNames).Count -ne 0) {
    throw 'The installation manifest contains unexpected container file names.'
}
foreach ($record in $records) {
    $destination = Join-Path $paks ([string]$record.name)
    if (-not (Test-Path -LiteralPath $destination -PathType Leaf) -or
        [string]$record.installedSha256 -notmatch '^[0-9A-F]{64}$' -or
        (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash -cne [string]$record.installedSha256) {
        throw "Installed probe is missing or changed; refusing partial removal: $destination"
    }
}
foreach ($record in $records) {
    Remove-Item -LiteralPath (Join-Path $paks ([string]$record.name)) -Force
}
Write-Host "Removed unchanged-roundtrip probe: $($manifest.containerName)"
