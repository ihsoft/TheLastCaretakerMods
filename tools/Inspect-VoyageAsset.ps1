param(
    [Parameter(Mandatory = $true)]
    [string]$Query,

    [string]$MappingsPath,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_7',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$OutputRoot = "$PSScriptRoot\..\artifacts\inspection"
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$paks = Join-Path $root 'Voyage\Content\Paks'
$exe = Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$project = Join-Path $PSScriptRoot 'VoyageAssetInspector\VoyageAssetInspector.csproj'
$cue4ParseBinary = Join-Path $PSScriptRoot '..\.tools\bin\CUE4Parse\CUE4Parse.dll'
if (-not (Test-Path -LiteralPath $cue4ParseBinary -PathType Leaf)) {
    throw 'Canonical CUE4Parse bundle is missing. Run tools\Publish-Cue4ParseBinary.ps1.'
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
$buildId = "steam-$steamBuildId-$($exeHash.Substring(0, 12))"
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
    'run',
    '--project', $project,
    '--configuration', 'Release',
    '--',
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

& dotnet @arguments
if ($LASTEXITCODE -ne 0) {
    throw "VoyageAssetInspector failed with exit code $LASTEXITCODE"
}

$manifest = [ordered]@{
    steamAppId = '1783560'
    steamBuildId = $steamBuildId
    gameVersion = $version
    executableSha256 = $exeHash
    query = $Query
    mappingsPath = if ($MappingsPath) { (Resolve-Path -LiteralPath $MappingsPath).Path } else { $null }
    cue4ParseBinaryPath = (Resolve-Path -LiteralPath $cue4ParseBinary).Path
    cue4ParseBinarySha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $cue4ParseBinary).Hash
    engineVersion = $EngineVersion
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'inspection-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Inspected current-game assets into: $output"
