[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OutputRoot,
    [Parameter(Mandatory = $true)][string]$GraphManifest,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$tools = Join-Path $repo 'tools'
$assetPath = 'Voyage/Content/Blueprints/Cables/BP_MooringCable_Socket_Male.uasset'
$query = '/Game/Blueprints/Cables/BP_MooringCable_Socket_Male'
$componentName = 'VoyageModuleSocketView_GEN_VARIABLE'
$expectedBuild = '25191271'
$expectedExe = '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B'
$version = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'version.txt') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Invalid owning mod version.' }
$originalCableName = 'BP_ModuleCable_Mooring'
$relocatedCableName = 'BP_ModuleCable_Orig060'
$graph = Get-Content -LiteralPath $GraphManifest -Raw | ConvertFrom-Json
if ($graph.status -ne 'cooked' -or $graph.gameBuild -ne $expectedBuild -or $graph.gameHash -ne $expectedExe) { throw 'Graph provenance mismatch.' }
$gui = Join-Path $repo '.tools/bin/UAssetGUI.exe'
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
$root = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $root) { throw 'Use a new output directory.' }
$fingerprint = & "$tools/Get-VoyageBuildFingerprint.ps1" -GameRoot $GameRoot | ConvertFrom-Json
if ($fingerprint.steam.buildId -ne $expectedBuild -or $fingerprint.executable.sha256 -ne $expectedExe) {
    throw 'Game fingerprint changed. Revalidate the mooring contract first.'
}
if ((Get-FileHash $gui).Hash -ne '42837CD279A78DF57B537020A0169C5D2259A4570D3B62F4B96852F0F5C27F96') {
    throw 'Revalidate the selected UAssetGUI checkpoint.'
}
if ((Get-FileHash $retoc).Hash -ne '6F8F86AE3FD747A3B785E787A33C24F9A11735D03664948D6B619F18861650F5') {
    throw 'Revalidate the selected retoc checkpoint.'
}
[void](New-Item -ItemType Directory -Path $root)
$mapping = & "$tools/Get-VoyageMappings.ps1" -GameRoot $GameRoot
$extract = & "$tools/Extract-VoyagePackage.ps1" -Filter 'Blueprints/Cables/BP_MooringCable_Socket_Male' -GameRoot $GameRoot -OutputRoot "$root/source"
if ($extract.assetCount -ne 1) { throw 'Extraction must contain exactly one asset.' }
$inputAsset = Join-Path $extract.outputPath $assetPath
$roundtrip = & "$tools/Test-UAssetGuiJsonRoundtrip.ps1" -CandidateGui $gui -InputAsset $inputAsset -Mappings $mapping.mappingsPath -EngineVersion 5.8
if ($roundtrip.status -ne 'passed') { throw 'Unchanged JSON roundtrip failed.' }
function Invoke-Gui([string[]]$Arguments) {
    $result = & "$tools/Invoke-VoyageBoundedTool.ps1" -Executable $gui -Arguments $Arguments -MemoryLimitMB 1024 -TimeoutSeconds 60
    if ($result.status -ne 'passed') { throw "GUI failed: $($result.reportPath)" }
}
$originalJson = "$root/original.json"
Invoke-Gui @('tojson', $inputAsset, $originalJson, '5.8', $mapping.mappingsPath)
$data = Get-Content $originalJson -Raw | ConvertFrom-Json
if (@($data.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count) { throw 'Raw exports are not supported.' }
$component = @($data.Exports | Where-Object ObjectName -eq $componentName)
if ($component.Count -ne 1) { throw 'Expected one exact mooring socket component.' }
$length = @($component[0].Data | Where-Object Name -eq IntegratedCableLength)
if ($length.Count -ne 1 -or $length[0].Value -ne 2000) { throw 'Stock socket length contract changed.' }
$length[0].Value = 6000.0
$changedJson = "$root/changed.json"
[IO.File]::WriteAllText($changedJson, ($data | ConvertTo-Json -Depth 100))
$legacy = "$root/legacy"
$outputAsset = Join-Path $legacy $assetPath
[void](New-Item -ItemType Directory -Path (Split-Path $outputAsset))
Invoke-Gui @('fromjson', $changedJson, $outputAsset, $mapping.mappingsPath, '5.8')
Copy-Item -LiteralPath (Join-Path $extract.outputPath 'scriptobjects.bin') -Destination $legacy
$cableAssetPath = 'Voyage/Content/Blueprints/Cables/BP_ModuleCable_Mooring.uasset'
$cableQuery = '/Game/Blueprints/Cables/BP_ModuleCable_Mooring'
$cableCdoName = 'Default__BP_ModuleCable_Mooring_C'
$cableExtract = & "$tools/Extract-VoyagePackage.ps1" -Filter 'Blueprints/Cables/BP_ModuleCable_Mooring' -GameRoot $GameRoot -OutputRoot "$root/cable-source"
if ($cableExtract.assetCount -ne 1) { throw 'Expected one cable asset.' }
$cableInput = Join-Path $cableExtract.outputPath $cableAssetPath
$cableRoundtrip = & "$tools/Test-UAssetGuiJsonRoundtrip.ps1" -CandidateGui $gui -InputAsset $cableInput -Mappings $mapping.mappingsPath -EngineVersion 5.8
if ($cableRoundtrip.status -ne 'passed') { throw 'Cable unchanged roundtrip failed.' }
Invoke-Gui @('tojson', $cableInput, "$root/cable-original.json", '5.8', $mapping.mappingsPath)
$cableData = Get-Content "$root/cable-original.json" -Raw | ConvertFrom-Json
if (@($cableData.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count) { throw 'Raw cable exports.' }
$cableCdo = @($cableData.Exports | Where-Object ObjectName -eq $cableCdoName)
if ($cableCdo.Count -ne 1) { throw 'Expected one mooring CDO.' }
if (@($cableCdo[0].Data | Where-Object Name -eq MaxLengthBase).Count) { throw 'Unexpected stock base length override.' }
$cableFloat = @($cableCdo[0].Data | Where-Object Name -eq CableSnapLength)
if ($cableFloat.Count -ne 1 -or $cableFloat[0].Value -ne 2000) { throw 'Unexpected snap contract.' }
$newFloat = $cableFloat[0] | ConvertTo-Json -Depth 20 | ConvertFrom-Json
$newFloat.Name = 'MaxLengthBase'; $newFloat.Value = 6000.0
$cableCdo[0].Data = @($cableCdo[0].Data) + @($newFloat)
[IO.File]::WriteAllText("$root/cable-changed.json", ($cableData | ConvertTo-Json -Depth 100))
$relocatedAssetPath = $cableAssetPath.Replace($originalCableName, $relocatedCableName)
$relocatedJson = (Get-Content "$root/cable-changed.json" -Raw).Replace($originalCableName, $relocatedCableName)
[IO.File]::WriteAllText("$root/cable-relocated.json", $relocatedJson)
Invoke-Gui @('fromjson', "$root/cable-relocated.json", (Join-Path $legacy $relocatedAssetPath), $mapping.mappingsPath, '5.8')
Copy-Item -LiteralPath $graph.graphAsset -Destination (Join-Path $legacy $cableAssetPath)
Copy-Item -LiteralPath $graph.graphPayload -Destination ([IO.Path]::ChangeExtension((Join-Path $legacy $cableAssetPath), '.uexp'))
$package = "$root/package"
[void](New-Item -ItemType Directory -Path $package)
$container = "$package/MooringCable60m_P.utoc"
$packResult = & "$tools/Invoke-VoyageBoundedTool.ps1" -Executable $retoc -Arguments @('to-zen', '--version', 'UE5_8', $legacy, $container)
if ($packResult.status -ne 'passed') { throw 'Packaging failed.' }
$expectedList = "$root/expected-packages.txt"
[IO.File]::WriteAllLines($expectedList, @($assetPath, $cableAssetPath, $relocatedAssetPath))
$verification = & "$tools/Test-VoyageContainer.ps1" -Container $container -ExpectedPackageList $expectedList
if ($verification.status -ne 'passed' -or $verification.packageSetMatches -ne $true) { throw 'Container verification failed.' }
$stockResult = & "$tools/Get-VoyageAssetJson.ps1" -Query $query -GameRoot $GameRoot
$candidateResult = & "$tools/Get-VoyageAssetJson.ps1" -Query $query -GameRoot $GameRoot -Source Mod -ModContainer $container
$stock = Get-Content $stockResult.jsonPath -Raw | ConvertFrom-Json
$candidate = Get-Content $candidateResult.jsonPath -Raw | ConvertFrom-Json
$candidateComponent = @($candidate | Where-Object Name -eq $componentName)
if ($candidateComponent.Count -ne 1 -or $candidateComponent[0].Properties.IntegratedCableLength -ne 6000) { throw 'Independent length readback failed.' }
if ($candidateComponent[0].Properties.IntegratedCable.ObjectPath -cne '/Game/Data/Assets/Cable/DA_Cable_Mooring.0') { throw 'Socket must supply the stock mooring cable.' }
$stockComponent = @($stock | Where-Object Name -eq $componentName)
if ($stockComponent.Count -ne 1 -or $stockComponent[0].Properties.IntegratedCableLength -ne 2000) { throw 'Independent stock contract changed.' }
$candidateComponent[0].Properties.IntegratedCableLength = $stockComponent[0].Properties.IntegratedCableLength
if (($stock | ConvertTo-Json -Depth 100 -Compress) -cne ($candidate | ConvertTo-Json -Depth 100 -Compress)) {
    throw 'Independent semantic comparison detected another change.'
}
$cableStockResult = & "$tools/Get-VoyageAssetJson.ps1" -Query $cableQuery -GameRoot $GameRoot
$cableCandidateResult = & "$tools/Get-VoyageAssetJson.ps1" -Query ($cableQuery.Replace($originalCableName, $relocatedCableName)) -GameRoot $GameRoot -Source Mod -ModContainer $container
$cableStock = Get-Content $cableStockResult.jsonPath -Raw | ConvertFrom-Json
$cableCandidate = (Get-Content $cableCandidateResult.jsonPath -Raw).Replace($relocatedCableName, $originalCableName) | ConvertFrom-Json
$readbackCdo = @($cableCandidate | Where-Object Name -eq $cableCdoName)
if ($readbackCdo.Count -ne 1 -or $readbackCdo[0].Properties.MaxLengthBase -ne 6000) { throw 'Cable base length readback failed.' }
$readbackCdo[0].Properties.PSObject.Properties.Remove('MaxLengthBase')
if (($cableStock | ConvertTo-Json -Depth 100 -Compress) -cne ($cableCandidate | ConvertTo-Json -Depth 100 -Compress)) { throw 'Unexpected cable semantic change.' }
$childSummary = & "$tools/Get-VoyageAssetSummary.ps1" -Query $cableQuery -Source Mod -ModContainer $container -Focus Overview
if ($childSummary.data[0].super.objectPath -notlike ('*'+$relocatedCableName+'*')) { throw 'Child must inherit freshly relocated stock mooring class.' }
$childJson = & "$tools/Get-VoyageAssetJson.ps1" -Query $cableQuery -Source Mod -ModContainer $container
$childData = Get-Content $childJson.jsonPath -Raw | ConvertFrom-Json
if (@($childData | Where-Object Name -eq 'MooringRefreshLimit').Count -ne 1) { throw 'Missing limit refresh event.' }
$childScs = @($childData | Where-Object Type -eq 'SimpleConstructionScript')
foreach ($scs in $childScs) {
    if ($scs.Properties.PSObject.Properties.Name -contains 'RootNodes' -and @($scs.Properties.RootNodes).Count) { throw 'Child must not introduce active construction-script roots.' }
}
$childCdo = @($childData | Where-Object Name -eq $cableCdoName)
if ($childCdo.Count -ne 1) { throw 'Expected one child CDO.' }
foreach ($p in $childCdo[0].Properties.PSObject.Properties.Name) {
    if ($p -notin @('UberGraphFrame','RootComponent')) { throw "Unexpected child CDO override: $p" }
}
$archive = "$root/MooringCable60m-$version.zip"
Compress-Archive -Path "$package/*" -DestinationPath $archive
$release = & "$tools/New-VoyageReleaseManifest.ps1" -ReleaseRoot $root -Mod MooringCable60m -Version $version -Container $container -Archive $archive -SourcePath $PSScriptRoot -GameRoot $GameRoot -AllowDirtySource
$report = [ordered]@{
    status = 'candidate-validated'; runtimeValidated = $false
    lengthCm = 6000; attachedPayoutLimitCm = 2000; packageCount = 3
    semanticChanges = @('Male mooring socket IntegratedCableLength: 2000 -> 6000', 'Fresh stock mooring parent relocated with MaxLengthBase 6000', 'Child refreshes MaxLength: both sockets valid 2000, otherwise 6000')
    graphManifest = (Resolve-Path $GraphManifest).Path; childSummaryPath = $childSummary.summaryPath; childJsonPath = $childJson.jsonPath
    steamBuildId = $expectedBuild; executableSha256 = $expectedExe
    extractionManifest = $extract.manifestPath; roundtripReport = $roundtrip.reportPath
    cableExtractionManifest = $cableExtract.manifestPath; cableRoundtripReport = $cableRoundtrip.reportPath
    cableIndependentJsonPath = $cableCandidateResult.jsonPath
    containerReport = $verification.reportPath; independentJsonPath = $candidateResult.jsonPath
    archive = $archive; release = $release
}
[IO.File]::WriteAllText("$root/build-report.json", ($report | ConvertTo-Json -Depth 20))
[pscustomobject]@{ status = $report.status; reportPath = "$root/build-report.json"; archive = $archive; releaseManifest = "$root/release-manifest.json" }
