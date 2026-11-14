# Install the selected candidate and its complete INI through the shared installer.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ReleaseManifest,
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [switch]$AllowDirtySource,
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath (Join-Path $GameRoot 'Voyage/Content/Paks/Railgun.autoload')) {
    throw 'Legacy Railgun.autoload requires review before installation.'
}
& (Join-Path $PSScriptRoot '../../tools/Install-VoyageRelease.ps1') `
    -ReleaseManifest $ReleaseManifest -GameRoot $GameRoot -ExpectedMod Railgun `
    -SettingsArchivePath 'Voyage/Content/Paks/Railgun.ini' `
    -AllowDirtySource:$AllowDirtySource -ValidateOnly:$ValidateOnly
