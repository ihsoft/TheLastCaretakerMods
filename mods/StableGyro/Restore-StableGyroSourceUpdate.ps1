[CmdletBinding()]
param([Parameter(Mandatory)][string]$RecoveryManifest)
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Restore StableGyro source update'
    return
}
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Build/SourceUpdate.psm1') -Force
Restore-StableGyroSourceUpdate -ModRoot $PSScriptRoot -RecoveryManifest $RecoveryManifest | ConvertTo-Json -Compress
