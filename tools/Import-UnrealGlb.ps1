# Import a rigid GLB assembly as project assets and a graph-free Actor Blueprint.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$Destination,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot 'unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Import complete GLB model'
    return
}
$python = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python -B (Join-Path $PSScriptRoot 'UnrealGlbImport/glb_request.py') --project $ProjectFile --source $Source --destination $Destination --output $OutputRoot
if ($LASTEXITCODE -ne 0) { throw 'GLB request rejected before Unreal launch.' }
$output = [IO.Path]::GetFullPath($OutputRoot)
$request = Get-Content -LiteralPath (Join-Path $output 'request.json') -Raw | ConvertFrom-Json
$workspace = Join-Path $PSScriptRoot ('../Tmp/BlueprintText/glb-import-' + [Guid]::NewGuid().ToString('N'))
& (Join-Path $PSScriptRoot 'BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare `
    -ProjectFile $request.project -WorkspaceRoot $workspace -EngineRoot $EngineRoot > (Join-Path $output 'project-support.json')
if ($LASTEXITCODE -ne 0) { throw "Project dependency preparation failed; diagnostics: $output" }
& (Join-Path $PSScriptRoot 'Prepare-UnrealEditorSupport.ps1') -ProjectFile $request.project `
    -PluginSource (Join-Path $PSScriptRoot 'UnrealGlbImport/EditorSupport/UnrealGlbImportSupport.uplugin') `
    -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'editor-support') > (Join-Path $output 'editor-support.json')
if ($LASTEXITCODE -ne 0) { throw "GLB editor support preparation failed; diagnostics: $output" }
Import-Module (Join-Path $PSScriptRoot 'VoyageModPipeline.psm1') -Force
$projectRoot = Split-Path -Parent $request.project
$before = @{}
if (Test-Path -LiteralPath (Join-Path $projectRoot 'Content')) { $before = Get-VoyageProjectContentSnapshot -ProjectRoot $projectRoot }
$oldRequest = $env:UNREAL_GLB_REQUEST
$oldPhase = $env:UNREAL_GLB_PHASE
try {
    $env:UNREAL_GLB_REQUEST = Join-Path $output 'request.json'
    foreach ($phase in @('import', 'reopen')) {
        $env:UNREAL_GLB_PHASE = $phase
        & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $request.project `
            '-run=pythonscript' ('-script=' + (Join-Path $PSScriptRoot 'UnrealGlbImport/editor.py')) `
            '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,Interchange,InterchangeEditor' `
            '-unattended' '-nop4' '-nosplash' '-nullrhi' `
            ('-abslog=' + (Join-Path $output ($phase + '.log'))) *> (Join-Path $output ($phase + '-console.log'))
        if ($LASTEXITCODE -ne 0) { throw "GLB $phase failed ($LASTEXITCODE); diagnostics: $output" }
        $report = Get-Content -LiteralPath (Join-Path $output ($phase + '.json')) -Raw | ConvertFrom-Json
        if ($report.status -cne 'passed') { throw "GLB $phase failed; diagnostics: $output" }
    }
    $prefix = 'Content/' + $request.destination.Substring('/Game/'.Length) + '/'
    $after = Get-VoyageProjectContentSnapshot -ProjectRoot $projectRoot
    $allowed = @($after.Keys | Where-Object { $_.Replace('\','/').StartsWith($prefix, [StringComparison]::Ordinal) })
    Assert-VoyageProjectContentUpdateScope -ProjectRoot $projectRoot -Before $before -AllowedPaths $allowed
    & (Join-Path $PSScriptRoot 'BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Export `
        -ProjectFile $request.project -WorkspaceRoot $workspace -Asset ($request.destination + '/BP_Model') `
        -EngineRoot $EngineRoot > (Join-Path $output 'bpt-export.json')
    if ($LASTEXITCODE -ne 0) { throw "GLB Blueprint text export failed; diagnostics: $output" }
    [ordered]@{ status='passed'; blueprint=$report.blueprint; reportPath=(Join-Path $output 'reopen.json');
        blueprintWorkspace=(Join-Path ([IO.Path]::GetFullPath($workspace)) 'workspace.json'); outputRoot=$output } | ConvertTo-Json
} finally {
    $env:UNREAL_GLB_REQUEST = $oldRequest
    $env:UNREAL_GLB_PHASE = $oldPhase
}
