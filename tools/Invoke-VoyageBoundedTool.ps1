[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [string[]]$Arguments = @(),
    [string]$WorkingDirectory,
    [ValidateRange(64, 65536)][int]$MemoryLimitMB = 1024,
    [ValidateRange(1, 3600)][int]$TimeoutSeconds = 60,
    [switch]$AllowFailure
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (-not $WorkingDirectory) { $WorkingDirectory = $repo }
$directory = (Resolve-Path -LiteralPath $WorkingDirectory).Path
$exe = (Resolve-Path -LiteralPath $Executable).Path
if (-not (Test-Path -LiteralPath $exe -PathType Leaf) -or [IO.Path]::GetExtension($exe) -ine '.exe') {
    throw 'Select one executable .exe, not a script or shell command string.'
}
$helper = Join-Path $PSScriptRoot 'BoundedProcess\WindowsJobRunner.cs'
$helperHash = (Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash
if (-not ('Voyage.Tools.WindowsJobRunner' -as [type])) {
    Add-Type -Path $helper
    [AppDomain]::CurrentDomain.SetData('VoyageJobRunnerHash', $helperHash)
}
elseif ([AppDomain]::CurrentDomain.GetData('VoyageJobRunnerHash') -cne $helperHash) {
    throw 'Runner source changed in this host. Start a fresh PowerShell process.'
}
$run = Join-Path $repo ('artifacts\tool-runs\' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($run) | Out-Null
$stdout = Join-Path $run 'stdout.log'
$stderr = Join-Path $run 'stderr.log'
$reportPath = Join-Path $run 'result.json'
$report = [ordered]@{
    schemaVersion = 1; status = 'launch-failed'; executable = $exe
    executableSha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    arguments = @($Arguments); workingDirectory = $directory
    memoryLimitBytes = [long]$MemoryLimitMB * 1MB; timeoutSeconds = $TimeoutSeconds
    runnerSourceSha256 = $helperHash; startedAtUtc = [DateTime]::UtcNow.ToString('o')
    stdoutPath = $stdout; stderrPath = $stderr; reportPath = $reportPath
}
try {
    $result = [Voyage.Tools.WindowsJobRunner]::Run($exe, $Arguments, $directory, $stdout, $stderr,
        [uint64]$report.memoryLimitBytes, [uint32]$TimeoutSeconds)
    $report.status = if ($result.TimedOut) { 'timeout' } elseif ($result.ExitCode -eq 0) { 'passed' } else { 'exit-failed' }
    $report.exitCode = $result.ExitCode
    $report.processId = $result.ProcessId
    $report.elapsedMilliseconds = $result.ElapsedMilliseconds
    $report.peakJobMemoryBytes = $result.PeakJobMemoryBytes
}
catch { $report.error = $_.Exception.Message }
$report.completedAtUtc = [DateTime]::UtcNow.ToString('o')
[IO.File]::WriteAllText($reportPath, ($report | ConvertTo-Json -Depth 5))
if ($report.status -ne 'passed' -and -not $AllowFailure) {
    throw "Bounded tool $($report.status). Report: $reportPath"
}
[pscustomobject]$report
