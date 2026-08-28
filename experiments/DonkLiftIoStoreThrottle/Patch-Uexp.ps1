param(
    [Parameter(Mandatory = $true)]
    [string] $Source,

    [Parameter(Mandatory = $true)]
    [string] $Destination,

    [double] $Replacement = 0.2
)

$ErrorActionPreference = 'Stop'

$expectedSourceSha256 = 'EF1B840F4751E63C0F537988934E56646A294640DEBBF7CE598D7EA18C016392'
$valueOffset = 17680
$expectedValue = [BitConverter]::GetBytes([double] 1.0)
$replacementValue = [BitConverter]::GetBytes($Replacement)

$sourcePath = (Resolve-Path -LiteralPath $Source).Path
$sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourcePath).Hash
if ($sourceHash -ne $expectedSourceSha256) {
    throw "Unexpected source SHA-256: $sourceHash"
}

$destinationPath = [IO.Path]::GetFullPath($Destination)
$destinationDirectory = [IO.Path]::GetDirectoryName($destinationPath)
if (-not [string]::IsNullOrEmpty($destinationDirectory)) {
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
}

$bytes = [IO.File]::ReadAllBytes($sourcePath)
for ($index = 0; $index -lt $expectedValue.Length; $index++) {
    if ($bytes[$valueOffset + $index] -ne $expectedValue[$index]) {
        throw "Expected throttle interpolation constant 1.0 was not found at offset $valueOffset"
    }
}

[Array]::Copy($replacementValue, 0, $bytes, $valueOffset, $replacementValue.Length)
[IO.File]::WriteAllBytes($destinationPath, $bytes)

$written = [IO.File]::ReadAllBytes($destinationPath)
$actualValue = [BitConverter]::ToDouble($written, $valueOffset)
if ($actualValue -ne $Replacement) {
    throw "Patched value verification failed: $actualValue"
}

$destinationHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destinationPath).Hash
Write-Host "Patched throttle interpolation speed: 1.0 -> $Replacement"
Write-Host "Source SHA-256:      $sourceHash"
Write-Host "Destination SHA-256: $destinationHash"
Write-Host "Destination:          $destinationPath"
