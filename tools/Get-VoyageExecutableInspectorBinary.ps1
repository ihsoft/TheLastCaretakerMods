# Read-only resolver. Never invokes a compiler, package manager, or publisher.
[CmdletBinding()]
param(
    [switch]$BuildInputsOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$sourceRoot = Join-Path $PSScriptRoot 'VoyageExecutableInspector'
$executablePath = Join-Path $repositoryRoot '.tools\bin\VoyageExecutableInspector.exe'
$manifestPath = Join-Path $repositoryRoot (
    '.tools\bin\VoyageExecutableInspector.publish-manifest.json')
$publishHint = 'Run tools\Publish-VoyageExecutableInspectorBinary.ps1 explicitly.'

$inputs = @()
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
$project = @($inputs | Where-Object path -ceq 'source/VoyageExecutableInspector.csproj')
if ($program.Count -ne 1 -or $project.Count -ne 1) {
    throw 'VoyageExecutableInspector source inputs are incomplete.'
}
$sourceInputPaths = @($inputs | Where-Object {
    [string]$_.path -like 'source/*'
} | ForEach-Object {
    'tools/VoyageExecutableInspector/' + ([string]$_.path).Substring('source/'.Length)
})
$sourceCommit = (& git -C $repositoryRoot log -1 --format=%H `
    -- $sourceInputPaths).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'Could not identify the VoyageExecutableInspector source checkpoint.'
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
    SourceCommit = $sourceCommit
    Files = $inputs
}
if ($BuildInputsOnly) {
    return $buildInputs
}

if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Canonical VoyageExecutableInspector binary/manifest is missing. $publishHint"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([int]$manifest.schemaVersion -ne 1 -or
    [string]$manifest.kind -cne 'Voyage canonical ExecutableInspector' -or
    [string]$manifest.sourceCommit -cne $sourceCommit -or
    [string]$manifest.inputFingerprint -cne $inputFingerprint -or
    [string]$manifest.runtimeIdentifier -cne 'win-x64' -or
    [bool]$manifest.selfContained -or
    -not [bool]$manifest.singleFile) {
    throw "Canonical VoyageExecutableInspector provenance is stale or invalid. $publishHint"
}
$executableSha256 = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash
if ($executableSha256 -cne [string]$manifest.sha256 -or
    (Get-Item -LiteralPath $executablePath).Length -ne [long]$manifest.length) {
    throw "Canonical VoyageExecutableInspector binary hash/size mismatch. $publishHint"
}

[pscustomobject][ordered]@{
    Path = $executablePath
    Sha256 = $executableSha256
    ManifestPath = $manifestPath
    InputFingerprint = $inputFingerprint
    SourceSha256 = $buildInputs.SourceSha256
    SourceCommit = $sourceCommit
}
