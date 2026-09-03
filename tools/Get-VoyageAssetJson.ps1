[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Query,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$CacheRoot = (Join-Path $PSScriptRoot '..\artifacts\asset-cache'),

    [string]$MappingsRoot = (Join-Path $PSScriptRoot '..\artifacts\mappings'),

    [string]$MappingsPath,

    [string]$MappingsManifestPath,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_8',

    [switch]$AllowAdditionalContainers
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$testMappingsScript = Join-Path $PSScriptRoot 'Test-VoyageMappings.ps1'
$inspectorProject = Join-Path $PSScriptRoot 'VoyageAssetInspector\VoyageAssetInspector.csproj'
$inspectorSource = Join-Path $PSScriptRoot 'VoyageAssetInspector\Program.cs'
$cue4ParseBinary = Join-Path $PSScriptRoot '..\.tools\bin\CUE4Parse\CUE4Parse.dll'
$sha256Pattern = '^[0-9A-F]{64}$'

function Get-TextSha256 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
    $hash = [Security.Cryptography.SHA256]::HashData($bytes)
    [Convert]::ToHexString($hash)
}

function Invoke-Inspector {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PaksDirectory,

        [Parameter(Mandatory = $true)]
        [string]$AssetQuery,

        [Parameter(Mandatory = $true)]
        [string]$OutputDirectory,

        [string]$MappingFile,

        [Parameter(Mandatory = $true)]
        [string]$LogPath
    )

    $arguments = @(
        'run',
        '--project', $inspectorProject,
        '--configuration', 'Release',
        '--',
        $PaksDirectory,
        $AssetQuery,
        $OutputDirectory,
        $(if ($MappingFile) { $MappingFile } else { '-' }),
        $EngineVersion
    )
    & dotnet @arguments *> $LogPath
    if ($LASTEXITCODE -ne 0) {
        throw "VoyageAssetInspector failed with exit code $LASTEXITCODE. Log: $LogPath"
    }
}

function Resolve-ValidatedMappings {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SteamBuildId,

        [Parameter(Mandatory = $true)]
        [string]$ExecutableSha256
    )

    if ($MappingsManifestPath -and -not $MappingsPath) {
        throw '-MappingsManifestPath requires -MappingsPath.'
    }

    if ($MappingsPath) {
        $mapping = (Resolve-Path -LiteralPath $MappingsPath).Path
        $manifest = if ($MappingsManifestPath) {
            (Resolve-Path -LiteralPath $MappingsManifestPath).Path
        }
        else {
            Join-Path (Split-Path -Parent $mapping) 'mapping-manifest.json'
        }
        if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
            throw "Mappings manifest not found: $manifest"
        }
        & $testMappingsScript `
            -MappingsPath $mapping `
            -ManifestPath $manifest `
            -ExpectedSteamBuildId $SteamBuildId `
            -ExpectedExecutableSha256 $ExecutableSha256 | Out-Null
        return [pscustomobject]@{
            mappingsPath = $mapping
            manifestPath = (Resolve-Path -LiteralPath $manifest).Path
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $mapping).Hash
        }
    }

    $root = (Resolve-Path -LiteralPath $MappingsRoot).Path
    $records = [Collections.Generic.List[object]]::new()
    foreach ($manifestFile in Get-ChildItem -LiteralPath $root -Filter 'mapping-manifest.json' -File -Recurse) {
        try {
            $manifest = Get-Content -Raw -LiteralPath $manifestFile.FullName | ConvertFrom-Json
            if ([string]$manifest.kind -cne 'Voyage reflection mappings' -or
                [string]$manifest.steamBuildId -cne $SteamBuildId -or
                [string]$manifest.executableSha256 -cne $ExecutableSha256) {
                continue
            }
            $mappingName = if ([string]::IsNullOrWhiteSpace([string]$manifest.mappingFile)) {
                'Mappings.usmap'
            }
            else {
                [string]$manifest.mappingFile
            }
            $mapping = Join-Path $manifestFile.DirectoryName $mappingName
            if (-not (Test-Path -LiteralPath $mapping -PathType Leaf)) {
                continue
            }
            $generated = [DateTime]::MinValue
            [void][DateTime]::TryParse([string]$manifest.generatedAtUtc, [ref]$generated)
            $records.Add([pscustomobject]@{
                mappingsPath = $mapping
                manifestPath = $manifestFile.FullName
                generatedAtUtc = $generated.ToUniversalTime()
            })
        }
        catch {
            continue
        }
    }

    foreach ($record in $records | Sort-Object generatedAtUtc -Descending) {
        try {
            & $testMappingsScript `
                -MappingsPath $record.mappingsPath `
                -ManifestPath $record.manifestPath `
                -ExpectedSteamBuildId $SteamBuildId `
                -ExpectedExecutableSha256 $ExecutableSha256 | Out-Null
            return [pscustomobject]@{
                mappingsPath = (Resolve-Path -LiteralPath $record.mappingsPath).Path
                manifestPath = (Resolve-Path -LiteralPath $record.manifestPath).Path
                sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $record.mappingsPath).Hash
            }
        }
        catch {
            continue
        }
    }

    throw "No validated mappings were found for Steam build $SteamBuildId. Run tools\New-VoyageMappings.ps1 first."
}

function Get-ContainerView {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PaksDirectory
    )

    $allContainers = @(
        Get-ChildItem -LiteralPath $PaksDirectory -Filter '*.utoc' -File |
            Sort-Object Name
    )
    $baseContainers = @(
        $allContainers |
            Where-Object { $_.Name -eq 'global.utoc' -or $_.Name -like 'pakchunk*.utoc' }
    )
    if ($baseContainers.Count -eq 0) {
        throw "No base Voyage IoStore containers were found in: $PaksDirectory"
    }
    $additional = @(
        $allContainers |
            Where-Object { $_.Name -ne 'global.utoc' -and $_.Name -notlike 'pakchunk*.utoc' } |
            Sort-Object Name
    )
    if ($additional.Count -gt 0 -and -not $AllowAdditionalContainers) {
        $names = $additional.Name -join ', '
        throw "Additional containers can shadow stock assets: $names. Remove them for the base cache or pass -AllowAdditionalContainers for an isolated mounted-view cache."
    }

    $baseRecords = @($baseContainers | ForEach-Object {
        [ordered]@{
            name = $_.Name
            length = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    })
    $baseDescriptor = $baseRecords | ConvertTo-Json -Depth 3 -Compress
    $baseHash = Get-TextSha256 -Text $baseDescriptor
    $additionalRecords = @($additional | ForEach-Object {
        [ordered]@{
            name = $_.Name
            length = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    })
    if ($additionalRecords.Count -eq 0) {
        return [pscustomobject]@{
            name = 'base'
            baseContainers = $baseRecords
            baseContainersSha256 = $baseHash
            additionalContainers = @()
        }
    }

    $descriptor = $additionalRecords | ConvertTo-Json -Depth 3 -Compress
    $viewHash = Get-TextSha256 -Text $descriptor
    [pscustomobject]@{
        name = "with-additional-$($viewHash.Substring(0, 12))"
        baseContainers = $baseRecords
        baseContainersSha256 = $baseHash
        additionalContainers = $additionalRecords
    }
}

function Get-PackageIndex {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PaksDirectory,

        [Parameter(Mandatory = $true)]
        [string]$ViewRoot,

        [Parameter(Mandatory = $true)]
        [string]$SteamBuildId,

        [Parameter(Mandatory = $true)]
        [string]$ExecutableSha256,

        [Parameter(Mandatory = $true)]
        [string]$Cue4ParseBinarySha256,

        [Parameter(Mandatory = $true)]
        [object]$ContainerView
    )

    $catalogRoot = Join-Path $ViewRoot '_catalog'
    $indexPath = Join-Path $catalogRoot 'packages.txt'
    $manifestPath = Join-Path $catalogRoot 'package-index-manifest.json'
    if ((Test-Path -LiteralPath $indexPath -PathType Leaf) -and
        (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $indexPath).Hash
        if ([string]$manifest.kind -cne 'Voyage asset package index' -or
            [string]$manifest.steamBuildId -cne $SteamBuildId -or
            [string]$manifest.executableSha256 -cne $ExecutableSha256 -or
            [string]$manifest.cue4ParseBinarySha256 -cne $Cue4ParseBinarySha256 -or
            [string]$manifest.baseContainersSha256 -cne [string]$ContainerView.baseContainersSha256 -or
            [string]$manifest.containerView -cne [string]$ContainerView.name -or
            [string]$manifest.packageIndexSha256 -cne $actualHash) {
            throw "The cached package index failed its provenance check: $catalogRoot"
        }
        return (Resolve-Path -LiteralPath $indexPath).Path
    }
    if ((Test-Path -LiteralPath $indexPath) -or (Test-Path -LiteralPath $manifestPath)) {
        throw "The cached package index is incomplete: $catalogRoot"
    }

    $stagingRoot = Join-Path $ViewRoot '_staging'
    $staging = Join-Path $stagingRoot ('catalog-' + [guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($staging) | Out-Null
    $logPath = Join-Path $staging 'inspector.log'
    Write-Host "Building the package index for $SteamBuildId..."
    Invoke-Inspector `
        -PaksDirectory $PaksDirectory `
        -AssetQuery 'list:' `
        -OutputDirectory $staging `
        -LogPath $logPath

    $matchesPath = Join-Path $staging 'matches.txt'
    if (-not (Test-Path -LiteralPath $matchesPath -PathType Leaf)) {
        throw "VoyageAssetInspector did not produce a package index. Staging: $staging"
    }
    $packages = @(
        Get-Content -LiteralPath $matchesPath |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )
    if ($packages.Count -eq 0) {
        throw "VoyageAssetInspector produced an empty package index. Staging: $staging"
    }

    [IO.Directory]::CreateDirectory($catalogRoot) | Out-Null
    [IO.File]::WriteAllLines($indexPath, $packages)
    Copy-Item -LiteralPath $logPath -Destination (Join-Path $catalogRoot 'generation.log')
    $indexManifest = [ordered]@{
        kind = 'Voyage asset package index'
        steamBuildId = $SteamBuildId
        executableSha256 = $ExecutableSha256
        cue4ParseBinarySha256 = $Cue4ParseBinarySha256
        engineVersion = $EngineVersion
        baseContainers = @($ContainerView.baseContainers)
        baseContainersSha256 = [string]$ContainerView.baseContainersSha256
        containerView = [string]$ContainerView.name
        additionalContainers = @($ContainerView.additionalContainers)
        packageCount = $packages.Count
        packageIndexSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $indexPath).Hash
        inspectorSourceSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $inspectorSource).Hash
        generatedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    [IO.File]::WriteAllText(
        $manifestPath,
        (($indexManifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine))
    [IO.Directory]::Delete($staging, $true)
    (Resolve-Path -LiteralPath $indexPath).Path
}

function Resolve-AssetVirtualPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PackageIndexPath
    )

    $packages = @(Get-Content -LiteralPath $PackageIndexPath)
    $normalized = $Query.Trim().Replace('\', '/').TrimStart('/')
    if ([string]::IsNullOrWhiteSpace($normalized)) {
        throw 'Asset query cannot be empty.'
    }

    $exactCandidates = [Collections.Generic.List[string]]::new()
    [void]$exactCandidates.Add($normalized)
    if ($normalized.StartsWith('Game/', [StringComparison]::OrdinalIgnoreCase)) {
        [void]$exactCandidates.Add('Voyage/Content/' + $normalized.Substring('Game/'.Length))
    }
    foreach ($candidate in @($exactCandidates)) {
        if (-not [IO.Path]::HasExtension($candidate)) {
            [void]$exactCandidates.Add($candidate + '.uasset')
            [void]$exactCandidates.Add($candidate + '.umap')
        }
    }

    $matches = @($packages | Where-Object {
        $package = $_
        @($exactCandidates | Where-Object {
            $package.Equals($_, [StringComparison]::OrdinalIgnoreCase)
        }).Count -gt 0
    })
    if ($matches.Count -eq 0) {
        $matches = @($packages | Where-Object {
            $_.Contains($normalized, [StringComparison]::OrdinalIgnoreCase)
        })
    }
    if ($matches.Count -eq 0) {
        throw "No asset package matches '$Query' in the current package index."
    }
    if ($matches.Count -gt 1) {
        $shown = @($matches | Select-Object -First 20) -join [Environment]::NewLine
        throw "Asset query '$Query' is ambiguous ($($matches.Count) matches). Use one exact virtual path:`n$shown"
    }
    $matches[0]
}

if (-not (Test-Path -LiteralPath $cue4ParseBinary -PathType Leaf)) {
    throw 'Canonical CUE4Parse bundle is missing. Run tools\Publish-Cue4ParseBinary.ps1.'
}
$cue4ParseBinarySha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $cue4ParseBinary).Hash
if (-not (Test-Path -LiteralPath $inspectorProject -PathType Leaf)) {
    throw "VoyageAssetInspector project is missing: $inspectorProject"
}

$fingerprintText = (& $fingerprintScript -GameRoot $GameRoot) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$steamBuildId = [string]$fingerprint.steam.buildId
$executableSha256 = [string]$fingerprint.executable.sha256
if ($steamBuildId -notmatch '^\d+$' -or $executableSha256 -notmatch $sha256Pattern) {
    throw 'The installed game fingerprint has no valid Steam build ID or executable SHA-256.'
}

$resolvedGameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$paksDirectory = Join-Path $resolvedGameRoot 'Voyage\Content\Paks'
$containerView = Get-ContainerView -PaksDirectory $paksDirectory
$versionName = "steam-$steamBuildId-$($executableSha256.Substring(0, 12))-base-$($containerView.baseContainersSha256.Substring(0, 12))"
$viewRoot = Join-Path (Join-Path ([IO.Path]::GetFullPath($CacheRoot)) $versionName) $containerView.name
$mapping = Resolve-ValidatedMappings `
    -SteamBuildId $steamBuildId `
    -ExecutableSha256 $executableSha256
$packageIndexPath = Get-PackageIndex `
    -PaksDirectory $paksDirectory `
    -ViewRoot $viewRoot `
    -SteamBuildId $steamBuildId `
    -ExecutableSha256 $executableSha256 `
    -Cue4ParseBinarySha256 $cue4ParseBinarySha256 `
    -ContainerView $containerView
$virtualPath = Resolve-AssetVirtualPath -PackageIndexPath $packageIndexPath

$relativeJsonPath = [IO.Path]::ChangeExtension($virtualPath, '.json').Replace('/', [IO.Path]::DirectorySeparatorChar)
$viewRootFull = [IO.Path]::GetFullPath($viewRoot)
$jsonPath = [IO.Path]::GetFullPath((Join-Path $viewRootFull $relativeJsonPath))
$requiredPrefix = $viewRootFull.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $jsonPath.StartsWith($requiredPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Resolved asset path escapes the cache root: $virtualPath"
}
$manifestPath = [IO.Path]::ChangeExtension($jsonPath, '.asset-manifest.json')

if ((Test-Path -LiteralPath $jsonPath -PathType Leaf) -and
    (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $actualJsonHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $jsonPath).Hash
    if ([string]$manifest.kind -cne 'Voyage cached asset JSON' -or
        [string]$manifest.steamBuildId -cne $steamBuildId -or
        [string]$manifest.executableSha256 -cne $executableSha256 -or
        [string]$manifest.baseContainersSha256 -cne [string]$containerView.baseContainersSha256 -or
        [string]$manifest.containerView -cne [string]$containerView.name -or
        [string]$manifest.virtualPath -cne $virtualPath -or
        [string]$manifest.mappingsSha256 -cne [string]$mapping.sha256 -or
        [string]$manifest.cue4ParseBinarySha256 -cne $cue4ParseBinarySha256 -or
        [string]$manifest.jsonSha256 -cne $actualJsonHash -or
        [long]$manifest.jsonLength -ne (Get-Item -LiteralPath $jsonPath).Length) {
        throw "The cached asset JSON failed its provenance check: $jsonPath"
    }
    [pscustomobject]@{
        cacheStatus = 'hit'
        steamBuildId = $steamBuildId
        containerView = [string]$containerView.name
        virtualPath = $virtualPath
        jsonPath = (Resolve-Path -LiteralPath $jsonPath).Path
        manifestPath = (Resolve-Path -LiteralPath $manifestPath).Path
        jsonSha256 = $actualJsonHash
    }
    return
}
if ((Test-Path -LiteralPath $jsonPath) -or (Test-Path -LiteralPath $manifestPath)) {
    throw "The cached asset entry is incomplete: $jsonPath"
}

$stagingRoot = Join-Path $viewRoot '_staging'
$staging = Join-Path $stagingRoot ('asset-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($staging) | Out-Null
$logPath = Join-Path $staging 'inspector.log'
Write-Host "Exporting $virtualPath..."
Invoke-Inspector `
    -PaksDirectory $paksDirectory `
    -AssetQuery $virtualPath `
    -OutputDirectory $staging `
    -MappingFile $mapping.mappingsPath `
    -LogPath $logPath

$matches = @(Get-Content -LiteralPath (Join-Path $staging 'matches.txt'))
if ($matches.Count -ne 1 -or $matches[0] -cne $virtualPath) {
    throw "Exact asset export resolved unexpectedly. Staging: $staging"
}
$errorPath = Join-Path $staging 'errors.txt'
if ((Test-Path -LiteralPath $errorPath -PathType Leaf) -and
    (Get-Item -LiteralPath $errorPath).Length -gt 0) {
    throw "VoyageAssetInspector could not parse the exact asset. Staging: $staging"
}
$safeName = $virtualPath.Replace('/', '_').Replace('\', '_').Replace('.', '_') + '.json'
$stagedJson = Join-Path $staging $safeName
if (-not (Test-Path -LiteralPath $stagedJson -PathType Leaf) -or
    (Get-Item -LiteralPath $stagedJson).Length -eq 0) {
    throw "VoyageAssetInspector did not produce the expected JSON. Staging: $staging"
}

[IO.Directory]::CreateDirectory((Split-Path -Parent $jsonPath)) | Out-Null
$temporaryJson = $jsonPath + '.new-' + [guid]::NewGuid().ToString('N')
$temporaryManifest = $manifestPath + '.new-' + [guid]::NewGuid().ToString('N')
Copy-Item -LiteralPath $stagedJson -Destination $temporaryJson
$jsonItem = Get-Item -LiteralPath $temporaryJson
$jsonSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $temporaryJson).Hash
$assetManifest = [ordered]@{
    kind = 'Voyage cached asset JSON'
    steamBuildId = $steamBuildId
    executableSha256 = $executableSha256
    engineVersion = $EngineVersion
    baseContainers = @($containerView.baseContainers)
    baseContainersSha256 = [string]$containerView.baseContainersSha256
    containerView = [string]$containerView.name
    additionalContainers = @($containerView.additionalContainers)
    virtualPath = $virtualPath
    packageIndexSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $packageIndexPath).Hash
    mappingsPath = [string]$mapping.mappingsPath
    mappingsManifestPath = [string]$mapping.manifestPath
    mappingsSha256 = [string]$mapping.sha256
    cue4ParseBinarySha256 = $cue4ParseBinarySha256
    inspectorSourceSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $inspectorSource).Hash
    jsonFile = [IO.Path]::GetFileName($jsonPath)
    jsonLength = $jsonItem.Length
    jsonSha256 = $jsonSha256
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    $temporaryManifest,
    (($assetManifest | ConvertTo-Json -Depth 6) + [Environment]::NewLine))
Move-Item -LiteralPath $temporaryJson -Destination $jsonPath
Move-Item -LiteralPath $temporaryManifest -Destination $manifestPath
[IO.Directory]::Delete($staging, $true)

[pscustomobject]@{
    cacheStatus = 'miss-generated'
    steamBuildId = $steamBuildId
    containerView = [string]$containerView.name
    virtualPath = $virtualPath
    jsonPath = (Resolve-Path -LiteralPath $jsonPath).Path
    manifestPath = (Resolve-Path -LiteralPath $manifestPath).Path
    jsonSha256 = $jsonSha256
}
