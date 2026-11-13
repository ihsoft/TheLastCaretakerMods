# Reserve Invoke-UnrealQueued for the whole test operation.
[CmdletBinding()]
param([string]$EngineRoot = 'P:/UnrealEngine/UE_5.8', [string]$OutputRoot)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../../tools/VoyageModPipeline.psm1')
Invoke-VoyageModEditorTest -ModRoot $PSScriptRoot -EngineRoot $EngineRoot -OutputRoot $OutputRoot `
    -ReportName 'editor-validation.json' -Run {
    param($run)
    $previousRun = $env:CONTAINER_LID_LABEL_TEST_RUN
    try {
        $env:CONTAINER_LID_LABEL_TEST_RUN = $run.output
        & (Join-Path $run.repo 'tools/Invoke-UnrealQueued.ps1') -Operation 'Test ContainerLidLabel' `
            -FilePath (Join-Path $run.engineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') `
            -ArgumentList @($run.project, ('-ExecutePythonScript=' + (Join-Path $run.projectRoot 'Tests/ValidateEditor.py')),
                '-unattended','-nop4','-nosplash','-nullrhi','-RenderOffscreen','-ddc=NoZenLocalFallback',
                ('-abslog=' + (Join-Path $run.output 'editor.log')))
    } finally { $env:CONTAINER_LID_LABEL_TEST_RUN = $previousRun }
} | ConvertTo-Json -Depth 8
