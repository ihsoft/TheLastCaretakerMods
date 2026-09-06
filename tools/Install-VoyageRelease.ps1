# Installs an already-built standalone Voyage IoStore release from its manifest.
# This script never builds, cooks, repackages, or edits the source release.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$ReleaseManifest,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EvidenceRoot,

    [switch]$AllowDirtySource,

    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$sha256Pattern = '^[0-9A-F]{64}$'
$containerExtensions = @('.pak', '.ucas', '.utoc')
$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $repositoryRoot 'artifacts\installations'
}

function Get-OptionalPropertyValue {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Object,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    $property.Value
}

function Assert-SafeLeafName {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Name) -or
        [IO.Path]::GetFileName($Name) -cne $Name -or
        $Name.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0) {
        throw "$Label is not a safe leaf filename: $Name"
    }
}

function Assert-GameClosed {
    $running = @(
        Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' `
            -ErrorAction SilentlyContinue
    )
    if ($running.Count -gt 0) {
        $processes = ($running | ForEach-Object {
            "$($_.ProcessName):$($_.Id)"
        }) -join ', '
        throw "Voyage must be closed before installing a release: $processes"
    }
}

function Resolve-ManifestArtifact {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ReleaseRoot,

        [Parameter(Mandatory = $true)]
        [object]$Record,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    $name = [string](Get-OptionalPropertyValue -Object $Record -Name 'name')
    $expectedHash = [string](Get-OptionalPropertyValue -Object $Record -Name 'sha256')
    $expectedSizeValue = Get-OptionalPropertyValue -Object $Record -Name 'size'
    Assert-SafeLeafName -Name $name -Label "$Label name"
    if ($expectedHash -cnotmatch $sha256Pattern) {
        throw "$Label has an invalid SHA-256: $expectedHash"
    }
    if ($null -eq $expectedSizeValue) {
        throw "$Label has no size."
    }
    $expectedSize = [Convert]::ToInt64(
        $expectedSizeValue,
        [Globalization.CultureInfo]::InvariantCulture)
    if ($expectedSize -lt 0) {
        throw "$Label has an invalid size: $expectedSize"
    }

    $declaredPath = [string](Get-OptionalPropertyValue -Object $Record -Name 'path')
    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($declaredPath)) {
        $candidate = [IO.Path]::GetFullPath((Join-Path $ReleaseRoot $declaredPath))
        $rootPrefix = $ReleaseRoot.TrimEnd('\') + '\'
        if (-not $candidate.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "$Label path escapes the release root: $declaredPath"
        }
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $candidates = @((Get-Item -LiteralPath $candidate))
        }
    }
    else {
        $candidates = @(
            Get-ChildItem -LiteralPath $ReleaseRoot -Recurse -File -Filter $name |
                Sort-Object FullName
        )
    }

    $valid = @(
        foreach ($candidate in $candidates) {
            if ($candidate.Name -cne $name -or $candidate.Length -ne $expectedSize) {
                continue
            }
            $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidate.FullName).Hash
            if ($actualHash -ceq $expectedHash) {
                $candidate
            }
        }
    )
    if ($valid.Count -lt 1) {
        throw "$Label could not be resolved with its declared size and hash below $ReleaseRoot"
    }

    [pscustomobject][ordered]@{
        name = $name
        path = $valid[0].FullName
        size = $expectedSize
        sha256 = $expectedHash
    }
}

function Write-JsonFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [object]$Value
    )

    [IO.File]::WriteAllText(
        $Path,
        (($Value | ConvertTo-Json -Depth 10) + [Environment]::NewLine),
        [Text.UTF8Encoding]::new($false))
}

$manifestPath = (Resolve-Path -LiteralPath $ReleaseManifest).Path
$releaseRoot = Split-Path -Parent $manifestPath
$releaseManifestSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $manifestPath).Hash
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([int](Get-OptionalPropertyValue -Object $manifest -Name 'schemaVersion') -ne 2) {
    throw 'Install-VoyageRelease supports release-manifest schemaVersion 2 only.'
}

$mod = [string](Get-OptionalPropertyValue -Object $manifest -Name 'mod')
$artifactVersion = [string](Get-OptionalPropertyValue -Object $manifest -Name 'version')
$modVersion = [string](Get-OptionalPropertyValue -Object $manifest -Name 'modVersion')
$sourceCommit = [string](Get-OptionalPropertyValue -Object $manifest -Name 'sourceCommit')
$dirtySourceValue = Get-OptionalPropertyValue -Object $manifest -Name 'dirtySource'
$gameFingerprint = Get-OptionalPropertyValue -Object $manifest -Name 'gameFingerprint'
$payload = @(Get-OptionalPropertyValue -Object $manifest -Name 'payload')
$archiveRecord = Get-OptionalPropertyValue -Object $manifest -Name 'archive'

if ([string]::IsNullOrWhiteSpace($mod) -or
    [string]::IsNullOrWhiteSpace($artifactVersion)) {
    throw 'Release manifest must identify both mod and artifact version.'
}
if ($sourceCommit -cnotmatch '^[0-9a-fA-F]{40}$') {
    throw "Release manifest has an invalid source commit: $sourceCommit"
}
if ($null -eq $dirtySourceValue -or $dirtySourceValue -isnot [bool]) {
    throw 'Release manifest must explicitly declare boolean dirtySource.'
}
$dirtySource = [Convert]::ToBoolean($dirtySourceValue)
if ($dirtySource -and -not $AllowDirtySource) {
    throw 'Refusing to install a dirty-source release without -AllowDirtySource.'
}

& git -C $repositoryRoot cat-file -e "$sourceCommit`^{commit}" 2>$null
if ($LASTEXITCODE -ne 0) {
    throw "Release source commit is not present in this repository: $sourceCommit"
}

if ($null -eq $gameFingerprint) {
    throw 'Release manifest has no gameFingerprint.'
}
$expectedBuildId = [string](Get-OptionalPropertyValue -Object $gameFingerprint -Name 'steamBuildId')
$expectedExecutableSha256 = [string](
    Get-OptionalPropertyValue -Object $gameFingerprint -Name 'executableSha256')
if ([string]::IsNullOrWhiteSpace($expectedBuildId) -or
    $expectedExecutableSha256 -cnotmatch $sha256Pattern) {
    throw 'Release manifest has an invalid game fingerprint.'
}

$containerRecords = @(
    $payload | Where-Object {
        $containerExtensions -contains [IO.Path]::GetExtension([string]$_.name).ToLowerInvariant()
    }
)
if ($containerRecords.Count -ne 3) {
    throw "Release manifest must contain exactly one .pak/.ucas/.utoc triplet; found $($containerRecords.Count) records."
}
$actualExtensions = @($containerRecords | ForEach-Object {
    [IO.Path]::GetExtension([string]$_.name).ToLowerInvariant()
} | Sort-Object -Unique)
if ($actualExtensions.Count -ne 3 -or
    @($containerExtensions | Where-Object { $actualExtensions -notcontains $_ }).Count -ne 0) {
    throw 'Release manifest payload is not a complete .pak/.ucas/.utoc triplet.'
}
$containerBases = @($containerRecords | ForEach-Object {
    [IO.Path]::GetFileNameWithoutExtension([string]$_.name)
} | Sort-Object -Unique)
if ($containerBases.Count -ne 1) {
    throw 'Release manifest container files do not share one basename.'
}
if ($containerBases[0] -match '^(?i:global$|pakchunk)') {
    throw 'Standalone mod installation must not replace stock global/pakchunk containers.'
}
$sidecarRecords = @($payload | Where-Object {
    [IO.Path]::GetExtension([string]$_.name).ToLowerInvariant() -ceq '.autoload'
})
if ($sidecarRecords.Count -gt 1 -or ($sidecarRecords.Count -eq 1 -and
    [string]$sidecarRecords[0].name -cne ($containerBases[0] + '.autoload'))) {
    throw 'An autoload sidecar must use the exact container basename and lowercase .autoload extension.'
}
if ($null -eq $archiveRecord) {
    throw 'Release manifest has no exact release archive record.'
}

$resolvedArtifacts = @(
    foreach ($record in @($containerRecords) + @($sidecarRecords)) {
        Resolve-ManifestArtifact -ReleaseRoot $releaseRoot -Record $record `
            -Label "Container artifact $([string]$record.name)"
    }
)
$resolvedArchive = Resolve-ManifestArtifact -ReleaseRoot $releaseRoot `
    -Record $archiveRecord -Label 'Release archive'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archiveReader = [IO.Compression.ZipFile]::OpenRead($resolvedArchive.path)
try {
    if ($archiveReader.Entries.Count -lt 1) {
        throw "Release archive is empty: $($resolvedArchive.path)"
    }
    $archiveContainers = @($archiveReader.Entries | Where-Object {
        @('.pak', '.ucas', '.utoc', '.autoload') -contains [IO.Path]::GetExtension($_.Name).ToLowerInvariant()
    })
    if ($archiveContainers.Count -ne $resolvedArtifacts.Count) {
        throw 'Release ZIP must contain exactly the manifest-owned triplet and optional autoload sidecar.'
    }
    foreach ($artifact in $resolvedArtifacts) {
        $entries = @($archiveContainers | Where-Object { $_.Name -ceq $artifact.name })
        if ($entries.Count -ne 1 -or $entries[0].Length -ne $artifact.size) {
            throw "Release ZIP does not match payload identity/size: $($artifact.name)"
        }
        $stream = $entries[0].Open()
        $hasher = [Security.Cryptography.SHA256]::Create()
        try {
            $entryHash = [BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-', '')
        }
        finally {
            $hasher.Dispose()
            $stream.Dispose()
        }
        if ($entryHash -cne $artifact.sha256) {
            throw "Release ZIP content disagrees with payload hash: $($artifact.name)"
        }
    }
}
finally {
    $archiveReader.Dispose()
}

$installedArchiveRecord = Get-OptionalPropertyValue -Object $manifest -Name 'installedArchive'
$installedArchiveName = $null
if ($null -ne $installedArchiveRecord) {
    $installedArchiveName = [string](
        Get-OptionalPropertyValue -Object $installedArchiveRecord -Name 'name')
}
if ([string]::IsNullOrWhiteSpace($installedArchiveName)) {
    $installedArchiveName = $mod + '_' + $artifactVersion + '.zip'
}
Assert-SafeLeafName -Name $installedArchiveName -Label 'Installed archive name'
if ([IO.Path]::GetExtension($installedArchiveName) -cne '.zip') {
    throw "Installed archive must have a .zip filename: $installedArchiveName"
}

$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$actualFingerprint = (& $fingerprintScript -GameRoot $GameRoot | ConvertFrom-Json)
$actualBuildId = [string]$actualFingerprint.steam.buildId
$actualExecutableSha256 = [string]$actualFingerprint.executable.sha256
if ($actualBuildId -cne $expectedBuildId -or
    $actualExecutableSha256 -cne $expectedExecutableSha256) {
    throw ('Installed Voyage fingerprint does not match the release. ' +
        "Expected Steam $expectedBuildId / $expectedExecutableSha256; " +
        "found Steam $actualBuildId / $actualExecutableSha256.")
}

$resolvedGameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$paksDirectory = (Resolve-Path -LiteralPath (
    Join-Path $resolvedGameRoot 'Voyage\Content\Paks')).Path
$safeMod = $mod -replace '[^0-9A-Za-z._-]', '_'
$safeVersion = $artifactVersion -replace '[^0-9A-Za-z._-]', '_'
$timestamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
$token = [Guid]::NewGuid().ToString('N')
$evidenceParent = [IO.Path]::GetFullPath($EvidenceRoot)
$evidence = Join-Path $evidenceParent (
    "$safeMod\$timestamp-$safeVersion-$($token.Substring(0, 8))")
if (Test-Path -LiteralPath $evidence) {
    throw "Installation evidence path already exists: $evidence"
}
$backupDirectory = Join-Path $evidence 'previous-files'
$transactionPath = Join-Path $evidence 'install-transaction.json'
$installManifestPath = Join-Path $evidence 'install-manifest.json'

$installItems = @(
    foreach ($artifact in $resolvedArtifacts) {
        [pscustomobject][ordered]@{
            kind = if ([IO.Path]::GetExtension($artifact.name) -ceq '.autoload') { 'autoload' } else { 'container' }
            name = $artifact.name
            sourcePath = $artifact.path
            size = $artifact.size
            sourceSha256 = $artifact.sha256
            destinationPath = Join-Path $paksDirectory $artifact.name
            stagingPath = Join-Path $paksDirectory ('.' + $artifact.name + ".installing-$token")
            hadExisting = $false
            previousSha256 = $null
            backupPath = $null
            installedSha256 = $null
        }
    }
    [pscustomobject][ordered]@{
        kind = 'archive'
        name = $installedArchiveName
        sourcePath = $resolvedArchive.path
        size = $resolvedArchive.size
        sourceSha256 = $resolvedArchive.sha256
        destinationPath = Join-Path $paksDirectory $installedArchiveName
        stagingPath = Join-Path $paksDirectory ('.' + $installedArchiveName + ".installing-$token")
        hadExisting = $false
        previousSha256 = $null
        backupPath = $null
        installedSha256 = $null
    }
)

foreach ($item in $installItems) {
    if (Test-Path -LiteralPath $item.destinationPath) {
        $target = Get-Item -LiteralPath $item.destinationPath -Force
        if ($target.PSIsContainer -or
            ($target.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing a directory or reparse-point installation target: $($item.destinationPath)"
        }
    }
}

if ($ValidateOnly) {
    Write-Host "Voyage release validated without installation: $mod $artifactVersion"
    [pscustomobject][ordered]@{
        validated = $true
        mod = $mod
        artifactVersion = $artifactVersion
        releaseManifestPath = $manifestPath
        releaseManifestSha256 = $releaseManifestSha256
        gameRoot = $resolvedGameRoot
        steamBuildId = $actualBuildId
        executableSha256 = $actualExecutableSha256
        paksDirectory = $paksDirectory
        installedArchiveName = $installedArchiveName
        files = @($installItems | Select-Object kind, name, size, sourceSha256)
    }
    return
}

[IO.Directory]::CreateDirectory($evidence) | Out-Null

$transaction = [ordered]@{
    schemaVersion = 1
    kind = 'Voyage release installation transaction'
    status = 'preparing'
    releaseManifestPath = $manifestPath
    releaseManifestSha256 = $releaseManifestSha256
    mod = $mod
    artifactVersion = $artifactVersion
    modVersion = $modVersion
    sourceCommit = $sourceCommit
    dirtySource = $dirtySource
    gameRoot = $resolvedGameRoot
    steamBuildId = $actualBuildId
    executableSha256 = $actualExecutableSha256
    paksDirectory = $paksDirectory
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    files = $installItems
}
Write-JsonFile -Path $transactionPath -Value $transaction

$mutationStarted = $false
try {
    Assert-GameClosed

    foreach ($item in $installItems) {
        Copy-Item -LiteralPath $item.sourcePath -Destination $item.stagingPath
        $stagedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.stagingPath).Hash
        if ($stagedHash -cne $item.sourceSha256) {
            throw "Staged file hash mismatch: $($item.name)"
        }
    }

    foreach ($item in $installItems) {
        if (Test-Path -LiteralPath $item.destinationPath -PathType Leaf) {
            $item.hadExisting = $true
            $item.previousSha256 = (
                Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
            [IO.Directory]::CreateDirectory($backupDirectory) | Out-Null
            $item.backupPath = Join-Path $backupDirectory $item.name
            Copy-Item -LiteralPath $item.destinationPath -Destination $item.backupPath
            $backupHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.backupPath).Hash
            if ($backupHash -cne $item.previousSha256) {
                throw "Backup hash mismatch: $($item.name)"
            }
        }
    }

    $transaction.status = 'prepared'
    $transaction.preparedAtUtc = [DateTime]::UtcNow.ToString('o')
    Write-JsonFile -Path $transactionPath -Value $transaction

    Assert-GameClosed
    foreach ($item in $installItems) {
        if ($item.hadExisting) {
            $currentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
            if ($currentHash -cne $item.previousSha256) {
                throw "Installation target changed after backup: $($item.name)"
            }
        }
        elseif (Test-Path -LiteralPath $item.destinationPath) {
            throw "Installation target appeared after preflight: $($item.name)"
        }
    }
    $mutationStarted = $true
    foreach ($item in $installItems) {
        Move-Item -LiteralPath $item.stagingPath -Destination $item.destinationPath -Force
        $item.installedSha256 = (
            Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
        if ($item.installedSha256 -cne $item.sourceSha256) {
            throw "Installed file hash mismatch: $($item.name)"
        }
    }

    $installedAtUtc = [DateTime]::UtcNow.ToString('o')
    $installationManifest = [ordered]@{
        schemaVersion = 1
        kind = 'Voyage release installation'
        status = 'installed'
        releaseManifestPath = $manifestPath
        releaseManifestSha256 = $releaseManifestSha256
        mod = $mod
        artifactVersion = $artifactVersion
        modVersion = $modVersion
        sourceCommit = $sourceCommit
        dirtySource = $dirtySource
        gameRoot = $resolvedGameRoot
        steamBuildId = $actualBuildId
        executableSha256 = $actualExecutableSha256
        paksDirectory = $paksDirectory
        installedAtUtc = $installedAtUtc
        installedArchive = [ordered]@{
            name = $installedArchiveName
            sha256 = $resolvedArchive.sha256
        }
        files = $installItems
    }
    Write-JsonFile -Path $installManifestPath -Value $installationManifest
    $transaction.status = 'installed'
    $transaction.completedAtUtc = $installedAtUtc
    $transaction.installManifestPath = $installManifestPath
    Write-JsonFile -Path $transactionPath -Value $transaction
}
catch {
    $originalError = $_
    $recoveryErrors = @()
    if ($mutationStarted) {
        foreach ($item in $installItems) {
            try {
                if ($item.hadExisting) {
                    if (-not (Test-Path -LiteralPath $item.backupPath -PathType Leaf)) {
                        throw "Backup is missing: $($item.backupPath)"
                    }
                    $backupHash = (
                        Get-FileHash -Algorithm SHA256 -LiteralPath $item.backupPath).Hash
                    if ($backupHash -cne $item.previousSha256) {
                        throw "Backup changed: $($item.backupPath)"
                    }
                    Copy-Item -LiteralPath $item.backupPath `
                        -Destination $item.destinationPath -Force
                    $restoredHash = (
                        Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
                    if ($restoredHash -cne $item.previousSha256) {
                        throw "Rollback hash mismatch: $($item.name)"
                    }
                }
                elseif (Test-Path -LiteralPath $item.destinationPath -PathType Leaf) {
                    $currentHash = (
                        Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
                    if ($currentHash -ceq $item.sourceSha256) {
                        Remove-Item -LiteralPath $item.destinationPath -Force
                    }
                    else {
                        throw "Refusing to remove unexpected file: $($item.destinationPath)"
                    }
                }
            }
            catch {
                $recoveryErrors += $_.Exception.Message
            }
        }
    }
    $transaction.status = if ($recoveryErrors.Count -eq 0) { 'rolled-back' } else { 'recovery-failed' }
    if (Test-Path -LiteralPath $installManifestPath -PathType Leaf) {
        Remove-Item -LiteralPath $installManifestPath -Force
    }
    $transaction.failedAtUtc = [DateTime]::UtcNow.ToString('o')
    $transaction.error = $originalError.Exception.Message
    $transaction.recoveryErrors = $recoveryErrors
    Write-JsonFile -Path $transactionPath -Value $transaction
    if ($recoveryErrors.Count -gt 0) {
        throw ($originalError.Exception.Message + ' Recovery also failed: ' +
            ($recoveryErrors -join ' | ') + ". Evidence: $transactionPath")
    }
    throw ($originalError.Exception.Message + ". Installation was rolled back. Evidence: $transactionPath")
}
finally {
    if ($transaction.status -cne 'recovery-failed') {
        foreach ($item in $installItems) {
            if (Test-Path -LiteralPath $item.stagingPath -PathType Leaf) {
                Remove-Item -LiteralPath $item.stagingPath -Force
            }
        }
    }
}

Write-Host "Voyage release installed with matching hashes: $mod $artifactVersion"
Write-Host "Installation evidence: $installManifestPath"
[pscustomobject][ordered]@{
    mod = $mod
    artifactVersion = $artifactVersion
    installManifestPath = $installManifestPath
    paksDirectory = $paksDirectory
    installedArchivePath = Join-Path $paksDirectory $installedArchiveName
    fileCount = $installItems.Count
}
