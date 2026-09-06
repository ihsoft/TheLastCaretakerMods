# Read-only against the installed game; diagnostics stay under artifacts/tests.
[CmdletBinding()]
param([string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$run = Join-Path $repo ('artifacts\tests\inspector-binary-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($run) | Out-Null
$checks = [Collections.Generic.List[string]]::new()
function Assert-InspectorTest([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
# Any regression to source-project launch in a child script fails immediately.
function dotnet { throw 'Normal Inspector operation attempted dotnet.' }
function dotnet.exe { throw 'Normal Inspector operation attempted dotnet.exe.' }

foreach ($name in @('Get-VoyageAssetInspectorBinary.ps1', 'Publish-VoyageAssetInspectorBinary.ps1',
    'Get-VoyageAssetJson.ps1', 'Get-VoyageAssetSummary.ps1', 'Inspect-VoyageAsset.ps1')) {
    $tokens = $null
    $parseErrors = $null
    [void][Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot $name),
        [ref]$tokens, [ref]$parseErrors)
    Assert-InspectorTest ($parseErrors.Count -eq 0) "PowerShell parse failure: $name"
}
$checks.Add('public-script-parse')
$binary = & (Join-Path $PSScriptRoot 'Get-VoyageAssetInspectorBinary.ps1')
$beforeTime = (Get-Item -LiteralPath $binary.Path).LastWriteTimeUtc
$published = & (Join-Path $PSScriptRoot 'Publish-VoyageAssetInspectorBinary.ps1')
Assert-InspectorTest (-not $published.Rebuilt -and $published.Sha256 -ceq $binary.Sha256 -and
    (Get-Item -LiteralPath $binary.Path).LastWriteTimeUtc -eq $beforeTime) 'Publisher rebuilt a current binary.'
$checks.Add('publisher-reuse-without-dotnet')

$inspectionRoot = Join-Path $run 'inspection'
& (Join-Path $PSScriptRoot 'Inspect-VoyageAsset.ps1') -GameRoot $GameRoot `
    -Query 'list:BP_VoyageCableUpdater' -OutputRoot $inspectionRoot *> (Join-Path $run 'inspect.log')
$inspection = @(Get-ChildItem -LiteralPath $inspectionRoot -Recurse -Filter inspection-manifest.json -File)
Assert-InspectorTest ($inspection.Count -eq 1) 'Legacy inspection did not produce one manifest.'
$manifest = Get-Content -LiteralPath $inspection[0].FullName -Raw | ConvertFrom-Json
Assert-InspectorTest ($manifest.inspectorBinarySha256 -ceq $binary.Sha256) 'Inspection used an unexpected binary.'
Assert-InspectorTest ($manifest.source -ceq 'Game' -and $null -eq $manifest.modContainer) `
    'Default inspection did not isolate stock game containers.'
$matchesFile = Join-Path $inspection[0].DirectoryName 'matches.txt'
$matches = @(Get-Content -LiteralPath $matchesFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
Assert-InspectorTest ($matches.Count -eq 1 -and $matches[0] -match 'BP_VoyageCableUpdater\.uasset$') 'Unexpected narrow inventory.'
$checks.Add('legacy-inspection-without-dotnet')

$sourceGuarded = $false
try {
    & (Join-Path $PSScriptRoot 'Inspect-VoyageAsset.ps1') -GameRoot $GameRoot `
        -Query 'list:BP_VoyageCableUpdater' -Source Mod -OutputRoot (Join-Path $run 'invalid-mod-source') *> $null
}
catch {
    $sourceGuarded = $_.Exception.Message -like '-Source Mod requires -ModContainer*'
}
Assert-InspectorTest $sourceGuarded 'Mod inspection accepted no exact mod container.'
$checks.Add('inspection-source-guard')

$referenceRoot = Join-Path $run 'reference-no-match'
$referenceResult = & (Join-Path $PSScriptRoot 'Inspect-VoyageAsset.ps1') -GameRoot $GameRoot `
    -Query 'references:__VOYAGE_PIPELINE_EXPECTED_NO_MATCH__|Voyage/Content/Blueprints/Vehicles/BP_Forklift_Possesable' `
    -OutputRoot $referenceRoot
Assert-InspectorTest ($referenceResult.status -ceq 'no-match' -and $referenceResult.matchCount -eq 0 -and
    (Test-Path -LiteralPath $referenceResult.resultPath -PathType Leaf)) `
    'Reference no-match was not returned as a successful structured result.'
$referenceManifest = Get-Content -LiteralPath $referenceResult.manifestPath -Raw | ConvertFrom-Json
Assert-InspectorTest ($referenceManifest.queryStatus -ceq 'no-match' -and $referenceManifest.matchCount -eq 0 -and
    $referenceManifest.errorCount -eq 0) 'Reference no-match manifest is incomplete.'
$checks.Add('reference-no-match-contract')

$strictFailed = $false
try {
    & (Join-Path $PSScriptRoot 'Inspect-VoyageAsset.ps1') -GameRoot $GameRoot `
        -Query 'references:__VOYAGE_PIPELINE_EXPECTED_NO_MATCH__|Voyage/Content/Blueprints/Vehicles/BP_Forklift_Possesable' `
        -OutputRoot (Join-Path $run 'reference-required') -RequireMatch *> $null
}
catch {
    $strictFailed = $_.Exception.Message -like 'Reference search completed successfully but found no matches:*'
}
Assert-InspectorTest $strictFailed 'Reference -RequireMatch did not fail a successful zero-match search.'
$checks.Add('reference-require-match-contract')

$getJson = Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1'
$packages = & $getJson -GameRoot $GameRoot -ListPackages
Assert-InspectorTest ($packages.packageCount -gt 0 -and
    (Test-Path -LiteralPath $packages.packageListPath -PathType Leaf)) 'Missing public game inventory.'
$checks.Add('game-inventory-without-dotnet')
$jsonInventoryText = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $getJson `
    -GameRoot $GameRoot -ListPackages -AsJson
Assert-InspectorTest ($LASTEXITCODE -eq 0) 'Get-VoyageAssetJson -AsJson failed through powershell.exe -File.'
$jsonInventory = ([string]::Join([Environment]::NewLine, @($jsonInventoryText))) | ConvertFrom-Json
Assert-InspectorTest ($jsonInventory.packageCount -eq $packages.packageCount -and
    $jsonInventory.packageListPath -ceq $packages.packageListPath -and
    $jsonInventory.packageListSha256 -ceq $packages.packageListSha256) `
    'Get-VoyageAssetJson -AsJson did not preserve the complete inventory result.'
$checks.Add('game-inventory-json-file-entry-point')
$formatterJsonText = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $getJson `
    -GameRoot $GameRoot `
    -Query 'Voyage/Content/Blueprints/Modules/Utility/Weapons/BP_Module_Turret.uasset' `
    -AsJson
Assert-InspectorTest ($LASTEXITCODE -eq 0) 'Formatter-only failure blocked Get-VoyageAssetJson.'
$formatterResult = ([string]::Join([Environment]::NewLine, @($formatterJsonText))) | ConvertFrom-Json
Assert-InspectorTest ($formatterResult.pseudocodeStatus -ceq 'unavailable' -and
    (Test-Path -LiteralPath $formatterResult.jsonPath -PathType Leaf) -and
    (Get-FileHash -LiteralPath $formatterResult.jsonPath -Algorithm SHA256).Hash -ceq $formatterResult.jsonSha256) `
    'Formatter-only failure did not return validated asset JSON with explicit status.'
$checks.Add('asset-json-independent-of-optional-pseudocode')
$summaryScript = Join-Path $PSScriptRoot 'Get-VoyageAssetSummary.ps1'
$summaryText = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $summaryScript `
    -GameRoot $GameRoot `
    -Query 'Voyage/Content/Blueprints/Modules/Utility/Weapons/BP_Module_Turret.uasset' `
    -Focus Overview -AsJson
Assert-InspectorTest ($LASTEXITCODE -eq 0) 'Get-VoyageAssetSummary overview failed through powershell.exe -File.'
$summaryResult = ([string]::Join([Environment]::NewLine, @($summaryText))) | ConvertFrom-Json
Assert-InspectorTest ($summaryResult.status -ceq 'summarized' -and
    $summaryResult.counts.generatedClasses -eq 1 -and $summaryResult.counts.functions -gt 0 -and
    $summaryResult.resultCount -eq 1 -and
    (Test-Path -LiteralPath $summaryResult.summaryPath -PathType Leaf) -and
    (Get-FileHash -LiteralPath $summaryResult.summaryPath -Algorithm SHA256).Hash -ceq $summaryResult.summarySha256) `
    'Compact asset overview is incomplete or its evidence hash is wrong.'
$checks.Add('compact-asset-structure-overview')
$functionText = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $summaryScript `
    -GameRoot $GameRoot `
    -Query 'Voyage/Content/Blueprints/Modules/Utility/Weapons/BP_Module_Turret.uasset' `
    -Focus Functions -FunctionName GetInteractiveProvidedActions -AsJson
Assert-InspectorTest ($LASTEXITCODE -eq 0) 'Get-VoyageAssetSummary exact function failed through powershell.exe -File.'
$functionResult = ([string]::Join([Environment]::NewLine, @($functionText))) | ConvertFrom-Json
Assert-InspectorTest ($functionResult.resultCount -eq 1 -and
    $functionResult.data[0].name -ceq 'GetInteractiveProvidedActions' -and
    $functionResult.summaryPath -ceq $summaryResult.summaryPath -and
    $functionResult.summarySha256 -ceq $summaryResult.summarySha256) `
    'Exact function focus did not reuse and filter the asset summary.'
$checks.Add('compact-asset-function-focus')
$asset = & $getJson -GameRoot $GameRoot -Query '/Game/Blueprints/BP_VoyageCableUpdater'
Assert-InspectorTest ((Test-Path -LiteralPath $asset.jsonPath -PathType Leaf) -and
    (Get-FileHash -LiteralPath $asset.jsonPath -Algorithm SHA256).Hash -ceq $asset.jsonSha256 -and
    -not [string]::IsNullOrWhiteSpace([string]$asset.pseudocodeStatus)) `
    'Asset JSON hash or pseudocode-status contract is invalid.'
$jsonTime = (Get-Item -LiteralPath $asset.jsonPath).LastWriteTimeUtc
$checks.Add('game-json-without-dotnet')
$again = & $getJson -GameRoot $GameRoot -Query '/Game/Blueprints/BP_VoyageCableUpdater'
Assert-InspectorTest ($again.jsonPath -ceq $asset.jsonPath -and $again.jsonSha256 -ceq $asset.jsonSha256 -and
    (Get-Item -LiteralPath $again.jsonPath).LastWriteTimeUtc -eq $jsonTime) 'Repeated asset request did not reuse the result.'
$checks.Add('game-json-reuse')
$result = [pscustomobject]@{
    status = 'passed'
    checks = @($checks)
    powerShellVersion = $PSVersionTable.PSVersion.ToString()
    inspectorBinarySha256 = $binary.Sha256
    packageCount = $packages.packageCount
    jsonPath = $asset.jsonPath
    jsonSha256 = $asset.jsonSha256
    evidencePath = $run
    gameFilesChanged = $false
}
[IO.File]::WriteAllText((Join-Path $run 'summary.json'), ($result | ConvertTo-Json -Depth 4))
$result
