# Read-only snapshot, not an install preflight or gameplay validation.
[CmdletBinding()]
param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [string]$InstallManifest,
    [switch]$HashModFiles,
    [switch]$Summary
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-Value($Object, [string]$Name) {
    $property = $Object.PSObject.Properties[$Name]
    if ($null -ne $property) { $property.Value }
}
function Get-Snapshot([string]$Path, [bool]$Hash) {
    if (-not (Test-Path -LiteralPath $Path)) {
        return [pscustomobject]@{ state = 'missing'; size = $null; sha256 = $null }
    }
    $item = Get-Item -LiteralPath $Path -Force
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        return [pscustomobject]@{ state = 'unsupported-path'; size = $null; sha256 = $null }
    }
    $digest = $null
    if ($Hash) { $digest = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
    $after = Get-Item -LiteralPath $Path -Force
    $state = 'present'
    if ($item.Length -ne $after.Length -or $item.LastWriteTimeUtc -ne $after.LastWriteTimeUtc) {
        $state = 'changed-during-read'
    }
    [pscustomobject]@{ state = $state; size = $after.Length; sha256 = $digest }
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$paks = Join-Path $root 'Voyage\Content\Paks'
$fingerprint = & (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $root |
    ConvertFrom-Json
$processes = @(
    Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue |
        ForEach-Object {
            $path = $null
            try { $path = $_.Path } catch { }
            [pscustomobject]@{ id = $_.Id; name = $_.ProcessName; executablePath = $path }
        }
)
# This inventory deliberately does not interpret the fingerprint tool's legacy
# container exclusions. Stock classification is a filename convention, not trust.
$files = @(
    Get-ChildItem -LiteralPath $paks -Force -File | Sort-Object Name |
        Where-Object { $_.Extension -in @('.pak', '.utoc', '.ucas', '.zip') } |
        ForEach-Object {
            $stock = $_.Name -match '^(global|pakchunk\d+(optional)?-Windows)(\.pak|\.utoc|(?:_s\d+)?\.ucas)$'
            $snapshot = Get-Snapshot $_.FullName ($HashModFiles -and -not $stock)
            [pscustomobject]@{
                name = $_.Name; path = $_.FullName
                category = $(if ($stock) { 'stock-name' } else { 'additional' })
                state = $snapshot.state; size = $snapshot.size; sha256 = $snapshot.sha256
            }
        }
)
$subdirectories = @(Get-ChildItem -LiteralPath $paks -Force -Directory |
    Select-Object -ExpandProperty FullName)
$comparison = $null
if ($InstallManifest) {
    $manifestPath = (Resolve-Path -LiteralPath $InstallManifest).Path
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ((Get-Value $manifest 'schemaVersion') -ne 1 -or
        (Get-Value $manifest 'kind') -cne 'Voyage release installation' -or
        (Get-Value $manifest 'status') -cne 'installed') {
        throw 'Expected a completed common installation manifest (schema 1).'
    }
    if ([string](Get-Value $manifest 'gameRoot') -ine $root -or
        [string](Get-Value $manifest 'paksDirectory') -ine $paks) {
        throw 'Installation manifest belongs to a different game/Paks directory.'
    }
    $records = @(Get-Value $manifest 'files')
    if ($records.Count -eq 0) { throw 'Installation manifest has no file records.' }
    $seen = @{}
    # Validate the entire record set before hashing any record-supplied target.
    foreach ($record in $records) {
        $name = [string](Get-Value $record 'name')
        if ([string]::IsNullOrWhiteSpace($name) -or
            [IO.Path]::GetFileName($name) -cne $name -or
            $name.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or
            $seen.ContainsKey($name)) { throw 'Unsafe or duplicate installation filename.' }
        $seen[$name] = $true
        if ([string](Get-Value $record 'destinationPath') -ine (Join-Path $paks $name) -or
            [string](Get-Value $record 'installedSha256') -notmatch '^[0-9a-fA-F]{64}$' -or
            $null -eq (Get-Value $record 'size') -or [long]$record.size -lt 0) {
            throw "Invalid installation file record: $name"
        }
    }
    $results = @(foreach ($record in $records) {
        $snapshot = Get-Snapshot (Join-Path $paks $record.name) $true
        $state = $snapshot.state
        if ($state -eq 'present') {
            $state = if ($snapshot.sha256 -ieq $record.installedSha256 -and
                $snapshot.size -eq $record.size) { 'match' } else { 'different' }
        }
        [pscustomobject]@{
            name = $record.name; state = $state; size = $snapshot.size
            expectedSha256 = $record.installedSha256; actualSha256 = $snapshot.sha256
        }
    })
    $comparison = [pscustomobject]@{
        manifestPath = $manifestPath
        mod = Get-Value $manifest 'mod'
        artifactVersion = Get-Value $manifest 'artifactVersion'
        filesMatch = @($results | Where-Object state -ne 'match').Count -eq 0
        gameFingerprintMatches = (
            [string](Get-Value $manifest 'steamBuildId') -ceq [string]$fingerprint.steam.buildId -and
            [string](Get-Value $manifest 'executableSha256') -ieq $fingerprint.executable.sha256)
        files = $results
    }
}
$fullResult = [pscustomobject][ordered]@{
    schemaVersion = 1
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    gameRoot = $root
    steamBuildId = $fingerprint.steam.buildId
    executableSha256 = $fingerprint.executable.sha256
    gameProcessObserved = $processes.Count -gt 0
    processes = $processes
    inventoryScope = 'Paks top-level only; not a runtime mount or compatibility check'
    unscannedSubdirectories = $subdirectories
    files = $files
    installation = $comparison
}
if ($Summary) {
    $additionalFiles = @($files | Where-Object category -eq 'additional' | ForEach-Object {
        [pscustomobject]@{
            name = $_.name
            state = $_.state
            size = $_.size
            sha256 = $_.sha256
        }
    })
    $installationSummary = $null
    if ($null -ne $comparison) {
        $installationSummary = [pscustomobject]@{
            manifestPath = $comparison.manifestPath
            mod = $comparison.mod
            artifactVersion = $comparison.artifactVersion
            filesMatch = $comparison.filesMatch
            gameFingerprintMatches = $comparison.gameFingerprintMatches
            fileCount = @($comparison.files).Count
            mismatchCount = @($comparison.files | Where-Object state -ne 'match').Count
        }
    }
    [pscustomobject][ordered]@{
        schemaVersion = 1
        capturedAtUtc = $fullResult.capturedAtUtc
        gameRoot = $fullResult.gameRoot
        steamBuildId = $fullResult.steamBuildId
        executableSha256 = $fullResult.executableSha256
        gameProcessObserved = $fullResult.gameProcessObserved
        processCount = @($fullResult.processes).Count
        topLevelFileCount = @($fullResult.files).Count
        additionalFileCount = $additionalFiles.Count
        additionalFiles = $additionalFiles
        unscannedSubdirectoryCount = @($fullResult.unscannedSubdirectories).Count
        installation = $installationSummary
        detail = 'summary; rerun without -Summary only when full process/file inventory is required'
    } | ConvertTo-Json -Depth 6 -Compress
}
else {
    $fullResult | ConvertTo-Json -Depth 8
}
