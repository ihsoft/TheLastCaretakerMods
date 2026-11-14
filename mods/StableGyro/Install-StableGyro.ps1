# Install the selected candidate and its complete INI through the shared installer.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ReleaseManifest,
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [switch]$AllowDirtySource,
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot '../../tools/Install-VoyageRelease.ps1') `
    -ReleaseManifest $ReleaseManifest -GameRoot $GameRoot -ExpectedMod StableGyro `
    -SettingsArchivePath 'StableGyro.ini' `
    -AllowDirtySource:$AllowDirtySource -ValidateOnly:$ValidateOnly
