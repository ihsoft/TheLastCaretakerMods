$ErrorActionPreference = 'Stop'
$mod = Split-Path -Parent $PSScriptRoot
$repo = [IO.Path]::GetFullPath((Join-Path $mod '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $mod -Kind test-install
$fixture = Join-Path $run 'fixture'
foreach ($path in @('mods/StableGyro/Build','tools/VoyageModSettings','game/Voyage/Content/Paks')) { $null=New-Item -ItemType Directory -Path (Join-Path $fixture $path) -Force }
Copy-Item -LiteralPath (Join-Path $mod 'Install-StableGyro.ps1') -Destination (Join-Path $fixture 'mods/StableGyro')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SettingsMigration.psd1') -Destination (Join-Path $fixture 'mods/StableGyro/Build')
foreach ($file in @('VoyageModPipeline.psm1','Merge-VoyageModIni.ps1','VoyageModSettings/VoyageModIniMerge.psm1')) {
    Copy-Item -LiteralPath (Join-Path $repo ('tools/' + $file)) -Destination (Join-Path $fixture ('tools/' + $file))
}
@'
param($ReleaseManifest,$GameRoot,[switch]$AllowDirtySource,[switch]$ValidateOnly,[switch]$KeepRecoveryEvidence)
if (-not $AllowDirtySource) { throw 'Dirty candidate rejected.' }
$root=Split-Path -Parent $PSScriptRoot
$paks=Join-Path $GameRoot 'Voyage/Content/Paks'
if ($ValidateOnly) { return [pscustomobject]@{mod='StableGyro';paksDirectory=$paks} }
$recovery=Join-Path $root ('Tmp/StableGyro/installations/' + [guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $recovery -Force
$target=Join-Path $paks 'StableGyro_P.pak'
Copy-Item -LiteralPath $target -Destination (Join-Path $recovery 'previous.pak')
'new container' | Set-Content -LiteralPath $target
$record=Join-Path $recovery 'install-manifest.json'
@{target=$target} | ConvertTo-Json | Set-Content -LiteralPath $record
[pscustomobject]@{installManifestPath=$record;paksDirectory=$paks}
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/Install-VoyageRelease.ps1')
@'
param($InstallManifest)
$data=Get-Content -LiteralPath $InstallManifest -Raw | ConvertFrom-Json
Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $InstallManifest) 'previous.pak') -Destination $data.target -Force
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/Restore-VoyageReleaseInstallation.ps1')
$game=Join-Path $fixture 'game'
$paks=Join-Path $game 'Voyage/Content/Paks'
$ini=Join-Path $paks 'StableGyro.ini'
$container=Join-Path $paks 'StableGyro_P.pak'
'old container' | Set-Content -LiteralPath $container
$archive=Join-Path $fixture 'candidate.zip'
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$zip=[IO.Compression.ZipFile]::Open($archive,[IO.Compression.ZipArchiveMode]::Create)
try {
    $entry=$zip.CreateEntry('Voyage/Content/Paks/StableGyro.ini')
    $writer=New-Object IO.StreamWriter($entry.Open())
    try { $writer.Write("FuelCapacity=50`nEnableAltitudeControl=true`n") } finally { $writer.Dispose() }
} finally { $zip.Dispose() }
$manifest=Join-Path $fixture 'release-manifest.json'
@{mod='StableGyro';archive=@{path='candidate.zip'}} | ConvertTo-Json | Set-Content -LiteralPath $manifest
$installer=Join-Path $fixture 'mods/StableGyro/Install-StableGyro.ps1'
function Check($condition,$label) { if (-not $condition) { throw "Failed: $label" } }
function Reject([scriptblock]$action,$label) { $failed=$false; try { & $action | Out-Null } catch { $failed=$true }; Check $failed $label }
$fixtureState=@{gameRunning=$false;failSettings=$false}
function Get-Process { [CmdletBinding()]param([string[]]$Name) if ($fixtureState.gameRunning) { [pscustomobject]@{Name='Voyage'} } }
$fixtureState.gameRunning=$true
Reject { & $installer -ReleaseManifest $manifest -GameRoot $game -AllowDirtySource } 'running game rejected'
$fixtureState=@{gameRunning=$false;failSettings=$false}
Reject { & $installer -ReleaseManifest $manifest -GameRoot $game } 'dirty policy forwarded'
$null=& $installer -ReleaseManifest $manifest -GameRoot $game -AllowDirtySource
Check ((Get-Content -LiteralPath $ini -Raw).Contains('FuelCapacity=50')) 'create from candidate'
"# user comment`nFuelCapacity=99`nEnableFuelDiagnostics=true`n" | Set-Content -LiteralPath $ini
$null=& $installer -ReleaseManifest $manifest -GameRoot $game -AllowDirtySource
$text=Get-Content -LiteralPath $ini -Raw
Check ($text.Contains('FuelCapacity=99') -and $text.Contains('# user comment') -and $text.Contains('EnableAltitudeControl=true') -and -not $text.Contains('EnableFuelDiagnostics')) 'preservation and migration'
# Fail after payload replacement, at the first settings write, then allow recovery.
$before=[IO.File]::ReadAllBytes($ini)
'rollback container' | Set-Content -LiteralPath $container
$fixtureState.failSettings=$true
function Copy-Item {
    [CmdletBinding()]param([string]$LiteralPath,[string]$Destination,[switch]$Force)
    if ($fixtureState.failSettings -and $LiteralPath.EndsWith('merged.ini')) { $fixtureState.failSettings=$false; throw 'Injected INI write failure' }
    Microsoft.PowerShell.Management\Copy-Item @PSBoundParameters
}
Reject { & $installer -ReleaseManifest $manifest -GameRoot $game -AllowDirtySource } 'coupled failure surfaced'
Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($ini)) -ceq [Convert]::ToBase64String($before)) 'exact INI rollback'
Check ((Get-Content -LiteralPath $container -Raw).Trim() -ceq 'rollback container') 'container rollback'
Remove-Item Function:Copy-Item
Remove-Item Function:Get-Process
# Reimport the real module after the fixture installer loaded its isolated copy.
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
Remove-VoyageModScratch -ModRoot $mod -RunRoot $run -Path $fixture
'Candidate INI, preservation, retired keys, dirty/game gates and coupled rollback passed.'
