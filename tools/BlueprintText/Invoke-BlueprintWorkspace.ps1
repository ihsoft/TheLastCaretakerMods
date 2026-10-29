<#
.SYNOPSIS
Prepare, export, safely apply, or preview complete compact Blueprint JSON in an existing project.
Preview refreshes project dependencies in Unreal, then renders offline. MarkStubs
persists an explicit role on existing unselected dependencies. Warn before engine work.
Edit workspace.json and blueprints/*.json. Recover restores interrupted asset writes.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Prepare','Export','Apply','Preview','MarkStubs','Recover')][string]$Action,
    [Parameter(Mandatory)][string]$WorkspaceRoot,
    [string]$ProjectFile,
    [string[]]$Asset,
    [string]$EngineRoot='K:\Epic Games\UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
if(-not $PythonPath){
    $PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    if(-not(Test-Path -LiteralPath $PythonPath)){$PythonPath=(Get-Command python -ErrorAction Stop).Source}
}
$arguments=@('-B',(Join-Path $PSScriptRoot 'blueprint_workspace.py'),$Action,'--workspace',$WorkspaceRoot,'--engine',$EngineRoot)
if($ProjectFile){$arguments+=@('--project',$ProjectFile)}
foreach($item in $Asset){$arguments+=@('--asset',$item)}
& $PythonPath @arguments
if($LASTEXITCODE -ne 0){throw 'Blueprint workspace failed; see the returned reason and diagnostic path.'}
