# Real-container, read-only integration test. Requires a candidate retoc supporting explicit selection.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Retoc,
    [Parameter(Mandatory = $true)][string]$ModContainer,
    [Parameter(Mandatory = $true)][string]$OverrideFilter,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$run = Join-Path $repo ('artifacts\tests\extraction-sources-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($run) | Out-Null
$extract = Join-Path $PSScriptRoot 'Extract-VoyagePackage.ps1'
$candidate = (Resolve-Path -LiteralPath $Retoc).Path
$canonical = Join-Path $repo '.tools\bin\retoc.exe'
$canonicalBefore = (Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash
$checks = [Collections.Generic.List[string]]::new()
function Assert-Extraction([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Get-PairHashes($Result) {
    $files = @(Get-ChildItem -LiteralPath $Result.outputPath -Recurse -File | Where-Object { $_.Extension -in @('.uasset', '.uexp') } | Sort-Object FullName)
    Assert-Extraction ($files.Count -eq 2) 'Expected one .uasset/.uexp pair.'
    ($files | ForEach-Object { (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }) -join ':'
}

# Copy only the named mod to ignored diagnostics and intentionally remove its _P naming priority.
$mod = Get-Item -LiteralPath $ModContainer
$fixture = Join-Path $run 'renamed-mod'
[IO.Directory]::CreateDirectory($fixture) | Out-Null
foreach ($extension in @('.utoc', '.ucas')) {
    Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($mod.FullName, $extension)) -Destination (Join-Path $fixture ('aaa' + $extension))
}
$renamed = Join-Path $fixture 'aaa.utoc'
$game = & $extract -GameRoot $GameRoot -Filter $OverrideFilter -Source Game -Retoc $candidate -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $run 'game')
$selected = & $extract -GameRoot $GameRoot -Filter $OverrideFilter -Source Mod -ModContainer $ModContainer -Retoc $candidate -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $run 'mod')
$lowPriority = & $extract -GameRoot $GameRoot -Filter $OverrideFilter -Source Mod -ModContainer $renamed -Retoc $candidate -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $run 'renamed')
$gameHashes = Get-PairHashes $game
$modHashes = Get-PairHashes $selected
Assert-Extraction ($gameHashes -cne $modHashes) 'Fixture must actually override the stock bytes.'
Assert-Extraction ((Get-PairHashes $lowPriority) -ceq $modHashes) 'Renamed selected mod lost precedence.'
$checks.Add('stock-vs-mod-distinct')
$checks.Add('selected-mod-priority-independent-of-name')
$gameManifest = Get-Content -LiteralPath $game.manifestPath -Raw | ConvertFrom-Json
Assert-Extraction (@($gameManifest.selectedContainers | Where-Object { [IO.Path]::GetFileName($_.path) -notmatch '^(global|pakchunk\d+(optional)?-Windows)\.utoc$' }).Count -eq 0) 'Game view mounted a mod.'
$checks.Add('stock-only-manifest')

# Canonical control uses the legacy installed view. Prove that no other installed mod owns the target.
$originalInventory = & (Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1') -GameRoot $GameRoot -ListPackages -Source Mod -ModContainer $ModContainer
Assert-Extraction (@(Get-Content -LiteralPath $originalInventory.packageListPath | Where-Object { $_.Contains($OverrideFilter) }).Count -eq 1) 'Override fixture is ambiguous.'
foreach ($other in @(Get-ChildItem -LiteralPath (Join-Path $GameRoot 'Voyage\Content\Paks') -Filter '*.utoc' -File |
    Where-Object { $_.Name -notmatch '^(global|pakchunk\d+(optional)?-Windows)\.utoc$' -and $_.FullName -ine $mod.FullName })) {
    $otherInventory = & (Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1') -GameRoot $GameRoot -ListPackages -Source Mod -ModContainer $other.FullName
    Assert-Extraction (@(Get-Content -LiteralPath $otherInventory.packageListPath | Where-Object { $_.Contains($OverrideFilter) }).Count -eq 0) "Another installed mod owns the control target: $($other.Name)"
}
# Deliberately omit -Retoc to exercise the fixed Windows PowerShell default.
$baseline = & $extract -GameRoot $GameRoot -Filter $OverrideFilter -AllowAdditionalContainers -OutputRoot (Join-Path $run 'canonical-control')
Assert-Extraction ((Get-PairHashes $baseline) -ceq $modHashes) 'Candidate changed selected-mod serialization versus canonical control.'
$checks.Add('canonical-byte-control')
$checks.Add('windows-powershell-default-retoc')

$emptyFilter = '__VoyageNoSuchPackage_' + [Guid]::NewGuid().ToString('N')
$rejected = $false
try { & $extract -GameRoot $GameRoot -Filter $emptyFilter -Source Mod -ModContainer $renamed -Retoc $candidate -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $run 'rejected') | Out-Null }
catch { if ($_.Exception.Message -notmatch 'owned by the selected mod') { throw }; $rejected = $true }
Assert-Extraction $rejected 'Unowned mod query was accepted.'
$checks.Add('unowned-query-rejected')
Assert-Extraction ((Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash -ceq $canonicalBefore) 'Canonical retoc was modified.'
$result = [pscustomobject]@{
    status = 'passed'; checks = @($checks); evidencePath = $run
    candidateSha256 = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash
    canonicalSha256 = $canonicalBefore; gamePairHashes = $gameHashes; modPairHashes = $modHashes
    powerShellVersion = $PSVersionTable.PSVersion.ToString(); gameFilesChanged = $false
}
[IO.File]::WriteAllText((Join-Path $run 'summary.json'), ($result | ConvertTo-Json -Depth 4))
$result
