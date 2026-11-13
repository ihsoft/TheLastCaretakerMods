# Install a candidate; keep an existing INI unless ResetSettings is requested.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ReleaseManifest,
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [switch]$AllowDirtySource,
    [switch]$ResetSettings
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$installer = Join-Path $repo 'tools/Install-VoyageRelease.ps1'
$manifestPath = (Resolve-Path -LiteralPath $ReleaseManifest).Path
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.mod -cne 'SmartOutlet') { throw 'Expected a SmartOutlet release manifest' }
$plan = & $installer -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource -ValidateOnly
function Assert-GameClosed {
    if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' -ErrorAction SilentlyContinue).Count) {
        throw 'Close Voyage before installing SmartOutlet settings and containers.'
    }
}
Assert-GameClosed
$target = Join-Path $plan.paksDirectory 'SmartOutlet.ini'
$existed = Test-Path -LiteralPath $target
if ($existed) {
    $item = Get-Item -LiteralPath $target -Force
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unsafe INI installation target' }
}
if ($existed -and -not $ResetSettings) {
    $containerResult = & $installer -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource
    [pscustomobject]@{status='installed';releaseManifest=$manifestPath;settingsPath=$target;created=$false;settingsAction='preserved';containerFiles=$containerResult.fileCount;recoveryDirectory=$null;installManifestPath=$null}
    return
}
$tmpOwner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/SmartOutlet/installations'))
$operation = Join-Path $tmpOwner ('settings-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($operation) | Out-Null
$template = Join-Path $operation 'template.ini'
$backup = Join-Path $operation 'original.ini'
$archivePath = [IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($manifestPath)) $manifest.archive.path))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $entries = @($zip.Entries | Where-Object FullName -CEQ 'Voyage/Content/Paks/LogicMods/SmartOutlet.ini')
    if ($entries.Count -ne 1) { throw 'Candidate must contain exactly one SmartOutlet.ini at the runtime path' }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0], $template)
} finally { $zip.Dispose() }
$originalHash = $null
if ($existed) {
    Copy-Item -LiteralPath $target -Destination $backup
    $originalHash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -cne $originalHash) { throw 'INI backup readback failed' }
}
$templateHash = (Get-FileHash -LiteralPath $template -Algorithm SHA256).Hash
$containerResult = $null
$iniWritten = $false
try {
    Assert-GameClosed
    $containerResult = & $installer -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource -KeepRecoveryEvidence -EvidenceRoot (Join-Path $operation 'containers')
    Assert-GameClosed
    if ($existed) {
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne $originalHash) { throw 'INI changed during installation' }
    } elseif (Test-Path -LiteralPath $target) { throw 'INI appeared during installation' }
    # Digests are used only for copy integrity and rollback ownership in this transaction.
    $iniWritten = $true
    Copy-Item -LiteralPath $template -Destination $target -Force
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne $templateHash) { throw 'Installed INI readback failed' }
} catch {
    $failure = $_
    try {
        Assert-GameClosed
        if ($iniWritten) {
            if ($existed) { Copy-Item -LiteralPath $backup -Destination $target -Force }
            else { Remove-Item -LiteralPath $target -Force }
            if ($existed -and (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -cne $originalHash) { throw 'INI restore readback failed' }
        }
        if ($containerResult -and $containerResult.installManifestPath) {
            & (Join-Path $repo 'tools/Restore-VoyageReleaseInstallation.ps1') -InstallManifest $containerResult.installManifestPath | Out-Null
        }
    } catch { throw "Installation recovery needs attention at $operation. Original failure: $failure. Recovery: $_" }
    throw "SmartOutlet installation failed; recovery evidence: $operation. $failure"
}
$resolvedOperation = (Resolve-Path -LiteralPath $operation).Path
if (-not $resolvedOperation.StartsWith($tmpOwner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected cleanup path' }
# Both mutations succeeded. Gameplay rollback uses the previous ready ZIP.
try { Remove-Item -LiteralPath $resolvedOperation -Recurse -Force }
catch { throw "SmartOutlet files and settings are installed and verified, but temporary recovery cleanup failed at $resolvedOperation. $_" }
[pscustomobject]@{status='installed';releaseManifest=$manifestPath;settingsPath=$target;created=(-not $existed);settingsAction=$(if ($existed) {'reset'} else {'created'});containerFiles=$containerResult.fileCount;recoveryDirectory=$null;installManifestPath=$null}
