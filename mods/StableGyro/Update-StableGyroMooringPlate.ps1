# Explicitly refresh the one maintained plate; never called by the release build.
# Run through tools/Invoke-UnrealQueued.ps1. No installation is performed.
[CmdletBinding()]
param(
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$BlenderPath = 'K:/Program Files/Blender Foundation/Blender 5.2/blender.exe',
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion
)
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'StableGyro source/build operation'
    return
}
& (Join-Path $PSScriptRoot 'Build/Update-StableGyroMooringPlate.ps1') `
    -GameRoot $GameRoot -EngineRoot $EngineRoot -BlenderPath $BlenderPath `
    -OutputRoot $OutputRoot -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
