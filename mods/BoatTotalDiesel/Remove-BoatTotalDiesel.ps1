# HAND-WRITTEN INSTALL TOOL: hash-guarded removal/restoration counterpart to the
# BoatTotalDiesel installer. It refuses to remove files changed after installation.

param(
    [Parameter(Mandatory = $true)]
    [string]$InstallManifest
)

$ErrorActionPreference = 'Stop'

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    $processes = ($running | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ', '
    throw "Voyage must be closed before removing BoatTotalDiesel: $processes"
}

$manifestPath = (Resolve-Path -LiteralPath $InstallManifest).Path
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$paks = (Resolve-Path -LiteralPath $manifest.paksDirectory).Path

foreach ($record in $manifest.files) {
    $installed = Join-Path $paks ([string]$record.name)
    if (-not (Test-Path -LiteralPath $installed -PathType Leaf)) {
        throw "Installed BoatTotalDiesel file is missing: $installed"
    }
    $currentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $installed).Hash
    if ($currentHash -cne [string]$record.installedSha256) {
        throw "Refusing to remove a changed installed file: $installed"
    }
}

foreach ($record in $manifest.files) {
    $installed = Join-Path $paks ([string]$record.name)
    Remove-Item -LiteralPath $installed -Force
    if ([bool]$record.hadExisting) {
        $backup = (Resolve-Path -LiteralPath ([string]$record.backupPath)).Path
        $backupHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $backup).Hash
        if ($backupHash -cne [string]$record.previousSha256) {
            throw "Previous-container backup hash mismatch: $backup"
        }
        Copy-Item -LiteralPath $backup -Destination $installed
    }
}

Write-Host 'BoatTotalDiesel removed; any previous container was restored.'
