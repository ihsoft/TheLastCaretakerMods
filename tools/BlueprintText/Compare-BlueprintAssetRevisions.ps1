<# Export two native Git asset revisions into disposable projects and render their BPT comparison. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory)][string]$BeforeRevision,
    [string]$AfterRevision='HEAD',
    [Parameter(Mandatory)][string[]]$Asset,
    [ValidateSet('Blueprint','DataAsset')][string]$Kind='Blueprint',
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8'
)
$ErrorActionPreference='Stop'
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$arguments=@('-B',(Join-Path $PSScriptRoot 'git_review.py'),'--project',$ProjectFile,
    '--before',$BeforeRevision,'--after',$AfterRevision,'--output',$OutputRoot,'--kind',$Kind,'--engine',$EngineRoot)
foreach($package in $Asset){$arguments+=@('--asset',$package)}
& $python @arguments
if($LASTEXITCODE){throw 'Git asset review failed; inspect its temporary projects and logs.'}
