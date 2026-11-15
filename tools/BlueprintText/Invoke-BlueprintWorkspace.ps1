<#
.SYNOPSIS
Prepare, export, safely apply, or preview complete compact Blueprint JSON in an existing project.
Preview refreshes project dependencies in Unreal, then renders offline. MarkStubs
persists an explicit role on existing unselected dependencies. Warn before engine work.
AddSource copies a reusable UObject/function-library source to -Asset in the workspace; it is offline.
Collapse extracts explicit -Node IDs from -Graph into -Name; it is offline.
CallTemplates reflects a batch of -Operation native or pure static Blueprint library functions in the selected
source parent context; it launches Unreal but does not edit Blueprint sources/assets.
ModelPreview renders one saved Static Mesh or Actor Blueprint selected by -Asset, from -ProjectFile
or the existing workspace project. It launches Unreal and does not apply pending JSON edits.
ExportDataAsset starts a separate existing-DataAsset workspace. Edit property values in
data-assets/*.json, then use Apply/Preview/Recover. DataAsset Preview is offline.
For a named DataAsset comparison, Preview accepts ComparisonWorkspace and ComparisonLabel
together; the reference is that workspace's initial export, independent of Apply baselines.
Edit workspace.json and blueprints/*.json. Recover restores interrupted asset writes.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Prepare','Export','ExportDataAsset','Apply','Preview','ModelPreview','MarkStubs','Recover','AddSource','Collapse','CallTemplates')][string]$Action,
    [Parameter(Mandatory)][string]$WorkspaceRoot,
    [string]$ProjectFile,
    [string[]]$Asset,
    [string]$SourcePath,
    [string]$Graph,
    [string[]]$Node,
    [string]$Name,
    [hashtable]$BoundaryNames,
    [string[]]$Operation,
    [string]$ComparisonWorkspace,
    [string]$ComparisonLabel,
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
if($SourcePath){$arguments+=@('--source',$SourcePath)}
if($Graph){$arguments+=@('--graph',$Graph)}
if($Name){$arguments+=@('--name',$Name)}
if($ComparisonWorkspace){$arguments+=@('--comparison-workspace',$ComparisonWorkspace)}
if($ComparisonLabel){$arguments+=@('--comparison-label',$ComparisonLabel)}
foreach($identity in $Node){$arguments+=@('--node',$identity)}
foreach($identity in $Operation){$arguments+=@('--operation',$identity)}
if($BoundaryNames){$arguments+=@('--boundary-names',($BoundaryNames | ConvertTo-Json -Compress))}
foreach($item in $Asset){$arguments+=@('--asset',$item)}
& $PythonPath @arguments
if($LASTEXITCODE -ne 0){throw 'Blueprint workspace failed; see the returned reason and diagnostic path.'}
