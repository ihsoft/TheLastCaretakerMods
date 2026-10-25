<#
.SYNOPSIS
Inspects exact cooked StaticMesh packages and optionally evaluates caller-selected geometry policy.
.DESCRIPTION
Normal use consumes a prepublished private validator bundle. The request schema is strict: unknown
fields are errors. Facts are written to inspection.json; only explicit expectations are written to
validation.json. This proves cooked serialization/decode facts, not material, shader, collision,
runtime-rendering, or gameplay parity.
.EXAMPLE
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Test-VoyageStaticMesh.ps1 -ShowExample
#>
[CmdletBinding()]
param(
    [string]$RequestPath,
    [string]$OutputDirectory,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [switch]$ShowSchema,
    [switch]$ShowExample
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$schema = @'
schemaVersion: 1
source: { kind: "stock" } OR { kind: "utoc", utoc: "exact-candidate.utoc" }
assets[]: {
  id: unique label,
  package: exact virtual package path without extension/object suffix,
  decodeNanite: optional boolean (default false),
  sourceGlb: optional { path, meshIndex OR meshName, positionMatchToleranceMeters,
    attributes: { POSITION|NORMAL|COLOR_0|TEXCOORD_n: maxAbsoluteError },
    topology: optional { maxExtraFaces, maxMissingNondegenerateFaces, maxMissingFaces? } },
  expectations: optional [{ fact: JSON pointer relative to facts, equals OR min OR max OR present }]
}
'@
$example = @'
{
  "schemaVersion": 1,
  "source": { "kind": "utoc", "utoc": "Tmp/example/container/Example_P.utoc" },
  "assets": [{
    "id": "mesh-a",
    "package": "/Game/Mods/Example/MeshA",
    "decodeNanite": true,
    "expectations": [
      { "fact": "/nanite/present", "equals": true },
      { "fact": "/nanite/decodedTriangles", "min": 1 }
    ]
  }]
}
'@
if ($ShowSchema) { $schema; exit 0 }
if ($ShowExample) { $example; exit 0 }
if ([string]::IsNullOrWhiteSpace($RequestPath) -or [string]::IsNullOrWhiteSpace($OutputDirectory)) {
    throw 'Supply -RequestPath and -OutputDirectory, or use -ShowSchema/-ShowExample.'
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$requestFile = [IO.Path]::GetFullPath($RequestPath)
$output = [IO.Path]::GetFullPath($OutputDirectory)
$tmpRoot = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($tmpRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputDirectory must be a fresh path below repository Tmp.'
}
if (Test-Path -LiteralPath $output) { throw 'OutputDirectory already exists; choose a fresh path.' }
$request = Get-Content -LiteralPath $requestFile -Raw | ConvertFrom-Json
function Assert-Fields([object]$Object, [string[]]$Allowed, [string[]]$Required, [string]$Context) {
    if ($null -eq $Object) { throw "$Context is required." }
    $names = @($Object.PSObject.Properties.Name)
    $unknown = @($names | Where-Object { $_ -notin $Allowed })
    if ($unknown.Count -ne 0) { throw "Unknown $Context field(s): $($unknown -join ', ')" }
    $missing = @($Required | Where-Object { $_ -notin $names })
    if ($missing.Count -ne 0) { throw "Missing $Context field(s): $($missing -join ', ')" }
}
function Get-Optional([object]$Object, [string]$Name) {
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}
$allowedRoot = @('schemaVersion','source','assets')
Assert-Fields $request $allowedRoot @('schemaVersion','source','assets') 'request'
if ($request.schemaVersion -ne 1) { throw 'schemaVersion must be 1.' }
$requestBase = Split-Path -Parent $requestFile
function Resolve-RequestPath([string]$Value) {
    if ([IO.Path]::IsPathRooted($Value)) { return [IO.Path]::GetFullPath($Value) }
    return [IO.Path]::GetFullPath((Join-Path $requestBase $Value))
}
Assert-Fields $request.source @('kind','utoc') @('kind') 'source'
if ($request.source.kind -ceq 'utoc') {
    if ($null -eq (Get-Optional $request.source 'utoc')) { throw 'source.utoc is required when source.kind is utoc.' }
    $request.source.utoc = Resolve-RequestPath ([string]$request.source.utoc)
}
foreach ($asset in @($request.assets)) {
    Assert-Fields $asset @('id','package','decodeNanite','sourceGlb','expectations') @('id','package') 'asset'
    $sourceGlb = Get-Optional $asset 'sourceGlb'
    if ($null -ne $sourceGlb) {
        Assert-Fields $sourceGlb @('path','meshIndex','meshName','positionMatchToleranceMeters','attributes','topology') @('path','positionMatchToleranceMeters','attributes') 'sourceGlb'
        $sourceGlb.path = Resolve-RequestPath ([string]$sourceGlb.path)
        $topology = Get-Optional $sourceGlb 'topology'
        if ($null -ne $topology) { Assert-Fields $topology @('maxExtraFaces','maxMissingNondegenerateFaces','maxMissingFaces') @('maxExtraFaces','maxMissingNondegenerateFaces') 'sourceGlb.topology' }
    }
    foreach ($expectation in @((Get-Optional $asset 'expectations'))) {
        if ($null -ne $expectation) { Assert-Fields $expectation @('fact','equals','min','max','present') @('fact') 'expectation' }
    }
}
$bundle = Join-Path $repo '.tools\bin\VoyageStaticMeshValidator'
$manifestPath = Join-Path $bundle 'publish-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath)) { throw 'Publish once with tools/VoyageStaticMeshValidator/Publish.ps1; normal validation never builds tools.' }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.kind -cne 'VoyageStaticMeshValidator') { throw 'Unexpected validator interface.' }
foreach ($file in $manifest.files) {
    $publishedFile = Join-Path $bundle $file.path
    if (-not (Test-Path -LiteralPath $publishedFile -PathType Leaf) -or
        (Get-Item -LiteralPath $publishedFile).Length -ne [long]$file.length) {
        throw "Validator bundle is incomplete: $($file.path)"
    }
}
$run = Join-Path $repo ('Tmp\tools\VoyageStaticMeshValidator\run-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $run)
$fingerprintPath = Join-Path $run 'fingerprint.json'
$fingerprintText = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot -OutputPath $fingerprintPath) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
if ($mapping.engineVersion -ne '5.8' -or $mapping.steamBuildId -cne $fingerprint.steam.buildId) { throw 'Unsupported/mismatched game mapping.' }
$normalized = [ordered]@{
    schemaVersion = 1; gameRoot = [IO.Path]::GetFullPath($GameRoot); mappingPath = $mapping.mappingsPath
    mappingManifestPath = $mapping.manifestPath; fingerprintPath = $fingerprintPath; outputDirectory = $output
    source = $request.source; assets = @($request.assets)
}
$normalizedPath = Join-Path $run 'normalized-request.json'
[IO.File]::WriteAllText($normalizedPath, ($normalized | ConvertTo-Json -Depth 20))
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = Join-Path $bundle 'VoyageStaticMeshValidator.exe'
$start.Arguments = '"' + $normalizedPath + '"'
$start.UseShellExecute = $false; $start.CreateNoWindow = $true; $start.RedirectStandardOutput = $true; $start.RedirectStandardError = $true
$process = [Diagnostics.Process]::Start($start); $stdout = $process.StandardOutput.ReadToEndAsync(); $stderr = $process.StandardError.ReadToEndAsync(); $process.WaitForExit()
$exitCode = $process.ExitCode; $outputText = $stdout.Result; $errorText = $stderr.Result; $process.Dispose()
if (Test-Path -LiteralPath $output) {
    Copy-Item -LiteralPath $requestFile -Destination (Join-Path $output 'request.json')
    Copy-Item -LiteralPath $normalizedPath -Destination (Join-Path $output 'normalized-request.json')
    Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $output 'tool-manifest.json')
    [IO.File]::WriteAllText((Join-Path $output 'validator.log'), ($outputText + [Environment]::NewLine + $errorText))
}
$lines = @($outputText -split '\r?\n' | Where-Object { $_.Trim() })
if ($lines.Count -gt 0) { $lines[-1] }
if ($exitCode -ne 0) {
    [Console]::Error.WriteLine("StaticMesh validation exited $exitCode. " + $(if (Test-Path -LiteralPath $output) { 'Evidence: ' + $output } else { $errorText.Trim() }) + " Internal evidence: $run")
    exit $exitCode
}
$expectedRunRoot = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp\tools\VoyageStaticMeshValidator')) + [IO.Path]::DirectorySeparatorChar
if (-not ([IO.Path]::GetFullPath($run) + [IO.Path]::DirectorySeparatorChar).StartsWith(
        $expectedRunRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean an unexpected validator run path: $run"
}
Remove-Item -LiteralPath $run -Recurse -Force
