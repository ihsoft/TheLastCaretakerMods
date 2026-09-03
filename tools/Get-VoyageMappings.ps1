[CmdletBinding()]
param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$testMappingsScript = Join-Path $PSScriptRoot 'Test-VoyageMappings.ps1'
$registryRoot = Join-Path $PSScriptRoot '..\mappings\Voyage'
$fingerprintText = (& $fingerprintScript -GameRoot $GameRoot) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$steamBuildId = [string]$fingerprint.steam.buildId
$executableSha256 = [string]$fingerprint.executable.sha256
if ($steamBuildId -notmatch '^\d+$' -or $executableSha256 -notmatch '^[0-9A-F]{64}$') {
    throw 'The installed game fingerprint has no valid Steam build ID or executable SHA-256.'
}

$root = (Resolve-Path -LiteralPath $registryRoot).Path
$candidates = [Collections.Generic.List[object]]::new()
foreach ($manifestFile in Get-ChildItem -LiteralPath $root -Filter 'mapping-manifest.json' -File -Recurse) {
    try {
        $manifest = Get-Content -Raw -LiteralPath $manifestFile.FullName | ConvertFrom-Json
        if ([string]$manifest.kind -cne 'Voyage reflection mappings' -or
            [string]$manifest.steamBuildId -cne $steamBuildId -or
            [string]$manifest.executableSha256 -cne $executableSha256) {
            continue
        }
        $mappingName = if ([string]::IsNullOrWhiteSpace([string]$manifest.mappingFile)) {
            'Mappings.usmap'
        }
        else {
            [string]$manifest.mappingFile
        }
        $mappingPath = Join-Path $manifestFile.DirectoryName $mappingName
        if (-not (Test-Path -LiteralPath $mappingPath -PathType Leaf)) {
            continue
        }
        $generatedAtUtc = [DateTime]::MinValue
        [void][DateTime]::TryParse([string]$manifest.generatedAtUtc, [ref]$generatedAtUtc)
        $candidates.Add([pscustomobject]@{
            mappingsPath = $mappingPath
            manifestPath = $manifestFile.FullName
            generatedAtUtc = $generatedAtUtc.ToUniversalTime()
            engineVersion = [string]$manifest.engineVersion
            uassetApiEngineVersion = [string]$manifest.uassetApiEngineVersion
        })
    }
    catch {
        continue
    }
}

$validationFailures = [Collections.Generic.List[string]]::new()
foreach ($candidate in $candidates | Sort-Object generatedAtUtc -Descending) {
    try {
        $validation = & $testMappingsScript `
            -MappingsPath $candidate.mappingsPath `
            -ManifestPath $candidate.manifestPath `
            -ExpectedSteamBuildId $steamBuildId `
            -ExpectedExecutableSha256 $executableSha256
        [pscustomobject]@{
            mappingsPath = (Resolve-Path -LiteralPath $candidate.mappingsPath).Path
            manifestPath = (Resolve-Path -LiteralPath $candidate.manifestPath).Path
            sha256 = [string]$validation.sha256
            length = [long]$validation.length
            steamBuildId = $steamBuildId
            executableSha256 = $executableSha256
            engineVersion = $candidate.engineVersion
            uassetApiEngineVersion = $candidate.uassetApiEngineVersion
        }
        return
    }
    catch {
        $validationFailures.Add("$($candidate.manifestPath): $($_.Exception.Message)")
        continue
    }
}

if ($validationFailures.Count -gt 0) {
    throw "A reviewed Voyage mapping matches the installed fingerprint but failed validation. Do not regenerate or replace it; diagnose the resolver/registry failure:`n$($validationFailures -join [Environment]::NewLine)"
}
throw "No reviewed Voyage mapping matches Steam build $steamBuildId and executable $executableSha256. Do not regenerate automatically: confirm that the installed fingerprint is new, ask the user to start the game, then run tools\New-VoyageMappings.ps1."
