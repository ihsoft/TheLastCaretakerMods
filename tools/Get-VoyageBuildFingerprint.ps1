param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$exe = Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$paks = Join-Path $root 'Voyage\Content\Paks'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Voyage executable not found: $exe"
}
if (-not (Test-Path -LiteralPath $paks -PathType Container)) {
    throw "Voyage Paks directory not found: $paks"
}

$steamApps = Split-Path -Parent (Split-Path -Parent $root)
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
$steamBuildId = $null
if (Test-Path -LiteralPath $steamManifest -PathType Leaf) {
    $buildMatch = [regex]::Match(
        [IO.File]::ReadAllText($steamManifest),
        '"buildid"\s+"(?<id>\d+)"')
    if ($buildMatch.Success) {
        $steamBuildId = $buildMatch.Groups['id'].Value
    }
}
$fingerprint = [ordered]@{
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    gameRoot = $root
    steam = [ordered]@{
        appId = '1783560'
        buildId = $steamBuildId
    }
}

$json = $fingerprint | ConvertTo-Json -Depth 5
if ($OutputPath) {
    $fullOutput = [IO.Path]::GetFullPath($OutputPath)
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($fullOutput)) | Out-Null
    [IO.File]::WriteAllText($fullOutput, $json + [Environment]::NewLine)
}
$json
