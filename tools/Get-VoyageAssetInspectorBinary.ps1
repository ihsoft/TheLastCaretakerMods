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
    $cue.sourceCommit -cne 'ec6595e46448a817ac21ea9bde01caa48f80a420' -or
    $cue.microsoftBclMemoryVersion -cne '10.0.11' -or [bool]$cue.nativeBackend) {
    throw 'CUE4Parse does not match the accepted managed-only checkpoint.'
}
$inputs = @()
foreach ($file in @($cue.files | Sort-Object path)) {
    $name = [string]$file.path
    if ([IO.Path]::GetFileName($name) -cne $name) { throw "Invalid CUE4Parse bundle path: $name" }
    $path = Join-Path $bundle $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
        (Get-Item -LiteralPath $path).Length -ne [long]$file.length -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -cne $file.sha256) {
        throw "Canonical CUE4Parse bundle hash/size mismatch: $name"
    }
    $inputs += [pscustomobject]@{ path = 'CUE4Parse/' + $name; sha256 = [string]$file.sha256 }
}
$cueDll = @($inputs | Where-Object path -ceq 'CUE4Parse/CUE4Parse.dll')
if ($cueDll.Count -ne 1 -or $cueDll[0].sha256 -cne
    'F304981BAD4C53D209DFDABA9EB65A01D825572E543A04914BEBFD3538DCF4FD') {
    throw 'CUE4Parse DLL does not match the accepted binary.'
}
$actualBundleNames = @(Get-ChildItem -LiteralPath $bundle -File |
    Where-Object Name -cne 'publish-manifest.json' | ForEach-Object Name | Sort-Object)
$declaredBundleNames = @($cue.files | ForEach-Object { [string]$_.path } | Sort-Object)
if (@(Compare-Object $actualBundleNames $declaredBundleNames).Count -ne 0) {
    throw 'Canonical CUE4Parse bundle contains undeclared or missing files.'
}

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
$descriptor = 'publish-v1|net10.0|win-x64|framework-dependent|single-file' + "`n" +
    ((@($inputs | Sort-Object path | ForEach-Object { $_.path + '=' + $_.sha256 })) -join "`n")
$hasher = [Security.Cryptography.SHA256]::Create()
try { $inputHash = [BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($descriptor))).Replace('-', '') }
finally { $hasher.Dispose() }
$buildInputs = [pscustomobject]@{
    Fingerprint = $inputHash
    SourceSha256 = [string]$program[0].sha256
    Cue4ParseSha256 = [string]$cueDll[0].sha256
    Files = $inputs
}
if ($BuildInputsOnly) { return $buildInputs }
if (-not (Test-Path -LiteralPath $exe -PathType Leaf) -or
    -not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Canonical Inspector binary/manifest is missing. $publishHint"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1 -or $manifest.kind -cne 'Voyage canonical AssetInspector' -or
    $manifest.inputFingerprint -cne $inputHash -or $manifest.runtimeIdentifier -cne 'win-x64' -or
    [bool]$manifest.selfContained -or -not [bool]$manifest.singleFile) {
    throw "Canonical Inspector provenance is stale or invalid. $publishHint"
}
$exeHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
if ($exeHash -cne $manifest.sha256 -or (Get-Item -LiteralPath $exe).Length -ne [long]$manifest.length) {
    throw "Canonical Inspector binary hash/size mismatch. $publishHint"
}
[pscustomobject]@{
    Path = $exe
    Sha256 = $exeHash
    ManifestPath = $manifestPath
    InputFingerprint = $inputHash
    SourceSha256 = $buildInputs.SourceSha256
    Cue4ParseSha256 = $buildInputs.Cue4ParseSha256
}
