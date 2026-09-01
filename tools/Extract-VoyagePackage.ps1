param(
    [Parameter(Mandatory = $true)]
    [string]$Filter,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc = 'R:\Codex\ToolCache\rust-retoc-master\source\target\release\retoc.exe',

    [string]$RetocEngineVersion = 'UE5_7',

    [string]$OutputRoot = "$PSScriptRoot\..\artifacts\extracted",

    [switch]$AllowAdditionalContainers
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$retocSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $retocPath).Hash
$paks = Join-Path $root 'Voyage\Content\Paks'
$exe = Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
if (-not (Test-Path -LiteralPath $paks -PathType Container)) {
    throw "Voyage Paks directory not found: $paks"
}
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Voyage executable not found: $exe"
}

# retoc resolves duplicate package paths across every container in the input
# directory. An installed mod can therefore silently shadow the game asset we
# intended to refresh. Refuse that ambiguous input unless the caller explicitly
# wants to inspect additional containers, and record that choice in provenance.
$additionalContainers = @(
    Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc' |
        Where-Object { $_.Name -ne 'global.utoc' -and $_.Name -notlike 'pakchunk*.utoc' }
)
if (-not $AllowAdditionalContainers) {
    if ($additionalContainers.Count -gt 0) {
        $names = ($additionalContainers.Name | Sort-Object) -join ', '
        throw "Additional IoStore containers can shadow game assets: $names. " +
            'Back them up and remove them while the game is closed, or pass ' +
            '-AllowAdditionalContainers only when that shadowing is intentional.'
    }
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
$buildId = "steam-$steamBuildId-$($exeHash.Substring(0, 12))"
$filterSafe = ($Filter -replace '[^A-Za-z0-9._-]', '_').Trim('_')
if (-not $filterSafe) {
    $filterSafe = 'query'
}
$output = [IO.Path]::GetFullPath((Join-Path $OutputRoot "$buildId\$filterSafe"))
if (Test-Path -LiteralPath $output) {
    throw "Output already exists; use a new query or remove the local artifact explicitly: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

& $retocPath to-legacy --version $RetocEngineVersion --filter $Filter $paks $output
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-legacy failed with exit code $LASTEXITCODE"
}
$extractedAssets = @(Get-ChildItem -LiteralPath $output -File -Recurse -Filter '*.uasset')
if ($extractedAssets.Count -eq 0) {
    throw "Filter extracted no assets: $Filter"
}

$manifest = [ordered]@{
    steamAppId = '1783560'
    steamBuildId = $steamBuildId
    gameVersion = $version
    executableSha256 = $exeHash
    retocEngineVersion = $RetocEngineVersion
    retocPath = $retocPath
    retocSha256 = $retocSha256
    filter = $Filter
    allowAdditionalContainers = [bool]$AllowAdditionalContainers
    additionalContainers = @($additionalContainers.Name | Sort-Object)
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'extraction-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Extracted current-game package data to: $output"
