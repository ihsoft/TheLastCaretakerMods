[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Operation,
    [Parameter(Mandatory)][string]$FilePath,
    [string[]]$ArgumentList=@(),
    [ValidateRange(0,2147483647)][int]$TimeoutSeconds=0
)
$ErrorActionPreference='Stop'
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$arguments=@('-B',(Join-Path $PSScriptRoot 'unreal_coordination.py'),'--operation',$Operation)
if($TimeoutSeconds -gt 0){$arguments+=@('--timeout',[string]$TimeoutSeconds)}
& $python @arguments -- $FilePath @ArgumentList
if($LASTEXITCODE -ne 0){throw "Queued Unreal operation failed ($LASTEXITCODE): $Operation"}
