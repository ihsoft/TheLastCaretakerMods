[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$ReleaseManifest,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [switch]$ReleaseCandidate,
    [switch]$AllowDirtySource,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$manifest = Get-Content -LiteralPath $ReleaseManifest -Raw | ConvertFrom-Json
if ([string]$manifest.mod -cne 'PalletContainerSnap') {
    throw 'The release manifest does not belong to PalletContainerSnap.'
}
& (Join-Path $repo 'tools\Install-VoyageRelease.ps1') `
    -ReleaseManifest $ReleaseManifest -GameRoot $GameRoot -LogicMods `
    -ReleaseCandidate:$ReleaseCandidate -AllowDirtySource:$AllowDirtySource `
    -ValidateOnly:$ValidateOnly
