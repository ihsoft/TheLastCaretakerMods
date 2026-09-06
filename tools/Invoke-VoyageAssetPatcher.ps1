[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet(
        'break-bottom-action-filter',
        'swap-forklift-horn-to-exit',
        'swap-hud-indicator-subclass',
        'roundtrip-unchanged',
        'export-json',
        'set-cable-updater-tick-interval',
        'break-cable-updater-super-index',
        'swap-hud-indicator-existing-control',
        'swap-diesel-socket-component-class')]
    [string]$Operation,

    [Parameter(Mandatory = $true)]
    [string]$InputAsset,

    [Parameter(Mandatory = $true)]
    [string]$OutputAsset,

    [string]$Mappings,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_8',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EvidenceRoot,

    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$binary = & (Join-Path $PSScriptRoot 'Get-VoyageAssetPatcherBinary.ps1')
$inputPath = (Resolve-Path -LiteralPath $InputAsset).Path
if ([IO.Path]::GetExtension($inputPath) -cne '.uasset') {
    throw "InputAsset must be an exact lowercase .uasset file: $inputPath"
}
$inputBase = Join-Path ([IO.Path]::GetDirectoryName($inputPath)) (
    [IO.Path]::GetFileNameWithoutExtension($inputPath))
$inputRecords = @(
    foreach ($extension in @('.uasset', '.uexp', '.ubulk', '.uptnl')) {
        $candidate = $inputBase + $extension
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $item = Get-Item -LiteralPath $candidate
            [pscustomobject][ordered]@{
                path = $item.FullName
                size = $item.Length
                sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
            }
        }
    }
)
$outputPath = [IO.Path]::GetFullPath($OutputAsset)
if ([StringComparer]::OrdinalIgnoreCase.Equals($inputPath, $outputPath)) {
    throw 'Refusing to overwrite the input asset.'
}
$outputExtension = [IO.Path]::GetExtension($outputPath)
if ($Operation -ceq 'export-json') {
    if ($outputExtension -cne '.json') {
        throw 'The export-json operation requires a lowercase .json output.'
    }
}
elseif ($outputExtension -cne '.uasset') {
    throw "$Operation requires a lowercase .uasset output."
}
$outputBase = Join-Path ([IO.Path]::GetDirectoryName($outputPath)) (
    [IO.Path]::GetFileNameWithoutExtension($outputPath))
$prospectiveOutputs = if ($outputExtension -ceq '.json') {
    @($outputPath)
}
else {
    @('.uasset', '.uexp', '.ubulk', '.uptnl') | ForEach-Object {
        $outputBase + $_
    }
}
foreach ($candidate in $prospectiveOutputs) {
    if (Test-Path -LiteralPath $candidate) {
        throw "Refusing to overwrite an existing output or companion: $candidate"
    }
}

$mappingRecord = $null
if ([string]::IsNullOrWhiteSpace($Mappings)) {
    if ($EngineVersion -cne 'UE5_8') {
        throw 'Legacy UE5_7 operations require one explicit provenance-checked -Mappings file.'
    }
    $mappingRecord = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') `
        -GameRoot $GameRoot
    $mappingsPath = [string]$mappingRecord.mappingsPath
}
else {
    $mappingsPath = (Resolve-Path -LiteralPath $Mappings).Path
}
$mappingsSha256 = (Get-FileHash -LiteralPath $mappingsPath -Algorithm SHA256).Hash

if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $repositoryRoot 'artifacts\tool-runs'
}
$runRoot = Join-Path ([IO.Path]::GetFullPath($EvidenceRoot)) (
    'asset-patcher-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($runRoot) | Out-Null
$logPath = Join-Path $runRoot 'patcher.log'

$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $binary.Path $Operation $inputPath $mappingsPath $outputPath `
        $EngineVersion *> $logPath
    $patcherExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $savedPreference
}
if ($patcherExitCode -ne 0) {
    throw "VoyageAssetPatcher failed ($patcherExitCode). Log: $logPath"
}
if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
    throw "VoyageAssetPatcher returned success without the requested output. Log: $logPath"
}

$outputFiles = @()
if ($outputExtension -ceq '.json') {
    $outputFiles = @((Get-Item -LiteralPath $outputPath))
}
else {
    $outputFiles = @(
        foreach ($extension in @('.uasset', '.uexp', '.ubulk', '.uptnl')) {
            $candidate = $outputBase + $extension
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                Get-Item -LiteralPath $candidate
            }
        }
    )
}
$outputRecords = @($outputFiles | ForEach-Object {
    [pscustomobject][ordered]@{
        path = $_.FullName
        size = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
})

$result = [pscustomobject][ordered]@{
    status = 'completed'
    operation = $Operation
    engineVersion = $EngineVersion
    inputPath = $inputPath
    inputSha256 = (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash
    inputs = $inputRecords
    mappingsPath = $mappingsPath
    mappingsSha256 = $mappingsSha256
    mappingManifestPath = if ($null -eq $mappingRecord) {
        $null
    }
    else {
        [string]$mappingRecord.manifestPath
    }
    mappingSource = if ($null -eq $mappingRecord) { 'explicit' } else { 'reviewed-current-game' }
    steamBuildId = if ($null -eq $mappingRecord) { $null } else { [string]$mappingRecord.steamBuildId }
    executableSha256 = if ($null -eq $mappingRecord) {
        $null
    }
    else {
        [string]$mappingRecord.executableSha256
    }
    patcherBinaryPath = $binary.Path
    patcherBinarySha256 = $binary.Sha256
    patcherBinaryManifestPath = $binary.ManifestPath
    outputPath = $outputPath
    outputs = $outputRecords
    logPath = $logPath
}
if ($AsJson) {
    $result | ConvertTo-Json -Depth 5 -Compress
}
else {
    $result
}
