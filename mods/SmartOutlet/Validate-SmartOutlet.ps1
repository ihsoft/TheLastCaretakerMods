# Execute all maintained editor checks. Wrap the whole script in Invoke-UnrealQueued.
[CmdletBinding()]
param([string]$EngineRoot = 'P:/UnrealEngine/UE_5.8')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
& (Join-Path $PSScriptRoot 'Tests/ValidateSettingsMigration.ps1') | Out-Null
$project = Join-Path $PSScriptRoot 'Voyage.uproject'
$output = Join-Path $repo 'Tmp/SmartOutlet'
[IO.Directory]::CreateDirectory($output) | Out-Null
$report = Join-Path $output 'checks-validation.json'
if (Test-Path -LiteralPath $report) { Remove-Item -LiteralPath $report }
& (Join-Path $repo 'tools/Prepare-UnrealEditorSupport.ps1') -ProjectFile $project -EngineRoot $EngineRoot | Out-Null
$log = Join-Path $output 'checks-validation.log'
& (Join-Path $repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Validate SmartOutlet Blueprints' `
    -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
    -ArgumentList @($project, '-run=pythonscript', ('-script=' + (Join-Path $PSScriptRoot 'Tests/RunAll.py')),
        '-unattended', '-nop4', '-nosplash', '-nullrhi', ('-abslog=' + $log)) `
    > (Join-Path $output 'checks-validation-console.log')
if (-not (Test-Path -LiteralPath $report)) { throw "Editor checks did not complete. See $log" }
$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if ($result.status -cne 'passed' -or @($result.tests).Count -ne 6) { throw "Editor checks failed. See $report" }
$result
