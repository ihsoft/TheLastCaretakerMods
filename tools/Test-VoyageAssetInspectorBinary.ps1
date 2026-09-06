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
    'Get-VoyageAssetJson.ps1', 'Inspect-VoyageAsset.ps1')) {
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
$matchesFile = Join-Path $inspection[0].DirectoryName 'matches.txt'
$matches = @(Get-Content -LiteralPath $matchesFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
Assert-InspectorTest ($matches.Count -eq 1 -and $matches[0] -match 'BP_VoyageCableUpdater\.uasset$') 'Unexpected narrow inventory.'
$checks.Add('legacy-inspection-without-dotnet')

$getJson = Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1'
$packages = & $getJson -GameRoot $GameRoot -ListPackages
Assert-InspectorTest ($packages.packageCount -gt 0 -and
    (Test-Path -LiteralPath $packages.packageListPath -PathType Leaf)) 'Missing public game inventory.'
$checks.Add('game-inventory-without-dotnet')
$asset = & $getJson -GameRoot $GameRoot -Query '/Game/Blueprints/BP_VoyageCableUpdater'
Assert-InspectorTest ((Test-Path -LiteralPath $asset.jsonPath -PathType Leaf) -and
    (Get-FileHash -LiteralPath $asset.jsonPath -Algorithm SHA256).Hash -ceq $asset.jsonSha256) 'Asset JSON hash mismatch.'
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
