# Creates and validates the common schema-2 manifest for an already-built Voyage release.
# This script does not build, cook, package, install, or modify release payload files.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ReleaseRoot,

    [Parameter(Mandatory = $true)]
    [string]$Mod,

    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$Container,

    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [Parameter(Mandatory = $true)]
    [string[]]$SourcePath,

    [string]$ModVersion,

    [string]$InstalledArchiveName,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$OutputPath,

    [switch]$AllowDirtySource,

    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$resolvedReleaseRoot = (Resolve-Path -LiteralPath $ReleaseRoot).Path
$installer = Join-Path $PSScriptRoot 'Install-VoyageRelease.ps1'
$containerExtensions = @('.pak', '.ucas', '.utoc')

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

function Assert-PathBelowRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    $rootPrefix = $Root.TrimEnd('\') + '\'
    if (-not $Path.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label must be below $Root`: $Path"
    }
}

function Get-RelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $rootUri = [Uri]($Root.TrimEnd('\') + '\')
    $pathUri = [Uri]$Path
    [Uri]::UnescapeDataString($rootUri.MakeRelativeUri($pathUri).ToString())
}

function New-ArtifactRecord {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $item = Get-Item -LiteralPath $Path
    [pscustomobject][ordered]@{
        name = $item.Name
        path = Get-RelativePath -Root $resolvedReleaseRoot -Path $item.FullName
        size = $item.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.FullName).Hash
    }
}

function Write-Utf8Json {
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

if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Release validator is missing: $installer"
}
if ([string]::IsNullOrWhiteSpace($Mod) -or [string]::IsNullOrWhiteSpace($Version)) {
    throw 'Mod and Version must be non-empty.'
}

$resolvedUtoc = (Resolve-Path -LiteralPath $Container).Path
Assert-PathBelowRoot -Path $resolvedUtoc -Root $resolvedReleaseRoot -Label 'Container'
if ([IO.Path]::GetExtension($resolvedUtoc) -cne '.utoc') {
    throw "Container must identify the exact .utoc file: $resolvedUtoc"
}
$containerBase = [IO.Path]::GetFileNameWithoutExtension($resolvedUtoc)
if ($containerBase -match '^(?i:global$|pakchunk)') {
    throw "Standalone release cannot own a stock container basename: $containerBase"
}
$containerDirectory = [IO.Path]::GetDirectoryName($resolvedUtoc)
$payloadPaths = @(
    foreach ($extension in $containerExtensions) {
        $candidate = Join-Path $containerDirectory ($containerBase + $extension)
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            throw "Release container triplet is incomplete; missing $candidate"
        }
        (Resolve-Path -LiteralPath $candidate).Path
    }
)
$autoloadPath = Join-Path $containerDirectory ($containerBase + '.autoload')
if (Test-Path -LiteralPath $autoloadPath -PathType Leaf) {
    $payloadPaths += (Resolve-Path -LiteralPath $autoloadPath).Path
}

$resolvedArchive = (Resolve-Path -LiteralPath $Archive).Path
Assert-PathBelowRoot -Path $resolvedArchive -Root $resolvedReleaseRoot -Label 'Archive'
if ([IO.Path]::GetExtension($resolvedArchive) -cne '.zip') {
    throw "Archive must be an exact .zip file: $resolvedArchive"
}

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path $resolvedReleaseRoot 'release-manifest.json'
}
else {
    [IO.Path]::GetFullPath($OutputPath)
}
if (-not [StringComparer]::OrdinalIgnoreCase.Equals(
    [IO.Path]::GetDirectoryName($resolvedOutputPath),
    $resolvedReleaseRoot)) {
    throw "Release manifest must be written directly in the release root: $resolvedReleaseRoot"
}
if ([IO.Path]::GetExtension($resolvedOutputPath) -cne '.json') {
    throw "Release manifest output must be a lowercase .json file: $resolvedOutputPath"
}
if (Test-Path -LiteralPath $resolvedOutputPath) {
    throw "Refusing to overwrite an existing release manifest: $resolvedOutputPath"
}

if ($SourcePath.Count -lt 1) {
    throw 'At least one exact SourcePath is required.'
}
$sourceRecords = @()
$gitPathSpecs = @()
foreach ($declaredSourcePath in $SourcePath) {
    $resolvedSourcePath = (Resolve-Path -LiteralPath $declaredSourcePath).Path
    Assert-PathBelowRoot -Path $resolvedSourcePath -Root $repositoryRoot -Label 'SourcePath'
    if ($resolvedSourcePath.StartsWith(
        (Join-Path $repositoryRoot '.git') + '\',
        [StringComparison]::OrdinalIgnoreCase)) {
        throw "SourcePath must not select repository metadata: $resolvedSourcePath"
    }
    $relativeSourcePath = Get-RelativePath -Root $repositoryRoot -Path $resolvedSourcePath
    $gitPathSpecs += $relativeSourcePath
    $sourceRecords += [pscustomobject][ordered]@{
        path = $relativeSourcePath
        type = if (Test-Path -LiteralPath $resolvedSourcePath -PathType Container) {
            'directory'
        }
        else {
            'file'
        }
    }
}

$sourceCommit = ((& git -C $repositoryRoot rev-parse HEAD) -join '').Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-fA-F]{40}$') {
    throw 'Could not resolve the repository HEAD for release provenance.'
}
$sourceStatus = @(& git -C $repositoryRoot status --porcelain=v1 `
    --untracked-files=all -- $gitPathSpecs)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not determine scoped source status for release provenance.'
}
$dirtySource = $sourceStatus.Count -gt 0
if ($dirtySource -and -not $AllowDirtySource) {
    throw ('The declared release source has uncommitted changes. Commit it first ' +
        'or explicitly use -AllowDirtySource for a test candidate.')
}

$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$fingerprint = (& $fingerprintScript -GameRoot $GameRoot | ConvertFrom-Json)
$buildId = [string]$fingerprint.steam.buildId
$executableSha256 = [string]$fingerprint.executable.sha256
if ([string]::IsNullOrWhiteSpace($buildId) -or
    $executableSha256 -notmatch '^[0-9A-F]{64}$') {
    throw 'The installed game did not return a complete release fingerprint.'
}

$manifest = [ordered]@{
    schemaVersion = 2
    mod = $Mod
    version = $Version
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    sourceCommit = $sourceCommit
    dirtySource = $dirtySource
    sourcePaths = $sourceRecords
    gameFingerprint = [ordered]@{
        steamBuildId = $buildId
        executableSha256 = $executableSha256
    }
    payload = @($payloadPaths | ForEach-Object { New-ArtifactRecord -Path $_ })
    archive = New-ArtifactRecord -Path $resolvedArchive
}
if (-not [string]::IsNullOrWhiteSpace($ModVersion)) {
    $manifest['modVersion'] = $ModVersion
}
if (-not [string]::IsNullOrWhiteSpace($InstalledArchiveName)) {
    Assert-SafeLeafName -Name $InstalledArchiveName -Label 'Installed archive name'
    if ([IO.Path]::GetExtension($InstalledArchiveName) -cne '.zip') {
        throw "Installed archive name must end in lowercase .zip: $InstalledArchiveName"
    }
    $manifest['installedArchive'] = [ordered]@{ name = $InstalledArchiveName }
}

$temporaryManifest = Join-Path $resolvedReleaseRoot (
    '.release-manifest.validating-' + [Guid]::NewGuid().ToString('N') + '.json')
try {
    Write-Utf8Json -Path $temporaryManifest -Value $manifest
    $validationArguments = @{
        ReleaseManifest = $temporaryManifest
        GameRoot = $GameRoot
        ValidateOnly = $true
    }
    if ($dirtySource) {
        $validationArguments.AllowDirtySource = $true
    }
    $validationOutput = @(& $installer @validationArguments 6>$null)
    $validationResult = @($validationOutput | Where-Object {
        $null -ne $_.PSObject.Properties['validated'] -and [bool]$_.validated
    })
    if ($validationResult.Count -ne 1) {
        throw 'Release validation did not return one successful validation result.'
    }
    [IO.File]::Move($temporaryManifest, $resolvedOutputPath)
}
finally {
    if (Test-Path -LiteralPath $temporaryManifest -PathType Leaf) {
        Remove-Item -LiteralPath $temporaryManifest -Force
    }
}

$result = [pscustomobject][ordered]@{
    status = 'created-and-validated'
    manifestPath = $resolvedOutputPath
    manifestSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolvedOutputPath).Hash
    mod = $Mod
    artifactVersion = $Version
    sourceCommit = $sourceCommit
    dirtySource = $dirtySource
    steamBuildId = $buildId
    executableSha256 = $executableSha256
    payloadCount = $payloadPaths.Count
    archiveName = [IO.Path]::GetFileName($resolvedArchive)
    validation = 'Install-VoyageRelease.ps1 -ValidateOnly'
}
if ($AsJson) {
    $result | ConvertTo-Json -Depth 5 -Compress
}
else {
    $result
}
