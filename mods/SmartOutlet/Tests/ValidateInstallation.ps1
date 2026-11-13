# Exercise the mod-owned INI transaction with a synthetic container installer.
# No engine or installed game is used.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$mod = Split-Path $PSScriptRoot -Parent
$repo = (Resolve-Path -LiteralPath (Join-Path $mod '../..')).Path
$owner = Join-Path $repo 'Tmp/SmartOutlet'
$work = Join-Path $owner ('test-installation-' + [guid]::NewGuid().ToString('N'))
$fixtureMod = Join-Path $work 'mods/SmartOutlet'
$fixtureTools = Join-Path $work 'tools'
$game = Join-Path $work 'game'
foreach ($directory in @($fixtureMod,$fixtureTools,$game)) { [IO.Directory]::CreateDirectory($directory) | Out-Null }
Copy-Item -LiteralPath (Join-Path $mod 'Install-SmartOutlet.ps1') -Destination $fixtureMod
Copy-Item -LiteralPath (Join-Path $mod 'Settings') -Destination $fixtureMod -Recurse
$entry = Join-Path $fixtureMod 'Install-SmartOutlet.ps1'
$manifest = Join-Path $work 'release-manifest.json'
'{"mod":"SmartOutlet","archive":{"path":"SmartOutlet-V1.zip"}}' | Set-Content -LiteralPath $manifest
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open((Join-Path $work 'SmartOutlet-V1.zip'), [IO.Compression.ZipArchiveMode]::Create)
try {
    [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, (Join-Path $mod 'Assets/SmartOutlet.ini'), 'Voyage/Content/Paks/LogicMods/SmartOutlet.ini') | Out-Null
} finally { $archive.Dispose() }
@'
param($ReleaseManifest,$GameRoot,[switch]$AllowDirtySource,[switch]$ValidateOnly,[switch]$KeepRecoveryEvidence,$EvidenceRoot)
if ($ValidateOnly) { return [pscustomobject]@{paksDirectory=$GameRoot} }
if (-not $KeepRecoveryEvidence) { throw 'Settings mutation requires container recovery until it completes' }
[IO.Directory]::CreateDirectory($EvidenceRoot) | Out-Null
$record=Join-Path $EvidenceRoot 'install-manifest.json'
[IO.File]::WriteAllText($record,'synthetic transaction')
[pscustomobject]@{fileCount=3;installManifestPath=$record}
'@ | Set-Content -LiteralPath (Join-Path $fixtureTools 'Install-VoyageRelease.ps1')
@'
param($InstallManifest)
if (-not (Test-Path -LiteralPath $InstallManifest)) { throw 'Missing container recovery record' }
[IO.File]::WriteAllText((Join-Path (Split-Path $InstallManifest -Parent) 'restored.txt'),'restored')
'@ | Set-Content -LiteralPath (Join-Path $fixtureTools 'Restore-VoyageReleaseInstallation.ps1')
# Isolate the process gate and inject one failed INI write in the synthetic game.
function Get-Process { param($Name,$ErrorAction) }
$copyFault = [pscustomobject]@{fail=$false;target=(Join-Path $game 'SmartOutlet.ini')}
function Copy-Item {
    param($LiteralPath,$Destination,[switch]$Force)
    Microsoft.PowerShell.Management\Copy-Item -LiteralPath $LiteralPath -Destination $Destination -Force:$Force
    if ($copyFault.fail -and $Destination -eq $copyFault.target) {
        $copyFault.fail = $false
        throw 'Injected settings write failure'
    }
}
function Assert-NoRecovery {
    $operations = Join-Path $work 'Tmp/SmartOutlet/installations'
    if (@(Get-ChildItem -LiteralPath $operations -Directory -ErrorAction SilentlyContinue).Count) { throw 'Successful transaction retained recovery files' }
}
try {
    $created = & $entry -ReleaseManifest $manifest -GameRoot $game
    if (-not $created.created -or $created.containerFiles -ne 3) { throw 'Fresh installation result is wrong' }
    Assert-NoRecovery
    $custom = "# Keep my comments`r`nUnlockSaltWaterTank = false`r`n[Custom.Custom_C:Output]`r`nEnablePortAtPercent = 91`r`nDisablePortAtPercent = 7`r`nGlowMaterialSlot = -1`r`n"
    [IO.File]::WriteAllText($copyFault.target,$custom)
    $preserved = & $entry -ReleaseManifest $manifest -GameRoot $game
    if ([IO.File]::ReadAllText($copyFault.target) -cne $custom -or $preserved.settingsMerge.Changed) { throw 'User settings changed' }
    if ($preserved.recoveryDirectory -or $preserved.installManifestPath) { throw 'Result exposes discarded recovery paths' }
    Assert-NoRecovery
    $copyFault.fail = $true
    $failed = $false
    try { & $entry -ReleaseManifest $manifest -GameRoot $game -ResetSettings | Out-Null }
    catch { $failed = $_.Exception.Message -match 'Injected settings write failure' }
    if (-not $failed) { throw 'Injected failure was not reported' }
    if ([IO.File]::ReadAllText($copyFault.target) -cne $custom) { throw 'Failed settings write was not restored' }
    $recovered = @(Get-ChildItem -LiteralPath (Join-Path $work 'Tmp') -Filter 'restored.txt' -Recurse)
    if ($recovered.Count -ne 1) { throw 'Containers were not restored after the settings failure' }
    [pscustomobject]@{status='passed';checks='fresh install; exact user INI preservation; success cleanup; failed INI write restores settings and containers and retains recovery'}
} catch { throw "Installation fixture failed; evidence: $work. $_" }
# The synthetic failure has been recovered and checked; its evidence is disposable.
$resolved = (Resolve-Path -LiteralPath $work).Path
if (-not $resolved.StartsWith($owner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture cleanup path' }
Remove-Item -LiteralPath $resolved -Recurse -Force
