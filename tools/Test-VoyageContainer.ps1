[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Container,
    [string]$ExpectedPackageList,
    [string]$Retoc,
    [ValidateRange(1, 3600)][int]$TimeoutSeconds = 60,
    [ValidateRange(64, 65536)][int]$MemoryLimitMB = 1024,
    [switch]$AllowFailure
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
$utoc = (Resolve-Path -LiteralPath $Container).Path
if ([IO.Path]::GetExtension($utoc) -ine '.utoc' -or -not (Test-Path -LiteralPath $utoc -PathType Leaf)) {
    throw 'Container must be one existing .utoc file, not a directory.'
}
$canonical = [string]::IsNullOrWhiteSpace($Retoc)
if ($canonical) { $Retoc = Join-Path $repo '.tools\bin\retoc.exe' }
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$retocHash = (Get-FileHash -LiteralPath $retocPath -Algorithm SHA256).Hash
if ($canonical) {
    $publication = Get-Content -LiteralPath (Join-Path $repo '.tools\bin\retoc.manifest.json') -Raw | ConvertFrom-Json
    if ($publication.kind -cne 'Voyage canonical retoc binary' -or
        $publication.executableSha256 -cne $retocHash -or
        $publication.executableLength -ne (Get-Item -LiteralPath $retocPath).Length) {
        throw 'Canonical retoc does not match its publication manifest; diagnose publication before retrying.'
    }
}
function Normalize-PackagePath([string]$Path) {
    $value = $Path.Trim().Replace('\', '/')
    if ($value.StartsWith('../../../', [StringComparison]::Ordinal)) { $value = $value.Substring(9) }
    if ($value -notmatch '^[^/]+/.+\.(uasset|umap)$' -or $value.Contains('|') -or
        @($value.Split('/') | Where-Object { $_ -in @('', '.', '..') }).Count -gt 0) {
        throw "Expected a container-relative asset path (not /Game or an inventory row): $Path"
    }
    $value
}
$expected = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$expectedHash = $null
if ($ExpectedPackageList) {
    $ExpectedPackageList = (Resolve-Path -LiteralPath $ExpectedPackageList).Path
    $expectedHash = (Get-FileHash -LiteralPath $ExpectedPackageList).Hash
    foreach ($line in [IO.File]::ReadAllLines($ExpectedPackageList)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $path = Normalize-PackagePath $line
        if (-not $expected.Add($path)) { throw "Duplicate expected package: $path" }
    }
    if ($expected.Count -eq 0) { throw 'Expected package list is empty.' }
}
$stem = [IO.Path]::GetFileNameWithoutExtension($utoc)
$parent = Split-Path -Parent $utoc
$filePattern = '^' + [regex]::Escape($stem) + '(\.utoc|\.pak|(?:_s\d+)?\.ucas)$'
function Get-ContainerFiles {
    @(Get-ChildItem -LiteralPath $parent -File | Where-Object Name -match $filePattern | Sort-Object Name | ForEach-Object {
        if ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Container symlinks are unsupported.' }
        [pscustomobject]@{ path = $_.FullName; name = $_.Name; size = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash }
    })
}
$before = Get-ContainerFiles
if (-not (Test-Path -LiteralPath (Join-Path $parent ($stem + '.ucas')) -PathType Leaf)) { throw 'Matching .ucas is missing.' }
$directory = Join-Path $repo ('artifacts\container-checks\' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($directory) | Out-Null
$reportPath = Join-Path $directory 'verification.json'
$packageListPath = Join-Path $directory 'packages.txt'
$runner = Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1'
$report = [ordered]@{
    schemaVersion = 1; status = 'failed'; container = $utoc; capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    retocPath = $retocPath; retocSha256 = $retocHash; canonicalRetoc = $canonical
    expectedPackageList = $ExpectedPackageList; expectedPackageListSha256 = $expectedHash
    files = $before; packageCount = 0; chunkCount = 0; chunkTypes = @()
    packageSetMatches = $null; missingPackages = @(); unexpectedPackages = @()
    verificationRun = $null; inventoryRun = $null; packageListPath = $null
    reportPath = $reportPath; error = $null
    scope = 'Retoc IoStore integrity plus optional exact ExportBundleData paths. PAK is fingerprinted only. Not UObject parsing, dependency resolution, mount precedence or runtime validation.'
}
try {
    $verify = & $runner -Executable $retocPath -Arguments @('verify', $utoc) `
        -MemoryLimitMB $MemoryLimitMB -TimeoutSeconds $TimeoutSeconds -AllowFailure
    $report.verificationRun = $verify.reportPath
    if ($verify.status -cne 'passed' -or (Get-Content -LiteralPath $verify.stdoutPath -Raw).Trim() -cne 'verified') {
        throw "Retoc verification failed or returned an unrecognized result: $($verify.reportPath)"
    }
    $list = & $runner -Executable $retocPath -Arguments @('list', '--path', $utoc) `
        -MemoryLimitMB $MemoryLimitMB -TimeoutSeconds $TimeoutSeconds -AllowFailure
    $report.inventoryRun = $list.reportPath
    if ($list.status -cne 'passed') { throw "Retoc inventory failed: $($list.reportPath)" }
    $packages = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $types = [Collections.Generic.List[string]]::new()
    foreach ($line in [IO.File]::ReadAllLines($list.stdoutPath)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^.*?\s+[0-9a-fA-F]{22,24}\s+(?<type>\w+)\s+(?<path>.+?)\s*$') {
            throw "Unrecognized retoc inventory row: $line"
        }
        $type = $Matches['type']
        $path = $Matches['path']
        $types.Add($type)
        if ($type -ceq 'ExportBundleData') {
            $normalized = Normalize-PackagePath $path
            if (-not $packages.Add($normalized)) { throw "Duplicate package entry: $normalized" }
        }
    }
    $report.chunkCount = $types.Count
    $report.chunkTypes = @($types | Group-Object | ForEach-Object { [pscustomobject]@{ type = $_.Name; count = $_.Count } })
    $report.packageCount = $packages.Count
    if ($types.Count -eq 0) { throw 'Retoc returned an empty inventory.' }
    [IO.File]::WriteAllLines($packageListPath, [string[]]@($packages | Sort-Object -CaseSensitive), [Text.UTF8Encoding]::new($false))
    $report.packageListPath = $packageListPath
    if ($ExpectedPackageList) {
        $report.missingPackages = @($expected | Where-Object { -not $packages.Contains($_) } | Sort-Object)
        $report.unexpectedPackages = @($packages | Where-Object { -not $expected.Contains($_) } | Sort-Object)
        $report.packageSetMatches = $report.missingPackages.Count -eq 0 -and $report.unexpectedPackages.Count -eq 0
        if (-not $report.packageSetMatches) { throw 'Actual package paths differ from the expected exact set.' }
        if ($expectedHash -cne (Get-FileHash -LiteralPath $ExpectedPackageList).Hash) { throw 'Expected list changed during verification.' }
    }
    if (($before | ConvertTo-Json -Compress) -cne ((Get-ContainerFiles) | ConvertTo-Json -Compress)) { throw 'Container files changed during verification.' }
    if ($retocHash -cne (Get-FileHash -LiteralPath $retocPath).Hash) { throw 'Retoc changed during verification.' }
    $report.status = 'passed'
} catch { $report.error = $_.Exception.Message }
[IO.File]::WriteAllText($reportPath, ($report | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
[pscustomobject]$report | Select-Object status, packageCount, packageSetMatches, reportPath, packageListPath, error
if ($report.status -cne 'passed' -and -not $AllowFailure) { throw "Container check failed: $($report.error) Report: $reportPath" }
