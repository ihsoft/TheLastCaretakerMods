# Creates and validates the common release manifest for an already-built Voyage release.
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

    [string]$ContentPluginDescriptor,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$OutputPath,

    [switch]$LogicMods,

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

function Get-ContentPluginRecord {
    param(
        [Parameter(Mandatory = $true)]
        [string]$DescriptorPath,
        [Parameter(Mandatory = $true)]
        [string]$ArchivePath
    )

    $resolvedDescriptor = (Resolve-Path -LiteralPath $DescriptorPath).Path
    Assert-PathBelowRoot -Path $resolvedDescriptor -Root $resolvedReleaseRoot `
        -Label 'Content plugin descriptor'
    if ([IO.Path]::GetExtension($resolvedDescriptor) -cne '.uplugin') {
        throw "Content plugin descriptor must be a lowercase .uplugin file: $resolvedDescriptor"
    }
    $pluginName = [IO.Path]::GetFileNameWithoutExtension($resolvedDescriptor)
    if ($pluginName -cnotmatch '^[0-9A-Za-z_]+$') {
        throw "Content plugin name is unsafe: $pluginName"
    }
    $descriptor = Get-Content -LiteralPath $resolvedDescriptor -Raw | ConvertFrom-Json
    if ($descriptor.CanContainContent -isnot [bool] -or
        -not [bool]$descriptor.CanContainContent -or
        $descriptor.EnabledByDefault -isnot [bool] -or
        -not [bool]$descriptor.EnabledByDefault -or
        $descriptor.ExplicitlyLoaded -isnot [bool] -or
        [bool]$descriptor.ExplicitlyLoaded -or
        $null -eq $descriptor.Modules -or @($descriptor.Modules).Count -ne 0) {
        throw ('Content plugin descriptor must declare CanContainContent=true, ' +
            'EnabledByDefault=true, ExplicitlyLoaded=false, and Modules=[].')
    }
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $newArchivePath = "Voyage/Mods/$pluginName/$pluginName.uplugin"
    $oldArchivePath = "Mods/$pluginName/$pluginName.uplugin"
    $zip = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
    try {
        $matches = @($zip.Entries | Where-Object {
            $_.FullName.Replace('\', '/') -ceq $newArchivePath -or
                $_.FullName.Replace('\', '/') -ceq $oldArchivePath
        })
        if ($matches.Count -ne 1) {
            throw 'Release ZIP must contain the content plugin at exactly one supported path.'
        }
        $descriptorArchivePath = $matches[0].FullName.Replace('\', '/')
    } finally { $zip.Dispose() }
    [ordered]@{
        type = 'content-plugin-descriptor'
        pluginName = $pluginName
        artifact = New-ArtifactRecord -Path $resolvedDescriptor
        archivePath = $descriptorArchivePath
        installRelativePath = "Voyage/Mods/$pluginName/$pluginName.uplugin"
    }
}

if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Release validator is missing: $installer"
}
if ([string]::IsNullOrWhiteSpace($Mod) -or [string]::IsNullOrWhiteSpace($Version)) {
    throw 'Mod and Version must be non-empty.'
}
if ($Version -cnotmatch '^V[1-9][0-9]*$') {
    throw 'Release Version must use the canonical V<N> form.'
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
$expectedArchiveName = "$Mod-$Version.zip"
if ([IO.Path]::GetFileName($resolvedArchive) -cne $expectedArchiveName) {
    throw "Release ZIP must use the canonical name $expectedArchiveName"
}
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archiveReadback = [IO.Compression.ZipFile]::OpenRead($resolvedArchive)
try {
    if (@($archiveReadback.Entries | Where-Object {
            [IO.Path]::GetFileName($_.FullName) -ieq 'CHANGELOG.md'
        }).Count -ne 0) {
        throw 'CHANGELOG.md is publication input and must not be included in the release ZIP.'
    }
}
finally {
    $archiveReadback.Dispose()
}

$changelogRecord = $null
$modRoot = Join-Path (Join-Path $repositoryRoot 'mods') $Mod
if (Test-Path -LiteralPath $modRoot -PathType Container) {
    $changelogPath = Join-Path $modRoot 'CHANGELOG.md'
    if (-not (Test-Path -LiteralPath $changelogPath -PathType Leaf)) {
        throw "Mod changelog is missing: $changelogPath"
    }
    $changelogText = Get-Content -LiteralPath $changelogPath -Raw
    if ($changelogText -cnotmatch '(?m)^## Next\s*$') {
        throw "Mod changelog must contain a ## Next section: $changelogPath"
    }
    $changelogRecord = [ordered]@{
        path = Get-RelativePath -Root $repositoryRoot -Path $changelogPath
    }
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
if ($buildId -notmatch '^\d+$') {
    throw 'The installed game did not return a valid Steam build ID.'
}

$contentPlugin = $null
if (-not [string]::IsNullOrWhiteSpace($ContentPluginDescriptor)) {
    $contentPlugin = Get-ContentPluginRecord -DescriptorPath $ContentPluginDescriptor -ArchivePath $resolvedArchive
}

$manifest = [ordered]@{
    schemaVersion = $(if ($null -eq $contentPlugin) { 2 } else { 3 })
    mod = $Mod
    version = $Version
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    sourceCommit = $sourceCommit
    dirtySource = $dirtySource
    sourcePaths = $sourceRecords
    gameFingerprint = [ordered]@{
        steamBuildId = $buildId
    }
    payload = @($payloadPaths | ForEach-Object { New-ArtifactRecord -Path $_ })
    archive = New-ArtifactRecord -Path $resolvedArchive
}
if ($null -ne $changelogRecord) {
    $manifest['changelog'] = $changelogRecord
}
if ($null -ne $contentPlugin) {
    $manifest['contentPlugin'] = $contentPlugin
}
if (-not [string]::IsNullOrWhiteSpace($ModVersion)) {
    $manifest['modVersion'] = $ModVersion
}
if (-not [string]::IsNullOrWhiteSpace($InstalledArchiveName)) {
    Assert-SafeLeafName -Name $InstalledArchiveName -Label 'Installed archive name'
    if ([IO.Path]::GetExtension($InstalledArchiveName) -cne '.zip') {
        throw "Installed archive name must end in lowercase .zip: $InstalledArchiveName"
    }
    if ($InstalledArchiveName -cne [IO.Path]::GetFileName($resolvedArchive)) {
        throw 'Retained archive name must match the ZIP name recorded in the manifest.'
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
        LogicMods = [bool]$LogicMods
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
    payloadCount = $payloadPaths.Count
    contentPlugin = if ($null -eq $contentPlugin) { $null } else { $contentPlugin.pluginName }
    archiveName = [IO.Path]::GetFileName($resolvedArchive)
    validation = 'Install-VoyageRelease.ps1 -ValidateOnly'
}
if ($AsJson) {
    $result | ConvertTo-Json -Depth 5 -Compress
}
else {
    $result
}
