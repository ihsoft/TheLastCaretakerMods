[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ModRoot,
    [Parameter(Mandatory = $true)][string]$ActualSteamBuildId,
    [switch]$IgnorePinnedGameVersion
)

$ErrorActionPreference = 'Stop'
$manifestPath = Join-Path $ModRoot 'game-build.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Mod game-build manifest missing: $manifestPath"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$fields = @($manifest.PSObject.Properties.Name | Sort-Object)
if ($manifest.schemaVersion -ne 1 -or
    (Compare-Object $fields @('pinnedSteamBuildId','schemaVersion')) -or
    [string]$manifest.pinnedSteamBuildId -cnotmatch '^[0-9]+$' -or
    $ActualSteamBuildId -cnotmatch '^[0-9]+$') {
    throw "Invalid game-build manifest or installed Steam build ID: $manifestPath"
}
$pinned = [string]$manifest.pinnedSteamBuildId
$matches = $ActualSteamBuildId -ceq $pinned
if (-not $matches -and -not $IgnorePinnedGameVersion) {
    throw "Installed Steam build ID $ActualSteamBuildId differs from $pinned for $ModRoot. Retry the same build with -IgnorePinnedGameVersion; this bypasses only the pinned ID comparison."
}
[pscustomobject]@{
    pinnedSteamBuildId = $pinned
    actualSteamBuildId = $ActualSteamBuildId
    ignoredPinnedGameVersion = [bool](-not $matches -and $IgnorePinnedGameVersion)
}
