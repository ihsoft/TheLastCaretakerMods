# INTERNAL BUILD STAGE: extracts the base-game item data asset required by the
# StableGyro package through the extractor's isolated stock-game source.

param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
$retocCompatibilityVersion = 'UE5_8'
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
$tmpPrefix = (Join-Path $repoRoot 'Tmp').TrimEnd('\') + '\'
$ownerPrefix = (Join-Path $repoRoot 'Tmp\StableGyro').TrimEnd('\') + '\'
if ($output.StartsWith($tmpPrefix, [StringComparison]::OrdinalIgnoreCase) -and
    -not $output.StartsWith($ownerPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot below Tmp must belong to Tmp/StableGyro.'
}
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$extractor = Join-Path $PSScriptRoot '..\..\tools\Extract-VoyagePackage.ps1'
& $extractor `
    -Source Game `
    -Filter 'Data/Assets/Modules/DA_Item_Module_GyroCopter' `
    -GameRoot $root `
    -Retoc $Retoc `
    -RetocEngineVersion $retocCompatibilityVersion `
    -OutputRoot $output

$manifests = @(Get-ChildItem -LiteralPath $output -File -Recurse -Filter 'extraction-manifest.json')
if ($manifests.Count -ne 1) {
    throw "Expected exactly one extraction manifest; found $($manifests.Count)."
}

Write-Host 'Prepared fresh StableGyro base-game inputs:'
$manifests | Sort-Object FullName | Select-Object -ExpandProperty DirectoryName
