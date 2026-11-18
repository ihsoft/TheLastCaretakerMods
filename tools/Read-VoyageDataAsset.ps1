[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][ValidatePattern('^/Game/[A-Za-z0-9_/]+$')][string]$Package,
    [Parameter(Mandatory)][ValidatePattern('^/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+$')][string]$ExpectedClass,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot 'unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Read native DataAsset'
    return
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
function Resolve-WorkingPath([string]$Path) {
    $absolute = [IO.Path]::GetFullPath($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path))
    if (-not $absolute.StartsWith((Join-Path $repo 'Tmp') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'DataAsset readback requires a working project and output below repository Tmp.'
    }
    for ($cursor = $absolute; $cursor; $cursor = Split-Path -Parent $cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Linked working path is unsupported: $cursor"
        }
    }
    $absolute
}
$project = Resolve-WorkingPath $ProjectFile
$output = Resolve-WorkingPath $OutputRoot
if ([IO.Path]::GetExtension($project) -ine '.uproject' -or -not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Working Unreal project missing: $project"
}
if (Test-Path -LiteralPath $output) { throw "Readback output must be fresh: $output" }
$null = New-Item -ItemType Directory -Path $output
$workspace = Join-Path $repo ('Tmp/BlueprintText/data-asset-read-' + [guid]::NewGuid().ToString('N'))
& (Join-Path $PSScriptRoot 'BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare `
    -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $workspace > (Join-Path $output 'project-support.json')
$prepare = Join-Path $PSScriptRoot 'Prepare-UnrealEditorSupport.ps1'
& $prepare -ProjectFile $project -EngineRoot $EngineRoot `
    -PluginSource (Join-Path $PSScriptRoot 'AssetPackaging/DataAssetReader/DataAssetReader.uplugin') `
    -OutputRoot (Join-Path $output 'reader-support') > (Join-Path $output 'reader-support.json')
$readbackPath = Join-Path $output 'native.json'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$log = Join-Path $output 'reader.log'
& $editor $project '-run=ReadVoyageDataAsset' ('-Package=' + $Package) ('-ExpectedClass=' + $ExpectedClass) `
    ('-Output=' + $readbackPath) '-unattended' '-nop4' '-nosplash' '-nullrhi' `
    ('-abslog=' + (Join-Path $output 'unreal.log')) *> $log
if ($LASTEXITCODE -ne 0) { throw "Native DataAsset read failed ($LASTEXITCODE); log: $log" }
if (-not (Test-Path -LiteralPath $readbackPath -PathType Leaf)) { throw "Native reader produced no report; log: $log" }
[pscustomobject]@{status='passed';readbackPath=$readbackPath} | ConvertTo-Json -Compress
