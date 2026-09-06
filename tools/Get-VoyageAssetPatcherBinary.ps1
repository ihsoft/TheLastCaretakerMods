# Read-only resolver. Never invokes a compiler, package manager, or publisher.
[CmdletBinding()]
param(
    [switch]$BuildInputsOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$sourceRoot = Join-Path $PSScriptRoot 'VoyageAssetPatcher'
$bundleRoot = Join-Path $repositoryRoot '.tools\bin\UAssetAPI'
$executablePath = Join-Path $repositoryRoot '.tools\bin\VoyageAssetPatcher.exe'
$manifestPath = Join-Path $repositoryRoot '.tools\bin\VoyageAssetPatcher.publish-manifest.json'
$publishHint = 'Run tools\Publish-VoyageAssetPatcherBinary.ps1 explicitly.'
$expectedUAssetApiCommit = '21c982fa8f04e12d5d216fdf330a2f206e81156f'
$expectedUAssetApiSha256 = '6DF2606BBA89987AEB4BF1EFBD3C64AC565DBC5D6113A0A7A5062C7CD8B249FD'
$runtimeBundleNames = @('Newtonsoft.Json.dll', 'UAssetAPI.dll', 'ZstdSharp.dll')

$bundleManifestPath = Join-Path $bundleRoot 'publish-manifest.json'
if (-not (Test-Path -LiteralPath $bundleManifestPath -PathType Leaf)) {
    throw 'Canonical UAssetAPI manifest is missing. Run tools\Publish-UAssetApiBinary.ps1.'
}
$bundleManifest = Get-Content -LiteralPath $bundleManifestPath -Raw | ConvertFrom-Json
if ([string]$bundleManifest.kind -cne 'Voyage canonical UAssetAPI bundle' -or
    [string]$bundleManifest.sourceCommit -cne $expectedUAssetApiCommit -or
    [string]$bundleManifest.targetFramework -cne 'net10.0') {
    throw 'UAssetAPI does not match the accepted Voyage checkpoint.'
}

$inputs = @()
foreach ($file in @($bundleManifest.files | Sort-Object path)) {
    $name = [string]$file.path
    if ([IO.Path]::GetFileName($name) -cne $name) {
        throw "Invalid UAssetAPI bundle path: $name"
    }
    $path = Join-Path $bundleRoot $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
        (Get-Item -LiteralPath $path).Length -ne [long]$file.length -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -cne
            [string]$file.sha256) {
        throw "Canonical UAssetAPI bundle hash/size mismatch: $name"
    }
    if ($runtimeBundleNames -contains $name) {
        $inputs += [pscustomobject][ordered]@{
            path = 'UAssetAPI/' + $name
            sha256 = [string]$file.sha256
        }
    }
}
$resolvedRuntimeNames = @($inputs | ForEach-Object {
    [IO.Path]::GetFileName([string]$_.path)
} | Sort-Object)
if (@(Compare-Object ($runtimeBundleNames | Sort-Object) $resolvedRuntimeNames).Count -ne 0) {
    throw 'Canonical UAssetAPI runtime inputs are incomplete.'
}
$uassetApiDll = @($inputs | Where-Object path -ceq 'UAssetAPI/UAssetAPI.dll')
if ($uassetApiDll.Count -ne 1 -or
    [string]$uassetApiDll[0].sha256 -cne $expectedUAssetApiSha256) {
    throw 'UAssetAPI DLL does not match the accepted binary.'
}
$actualBundleNames = @(Get-ChildItem -LiteralPath $bundleRoot -File |
    Where-Object Name -cne 'publish-manifest.json' |
    ForEach-Object Name |
    Sort-Object)
$declaredBundleNames = @($bundleManifest.files |
    ForEach-Object { [string]$_.path } |
    Sort-Object)
if (@(Compare-Object $actualBundleNames $declaredBundleNames).Count -ne 0) {
    throw 'Canonical UAssetAPI bundle contains undeclared or missing files.'
}

foreach ($file in @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File |
    Where-Object {
        $_.FullName -notmatch '[\\/](bin|obj)[\\/]' -and
        $_.Extension -in @('.cs', '.csproj', '.props', '.targets', '.json')
    } |
    Sort-Object FullName)) {
    $inputs += [pscustomobject][ordered]@{
        path = 'source/' + $file.FullName.Substring($sourceRoot.Length + 1).Replace('\', '/')
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    }
}
foreach ($name in @(
    'Directory.Build.props',
    'Directory.Build.targets',
    'Directory.Packages.props',
    'global.json',
    'NuGet.Config')) {
    foreach ($parent in @($repositoryRoot, $PSScriptRoot)) {
        $path = Join-Path $parent $name
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $inputs += [pscustomobject][ordered]@{
                path = $path.Substring($repositoryRoot.Length + 1).Replace('\', '/')
                sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
            }
        }
    }
}

$program = @($inputs | Where-Object path -ceq 'source/Program.cs')
$project = @($inputs | Where-Object path -ceq 'source/VoyageAssetPatcher.csproj')
if ($program.Count -ne 1 -or $project.Count -ne 1) {
    throw 'VoyageAssetPatcher source inputs are incomplete.'
}
$sourceInputPaths = @($inputs | Where-Object {
    [string]$_.path -like 'source/*'
} | ForEach-Object {
    'tools/VoyageAssetPatcher/' + ([string]$_.path).Substring('source/'.Length)
})
$sourceCommit = (& git -C $repositoryRoot log -1 --format=%H `
    -- $sourceInputPaths).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'Could not identify the VoyageAssetPatcher source checkpoint.'
}
$descriptor = 'publish-v1|net10.0|win-x64|framework-dependent|single-file' + "`n" +
    ((@($inputs | Sort-Object path | ForEach-Object {
        $_.path + '=' + $_.sha256
    })) -join "`n")
$hasher = [Security.Cryptography.SHA256]::Create()
try {
    $inputFingerprint = [BitConverter]::ToString($hasher.ComputeHash(
        [Text.Encoding]::UTF8.GetBytes($descriptor))).Replace('-', '')
}
finally {
    $hasher.Dispose()
}
$buildInputs = [pscustomobject][ordered]@{
    Fingerprint = $inputFingerprint
    SourceSha256 = [string]$program[0].sha256
    UAssetApiSha256 = [string]$uassetApiDll[0].sha256
    SourceCommit = $sourceCommit
    Files = $inputs
}
if ($BuildInputsOnly) {
    return $buildInputs
}

if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Canonical VoyageAssetPatcher binary/manifest is missing. $publishHint"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([int]$manifest.schemaVersion -ne 1 -or
    [string]$manifest.kind -cne 'Voyage canonical AssetPatcher' -or
    [string]$manifest.sourceCommit -cne $sourceCommit -or
    [string]$manifest.inputFingerprint -cne $inputFingerprint -or
    [string]$manifest.runtimeIdentifier -cne 'win-x64' -or
    [bool]$manifest.selfContained -or
    -not [bool]$manifest.singleFile) {
    throw "Canonical VoyageAssetPatcher provenance is stale or invalid. $publishHint"
}
$executableSha256 = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash
if ($executableSha256 -cne [string]$manifest.sha256 -or
    (Get-Item -LiteralPath $executablePath).Length -ne [long]$manifest.length) {
    throw "Canonical VoyageAssetPatcher binary hash/size mismatch. $publishHint"
}

[pscustomobject][ordered]@{
    Path = $executablePath
    Sha256 = $executableSha256
    ManifestPath = $manifestPath
    InputFingerprint = $inputFingerprint
    SourceSha256 = $buildInputs.SourceSha256
    UAssetApiSha256 = $buildInputs.UAssetApiSha256
}
