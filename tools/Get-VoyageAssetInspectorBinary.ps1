# Read-only resolver. Never invokes a compiler, package manager, or publisher.
[CmdletBinding()]
param([switch]$BuildInputsOnly)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $PSScriptRoot 'VoyageAssetInspector'
$bundle = Join-Path $repo '.tools\bin\CUE4Parse'
$exe = Join-Path $repo '.tools\bin\VoyageAssetInspector.exe'
$manifestPath = Join-Path $repo '.tools\bin\VoyageAssetInspector.publish-manifest.json'
$publishHint = 'Run tools\Publish-VoyageAssetInspectorBinary.ps1 explicitly.'

$cueManifestPath = Join-Path $bundle 'publish-manifest.json'
if (-not (Test-Path -LiteralPath $cueManifestPath -PathType Leaf)) {
    throw 'Canonical CUE4Parse manifest is missing. Run tools\Publish-Cue4ParseBinary.ps1.'
}
$cue = Get-Content -LiteralPath $cueManifestPath -Raw | ConvertFrom-Json
if ($cue.kind -cne 'Voyage canonical CUE4Parse bundle' -or
    $cue.sourceRepository -cne 'https://github.com/ihsoft/CUE4Parse.git' -or
    $cue.upstreamRepository -cne 'https://github.com/FabianFG/CUE4Parse.git' -or
    $cue.microsoftBclMemoryVersion -cne '10.0.11' -or [bool]$cue.nativeBackend) {
    throw 'CUE4Parse does not match the accepted managed-only checkpoint.'
}
$inputs = @()
foreach ($file in @($cue.files | Sort-Object path)) {
    $name = [string]$file.path
    if ([IO.Path]::GetFileName($name) -cne $name) { throw "Invalid CUE4Parse bundle path: $name" }
    $path = Join-Path $bundle $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
        (Get-Item -LiteralPath $path).Length -ne [long]$file.length) {
        throw "Canonical CUE4Parse bundle is incomplete: $name"
    }
    $inputs += [pscustomobject]@{ path = 'CUE4Parse/' + $name; source = $path }
}
$cueDll = @($inputs | Where-Object path -ceq 'CUE4Parse/CUE4Parse.dll')
if ($cueDll.Count -ne 1) { throw 'CUE4Parse DLL is missing from the published bundle.' }
$actualBundleNames = @(Get-ChildItem -LiteralPath $bundle -File |
    Where-Object Name -cne 'publish-manifest.json' | ForEach-Object Name | Sort-Object)
$declaredBundleNames = @($cue.files | ForEach-Object { [string]$_.path } | Sort-Object)
if (@(Compare-Object $actualBundleNames $declaredBundleNames).Count -ne 0) {
    throw 'Canonical CUE4Parse bundle contains undeclared or missing files.'
}

if ($BuildInputsOnly) {
    foreach ($file in @(Get-ChildItem -LiteralPath $source -Recurse -File |
        Where-Object { $_.FullName -notmatch '[\\/](bin|obj)[\\/]' } | Sort-Object FullName)) {
        $inputs += [pscustomobject]@{
            path = 'source/' + $file.FullName.Substring($source.Length + 1).Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        }
    }
    foreach ($name in @('Directory.Build.props', 'Directory.Build.targets', 'Directory.Packages.props',
        'global.json', 'NuGet.Config')) {
        foreach ($parent in @($repo, $PSScriptRoot)) {
            $path = Join-Path $parent $name
            if (Test-Path -LiteralPath $path -PathType Leaf) {
                $inputs += [pscustomobject]@{
                    path = $path.Substring($repo.Length + 1).Replace('\', '/')
                    sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
                }
            }
        }
    }
    $program = @($inputs | Where-Object path -ceq 'source/Program.cs')
    $project = @($inputs | Where-Object path -ceq 'source/VoyageAssetInspector.csproj')
    if ($program.Count -ne 1 -or $project.Count -ne 1) { throw 'Inspector source inputs are incomplete.' }
    $descriptor = 'publish-v2|net10.0|win-x64|framework-dependent|single-file' + "`n" +
        ((@($inputs | Sort-Object path | ForEach-Object {
            $inputDigest = if ($_.PSObject.Properties['source']) {
                (Get-FileHash -LiteralPath $_.source -Algorithm SHA256).Hash
            } else { [string]$_.sha256 }
            $_.path + '=' + $inputDigest
        })) -join "`n")
    $hasher = [Security.Cryptography.SHA256]::Create()
    try { $inputHash = [BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($descriptor))).Replace('-', '') }
    finally { $hasher.Dispose() }
    return [pscustomobject]@{ Fingerprint = $inputHash }
}
if (-not (Test-Path -LiteralPath $exe -PathType Leaf) -or
    -not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Canonical Inspector binary/manifest is missing. $publishHint"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -notin @(1, 2) -or $manifest.kind -cne 'Voyage canonical AssetInspector' -or
    $manifest.runtimeIdentifier -cne 'win-x64' -or
    [bool]$manifest.selfContained -or -not [bool]$manifest.singleFile) {
    throw "Canonical Inspector provenance is stale or invalid. $publishHint"
}
if ((Get-Item -LiteralPath $exe).Length -ne [long]$manifest.length) {
    throw "Canonical Inspector binary is incomplete. $publishHint"
}
[pscustomobject]@{
    Path = $exe
    ManifestPath = $manifestPath
}
