# Execute editor checks on a disposable copy. Wrap the whole script in Invoke-UnrealQueued.
[CmdletBinding()]
param([string]$EngineRoot = 'P:/UnrealEngine/UE_5.8', [string]$OutputRoot)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$owner = Join-Path $repo 'Tmp/SmartOutlet'
if (-not $OutputRoot) { $OutputRoot = Join-Path $owner ('test-editor-' + [guid]::NewGuid().ToString('N')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not $output.StartsWith($owner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw "OutputRoot must be below $owner" }
if (Test-Path -LiteralPath $output) { throw "Use a fresh output directory: $output" }
[IO.Directory]::CreateDirectory($output) | Out-Null
& (Join-Path $PSScriptRoot 'Tests/ValidateSettingsMigration.ps1') | Out-Null
& (Join-Path $PSScriptRoot 'Tests/ValidateInstallation.ps1') | Out-Null
$project = & (Join-Path $PSScriptRoot 'New-SmartOutletProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$workspace = Join-Path $repo ('Tmp/BlueprintText/SmartOutlet-tests-' + [guid]::NewGuid().ToString('N'))
& (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $workspace > (Join-Path $output 'prepare.log')
$report = Join-Path $output 'checks-validation.json'
$log = Join-Path $output 'checks-validation.log'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Assets/SmartOutlet.ini') -Destination (Join-Path $output 'SmartOutlet.ini')
$previousOutput = $env:SMARTOUTLET_TEST_OUTPUT
try {
    $env:SMARTOUTLET_TEST_OUTPUT = $output
    & (Join-Path $repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Validate SmartOutlet Blueprints' `
        -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
        -ArgumentList @($project, '-run=pythonscript', ('-script=' + (Join-Path $output 'Project/Tests/RunAll.py')),
            '-unattended', '-nop4', '-nosplash', '-nullrhi', ('-abslog=' + $log)) `
        > (Join-Path $output 'checks-validation-console.log')
} finally { $env:SMARTOUTLET_TEST_OUTPUT = $previousOutput }
if (-not (Test-Path -LiteralPath $report)) { throw "Editor checks did not complete. See $log" }
$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if ($result.status -cne 'passed' -or @($result.tests).Count -ne 6) { throw "Editor checks failed. See $report" }
$projectPath = (Resolve-Path -LiteralPath (Split-Path $project -Parent)).Path
if (-not $projectPath.StartsWith($output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected project cleanup path' }
Remove-Item -LiteralPath $projectPath -Recurse -Force
Remove-Item -LiteralPath (Join-Path $output 'SmartOutlet.ini')
[pscustomobject]@{status='passed';suites=@($result.tests).Count;reportPath=$report;boundary=$result.boundary}
