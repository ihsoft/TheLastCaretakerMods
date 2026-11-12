# Run through Invoke-UnrealQueued.ps1, keeping the reservation for the whole test.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$OutputRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$owner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/ContainerLidLabel'))
if (-not $OutputRoot) { $OutputRoot = Join-Path $owner ('test-editor-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
if ((Split-Path -Parent $output) -ne $owner) { throw 'OutputRoot must be a direct child of Tmp/ContainerLidLabel' }
if (Test-Path -LiteralPath $output) { throw 'Use a fresh test output directory' }
$project = & (Join-Path $PSScriptRoot 'New-ContainerLidLabelProjectCopy.ps1') -Destination (Join-Path $output 'Project')
$projectRoot = Split-Path -Parent $project
$workspace = Join-Path $repo ('Tmp/BlueprintText/ContainerLidLabel-test-' + [guid]::NewGuid().ToString('N'))
$previousRun = $env:CONTAINER_LID_LABEL_TEST_RUN
try {
    & (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $workspace
    $env:CONTAINER_LID_LABEL_TEST_RUN = $output
    & (Join-Path $repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Validate ContainerLidLabel copy' `
        -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') `
        -ArgumentList @($project, ('-ExecutePythonScript=' + (Join-Path $projectRoot 'Tests/ValidateEditor.py')),
            '-unattended', '-nop4', '-nosplash', '-nullrhi', '-RenderOffscreen', '-ddc=NoZenLocalFallback',
            ('-abslog=' + (Join-Path $output 'editor.log'))) > (Join-Path $output 'editor-console.log')
    $reportPath = Join-Path $output 'editor-validation.json'
    $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    if ($report.status -ne 'passed') { throw "Editor assertions failed: $reportPath" }
    $resolvedProject = (Resolve-Path -LiteralPath $projectRoot).Path
    if ($resolvedProject -ne (Join-Path $output 'Project')) { throw 'Unsafe test scratch path' }
    Remove-Item -LiteralPath $resolvedProject -Recurse -Force
    [ordered]@{ status='passed'; report=$reportPath; evidence=$output; bptWorkspace=$workspace } | ConvertTo-Json
}
catch {
    Write-Warning "Test failed; temporary project and diagnostics remain at $output (BPT workspace: $workspace)"
    throw
}
finally { $env:CONTAINER_LID_LABEL_TEST_RUN = $previousRun }
