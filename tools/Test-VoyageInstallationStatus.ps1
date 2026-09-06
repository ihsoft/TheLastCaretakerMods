[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Join-Path (Split-Path -Parent $PSScriptRoot) ('artifacts\tests\installation-status-' + [Guid]::NewGuid().ToString('N'))
$game = Join-Path $root 'steamapps\common\Voyage'
$paks = Join-Path $game 'Voyage\Content\Paks'
$exe = Join-Path $game 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
[IO.Directory]::CreateDirectory((Split-Path -Parent $exe)) | Out-Null
[IO.Directory]::CreateDirectory($paks) | Out-Null
[IO.Directory]::CreateDirectory((Join-Path $paks '~mods')) | Out-Null
[IO.File]::WriteAllText($exe, 'synthetic exe, never executed')
[IO.File]::WriteAllText((Join-Path $root 'steamapps\appmanifest_1783560.acf'), '"buildid" "99999999"')
$target = Join-Path $paks 'Probe_P.utoc'
[IO.File]::WriteAllText($target, 'synthetic mod')
[IO.File]::WriteAllText((Join-Path $paks 'pakchunk0-Windows.utoc'), 'stock')
$tool = Join-Path $PSScriptRoot 'Get-VoyageInstallationStatus.ps1'
$manifestPath = Join-Path $root 'install.json'
$checks = [Collections.Generic.List[string]]::new()
function Assert($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Save-Manifest {
    [IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 8))
}
function Read-Status([switch]$Manifest) {
    $argsForTool = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $tool, '-GameRoot', $game)
    if ($Manifest) { $argsForTool += @('-InstallManifest', $manifestPath) }
    $output = & powershell.exe @argsForTool
    if ($LASTEXITCODE -ne 0) { throw 'Status subprocess failed.' }
    $output | ConvertFrom-Json
}
function Expect-Rejection {
    try {
        & $tool -GameRoot $game -InstallManifest $manifestPath | Out-Null
    } catch { return }
    throw 'Malformed manifest was accepted.'
}
$before = @(Get-ChildItem -LiteralPath $game -Recurse -File | ForEach-Object {
    "$($_.FullName):$($_.LastWriteTimeUtc.Ticks):$((Get-FileHash -LiteralPath $_.FullName).Hash)"
}) -join '|'
$status = Read-Status
Assert ($null -eq $status.installation -and $status.files.Count -eq 2) 'Inventory/no-manifest contract.'
Assert ($status.unscannedSubdirectories.Count -eq 1) 'Nested directory must be disclosed.'
Assert (@($status.files | Where-Object category -eq 'stock-name').Count -eq 1) 'Stock classification.'
$checks.Add('inventory-and-explicit-scope')
$manifest = [ordered]@{
    schemaVersion = 1; kind = 'Voyage release installation'; status = 'installed'
    gameRoot = $game; paksDirectory = $paks; mod = 'Probe'; artifactVersion = 'test'
    steamBuildId = $status.steamBuildId; executableSha256 = $status.executableSha256
    files = @([ordered]@{
        name = 'Probe_P.utoc'; destinationPath = $target
        size = (Get-Item -LiteralPath $target).Length
        installedSha256 = (Get-FileHash -LiteralPath $target).Hash
    })
}
Save-Manifest
$status = Read-Status -Manifest
Assert ($status.installation.filesMatch -and $status.installation.gameFingerprintMatches) 'Matching installation.'
$checks.Add('manifest-hash-match')
$after = @(Get-ChildItem -LiteralPath $game -Recurse -File | ForEach-Object {
    "$($_.FullName):$($_.LastWriteTimeUtc.Ticks):$((Get-FileHash -LiteralPath $_.FullName).Hash)"
}) -join '|'
Assert ($before -ceq $after) 'Read-only check changed game files.'
$checks.Add('read-only-hashes-and-write-times')
$manifest.steamBuildId = 'old-build'
Save-Manifest
$status = Read-Status -Manifest
Assert ($status.installation.filesMatch -and -not $status.installation.gameFingerprintMatches) 'Fingerprint separate from file state.'
$checks.Add('fingerprint-independent-of-file-match')
[IO.File]::WriteAllText($target, 'different mod')
$status = Read-Status -Manifest
Assert ($status.installation.files[0].state -eq 'different') 'Changed file not detected.'
$checks.Add('changed-file')
Move-Item -LiteralPath $target -Destination (Join-Path $root 'original-test.utoc')
$status = Read-Status -Manifest
Assert ($status.installation.files[0].state -eq 'missing') 'Missing file not detected.'
$checks.Add('missing-file')
[IO.Directory]::CreateDirectory($target) | Out-Null
$status = Read-Status -Manifest
Assert ($status.installation.files[0].state -eq 'unsupported-path') 'Directory accepted as file.'
$checks.Add('directory-target')
$manifest.files[0].name = '..\outside.utoc'
Save-Manifest
Expect-Rejection
$checks.Add('unsafe-record-rejected')
$manifest.files[0].name = 'Probe_P.utoc'
$manifest.gameRoot = $root
Save-Manifest
Expect-Rejection
$checks.Add('wrong-installation-root-rejected')
$summary = [pscustomobject]@{ passed = $checks.Count; checks = @($checks); evidencePath = $root }
[IO.File]::WriteAllText((Join-Path $root 'summary.json'), ($summary | ConvertTo-Json -Depth 5))
$summary | ConvertTo-Json -Depth 5
