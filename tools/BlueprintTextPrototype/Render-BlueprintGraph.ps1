<#
.SYNOPSIS
Render compact Blueprint JSON as offline SVG and an HTML viewer, without Unreal.
Defaults to simplified top-to-bottom. HTML can switch both view and direction;
an adjacent .full.svg always preserves access to every node and link.
.EXAMPLE
.\tools\BlueprintTextPrototype\Render-BlueprintGraph.ps1 -InputPath blueprint.json -OutputPath Tmp/BlueprintTextPrototype/diagram.svg
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InputPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$Graph,
    [ValidateSet('simplified','full')][string]$View = 'simplified',
    [ValidateSet('TB','LR')][string]$Direction = 'TB',
    [string]$PythonPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ([string]::IsNullOrWhiteSpace($PythonPath)) {
    $bundledPython = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    if (Test-Path -LiteralPath $bundledPython -PathType Leaf) {
        $PythonPath = $bundledPython
    } else {
        $command = Get-Command python -ErrorAction SilentlyContinue |
            Where-Object { $_.Source -notlike '*\WindowsApps\*' } | Select-Object -First 1
        if ($null -eq $command) { throw 'Python 3.9+ is required; supply -PythonPath with the interpreter path.' }
        $PythonPath = $command.Source
    }
}
$arguments = @('-B', (Join-Path $PSScriptRoot 'render_graph.py'), (Resolve-Path -LiteralPath $InputPath).Path, '--output', $OutputPath, '--view', $View, '--direction', $Direction)
if (-not [string]::IsNullOrWhiteSpace($Graph)) { $arguments += @('--graph', $Graph) }
$result = & $PythonPath @arguments
if ($LASTEXITCODE -ne 0) { throw 'Graph rendering failed; see the diagnostic above.' }
$result | ConvertFrom-Json
