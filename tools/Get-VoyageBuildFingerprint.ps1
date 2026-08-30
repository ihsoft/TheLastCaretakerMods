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

$exeItem = Get-Item -LiteralPath $exe
$exeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash
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
$containers = Get-ChildItem -LiteralPath $paks -Filter '*.utoc' -File |
    Where-Object { $_.Name -notmatch 'DonkLift|KeyboardControl|HudProbe' } |
    Sort-Object Name |
    ForEach-Object {
        [ordered]@{
            name = $_.Name
            length = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    }

$fingerprint = [ordered]@{
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    gameRoot = $root
    steam = [ordered]@{
        appId = '1783560'
        buildId = $steamBuildId
    }
    executable = [ordered]@{
        name = $exeItem.Name
        length = $exeItem.Length
        fileVersion = $exeItem.VersionInfo.FileVersion
        productVersion = $exeItem.VersionInfo.ProductVersion
        sha256 = $exeHash
    }
    containers = @($containers)
}

$json = $fingerprint | ConvertTo-Json -Depth 5
if ($OutputPath) {
    $fullOutput = [IO.Path]::GetFullPath($OutputPath)
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($fullOutput)) | Out-Null
    [IO.File]::WriteAllText($fullOutput, $json + [Environment]::NewLine)
}
$json
