[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$Recipe,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8',
    [string]$GameRoot='P:/SteamLibrary/steamapps/common/Voyage',
    [string]$OutputRoot
)
$ErrorActionPreference='Stop'
$repository=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if(-not $OutputRoot) { $OutputRoot=Join-Path $repository ('Tmp/VoyageItemPatch/update-'+[guid]::NewGuid().ToString('N')) }
$arguments=@('-NoProfile','-File',(Join-Path $PSScriptRoot 'VoyageItemPatch/Update-Item.ps1'),
    '-Project',[IO.Path]::GetFullPath($Project),'-Recipe',[IO.Path]::GetFullPath($Recipe),
    '-EngineRoot',[IO.Path]::GetFullPath($EngineRoot),'-GameRoot',[IO.Path]::GetFullPath($GameRoot),
    '-OutputRoot',[IO.Path]::GetFullPath($OutputRoot))
& (Join-Path $PSScriptRoot 'Invoke-UnrealQueued.ps1') -Operation 'Refresh project VoyageItem from recipe' -FilePath pwsh -ArgumentList $arguments
