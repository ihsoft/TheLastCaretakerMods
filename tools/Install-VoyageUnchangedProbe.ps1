param(
    [Parameter(Mandatory = $true)]
    [string]$PackageRoot,

    [Parameter(Mandatory = $true)]
    [string]$EvidenceRoot,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)

$ErrorActionPreference = 'Stop'

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    throw 'Voyage must be closed before installing an unchanged-roundtrip probe.'
}

$package = (Resolve-Path -LiteralPath $PackageRoot).Path
$buildManifestPath = Join-Path $package 'build-manifest.json'
$build = Get-Content -Raw -LiteralPath $buildManifestPath | ConvertFrom-Json
$containerName = [string]$build.containerName
$sha256Pattern = '^[0-9A-F]{64}$'
$steamBuildIdPattern = '^\d+$'
if ([string]$build.kind -cne 'Voyage unchanged roundtrip probe' -or
    [string]$build.probeLayer -cnotin @('retoc-only', 'uassetapi-save') -or
    $containerName -cnotmatch '^Voyage[A-Za-z0-9]+Roundtrip_P$' -or
    [string]$build.sourcePackage -cnotmatch '^/Game/[A-Za-z0-9_/-]+$' -or
    -not [bool]$build.retocRoundtripByteIdentical -or
    [string]$build.steamBuildId -cnotmatch $steamBuildIdPattern -or
    [string]$build.executableSha256 -cnotmatch $sha256Pattern -or
    [string]$build.retocSha256 -notmatch $sha256Pattern) {
    throw 'The package is not an allowed current-build unchanged-roundtrip probe.'
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$exe = Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$actualExeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash
$steamApps = Split-Path -Parent (Split-Path -Parent $root)
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
$buildMatch = [regex]::Match([IO.File]::ReadAllText($steamManifest), '"buildid"\s+"(?<id>\d+)"')
if (-not $buildMatch.Success -or
    $buildMatch.Groups['id'].Value -cne [string]$build.steamBuildId -or
    $actualExeHash -cne [string]$build.executableSha256) {
    throw 'Installed Voyage fingerprint does not match the probe build.'
}

$paks = (Resolve-Path -LiteralPath (Join-Path $root 'Voyage\Content\Paks')).Path
$evidence = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $evidence) {
    throw "EvidenceRoot already exists: $evidence"
}
$extensions = @('.pak', '.ucas', '.utoc')
$records = @()
foreach ($extension in $extensions) {
    $name = $containerName + $extension
    $source = Join-Path $package $name
    $declared = @($build.containerFiles | Where-Object { [string]$_.name -ceq $name })
    if ($declared.Count -ne 1 -or -not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Probe file or its unique manifest record is missing: $name"
    }
    $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
    if ($sourceHash -cne [string]$declared[0].sha256 -or
        (Get-Item -LiteralPath $source).Length -ne [long]$declared[0].length) {
        throw "Probe file does not match its build manifest: $source"
    }
    $destination = Join-Path $paks $name
    if (Test-Path -LiteralPath $destination) {
        throw "A same-name container already exists; refusing replacement: $destination"
    }
    $records += [pscustomobject][ordered]@{
        name = $name
        sourceSha256 = $sourceHash
        installedSha256 = $sourceHash
    }
}

[IO.Directory]::CreateDirectory($evidence) | Out-Null
foreach ($record in $records) {
    Copy-Item -LiteralPath (Join-Path $package $record.name) -Destination (Join-Path $paks $record.name)
    $installedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $paks $record.name)).Hash
    if ($installedHash -cne $record.installedSha256) {
        throw "Installed probe hash mismatch: $($record.name)"
    }
}

$install = [ordered]@{
    kind = 'Voyage unchanged roundtrip probe installation'
    probeLayer = [string]$build.probeLayer
    sourcePackage = [string]$build.sourcePackage
    containerName = $containerName
    paksDirectory = $paks
    files = $records
    installedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$installPath = Join-Path $evidence 'install-manifest.json'
[IO.File]::WriteAllText($installPath, (($install | ConvertTo-Json -Depth 4) + [Environment]::NewLine))
Write-Host "Installed unchanged-roundtrip probe: $containerName"
Write-Host "Removal manifest: $installPath"
