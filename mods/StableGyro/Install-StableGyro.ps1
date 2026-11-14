[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ReleaseManifest,
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [switch]$AllowDirtySource,
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$installer = Join-Path $repo 'tools/Install-VoyageRelease.ps1'
$plan = & $installer -ReleaseManifest $ReleaseManifest -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource -ValidateOnly
if ($plan.mod -cne 'StableGyro') { throw 'Expected a StableGyro release manifest.' }
if ($ValidateOnly) { return $plan }
function Assert-Closed {
    if (@(Get-Process -Name Voyage,VoyageSteam-Win64-Shipping -ErrorAction SilentlyContinue).Count) {
        throw 'Close Voyage before installing StableGyro.'
    }
}
Assert-Closed
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $PSScriptRoot -Kind install
$transaction = Join-Path $run 'settings'
$null = New-Item -ItemType Directory -Path $transaction
$manifestPath = (Resolve-Path -LiteralPath $ReleaseManifest).Path
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$archive = Join-Path (Split-Path -Parent $manifestPath) $manifest.archive.path
$template = Join-Path $transaction 'template.ini'
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $entries = @($zip.Entries | Where-Object { $_.FullName -ceq 'StableGyro.ini' })
    if ($entries.Count -ne 1) { throw 'Candidate must contain exactly one StableGyro.ini.' }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0], $template)
} finally { $zip.Dispose() }
$target = Join-Path $plan.paksDirectory 'StableGyro.ini'
$existed = Test-Path -LiteralPath $target -PathType Leaf
$backup = Join-Path $transaction 'previous.ini'
if ($existed) { Copy-Item -LiteralPath $target -Destination $backup }
$installed = $null
$iniChanged = $false
try {
    Assert-Closed
    $installed = & $installer -ReleaseManifest $manifestPath -GameRoot $GameRoot `
        -AllowDirtySource:$AllowDirtySource -KeepRecoveryEvidence
    Assert-Closed
    # The stock installer owns the triplet; this coupled transaction owns the INI.
    $iniChanged = $true
    Copy-Item -LiteralPath $template -Destination $target -Force
    if ((Get-FileHash -LiteralPath $target).Hash -cne (Get-FileHash -LiteralPath $template).Hash) {
        throw 'Installed StableGyro.ini failed copy readback.'
    }
} catch {
    $failure = $_
    try {
        if ($iniChanged) {
            Assert-Closed
            if ($existed) {
                Copy-Item -LiteralPath $backup -Destination $target -Force
                if ((Get-FileHash -LiteralPath $target).Hash -cne (Get-FileHash -LiteralPath $backup).Hash) { throw 'INI recovery readback failed.' }
            } elseif (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target }
        }
        if ($null -ne $installed) {
            & (Join-Path $repo 'tools/Restore-VoyageReleaseInstallation.ps1') -InstallManifest $installed.installManifestPath | Out-Null
        }
    } catch { throw "Installation failed: $failure; recovery failed: $_. Preserve $run and installation recovery evidence." }
    throw $failure
}
$report = Join-Path $run 'install-result.json'
$result = [pscustomobject]@{status='installed'; settingsPath=$target; reportPath=$report}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $report -Encoding UTF8
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $run -Path $transaction
$recovery = Split-Path -Parent $installed.installManifestPath
# Shared installer recovery is below Tmp/StableGyro/installations, independent of this run.
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot (Split-Path -Parent $recovery) -Path $recovery
$result
