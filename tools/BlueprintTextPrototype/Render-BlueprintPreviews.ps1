<#
.SYNOPSIS
Generate all graph/widget previews and a review index from an explicit asset list.
No Unreal launch. Input paths are relative to the manifest. Output must be new or empty.
Manifest assets accept source paths, or source/before records with placeholder,
status (unchanged/added/changed/removed), note, and per-graph reviews ID lists.
.EXAMPLE
.\tools\BlueprintTextPrototype\Render-BlueprintPreviews.ps1 -InputPath previews.json -OutputPath Tmp/BlueprintTextPrototype/previews
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InputPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($PythonPath)){
    $PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    if(-not(Test-Path -LiteralPath $PythonPath)){ $PythonPath=(Get-Command python -ErrorAction Stop).Source }
}
$result=& $PythonPath -B (Join-Path $PSScriptRoot 'render_preview_index.py') (Resolve-Path -LiteralPath $InputPath).Path --output $OutputPath
if($LASTEXITCODE -ne 0){throw 'Blueprint preview batch failed; see diagnostic above.'}
$result | ConvertFrom-Json
