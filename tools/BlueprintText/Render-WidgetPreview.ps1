<#
.SYNOPSIS
Render a schema-2 widget tree as a standalone offline HTML preview.
Export it first with Invoke-BlueprintAssetText.ps1 -Action Export -PreviewResources.
Consumes that widget export and its adjacent .resources/manifest.json.
PNG resources come from editor Texture2D assets, never mod source-image paths.
Visibility checkboxes affect only the preview; bindings/animations do not run.
Unreal is not launched by this command. Requires Python 3.9+.
.EXAMPLE
.\tools\BlueprintText\Render-WidgetPreview.ps1 -InputPath Tmp/BlueprintText/HUD.json -OutputPath Tmp/BlueprintText/HUD.html
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InputPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$ResourcesPath,
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if([string]::IsNullOrWhiteSpace($PythonPath)){
    $PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    if(-not(Test-Path -LiteralPath $PythonPath)){ $PythonPath=(Get-Command python -ErrorAction Stop).Source }
}
$arguments=@('-B',(Join-Path $PSScriptRoot 'render_widget.py'),(Resolve-Path -LiteralPath $InputPath).Path,'--output',$OutputPath)
if($ResourcesPath){$arguments+=@('--resources',(Resolve-Path -LiteralPath $ResourcesPath).Path)}
$result=& $PythonPath @arguments
if($LASTEXITCODE -ne 0){throw 'Widget preview failed; see diagnostic above.'}
$result | ConvertFrom-Json
