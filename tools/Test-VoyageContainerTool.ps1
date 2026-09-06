[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$KnownGoodContainer)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$source = (Resolve-Path -LiteralPath $KnownGoodContainer).Path
$root = Join-Path (Split-Path -Parent $PSScriptRoot) ('artifacts\tests\container-tool-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($root) | Out-Null
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
$baseline = & $tool -Container $container
Assert ($baseline.status -ceq 'passed' -and $baseline.packageCount -gt 0 -and $null -eq $baseline.packageSetMatches) 'Inventory-only result incorrect.'
$checks.Add('verify-and-inventory-without-expectations')
# Derived list is only a comparator fixture, NOT independent release acceptance.
$exact = & $tool -Container $container -ExpectedPackageList $baseline.packageListPath
Assert ($exact.status -ceq 'passed' -and $exact.packageSetMatches) 'Identical expected set rejected.'
$checks.Add('exact-set-comparison')
$paths = @(Get-Content -LiteralPath $baseline.packageListPath)
$expected = Join-Path $root 'wrong-expected.txt'
$different = @($paths | Select-Object -Skip 1) + @('Voyage/Content/MissingFixture.uasset')
[IO.File]::WriteAllLines($expected, [string[]]$different)
$mismatch = & $tool -Container $container -ExpectedPackageList $expected -AllowFailure
$detail = Get-Content -LiteralPath $mismatch.reportPath -Raw | ConvertFrom-Json
Assert ($mismatch.status -ceq 'failed' -and -not $mismatch.packageSetMatches -and
    $detail.missingPackages.Count -eq 1 -and $detail.unexpectedPackages.Count -eq 1) 'Mismatch not explained.'
$checks.Add('missing-and-unexpected-package-report')
$rejected = $false
try { & $tool -Container $container -ExpectedPackageList $expected | Out-Null } catch { $rejected = $true }
Assert $rejected 'Default mismatch did not throw.'
$checks.Add('failure-exit-contract')
[IO.File]::WriteAllText($expected, $paths[0] + "`n" + $paths[0])
$rejected = $false
try { & $tool -Container $container -ExpectedPackageList $expected | Out-Null } catch { $rejected = $true }
Assert $rejected 'Duplicate expected entries accepted.'
$checks.Add('duplicate-expectation-rejected')
[IO.File]::WriteAllLines($expected, [string[]]@($paths | ForEach-Object { '../../../' + $_ }))
$normalized = & $tool -Container $container -ExpectedPackageList $expected
Assert $normalized.packageSetMatches 'Mount prefix normalization failed.'
$checks.Add('container-path-prefix')
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $tool -Container $container | Out-Null
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
$corrupt = & $tool -Container $container -MemoryLimitMB 512 -TimeoutSeconds 15 -AllowFailure
Assert ($corrupt.status -ceq 'failed') 'Corrupted data accepted.'
$checks.Add('corrupted-data-rejected')
Assert (($hashes -join ',') -ceq ((@($inputs | ForEach-Object { (Get-FileHash -LiteralPath $_.FullName).Hash })) -join ',')) 'Original container changed.'
$checks.Add('source-preserved')
$summary = [pscustomobject]@{ passed = $checks.Count; checks = @($checks); evidencePath = $root; baselineReport = $baseline.reportPath; corruptReport = $corrupt.reportPath }
[IO.File]::WriteAllText((Join-Path $root 'summary.json'), ($summary | ConvertTo-Json -Depth 6))
$summary | ConvertTo-Json -Depth 6
