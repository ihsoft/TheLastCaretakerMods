param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
$testRoot = Join-Path $repoRoot "artifacts/tests/repository-git-coordination"
if (Test-Path -LiteralPath $testRoot) {
    Remove-Item -LiteralPath $testRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $testRoot | Out-Null

$modulePath = Join-Path $PSScriptRoot "repository-coordination.psm1"
$workerPath = Join-Path $testRoot "lock-worker.ps1"
$eventsPath = Join-Path $testRoot "events.jsonl"
$workerSource = @'
param(
    [string] $ModulePath,
    [string] $RepositoryRoot,
    [string] $EventsPath,
    [string] $Owner,
    [int] $HoldMilliseconds
)
Import-Module $ModulePath -Force
Invoke-WithRepositoryLock -RepositoryRoot $RepositoryRoot -Resource "git-transaction" `
    -Operation "semaphore test" -Owner $Owner -WaitSeconds 10 -Action {
    $start = [pscustomobject]@{
        owner = $Owner
        event = "start"
        ticks = [System.DateTimeOffset]::UtcNow.Ticks
    } | ConvertTo-Json -Compress
    Add-Content -LiteralPath $EventsPath -Value $start
    Start-Sleep -Milliseconds $HoldMilliseconds
    $end = [pscustomobject]@{
        owner = $Owner
        event = "end"
        ticks = [System.DateTimeOffset]::UtcNow.Ticks
    } | ConvertTo-Json -Compress
    Add-Content -LiteralPath $EventsPath -Value $end
}
'@
[System.IO.File]::WriteAllText(
    $workerPath,
    $workerSource,
    [System.Text.UTF8Encoding]::new($false))

$powerShell = (Get-Process -Id $PID).Path
$first = Start-Process -FilePath $powerShell -PassThru -WindowStyle Hidden -ArgumentList @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", $workerPath,
    "-ModulePath", $modulePath,
    "-RepositoryRoot", $repoRoot,
    "-EventsPath", $eventsPath,
    "-Owner", "first",
    "-HoldMilliseconds", "1000")
Start-Sleep -Milliseconds 150
$second = Start-Process -FilePath $powerShell -PassThru -WindowStyle Hidden -ArgumentList @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", $workerPath,
    "-ModulePath", $modulePath,
    "-RepositoryRoot", $repoRoot,
    "-EventsPath", $eventsPath,
    "-Owner", "second",
    "-HoldMilliseconds", "100")
$first.WaitForExit()
$second.WaitForExit()
if ($first.ExitCode -ne 0 -or $second.ExitCode -ne 0) {
    throw "Semaphore worker failed: first=$($first.ExitCode), second=$($second.ExitCode)."
}

$events = @(Get-Content -LiteralPath $eventsPath | ForEach-Object { $_ | ConvertFrom-Json })
if ($events.Count -ne 4) {
    throw "Expected four lock events, found $($events.Count)."
}
$firstEnd = @($events | Where-Object { $_.owner -eq "first" -and $_.event -eq "end" })[0]
$secondStart = @($events | Where-Object { $_.owner -eq "second" -and $_.event -eq "start" })[0]
if ([long]$secondStart.ticks -lt [long]$firstEnd.ticks) {
    throw "Repository lock intervals overlapped."
}

$fixtureRoot = Join-Path $testRoot "fixture-repo"
$fixtureTools = Join-Path $fixtureRoot "tools"
New-Item -ItemType Directory -Path $fixtureTools | Out-Null
Copy-Item -LiteralPath $modulePath -Destination $fixtureTools
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "commit-repository-changes.ps1") `
    -Destination $fixtureTools

& git -C $testRoot init fixture-repo | Out-Null
& git -C $fixtureRoot config user.name "Repository Coordination Test"
& git -C $fixtureRoot config user.email "repository-coordination@example.invalid"
[System.IO.File]::WriteAllText((Join-Path $fixtureRoot "one.txt"), "base one`n")
[System.IO.File]::WriteAllText((Join-Path $fixtureRoot "two.txt"), "base two`n")
& git -C $fixtureRoot add -- one.txt two.txt
& git -C $fixtureRoot commit -m "Base" | Out-Null

[System.IO.File]::WriteAllText((Join-Path $fixtureRoot "one.txt"), "changed one`n")
[System.IO.File]::WriteAllText((Join-Path $fixtureRoot "two.txt"), "changed two`n")
$fixtureCommit = Join-Path $fixtureTools "commit-repository-changes.ps1"
& $powerShell -NoProfile -ExecutionPolicy Bypass -File $fixtureCommit `
    -Path one.txt -Message "Commit one" -Owner "coordination-test" | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Exact-path commit test failed with exit code $LASTEXITCODE."
}
$committedPaths = @(& git -C $fixtureRoot diff-tree --no-commit-id --name-only -r HEAD)
if ($committedPaths.Count -ne 1 -or $committedPaths[0] -ne "one.txt") {
    throw "Exact-path commit included unexpected paths: $($committedPaths -join ', ')."
}
$remainingStatus = @(& git -C $fixtureRoot status --porcelain=v1 -- two.txt)
if ($remainingStatus.Count -ne 1 -or $remainingStatus[0] -ne " M two.txt") {
    throw "Unrelated worktree change was not preserved: $($remainingStatus -join ', ')."
}

& git -C $fixtureRoot add -- two.txt
[System.IO.File]::WriteAllText((Join-Path $fixtureRoot "three.txt"), "new three`n")
$previousPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = "Continue"
    & $powerShell -NoProfile -ExecutionPolicy Bypass -File $fixtureCommit `
        -Path three.txt -Message "Must reject occupied index" -Owner "coordination-test" `
        2>$null | Out-Null
    $rejectionExitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $previousPreference
}
if ($rejectionExitCode -eq 0) {
    throw "Commit wrapper accepted an index owned by another transaction."
}
$stagedAfterRejection = @(& git -C $fixtureRoot diff --cached --name-only --)
if ($stagedAfterRejection.Count -ne 1 -or $stagedAfterRejection[0] -ne "two.txt") {
    throw "Occupied-index rejection changed staged state."
}

& $powerShell -NoProfile -ExecutionPolicy Bypass -File $fixtureCommit `
    -Path two.txt -Message "Adopt exact staged recovery" -Owner "coordination-test" `
    -UseExistingIndex | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Explicit existing-index recovery failed with exit code $LASTEXITCODE."
}

Write-Host "Repository Git coordination passed: serialized processes, exact commit scope, occupied-index rejection, and explicit recovery."
