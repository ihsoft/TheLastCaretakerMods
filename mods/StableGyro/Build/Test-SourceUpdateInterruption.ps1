$ErrorActionPreference='Stop'
$mod=Split-Path -Parent $PSScriptRoot
$repo=[IO.Path]::GetFullPath((Join-Path $mod '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SourceUpdate.psm1') -Force
$run=New-VoyageModRun -ModRoot $mod -Kind test-source-interruption
$source=Join-Path $run 'fixture/mods/StableGyro'
$updated=Join-Path $run 'updated'
$null=New-Item -ItemType Directory -Path (Join-Path $source 'Build'),$updated -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SourceUpdate.psm1') -Destination (Join-Path $source 'Build')
Copy-Item -LiteralPath (Join-Path $mod 'Restore-StableGyroSourceUpdate.ps1') -Destination $source
foreach ($name in @('asset','state')) {
    [IO.File]::WriteAllText((Join-Path $source $name),"uncommitted original $name")
    [IO.File]::WriteAllText((Join-Path $updated $name),"replacement $name")
}
$signal=Join-Path $run 'first-file-replaced'
$worker=Join-Path $run 'worker.ps1'
@'
param($ModulePath,$Source,$Updated,$Signal)
$ErrorActionPreference='Stop'
Import-Module $ModulePath -Force
$module=Get-Module SourceUpdate
& $module {
    param($Signal)
    $script:signal=$Signal
    $script:replaceOriginal=(Get-Command Set-StableGyroSourceFile).ScriptBlock
    function script:Set-StableGyroSourceFile([string]$Destination,[string]$BytesPath,[string]$Stage) {
        & $script:replaceOriginal $Destination $BytesPath $Stage
        [IO.File]::WriteAllText($script:signal,'first replacement completed')
        while ($true) { Start-Sleep -Milliseconds 100 }
    }
} $Signal
$snapshot=Get-StableGyroSourceSnapshot -ModRoot $Source -Paths @('asset','state')
Save-StableGyroSourceUpdate -ModRoot $Source -RunRoot $Updated -Paths @('asset','state') -Snapshot $snapshot
'@ | Set-Content -LiteralPath $worker
$arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$worker+'"'),
    '-ModulePath',('"'+(Join-Path $PSScriptRoot 'SourceUpdate.psm1')+'"'),'-Source',('"'+$source+'"'),
    '-Updated',('"'+$updated+'"'),'-Signal',('"'+$signal+'"'))
$process=Start-Process -FilePath powershell.exe -ArgumentList $arguments -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput (Join-Path $run 'worker.log') -RedirectStandardError (Join-Path $run 'worker-error.log')
try {
    $deadline=[DateTime]::UtcNow.AddSeconds(30)
    while (-not (Test-Path -LiteralPath $signal)) {
        if ($process.HasExited -or [DateTime]::UtcNow -gt $deadline) { throw "Interruption fixture did not reach first replacement; inspect $run" }
        Start-Sleep -Milliseconds 100
    }
    # Kill the exact fixture process: no catch/finally in the worker can recover.
    Stop-Process -Id $process.Id -Force
    $process.WaitForExit()
} finally { if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force; $process.WaitForExit() } }
if ([IO.File]::ReadAllText((Join-Path $source 'asset')) -cne 'replacement asset' -or
    [IO.File]::ReadAllText((Join-Path $source 'state')) -cne 'uncommitted original state') { throw 'Worker was not stopped between replacements.' }
$active=Join-Path $source 'Project/Intermediate/StableGyroSourceUpdate.json'
$manifest=(Get-Content -LiteralPath $active -Raw | ConvertFrom-Json).recoveryManifest
$journal=Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if (@($journal.entries).Count -ne 2) { throw 'Recovery did not capture the full acceptance set.' }
function Reject([scriptblock]$Action) { $failed=$false; try { & $Action | Out-Null } catch { $failed=$true }; if (-not $failed) { throw 'Expected rejection.' } }
Reject { Assert-StableGyroSourceReady -ModRoot $source }
[IO.File]::WriteAllText((Join-Path $source 'state'),'later user edit')
Reject { Restore-StableGyroSourceUpdate -ModRoot $source -RecoveryManifest $manifest }
if ([IO.File]::ReadAllText((Join-Path $source 'asset')) -cne 'replacement asset') { throw 'Recovery wrote before checking all conflicts.' }
[IO.File]::WriteAllText((Join-Path $source 'state'),'uncommitted original state')
$backup=Join-Path (Split-Path -Parent $manifest) $journal.entries[0].before
$backupBytes=[IO.File]::ReadAllBytes($backup)
[IO.File]::WriteAllText($backup,'damaged backup')
Reject { Restore-StableGyroSourceUpdate -ModRoot $source -RecoveryManifest $manifest }
[IO.File]::WriteAllBytes($backup,$backupBytes)
$lease=$env:CODEX_UNREAL_QUEUE_LEASE
try {
    $env:CODEX_UNREAL_QUEUE_LEASE='offline-fixture'
    $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+(Join-Path $source 'Restore-StableGyroSourceUpdate.ps1')+'"'),'-RecoveryManifest',('"'+$manifest+'"'))
    $restore=Start-Process powershell.exe -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru `
        -RedirectStandardOutput (Join-Path $run 'restore.log') -RedirectStandardError (Join-Path $run 'restore-error.log')
    if ($restore.ExitCode -ne 0) { throw "Public recovery failed; inspect $run" }
} finally { $env:CODEX_UNREAL_QUEUE_LEASE=$lease }
foreach ($name in @('asset','state')) {
    if ([IO.File]::ReadAllText((Join-Path $source $name)) -cne "uncommitted original $name") { throw "Original bytes not restored: $name" }
}
Assert-StableGyroSourceReady -ModRoot $source
$again=Restore-StableGyroSourceUpdate -ModRoot $source -RecoveryManifest $manifest
if ($again.status -cne 'restored') { throw 'Repeated recovery not idempotent.' }
@{status='passed';checks=@('child forcibly terminated between atomic replacements','persistent backups cover all paths','pending marker blocks reuse','later edit and damaged backup rejected before writes','public recovery restores uncommitted bytes','recovery repeat succeeds');recoveryManifest=$manifest;boundary='Synthetic source files; no Unreal or game writes.'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $run 'report.json')
"Process interruption and public recovery report: $run/report.json"
