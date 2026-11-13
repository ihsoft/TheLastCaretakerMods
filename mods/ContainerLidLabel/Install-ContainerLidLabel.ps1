# Installs an additive candidate and preserves user settings in LogicMods.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ReleaseManifest,
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$installer = Join-Path $repo 'tools/Install-VoyageRelease.ps1'
$plan = & $installer -ReleaseManifest $ReleaseManifest -GameRoot $GameRoot -AllowDirtySource -ValidateOnly
$logic = [IO.Path]::GetFullPath((Join-Path $GameRoot 'Voyage/Content/Paks/LogicMods'))
if ($plan.mod -cne 'ContainerLidLabel' -or -not [StringComparer]::OrdinalIgnoreCase.Equals($plan.paksDirectory, $logic)) { throw 'Expected ContainerLidLabel LogicMods candidate' }
function Assert-Closed {
    if (@(Get-Process -ErrorAction Stop | Where-Object { $_.ProcessName -in @('VoyageSteam-Win64-Shipping','Voyage') }).Count) { throw 'Voyage must be closed before changing mod files.' }
}
Assert-Closed
$target = Join-Path $logic 'ContainerLidLabel.ini'
if (Test-Path -LiteralPath $target) {
    $item = Get-Item -LiteralPath $target -Force
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Unsupported INI target: $target" }
}
$targetExisted = Test-Path -LiteralPath $target -PathType Leaf
$transaction = Join-Path $repo ('Tmp/ContainerLidLabel/installations/settings-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($transaction) | Out-Null
$template = Join-Path $transaction 'default.ini'
$prepared = Join-Path $transaction 'ContainerLidLabel.ini'
$backup = Join-Path $transaction 'previous.ini'
$record = Get-Content -LiteralPath $ReleaseManifest -Raw | ConvertFrom-Json
$archive = Join-Path (Split-Path -Parent (Resolve-Path -LiteralPath $ReleaseManifest).Path) $record.archive.path
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $entry = $zip.GetEntry('Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini')
    if ($null -eq $entry) { throw 'Candidate ZIP has no INI at the runtime path' }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $template)
} finally { $zip.Dispose() }
$previousHash = $null
if ($targetExisted) {
    Copy-Item -LiteralPath $target -Destination $backup
    $previousHash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -cne $previousHash) { throw 'INI backup readback failed' }
    $mergeInput = $backup
} else { $mergeInput = $null }
& (Join-Path $repo 'tools/Merge-VoyageModIni.ps1') -TemplatePath $template -CurrentPath $mergeInput -OutputPath $prepared | Out-Null
$state = [ordered]@{status='prepared';target=$target;targetExisted=$targetExisted;backup=$backup;installManifest=$null}
$statePath = Join-Path $transaction 'settings-transaction.json'
$state | ConvertTo-Json | Set-Content -LiteralPath $statePath
$installed = $null
$iniChanged = $false
try {
    Assert-Closed
    if ($targetExisted -and (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne $previousHash) { throw 'INI changed while preparing installation' }
    $installed = & $installer -ReleaseManifest $ReleaseManifest -GameRoot $GameRoot -AllowDirtySource -KeepRecoveryEvidence
    $state.installManifest = $installed.installManifestPath
    $state.status = 'containers-installed'
    $state | ConvertTo-Json | Set-Content -LiteralPath $statePath
    Assert-Closed
    if ($targetExisted -and (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne $previousHash) { throw 'INI changed during container installation' }
    if (-not $targetExisted -and (Test-Path -LiteralPath $target)) { throw 'New INI appeared during container installation' }
    $iniChanged = $true
    Copy-Item -LiteralPath $prepared -Destination $target -Force
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne (Get-FileHash -LiteralPath $prepared -Algorithm SHA256).Hash) { throw 'Installed INI readback failed' }
} catch {
    $failure = $_
    try { Assert-Closed } catch { throw "Game started during installation; retain recovery evidence $statePath. $failure" }
    if ($iniChanged) {
        if ($targetExisted) { Copy-Item -LiteralPath $backup -Destination $target -Force }
        elseif (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Force }
        if ($targetExisted -and (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne $previousHash) { throw "INI recovery failed; retain $statePath" }
    }
    if ($null -ne $installed) { & (Join-Path $repo 'tools/Restore-VoyageReleaseInstallation.ps1') -InstallManifest $installed.installManifestPath | Out-Null }
    throw "Installation failed; recovery evidence: $statePath. $failure"
}
foreach ($directory in @($transaction, (Split-Path -Parent $installed.installManifestPath))) {
    $resolved = (Resolve-Path -LiteralPath $directory).Path
    $boundary = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/ContainerLidLabel')) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($boundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe recovery cleanup path' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
[pscustomobject]@{status='installed';fileCount=$installed.fileCount+1;ini=$target;preservedUserSettings=$targetExisted}
