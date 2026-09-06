[CmdletBinding()]
param(
    [string[]]$Query,

    [string[]]$MemberOffset,

    [string[]]$TargetVirtualAddress,

    [ValidateRange(16, 1048576)]
    [int]$WindowBytes = 2048,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$OutputPath,

    [switch]$RequireMatch,

    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$queries = @($Query | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
$memberOffsets = @($MemberOffset | Where-Object {
    -not [string]::IsNullOrWhiteSpace($_)
})
$targetAddresses = @($TargetVirtualAddress | Where-Object {
    -not [string]::IsNullOrWhiteSpace($_)
})
if ($queries.Count -eq 0 -and $memberOffsets.Count -eq 0 -and
    $targetAddresses.Count -eq 0) {
    throw 'Provide at least one Query, MemberOffset, or TargetVirtualAddress.'
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$binary = & (Join-Path $PSScriptRoot 'Get-VoyageExecutableInspectorBinary.ps1')
$fingerprint = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') `
    -GameRoot $GameRoot | ConvertFrom-Json)
$resolvedGameRoot = [string]$fingerprint.gameRoot
$gameExecutable = Join-Path $resolvedGameRoot (
    'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe')

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $repositoryRoot (
        'artifacts\inspection\executable-' + [Guid]::NewGuid().ToString('N') + '.txt')
}
$resolvedOutputPath = [IO.Path]::GetFullPath($OutputPath)
if ([IO.Path]::GetExtension($resolvedOutputPath) -cne '.txt') {
    throw "Inspector output must be a lowercase .txt file: $resolvedOutputPath"
}
if (Test-Path -LiteralPath $resolvedOutputPath) {
    throw "Refusing to overwrite an existing inspector output: $resolvedOutputPath"
}

$arguments = @($gameExecutable, $resolvedOutputPath, "--window=$WindowBytes")
if ($memberOffsets.Count -gt 0) {
    $arguments += '--member-offsets=' + ($memberOffsets -join ',')
}
if ($targetAddresses.Count -gt 0) {
    $arguments += '--target-va=' + ($targetAddresses -join ',')
}
$arguments += $queries

$logRoot = Join-Path $repositoryRoot (
    'artifacts\tool-runs\executable-inspector-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($logRoot) | Out-Null
$logPath = Join-Path $logRoot 'inspector.log'
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $binary.Path @arguments *> $logPath
    $inspectorExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $savedPreference
}
if ($inspectorExitCode -notin @(0, 1) -or
    -not (Test-Path -LiteralPath $resolvedOutputPath -PathType Leaf)) {
    throw "VoyageExecutableInspector failed ($inspectorExitCode). Log: $logPath"
}

$matchesLine = @(Get-Content -LiteralPath $resolvedOutputPath -TotalCount 16 |
    Where-Object { $_ -match '^MATCHES\t(?<count>\d+)$' })
if ($matchesLine.Count -ne 1) {
    throw "Inspector output has no unique MATCHES header. Output: $resolvedOutputPath"
}
$matchCount = [int]([regex]::Match(
    $matchesLine[0],
    '^MATCHES\t(?<count>\d+)$').Groups['count'].Value)
$status = if ($queries.Count -gt 0 -and $matchCount -eq 0 -and
    $memberOffsets.Count -eq 0 -and $targetAddresses.Count -eq 0) {
    'no-match'
}
else {
    'completed'
}
if ($RequireMatch -and $matchCount -eq 0) {
    throw "Executable inspection completed but found no string matches. Output: $resolvedOutputPath"
}

$result = [pscustomobject][ordered]@{
    status = $status
    matchCount = $matchCount
    query = $queries
    memberOffsets = $memberOffsets
    targetVirtualAddresses = $targetAddresses
    windowBytes = $WindowBytes
    outputPath = $resolvedOutputPath
    outputSha256 = (Get-FileHash -LiteralPath $resolvedOutputPath -Algorithm SHA256).Hash
    outputSize = (Get-Item -LiteralPath $resolvedOutputPath).Length
    logPath = $logPath
    inspectorBinaryPath = $binary.Path
    inspectorBinarySha256 = $binary.Sha256
    inspectorBinaryManifestPath = $binary.ManifestPath
    steamBuildId = [string]$fingerprint.steam.buildId
    executablePath = $gameExecutable
    executableSha256 = [string]$fingerprint.executable.sha256
    boundary = 'Correlation aid only; names, bytes, pointers, and offsets do not prove reflected ownership, call relations, or lifecycle.'
}
if ($AsJson) {
    $result | ConvertTo-Json -Depth 5 -Compress
}
else {
    $result
}
