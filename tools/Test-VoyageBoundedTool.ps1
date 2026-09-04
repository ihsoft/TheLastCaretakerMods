[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$run = Join-Path $repo ('artifacts\tests\bounded-tool-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($run) | Out-Null
$probe = Join-Path $run 'runner probe.exe'
Add-Type -Path (Join-Path $PSScriptRoot 'BoundedProcess\RunnerProbe.cs') -OutputAssembly $probe `
    -OutputType ConsoleApplication
$invoke = Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1'
$checks = [Collections.Generic.List[string]]::new()
$reports = [Collections.Generic.List[string]]::new()
function Assert-Bounded([bool]$Value, [string]$Message) { if (-not $Value) { throw $Message } }
function Assert-ChildGone($Report) {
    $childId = [int](Get-Content -LiteralPath $Report.stdoutPath -Raw).Trim()
    try { $child = [Diagnostics.Process]::GetProcessById($childId) }
    catch [ArgumentException] { return }
    try { Assert-Bounded ($child.WaitForExit(5000)) "Descendant $childId survived job termination." }
    finally { $child.Dispose() }
}
$echoArgs = @('', 'with spaces', 'trailing\', 'quote"inside', 'slashes\\"quote', ([string][char]0x0410))
$echo = & $invoke -Executable $probe -Arguments (@('echo') + $echoArgs) -MemoryLimitMB 128 -TimeoutSeconds 5
$reports.Add($echo.reportPath)
$actual = @(Get-Content -LiteralPath $echo.stdoutPath)
$expected = @($echoArgs | ForEach-Object { [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($_)) })
Assert-Bounded (($actual -join '|') -ceq ($expected -join '|')) 'Argument encoding changed values.'
Assert-Bounded ((Get-Content -LiteralPath $echo.stderrPath -Raw).Trim() -ceq 'probe-stderr') 'stderr capture failed.'
$checks.Add('arguments-and-separate-logs')
$failed = & $invoke -Executable $probe -Arguments 'fail' -AllowFailure -MemoryLimitMB 128 -TimeoutSeconds 5
$reports.Add($failed.reportPath)
Assert-Bounded ($failed.status -ceq 'exit-failed' -and $failed.exitCode -eq 7) 'Native exit code lost.'
$checks.Add('nonzero-exit-evidence')
$threw = $false
try { & $invoke -Executable $probe -Arguments 'fail' -MemoryLimitMB 128 -TimeoutSeconds 5 | Out-Null }
catch { if ($_.Exception.Message -notmatch 'Bounded tool exit-failed.*Report:') { throw }; $threw = $true }
Assert-Bounded $threw 'Default failure mode did not throw.'
$checks.Add('default-failure-throws')
$memory = & $invoke -Executable $probe -Arguments 'memory' -AllowFailure -MemoryLimitMB 128 -TimeoutSeconds 5
$reports.Add($memory.reportPath)
Assert-Bounded ($memory.exitCode -eq 91 -and $memory.peakJobMemoryBytes -le 128MB -and
    (Get-Content -LiteralPath $memory.stdoutPath -Raw).Trim() -ceq 'allocation-denied') 'Job memory limit was not enforced.'
$checks.Add('memory-limit-enforced')
$aggregate = & $invoke -Executable $probe -Arguments 'aggregate' -AllowFailure -MemoryLimitMB 128 -TimeoutSeconds 5
$reports.Add($aggregate.reportPath)
Assert-Bounded ($aggregate.exitCode -eq 91) 'Job-wide aggregate memory limit was not enforced.'
Assert-ChildGone $aggregate
$checks.Add('aggregate-descendant-memory-limit')
$timed = & $invoke -Executable $probe -Arguments @('child', 'wait') -AllowFailure -MemoryLimitMB 128 -TimeoutSeconds 1
$reports.Add($timed.reportPath)
Assert-Bounded ($timed.status -ceq 'timeout' -and $timed.elapsedMilliseconds -lt 6000) 'Timeout was not enforced.'
Assert-ChildGone $timed
$checks.Add('timeout-kills-root-and-descendant')
$orphan = & $invoke -Executable $probe -Arguments 'child' -MemoryLimitMB 128 -TimeoutSeconds 5
$reports.Add($orphan.reportPath)
Assert-ChildGone $orphan
$checks.Add('successful-root-cannot-leave-descendant')
$invalid = Join-Path $run 'invalid.exe'
[IO.File]::WriteAllText($invalid, 'not an executable')
$launch = & $invoke -Executable $invalid -AllowFailure -MemoryLimitMB 128 -TimeoutSeconds 5
$reports.Add($launch.reportPath)
Assert-Bounded ($launch.status -ceq 'launch-failed' -and $launch.error) 'Launch failure not recorded.'
$checks.Add('invalid-executable-fails-closed')
$summary = [pscustomobject]@{ status = 'passed'; checks = @($checks); reports = @($reports); powerShellVersion = $PSVersionTable.PSVersion.ToString(); evidencePath = $run }
[IO.File]::WriteAllText((Join-Path $run 'summary.json'), ($summary | ConvertTo-Json -Depth 4))
$summary
