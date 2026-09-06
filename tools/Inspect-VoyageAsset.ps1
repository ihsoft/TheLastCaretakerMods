param(
    [Parameter(Mandatory = $true)]
    [string]$Query,

    [string]$MappingsPath,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_8',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$OutputRoot,

    [ValidateSet('Game', 'Mod')]
    [string]$Source = 'Game',

    [string]$ModContainer,

    [switch]$RequireMatch
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $PSScriptRoot '..\artifacts\inspection'
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$paks = Join-Path $root 'Voyage\Content\Paks'
$exe = Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$inspector = & (Join-Path $PSScriptRoot 'Get-VoyageAssetInspectorBinary.ps1')
$cue4ParseBinary = Join-Path $PSScriptRoot '..\.tools\bin\CUE4Parse\CUE4Parse.dll'
$getMappingsScript = Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1'
$testMappingsScript = Join-Path $PSScriptRoot 'Test-VoyageMappings.ps1'
if ($Source -eq 'Game' -and -not [string]::IsNullOrWhiteSpace($ModContainer)) {
    throw '-ModContainer is valid only with -Source Mod.'
}
if ($Source -eq 'Mod' -and [string]::IsNullOrWhiteSpace($ModContainer)) {
    throw '-Source Mod requires -ModContainer with one exact mod .utoc file.'
}
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Voyage executable not found: $exe"
}

$version = (Get-Item -LiteralPath $exe).VersionInfo.ProductVersion
$exeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash
$steamApps = Split-Path -Parent (Split-Path -Parent $root)
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
$steamBuildId = 'unknown'
if (Test-Path -LiteralPath $steamManifest -PathType Leaf) {
    $buildMatch = [regex]::Match([IO.File]::ReadAllText($steamManifest), '"buildid"\s+"(?<id>\d+)"')
    if ($buildMatch.Success) {
        $steamBuildId = $buildMatch.Groups['id'].Value
    }
}
$mappingsManifestPath = $null
if ($MappingsPath) {
    $MappingsPath = (Resolve-Path -LiteralPath $MappingsPath).Path
    $mappingsManifestPath = Join-Path (Split-Path -Parent $MappingsPath) 'mapping-manifest.json'
    if (-not (Test-Path -LiteralPath $mappingsManifestPath -PathType Leaf)) {
        throw "Mappings manifest not found beside the explicit mapping: $mappingsManifestPath"
    }
    & $testMappingsScript `
        -MappingsPath $MappingsPath `
        -ManifestPath $mappingsManifestPath `
        -ExpectedSteamBuildId $steamBuildId `
        -ExpectedExecutableSha256 $exeHash | Out-Null
}
elseif (-not $Query.StartsWith('list:', [StringComparison]::OrdinalIgnoreCase)) {
    $resolvedMappings = & $getMappingsScript -GameRoot $GameRoot
    $MappingsPath = [string]$resolvedMappings.mappingsPath
    $mappingsManifestPath = [string]$resolvedMappings.manifestPath
}
$selectedMod = $null
$sourceIdentity = 'game'
if ($Source -eq 'Mod') {
    $selectedModPath = (Resolve-Path -LiteralPath $ModContainer).Path
    $selectedModItem = Get-Item -LiteralPath $selectedModPath
    if ($selectedModItem.PSIsContainer -or $selectedModItem.Extension -ine '.utoc') {
        throw '-ModContainer must be one exact .utoc file.'
    }
    if ($selectedModItem.Name -match '^(?i:global|pakchunk\d+(?:optional)?-Windows)\.utoc$') {
        throw "-ModContainer must identify an additional mod container, not stock: $selectedModPath"
    }
    $selectedModData = [IO.Path]::ChangeExtension($selectedModPath, '.ucas')
    if (-not (Test-Path -LiteralPath $selectedModData -PathType Leaf)) {
        throw "Selected mod container has no adjacent .ucas file: $selectedModData"
    }
    $isInstalledMod = (Split-Path -Parent $selectedModPath) -ieq $paks
    $selectedMod = [pscustomobject]@{
        name = $selectedModItem.Name
        path = $selectedModPath
        length = $selectedModItem.Length
        sha256 = (Get-FileHash -LiteralPath $selectedModPath -Algorithm SHA256).Hash
        isInstalled = $isInstalledMod
    }
    $sourceLocation = if ($isInstalledMod) { 'installed' } else { 'external' }
    $sourceIdentity = 'mod-' + $sourceLocation + '-' + $selectedModItem.BaseName +
        '-' + $selectedMod.sha256.Substring(0, 12)
}
$buildId = "steam-$steamBuildId-$($exeHash.Substring(0, 12))-$sourceIdentity"
$querySafe = ($Query -replace '[^A-Za-z0-9._-]', '_').Trim('_')
if (-not $querySafe) {
    $querySafe = 'query'
}
$output = [IO.Path]::GetFullPath((Join-Path $OutputRoot "$buildId\$querySafe"))
if (Test-Path -LiteralPath $output) {
    throw "Output already exists; use a new query or remove the local artifact explicitly: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

$arguments = @(
    $paks,
    $Query,
    $output
)
if ($MappingsPath) {
    $arguments += (Resolve-Path -LiteralPath $MappingsPath).Path
}
else {
    $arguments += '-'
}
$arguments += $EngineVersion
$arguments += '-'
$arguments += $Source
$arguments += $(if ($null -ne $selectedMod) { $selectedMod.path } else { '-' })

$isReferenceQuery = $Query.StartsWith('references:', [StringComparison]::OrdinalIgnoreCase)
$logPath = Join-Path $output 'inspector.log'
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $inspector.Path @arguments *> $logPath
    $inspectorExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $savedPreference
}

$queryStatus = 'completed'
$matchCount = $null
$resultPath = $null
$errorCount = 0
if ($isReferenceQuery) {
    $resultPath = Join-Path $output 'reference-matches.txt'
    $errorPath = Join-Path $output 'reference-search-errors.txt'
    if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
        throw "VoyageAssetInspector did not produce its reference result: $resultPath"
    }
    $matches = @(Get-Content -LiteralPath $resultPath | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_)
    })
    $matchCount = $matches.Count
    if (Test-Path -LiteralPath $errorPath -PathType Leaf) {
        $errors = @(Get-Content -LiteralPath $errorPath | Where-Object {
            -not [string]::IsNullOrWhiteSpace($_)
        })
        $errorCount = $errors.Count
    }

    $isCleanNoMatch = $inspectorExitCode -eq 1 -and $matchCount -eq 0 -and $errorCount -eq 0
    if ($inspectorExitCode -ne 0 -and -not $isCleanNoMatch) {
        throw "VoyageAssetInspector reference search failed with exit code $inspectorExitCode. Log: $logPath"
    }
    if ($errorCount -ne 0) {
        throw "VoyageAssetInspector reference search reported $errorCount error line(s); inspect $errorPath"
    }
    $queryStatus = if ($matchCount -eq 0) { 'no-match' } else { 'matched' }
}
elseif ($inspectorExitCode -ne 0) {
    throw "VoyageAssetInspector failed with exit code $inspectorExitCode. Log: $logPath"
}
else {
    $matchesPath = Join-Path $output 'matches.txt'
    if (Test-Path -LiteralPath $matchesPath -PathType Leaf) {
        $matchCount = @(Get-Content -LiteralPath $matchesPath | Where-Object {
            -not [string]::IsNullOrWhiteSpace($_)
        }).Count
    }
}

$manifest = [ordered]@{
    steamAppId = '1783560'
    steamBuildId = $steamBuildId
    gameVersion = $version
    executableSha256 = $exeHash
    query = $Query
    mappingsPath = $MappingsPath
    mappingsManifestPath = $mappingsManifestPath
    cue4ParseBinaryPath = (Resolve-Path -LiteralPath $cue4ParseBinary).Path
    cue4ParseBinarySha256 = $inspector.Cue4ParseSha256
    inspectorBinaryPath = $inspector.Path
    inspectorBinarySha256 = $inspector.Sha256
    inspectorInputFingerprint = $inspector.InputFingerprint
    engineVersion = $EngineVersion
    source = $Source
    modContainer = $selectedMod
    queryStatus = $queryStatus
    matchCount = $matchCount
    errorCount = $errorCount
    logPath = $logPath
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$manifestPath = Join-Path $output 'inspection-manifest.json'
[IO.File]::WriteAllText(
    $manifestPath,
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

$result = [pscustomobject][ordered]@{
    status = $queryStatus
    matchCount = $matchCount
    resultPath = if ($isReferenceQuery) { $resultPath } else { $output }
    manifestPath = $manifestPath
    logPath = $logPath
    query = $Query
    source = $Source
    modContainer = $selectedMod
}
if ($isReferenceQuery -and $RequireMatch -and $matchCount -eq 0) {
    throw "Reference search completed successfully but found no matches: $Query"
}
Write-Output $result
