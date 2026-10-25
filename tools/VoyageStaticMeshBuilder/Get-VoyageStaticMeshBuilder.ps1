[CmdletBinding()]
param(
    [switch]$BuildInputsOnly,
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$source = $PSScriptRoot
$published = Join-Path $repo '.tools\bin\VoyageStaticMeshBuilder'
$manifestPath = Join-Path $published 'publish-manifest.json'
$publishHint = 'Run tools\VoyageStaticMeshBuilder\Publish-VoyageStaticMeshBuilder.ps1 explicitly.'
$engine = & (Join-Path $source 'Get-EngineIdentity.ps1') -EngineRoot $EngineRoot

if ($BuildInputsOnly) {
$inputs = @()
foreach ($file in @(Get-ChildItem -LiteralPath $source -Recurse -File |
    Where-Object {
        $_.FullName -notmatch '[\\/](Binaries|Intermediate|Saved)[\\/]' -and
        ($_.Extension -in @('.cpp','.h','.cs','.uproject','.ini'))
    } | Sort-Object FullName)) {
    $inputs += [pscustomobject]@{
        path = $file.FullName.Substring($source.Length + 1).Replace('\', '/')
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    }
}
$descriptor = 'voyage-static-mesh-builder-v1|UE5.8' + "`n" +
    ((@($inputs | Sort-Object path | ForEach-Object {
        $_.path + '=' + $_.sha256
    })) -join "`n")
$hasher = [Security.Cryptography.SHA256]::Create()
try {
    $fingerprint = [BitConverter]::ToString($hasher.ComputeHash(
        [Text.Encoding]::UTF8.GetBytes($descriptor))).Replace('-', '')
} finally { $hasher.Dispose() }
    return [pscustomobject]@{ Fingerprint=$fingerprint; Files=$inputs; Engine=$engine }
}
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Canonical StaticMesh builder is missing. $publishHint"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -notin @(1, 2) -or
    $manifest.kind -cne 'Voyage canonical StaticMesh builder' -or
    $manifest.engineVersion -cne '5.8' -or
    $manifest.engine.buildId -cne $engine.buildId) {
    throw "Canonical StaticMesh builder interface is invalid. $publishHint"
}
foreach ($file in @($manifest.files)) {
    $path = Join-Path $published ([string]$file.path)
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
        (Get-Item -LiteralPath $path).Length -ne [long]$file.length) {
        throw "Canonical StaticMesh builder file differs: $($file.path). $publishHint"
    }
}
$actual = @(Get-ChildItem -LiteralPath $published -File -Recurse |
    Where-Object Name -cne 'publish-manifest.json' | ForEach-Object {
        $_.FullName.Substring($published.Length + 1).Replace('\', '/')
    } | Sort-Object)
$declared = @($manifest.files | ForEach-Object { [string]$_.path } | Sort-Object)
if (@(Compare-Object $actual $declared).Count -ne 0) {
    throw "Canonical StaticMesh builder has undeclared or missing files. $publishHint"
}
[pscustomobject]@{
    ProjectPath = Join-Path $published 'VoyageStaticMeshBuilder.uproject'
    ManifestPath = $manifestPath
    EngineVersion = [string]$manifest.engineVersion
    Engine = $engine
    Files = $manifest.files
}
