<#
.SYNOPSIS
Build or reuse a native support plugin and attach editor binaries to an Unreal project.
#>
[CmdletBinding(DefaultParameterSetName='Package')]
param(
    [Parameter(Mandatory)][string]$ProjectFile,
    [Parameter(Mandatory,ParameterSetName='Source')][string]$PluginSource,
    [Parameter(Mandatory,ParameterSetName='Package')][string]$PluginPackage,
    [Parameter(Mandatory,ParameterSetName='Recover')][switch]$Recover,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8',
    [string]$OutputRoot
)
$ErrorActionPreference='Stop'
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$arguments=@('-B',(Join-Path $PSScriptRoot 'UnrealEditorSupport/editor_support.py'),
    '--project',[IO.Path]::GetFullPath($ProjectFile),'--engine',[IO.Path]::GetFullPath($EngineRoot))
if($PluginSource) { $arguments+=@('--source',[IO.Path]::GetFullPath($PluginSource)) }
if($PluginPackage) { $arguments+=@('--package',[IO.Path]::GetFullPath($PluginPackage)) }
if($Recover) { $arguments+='--recover' }
if($OutputRoot) { $arguments+=@('--output',[IO.Path]::GetFullPath($OutputRoot)) }
& (Join-Path $PSScriptRoot 'Invoke-UnrealQueued.ps1') -Operation 'Prepare Unreal editor support' -FilePath $python -ArgumentList $arguments
