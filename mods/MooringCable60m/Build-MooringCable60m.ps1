# Public release producer: build, cook, package and validate one candidate.
[CmdletBinding()]
param(
    [string]$OutputRoot,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',
    [switch]$IgnorePinnedGameVersion
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$tools = Join-Path $repo 'tools'
$expectedBuild = '25956314'
$tmpOwnerRoot = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/MooringCable60m'))
$version = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'version.txt') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Invalid owning mod version.' }
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $identity = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' +
        [Guid]::NewGuid().ToString('N').Substring(0, 8)
    $OutputRoot = Join-Path $tmpOwnerRoot "build-$version-$identity"
}
$runRoot = [IO.Path]::GetFullPath($OutputRoot)
$tmpBoundary = $tmpOwnerRoot.TrimEnd('\') + [IO.Path]::DirectorySeparatorChar
if (-not $runRoot.StartsWith($tmpBoundary, [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must be a fresh directory below $tmpOwnerRoot."
}
if (Test-Path -LiteralPath $runRoot) { throw "OutputRoot already exists: $runRoot" }

$graphRoot = Join-Path $runRoot 'graph'
$releaseRoot = Join-Path $runRoot 'release'

# Build and cook the generated child.
$output = $graphRoot
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$steamApps = Split-Path -Parent (Split-Path -Parent $game)
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
if (-not (Test-Path -LiteralPath $steamManifest -PathType Leaf)) {
    throw "Steam app manifest not found: $steamManifest"
}
$buildMatch = [regex]::Match([IO.File]::ReadAllText($steamManifest), '"buildid"\s+"(?<id>\d+)"')
if (-not $buildMatch.Success) { throw "Steam build ID not found: $steamManifest" }
$steamBuildId = $buildMatch.Groups['id'].Value
$pinnedGameVersionMatches = $steamBuildId -eq $expectedBuild
if (-not $pinnedGameVersionMatches -and -not $IgnorePinnedGameVersion) {
    throw 'Steam build ID differs from the pinned build. Retry with -IgnorePinnedGameVersion to build from current stock assets and validate the candidate in game.'
}
$mapping = & (Join-Path $tools 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
[IO.Directory]::CreateDirectory($output)|Out-Null
$project=Join-Path $PSScriptRoot 'Voyage.uproject'
$engine=Join-Path $EngineRoot 'Engine'
$editor=Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
function Invoke-Stage([string]$Name,[string]$Executable,[string[]]$Arguments){
    $log=Join-Path $output ($Name+'.log')
    & $Executable @Arguments *> $log
    if($LASTEXITCODE -ne 0){throw "$Name failed ($LASTEXITCODE): $log"}
}
Invoke-Stage 'build' (Join-Path $engine 'Build/BatchFiles/Build.bat') @('VoyageEditor','Win64','Development',$project,'-WaitMutex','-NoHotReloadFromIDE')
$content=Join-Path $PSScriptRoot 'Content'
if(Test-Path -LiteralPath $content){
    $resolved=(Resolve-Path -LiteralPath $content).Path
    if($resolved -cne [IO.Path]::GetFullPath($content)){throw 'Unexpected generated content location.'}
    Move-Item -LiteralPath $resolved -Destination (Join-Path $output 'previous-content')
}
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',(Join-Path $PSScriptRoot '.ddc'),'Process')
Invoke-Stage 'generate' $editor @($project,'-run=GenerateMooring','-unattended','-nop4','-nosplash','-nullrhi',('-abslog='+(Join-Path $output 'generate-unreal.log')))
# Tagged properties keep the minimal editor mirror's property indices out of
# the runtime CDO contract; stock packages retain their original serialization.
Invoke-Stage 'cook' $editor @($project,'-run=cook','-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs','-Package=/Game/Blueprints/Cables/BP_ModuleCable_Mooring','-unattended','-nop4','-nosplash','-nullrhi',('-abslog='+(Join-Path $output 'cook-unreal.log')))
$relative='Voyage/Content/Blueprints/Cables/BP_ModuleCable_Mooring'
$cooked=Join-Path $PSScriptRoot ('Saved/Cooked/Windows/'+$relative)
$graph = [pscustomobject]@{graphAsset=$cooked+'.uasset';graphPayload=$cooked+'.uexp'}
foreach($p in @($graph.graphAsset,$graph.graphPayload)){if(-not(Test-Path -LiteralPath $p)){throw "Missing cooked file: $p"}}

# Extract stock packages, assemble the override and validate the release.
$assetPath = 'Voyage/Content/Blueprints/Cables/BP_MooringCable_Socket_Male.uasset'
$query = '/Game/Blueprints/Cables/BP_MooringCable_Socket_Male'
$componentName = 'VoyageModuleSocketView_GEN_VARIABLE'
$originalCableName = 'BP_ModuleCable_Mooring'
$relocatedCableName = 'BP_ModuleCable_Orig060'
$gui = Join-Path $repo '.tools/bin/UAssetGUI.exe'
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
$root = $releaseRoot
if ((Get-FileHash $gui).Hash -ne '42837CD279A78DF57B537020A0169C5D2259A4570D3B62F4B96852F0F5C27F96') {
    throw 'Revalidate the selected UAssetGUI checkpoint.'
}
if ((Get-FileHash $retoc).Hash -ne '6F8F86AE3FD747A3B785E787A33C24F9A11735D03664948D6B619F18861650F5') {
    throw 'Revalidate the selected retoc checkpoint.'
}
[void](New-Item -ItemType Directory -Path $root)
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
    childSummaryPath = $childSummary.summaryPath; childJsonPath = $childJson.jsonPath
    steamBuildId = $mapping.steamBuildId; executableSha256 = $mapping.executableSha256
    pinnedGameVersionMatches = $pinnedGameVersionMatches
    ignorePinnedGameVersion = [bool]$IgnorePinnedGameVersion
    extractionManifest = $extract.manifestPath; roundtripReport = $roundtrip.reportPath
    cableExtractionManifest = $cableExtract.manifestPath; cableRoundtripReport = $cableRoundtrip.reportPath
    cableIndependentJsonPath = $cableCandidateResult.jsonPath
    containerReport = $verification.reportPath; independentJsonPath = $candidateResult.jsonPath
    archive = $archive; release = $release
}
[IO.File]::WriteAllText("$root/build-report.json", ($report | ConvertTo-Json -Depth 20))
[pscustomobject][ordered]@{
    status = 'candidate-validated'
    version = $version
    runRoot = $runRoot
    releaseRoot = $releaseRoot
    releaseManifest = Join-Path $root 'release-manifest.json'
    archive = $archive
    buildReport = Join-Path $root 'build-report.json'
    runtimeValidated = $false
}
