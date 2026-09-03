[CmdletBinding(DefaultParameterSetName = 'Asset')]
param(
    [Parameter(Mandatory = $true, Position = 0, ParameterSetName = 'Asset')]
    [string]$Query,

    [Parameter(Mandatory = $true, ParameterSetName = 'List')]
    [switch]$ListPackages,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$MappingsPath,

    [string]$MappingsManifestPath,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_8',

    [ValidateSet('Game', 'Mod')]
    [string]$Source = 'Game',

    [string]$ModContainer
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($Source -eq 'Game' -and -not [string]::IsNullOrWhiteSpace($ModContainer)) {
    throw '-ModContainer is valid only with -Source Mod.'
}
if ($Source -eq 'Mod' -and [string]::IsNullOrWhiteSpace($ModContainer)) {
    throw '-Source Mod requires -ModContainer with one exact mod .utoc file.'
}

$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$getMappingsScript = Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1'
$testMappingsScript = Join-Path $PSScriptRoot 'Test-VoyageMappings.ps1'
$inspectorProject = Join-Path $PSScriptRoot 'VoyageAssetInspector\VoyageAssetInspector.csproj'
$inspectorSource = Join-Path $PSScriptRoot 'VoyageAssetInspector\Program.cs'
$cue4ParseBinary = Join-Path $PSScriptRoot '..\.tools\bin\CUE4Parse\CUE4Parse.dll'
$cacheRoot = Join-Path $PSScriptRoot '..\artifacts\asset-cache'
$inspectionRoot = Join-Path $PSScriptRoot '..\artifacts\asset-inspections'
$cacheSchemaVersion = 2
$sha256Pattern = '^[0-9A-F]{64}$'

function Get-TextSha256 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $hash = $hasher.ComputeHash($bytes)
    }
    finally {
        $hasher.Dispose()
    }
    [BitConverter]::ToString($hash).Replace('-', '')
}

function Get-ContainerDescriptor {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Records
    )

    $items = @($Records | ForEach-Object {
        $name = [Convert]::ToString($_['name'], [Globalization.CultureInfo]::InvariantCulture)
        $length = [Convert]::ToInt64($_['length'], [Globalization.CultureInfo]::InvariantCulture)
        $sha256 = [Convert]::ToString($_['sha256'], [Globalization.CultureInfo]::InvariantCulture)
        '{"name":"' + $name + '","length":' +
            $length.ToString([Globalization.CultureInfo]::InvariantCulture) +
            ',"sha256":"' + $sha256 + '"}'
    })
    '[' + ($items -join ',') + ']'
}

function Get-FilesInOrdinalNameOrder {
    param(
        [Parameter(Mandatory = $true)]
        [IO.FileInfo[]]$Files
    )

    $filesByName = @{}
    foreach ($file in $Files) {
        $filesByName[$file.Name] = $file
    }
    $names = [string[]]@($filesByName.Keys)
    [Array]::Sort($names, [StringComparer]::Ordinal)
    foreach ($name in $names) {
        $filesByName[$name]
    }
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

function Test-IsGameContainerName {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $Name.Equals('global.utoc', [StringComparison]::OrdinalIgnoreCase) -or
        $Name -match '^(?i:pakchunk\d+(?:optional)?-Windows\.utoc)$'
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

        [ValidateSet('Game', 'Mod', 'ModOnly')]
        [string]$ContainerSelection = 'Game',

        [string]$SelectedModContainer,

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
        $EngineVersion,
        '-',
        $ContainerSelection,
        $(if ($SelectedModContainer) { $SelectedModContainer } else { '-' })
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

    $resolved = & $getMappingsScript -GameRoot $GameRoot
    if ([string]$resolved.steamBuildId -cne $SteamBuildId -or
        [string]$resolved.executableSha256 -cne $ExecutableSha256) {
        throw 'The reviewed mapping resolver returned a mismatched game fingerprint.'
    }
    [pscustomobject]@{
        mappingsPath = [string]$resolved.mappingsPath
        manifestPath = [string]$resolved.manifestPath
        sha256 = [string]$resolved.sha256
    }
}

function Get-GameContainerSet {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PaksDirectory
    )

    $allContainers = @(Get-FilesInOrdinalNameOrder -Files @(
        Get-ChildItem -LiteralPath $PaksDirectory -Filter '*.utoc' -File
    ))
    $gameContainers = @(
        $allContainers |
            Where-Object { Test-IsGameContainerName -Name $_.Name }
    )
    if ($gameContainers.Count -eq 0) {
        throw "No stock Voyage IoStore containers were found in: $PaksDirectory"
    }
    $gameRecords = @($gameContainers | ForEach-Object {
        [ordered]@{
            name = $_.Name
            length = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    })
    $gameDescriptor = Get-ContainerDescriptor -Records $gameRecords
    $gameHash = Get-TextSha256 -Text $gameDescriptor
    [pscustomobject]@{
        gameContainers = $gameRecords
        gameContainersSha256 = $gameHash
    }
}

function Resolve-ModContainer {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PaksDirectory,

        [Parameter(Mandatory = $true)]
        [string]$Container
    )

    $candidate = if ([IO.Path]::IsPathRooted($Container)) {
        $Container
    }
    else {
        Join-Path $PaksDirectory $Container
    }
    if (-not $candidate.EndsWith('.utoc', [StringComparison]::OrdinalIgnoreCase)) {
        $candidate += '.utoc'
    }
    $resolved = (Resolve-Path -LiteralPath $candidate).Path
    $name = [IO.Path]::GetFileName($resolved)
    if (Test-IsGameContainerName -Name $name) {
        throw "The selected container is a stock game container, not a mod: $resolved"
    }
    [pscustomobject]@{
        path = $resolved
        name = $name
        length = (Get-Item -LiteralPath $resolved).Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolved).Hash
    }
}

function Get-PackageIndex {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PaksDirectory,

        [Parameter(Mandatory = $true)]
        [string]$CacheVersionRoot,

        [Parameter(Mandatory = $true)]
        [string]$SteamBuildId,

        [Parameter(Mandatory = $true)]
        [string]$ExecutableSha256,

        [Parameter(Mandatory = $true)]
        [string]$Cue4ParseBinarySha256,

        [Parameter(Mandatory = $true)]
        [object]$GameContainerSet
    )

    $catalogRoot = Join-Path $CacheVersionRoot '_catalog'
    $indexPath = Join-Path $catalogRoot 'packages.txt'
    $manifestPath = Join-Path $catalogRoot 'package-index-manifest.json'
    if ((Test-Path -LiteralPath $indexPath -PathType Leaf) -and
        (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $indexPath).Hash
        if ([int](Get-OptionalPropertyValue -Object $manifest -Name 'cacheSchemaVersion') -eq $cacheSchemaVersion -and
            [string](Get-OptionalPropertyValue -Object $manifest -Name 'source') -ceq 'Game' -and
            [string]$manifest.kind -ceq 'Voyage asset package index' -and
            [string]$manifest.steamBuildId -ceq $SteamBuildId -and
            [string]$manifest.executableSha256 -ceq $ExecutableSha256 -and
            [string]$manifest.cue4ParseBinarySha256 -ceq $Cue4ParseBinarySha256 -and
            [string]$manifest.inspectorSourceSha256 -ceq (Get-FileHash -Algorithm SHA256 -LiteralPath $inspectorSource).Hash -and
            [string]$manifest.gameContainersSha256 -ceq [string]$GameContainerSet.gameContainersSha256 -and
            [string]$manifest.packageIndexSha256 -ceq $actualHash) {
            return (Resolve-Path -LiteralPath $indexPath).Path
        }
        Write-Warning "Discarding a stale game package index: $catalogRoot"
        [IO.Directory]::Delete($catalogRoot, $true)
    }
    elseif ((Test-Path -LiteralPath $indexPath) -or (Test-Path -LiteralPath $manifestPath)) {
        Write-Warning "Discarding an incomplete game package index: $catalogRoot"
        [IO.Directory]::Delete($catalogRoot, $true)
    }

    $stagingRoot = Join-Path $CacheVersionRoot '_staging'
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
        cacheSchemaVersion = $cacheSchemaVersion
        kind = 'Voyage asset package index'
        source = 'Game'
        steamBuildId = $SteamBuildId
        executableSha256 = $ExecutableSha256
        cue4ParseBinarySha256 = $Cue4ParseBinarySha256
        engineVersion = $EngineVersion
        gameContainers = @($GameContainerSet.gameContainers)
        gameContainersSha256 = [string]$GameContainerSet.gameContainersSha256
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

    $packages = @(Get-Content -LiteralPath $PackageIndexPath | ForEach-Object { [string]$_ })
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
            ([string]$_).IndexOf($normalized, [StringComparison]::OrdinalIgnoreCase) -ge 0
        })
    }
    if ($matches.Count -eq 0) {
        throw "No asset package matches '$Query' in the current package index."
    }
    if ($matches.Count -gt 1) {
        $shown = @($matches | Select-Object -First 20) -join [Environment]::NewLine
        throw "Asset query '$Query' is ambiguous ($($matches.Count) matches). Use one exact virtual path:`n$shown"
    }
    [string]$matches[0]
}

function New-PackageListResult {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('Game', 'Mod')]
        [string]$ResultSource,

        [Parameter(Mandatory = $true)]
        [string]$PackageIndexPath
    )

    $resolvedPath = (Resolve-Path -LiteralPath $PackageIndexPath).Path
    $packageCount = @(
        Get-Content -LiteralPath $resolvedPath |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }
    ).Count
    [pscustomobject]@{
        source = $ResultSource
        packageListPath = $resolvedPath
        packageCount = $packageCount
        packageListSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolvedPath).Hash
    }
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
$gameContainerSet = Get-GameContainerSet -PaksDirectory $paksDirectory
$versionName = "steam-$steamBuildId-$($executableSha256.Substring(0, 12))-game-$($gameContainerSet.gameContainersSha256.Substring(0, 12))"
$cacheVersionRoot = Join-Path ([IO.Path]::GetFullPath($cacheRoot)) $versionName

if ($Source -eq 'Mod') {
    $selectedMod = Resolve-ModContainer -PaksDirectory $paksDirectory -Container $ModContainer
    $runName = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
    $runRoot = Join-Path ([IO.Path]::GetFullPath($inspectionRoot)) $runName
    $catalogRoot = Join-Path $runRoot 'catalog'
    [IO.Directory]::CreateDirectory($catalogRoot) | Out-Null
    $catalogLog = Join-Path $catalogRoot 'inspector.log'
    Write-Host "Indexing assets from mod container $($selectedMod.name)..."
    Invoke-Inspector `
        -PaksDirectory $paksDirectory `
        -AssetQuery 'list:' `
        -OutputDirectory $catalogRoot `
        -ContainerSelection 'ModOnly' `
        -SelectedModContainer $selectedMod.path `
        -LogPath $catalogLog

    $packageIndexPath = Join-Path $catalogRoot 'matches.txt'
    if (-not (Test-Path -LiteralPath $packageIndexPath -PathType Leaf)) {
        throw "VoyageAssetInspector did not produce a mod package index. Inspection: $runRoot"
    }
    if ($ListPackages) {
        $listResult = New-PackageListResult -ResultSource 'Mod' -PackageIndexPath $packageIndexPath
        $inspectionManifest = [ordered]@{
            kind = 'Voyage one-off mod package listing'
            source = 'Mod'
            steamBuildId = $steamBuildId
            executableSha256 = $executableSha256
            engineVersion = $EngineVersion
            modContainer = $selectedMod
            packageListPath = $listResult.packageListPath
            packageCount = $listResult.packageCount
            packageListSha256 = $listResult.packageListSha256
            cue4ParseBinarySha256 = $cue4ParseBinarySha256
            inspectorSourceSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $inspectorSource).Hash
            generatedAtUtc = [DateTime]::UtcNow.ToString('o')
        }
        [IO.File]::WriteAllText(
            (Join-Path $runRoot 'inspection-manifest.json'),
            (($inspectionManifest | ConvertTo-Json -Depth 6) + [Environment]::NewLine))
        $listResult
        return
    }

    $mapping = Resolve-ValidatedMappings `
        -SteamBuildId $steamBuildId `
        -ExecutableSha256 $executableSha256
    $virtualPath = Resolve-AssetVirtualPath -PackageIndexPath $packageIndexPath
    $exportRoot = Join-Path $runRoot 'export'
    [IO.Directory]::CreateDirectory($exportRoot) | Out-Null
    $exportLog = Join-Path $exportRoot 'inspector.log'
    Write-Host "Exporting $virtualPath from $($selectedMod.name)..."
    Invoke-Inspector `
        -PaksDirectory $paksDirectory `
        -AssetQuery $virtualPath `
        -OutputDirectory $exportRoot `
        -MappingFile $mapping.mappingsPath `
        -ContainerSelection 'Mod' `
        -SelectedModContainer $selectedMod.path `
        -LogPath $exportLog

    $matches = @(Get-Content -LiteralPath (Join-Path $exportRoot 'matches.txt'))
    $unexpectedMatches = @($matches | Where-Object { $_ -cne $virtualPath })
    if ($matches.Count -lt 1 -or $unexpectedMatches.Count -ne 0) {
        throw "Exact mod asset export resolved unexpectedly. Inspection: $runRoot"
    }
    $errorPath = Join-Path $exportRoot 'errors.txt'
    if ((Test-Path -LiteralPath $errorPath -PathType Leaf) -and
        (Get-Item -LiteralPath $errorPath).Length -gt 0) {
        throw "VoyageAssetInspector could not parse the exact mod asset. Inspection: $runRoot"
    }
    $safeName = $virtualPath.Replace('/', '_').Replace('\', '_').Replace('.', '_') + '.json'
    $jsonPath = Join-Path $exportRoot $safeName
    if (-not (Test-Path -LiteralPath $jsonPath -PathType Leaf) -or
        (Get-Item -LiteralPath $jsonPath).Length -eq 0) {
        throw "VoyageAssetInspector did not produce the expected mod JSON. Inspection: $runRoot"
    }
    $jsonSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $jsonPath).Hash
    $inspectionManifest = [ordered]@{
        kind = 'Voyage one-off mod asset inspection'
        source = 'Mod'
        steamBuildId = $steamBuildId
        executableSha256 = $executableSha256
        engineVersion = $EngineVersion
        modContainer = $selectedMod
        virtualPath = $virtualPath
        mappingsPath = [string]$mapping.mappingsPath
        mappingsManifestPath = [string]$mapping.manifestPath
        mappingsSha256 = [string]$mapping.sha256
        cue4ParseBinarySha256 = $cue4ParseBinarySha256
        inspectorSourceSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $inspectorSource).Hash
        jsonPath = (Resolve-Path -LiteralPath $jsonPath).Path
        jsonLength = (Get-Item -LiteralPath $jsonPath).Length
        jsonSha256 = $jsonSha256
        generatedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    [IO.File]::WriteAllText(
        (Join-Path $runRoot 'inspection-manifest.json'),
        (($inspectionManifest | ConvertTo-Json -Depth 6) + [Environment]::NewLine))
    [pscustomobject]@{
        virtualPath = $virtualPath
        jsonPath = (Resolve-Path -LiteralPath $jsonPath).Path
        jsonSha256 = $jsonSha256
    }
    return
}

$packageIndexPath = Get-PackageIndex `
    -PaksDirectory $paksDirectory `
    -CacheVersionRoot $cacheVersionRoot `
    -SteamBuildId $steamBuildId `
    -ExecutableSha256 $executableSha256 `
    -Cue4ParseBinarySha256 $cue4ParseBinarySha256 `
    -GameContainerSet $gameContainerSet
if ($ListPackages) {
    New-PackageListResult -ResultSource 'Game' -PackageIndexPath $packageIndexPath
    return
}

$mapping = Resolve-ValidatedMappings `
    -SteamBuildId $steamBuildId `
    -ExecutableSha256 $executableSha256
$virtualPath = Resolve-AssetVirtualPath -PackageIndexPath $packageIndexPath

$relativeJsonPath = [IO.Path]::ChangeExtension($virtualPath, '.json').Replace('/', [IO.Path]::DirectorySeparatorChar)
$cacheVersionRootFull = [IO.Path]::GetFullPath($cacheVersionRoot)
$jsonPath = [IO.Path]::GetFullPath((Join-Path $cacheVersionRootFull $relativeJsonPath))
$requiredPrefix = $cacheVersionRootFull.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $jsonPath.StartsWith($requiredPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Resolved asset path escapes the cache root: $virtualPath"
}
$manifestPath = [IO.Path]::ChangeExtension($jsonPath, '.asset-manifest.json')

if ((Test-Path -LiteralPath $jsonPath -PathType Leaf) -and
    (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $actualJsonHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $jsonPath).Hash
    $provenanceFailures = [Collections.Generic.List[string]]::new()
    if ([int](Get-OptionalPropertyValue -Object $manifest -Name 'cacheSchemaVersion') -ne $cacheSchemaVersion) { $provenanceFailures.Add('cacheSchemaVersion') }
    if ([string](Get-OptionalPropertyValue -Object $manifest -Name 'source') -cne 'Game') { $provenanceFailures.Add('source') }
    if ([string]$manifest.kind -cne 'Voyage cached asset JSON') { $provenanceFailures.Add('kind') }
    if ([string]$manifest.steamBuildId -cne $steamBuildId) { $provenanceFailures.Add('steamBuildId') }
    if ([string]$manifest.executableSha256 -cne $executableSha256) { $provenanceFailures.Add('executableSha256') }
    if ([string]$manifest.gameContainersSha256 -cne [string]$gameContainerSet.gameContainersSha256) { $provenanceFailures.Add('gameContainersSha256') }
    if ([string]$manifest.virtualPath -cne $virtualPath) { $provenanceFailures.Add('virtualPath') }
    if ([string]$manifest.mappingsSha256 -cne [string]$mapping.sha256) { $provenanceFailures.Add('mappingsSha256') }
    if ([string]$manifest.cue4ParseBinarySha256 -cne $cue4ParseBinarySha256) { $provenanceFailures.Add('cue4ParseBinarySha256') }
    if ([string]$manifest.inspectorSourceSha256 -cne (Get-FileHash -Algorithm SHA256 -LiteralPath $inspectorSource).Hash) { $provenanceFailures.Add('inspectorSourceSha256') }
    if ([string]$manifest.jsonSha256 -cne $actualJsonHash) { $provenanceFailures.Add('jsonSha256') }
    if ([long]$manifest.jsonLength -ne (Get-Item -LiteralPath $jsonPath).Length) { $provenanceFailures.Add('jsonLength') }
    if ($provenanceFailures.Count -gt 0) {
        Write-Warning "Discarding stale game asset JSON ($($provenanceFailures -join ', ')): $jsonPath"
        [IO.File]::Delete($jsonPath)
        [IO.File]::Delete($manifestPath)
    }
    else {
        [pscustomobject]@{
            virtualPath = $virtualPath
            jsonPath = (Resolve-Path -LiteralPath $jsonPath).Path
            jsonSha256 = $actualJsonHash
        }
        return
    }
}
elseif ((Test-Path -LiteralPath $jsonPath) -or (Test-Path -LiteralPath $manifestPath)) {
    Write-Warning "Discarding an incomplete game asset cache entry: $jsonPath"
    [IO.File]::Delete($jsonPath)
    [IO.File]::Delete($manifestPath)
}

$stagingRoot = Join-Path $cacheVersionRoot '_staging'
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
    cacheSchemaVersion = $cacheSchemaVersion
    kind = 'Voyage cached asset JSON'
    source = 'Game'
    steamBuildId = $steamBuildId
    executableSha256 = $executableSha256
    engineVersion = $EngineVersion
    gameContainers = @($gameContainerSet.gameContainers)
    gameContainersSha256 = [string]$gameContainerSet.gameContainersSha256
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
    virtualPath = $virtualPath
    jsonPath = (Resolve-Path -LiteralPath $jsonPath).Path
    jsonSha256 = $jsonSha256
}
