# Validate a producer-supplied clean source against a cooked/re-extracted package.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OriginalAsset,
    [Parameter(Mandatory)][string]$CookedAsset,
    [Parameter(Mandatory)][string]$MappingsPath,
    [Parameter(Mandatory)][string]$Recipe,
    [Parameter(Mandatory)][string]$OutputRoot
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputRoot) { throw 'Validation output already exists.' }
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$source=Join-Path $OutputRoot 'source.json'
$cooked=Join-Path $OutputRoot 'cooked.json'
foreach($pair in @(@($OriginalAsset,$source),@($CookedAsset,$cooked))) {
    & pwsh -NoProfile -File (Join-Path $PSScriptRoot 'VoyageItemPatch/Read-Item.ps1') -AssetPath $pair[0] -MappingsPath $MappingsPath -OutputPath $pair[1] | Out-Null
    if($LASTEXITCODE -ne 0) { throw 'Complete DataAsset read failed.' }
}
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python -B (Join-Path $PSScriptRoot 'VoyageItemPatch/item_patch.py') compare --source $source --cooked $cooked --recipe $Recipe --output (Join-Path $OutputRoot 'summary.json')
if($LASTEXITCODE -ne 0) { throw "Cooked DataAsset differs from recipe. Evidence: $OutputRoot" }
foreach($path in @($source,$cooked)) { Remove-Item -LiteralPath $path }
