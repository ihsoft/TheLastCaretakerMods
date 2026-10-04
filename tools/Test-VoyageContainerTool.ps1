[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$KnownGoodContainer)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$source = (Resolve-Path -LiteralPath $KnownGoodContainer).Path
$repo = Split-Path -Parent $PSScriptRoot
$tmpBoundary = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp\tests'))
$root = Join-Path $tmpBoundary ('container-tool-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($root) | Out-Null
$containerChecks = Join-Path $root 'container-checks'
$sourceParent = Split-Path -Parent $source
$stem = [IO.Path]::GetFileNameWithoutExtension($source)
$pattern = '^' + [regex]::Escape($stem) + '(\.utoc|\.pak|(?:_s\d+)?\.ucas)$'
$inputs = @(Get-ChildItem -LiteralPath $sourceParent -File | Where-Object Name -match $pattern)
$hashes = @($inputs | ForEach-Object { (Get-FileHash -LiteralPath $_.FullName).Hash })
foreach ($inputFile in $inputs) { Copy-Item -LiteralPath $inputFile.FullName -Destination (Join-Path $root $inputFile.Name) }
$container = Join-Path $root ([IO.Path]::GetFileName($source))
$tool = Join-Path $PSScriptRoot 'Test-VoyageContainer.ps1'
$checks = [Collections.Generic.List[string]]::new()
function Assert($Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
$baseline = & $tool -Container $container -OutputRoot $containerChecks
Assert ($baseline.status -ceq 'passed' -and $baseline.packageCount -gt 0 -and $null -eq $baseline.packageSetMatches) 'Inventory-only result incorrect.'
$checks.Add('verify-and-inventory-without-expectations')
$defaultOutput = & $tool -Container $container
$defaultRun = Split-Path -Parent $defaultOutput.reportPath
$defaultBoundary = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp\container-checks')) +
    [IO.Path]::DirectorySeparatorChar
Assert ($defaultRun.StartsWith(
        $defaultBoundary, [StringComparison]::OrdinalIgnoreCase)) `
    'Default container-check output did not use repository Tmp.'
[IO.Directory]::Delete($defaultRun, $true)
$checks.Add('default-output-is-temporary-and-cleanable')
# Derived list is only a comparator fixture, NOT independent release acceptance.
$exact = & $tool -Container $container -ExpectedPackageList $baseline.packageListPath `
    -OutputRoot $containerChecks
Assert ($exact.status -ceq 'passed' -and $exact.packageSetMatches) 'Identical expected set rejected.'
$checks.Add('exact-set-comparison')
$paths = @(Get-Content -LiteralPath $baseline.packageListPath)
$expected = Join-Path $root 'wrong-expected.txt'
$different = @($paths | Select-Object -Skip 1) + @('Voyage/Content/MissingFixture.uasset')
[IO.File]::WriteAllLines($expected, [string[]]$different)
$mismatch = & $tool -Container $container -ExpectedPackageList $expected `
    -OutputRoot $containerChecks -AllowFailure
$detail = Get-Content -LiteralPath $mismatch.reportPath -Raw | ConvertFrom-Json
Assert ($mismatch.status -ceq 'failed' -and -not $mismatch.packageSetMatches -and
    $detail.missingPackages.Count -eq 1 -and $detail.unexpectedPackages.Count -eq 1) 'Mismatch not explained.'
$checks.Add('missing-and-unexpected-package-report')
$rejected = $false
try { & $tool -Container $container -ExpectedPackageList $expected `
        -OutputRoot $containerChecks | Out-Null } catch { $rejected = $true }
Assert $rejected 'Default mismatch did not throw.'
$checks.Add('failure-exit-contract')
[IO.File]::WriteAllText($expected, $paths[0] + "`n" + $paths[0])
$rejected = $false
try { & $tool -Container $container -ExpectedPackageList $expected `
        -OutputRoot $containerChecks | Out-Null } catch { $rejected = $true }
Assert $rejected 'Duplicate expected entries accepted.'
$checks.Add('duplicate-expectation-rejected')
[IO.File]::WriteAllLines($expected, [string[]]@($paths | ForEach-Object { '../../../' + $_ }))
$normalized = & $tool -Container $container -ExpectedPackageList $expected `
    -OutputRoot $containerChecks
Assert $normalized.packageSetMatches 'Mount prefix normalization failed.'
$checks.Add('container-path-prefix')
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $tool `
    -Container $container -OutputRoot $containerChecks | Out-Null
Assert ($LASTEXITCODE -eq 0) 'Public PS5.1 -File invocation failed.'
$checks.Add('public-file-invocation')
$ucas = Join-Path $root ($stem + '.ucas')
$stream = [IO.File]::Open($ucas, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite)
try {
    $value = $stream.ReadByte()
    Assert ($value -ge 0) 'Test requires nonempty data.'
    $stream.Position = 0
    $stream.WriteByte([byte]($value -bxor 255))
} finally { $stream.Dispose() }
$corrupt = & $tool -Container $container -OutputRoot $containerChecks `
    -MemoryLimitMB 512 -TimeoutSeconds 15 -AllowFailure
Assert ($corrupt.status -ceq 'failed') 'Corrupted data accepted.'
$checks.Add('corrupted-data-rejected')
Assert (($hashes -join ',') -ceq ((@($inputs | ForEach-Object { (Get-FileHash -LiteralPath $_.FullName).Hash })) -join ',')) 'Original container changed.'
$checks.Add('source-preserved')
$summary = [pscustomobject]@{
    status = 'passed'
    passed = $checks.Count
    checks = @($checks)
    scratchCleaned = $true
}
[IO.File]::WriteAllText((Join-Path $root 'summary.json'), ($summary | ConvertTo-Json -Depth 6))
$resolvedRoot = (Resolve-Path -LiteralPath $root).Path
if (-not $resolvedRoot.StartsWith(
        ($tmpBoundary + [IO.Path]::DirectorySeparatorChar),
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing to remove an unowned container-tool test directory.'
}
[IO.Directory]::Delete($resolvedRoot, $true)
$summary | ConvertTo-Json -Depth 6
