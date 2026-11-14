[CmdletBinding()]
param(
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion
)
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Update StableGyro item'
    return
}
& (Join-Path $PSScriptRoot 'Build/Update-StableGyroItemAsset.ps1') `
    -GameRoot $GameRoot -EngineRoot $EngineRoot -OutputRoot $OutputRoot `
    -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
