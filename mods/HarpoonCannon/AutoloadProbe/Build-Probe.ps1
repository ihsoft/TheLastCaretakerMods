[CmdletBinding()]
param([string]$OutputRoot = '', [switch]$SkipBuild, [switch]$StationInputsOnly, [switch]$StationPrototype)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($StationInputsOnly.IsPresent -eq $StationPrototype.IsPresent) { throw 'Select exactly one: -StationInputsOnly (authoring only) or -StationPrototype (HC33 all-hit target names experiment).' }
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$project = Join-Path $PSScriptRoot 'Voyage.uproject'
$engine = 'K:\Epic Games\UE_5.8\Engine'
$editor = Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not $OutputRoot) {
    $prefix = if ($StationPrototype) { 'hc33-' } else { 'hc25-inputs-' }
    $OutputRoot = Join-Path $repo ('artifacts/harpoon-cannon/' + $prefix + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss'))
}
$output = [IO.Path]::GetFullPath($OutputRoot)
$artifactBoundary = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifactBoundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be under repository artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Output already exists; use a new candidate identity.' }
$null = New-Item -ItemType Directory -Path $output
$sourcePaths = @('mods/HarpoonCannon/AutoloadProbe','tools/UnrealEditorGeneratorCommon/Public/ActorLifecycleGraphNames.h','tools/UnrealEditorGeneratorCommon/Public/AssetLoadingGraphNames.h','tools/UnrealEditorGeneratorCommon/Public/BlueprintGraphNames.h','tools/UnrealEditorGeneratorCommon/Public/CharacterObservationGraphNames.h','tools/UnrealEditorGeneratorCommon/Public/CharacterStationGraphNames.h','tools/UnrealEditorGeneratorCommon/Public/OpticalCameraGraphNames.h')
$sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
$sourcePaths += 'tools/UnrealEditorGeneratorCommon/Public/ActorScanGraphNames.h'
$sourceStatus = @(& git -C $repo status --porcelain -- $sourcePaths)
function Get-ProbeSourceHashes {
    @(& git -C $repo ls-files --cached --others --exclude-standard -- $sourcePaths | Sort-Object -Unique | ForEach-Object {
        [pscustomobject]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_) -Algorithm SHA256).Hash}
    })
}
$sourceHashes = @(Get-ProbeSourceHashes)
$fingerprint = (& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -OutputPath (Join-Path $output 'fingerprint.json')) | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne '25191271' -or $fingerprint.executable.sha256 -cne '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B') { throw 'HC01 game provenance mismatch.' }
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1')
$engineVersion = Get-Content -LiteralPath (Join-Path $engine 'Build/Build.version') -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or $engineVersion.PatchVersion -ne 2) { throw 'HC01 requires reviewed editor 5.8.2.' }

function Invoke-NativeStage([string]$Name, [string]$Executable, [string[]]$NativeArguments) {
    $log = Join-Path $output ($Name + '.log')
    & $Executable @NativeArguments *> $log
    if ($LASTEXITCODE -ne 0) { throw "$Name failed ($LASTEXITCODE); log: $log" }
    Write-Host "$Name passed; log: $log"
}
if (-not $SkipBuild) {
    Invoke-NativeStage 'build' (Join-Path $engine 'Build/BatchFiles/Build.bat') @('VoyageEditor','Win64','Development',('-Project=' + $project),'-WaitMutex','-NoHotReloadFromIDE',('-Log=' + (Join-Path $output 'ubt.log')))
}
$content = Join-Path $PSScriptRoot 'Content'
if (Test-Path -LiteralPath $content) {
    # Exact owned generated tree, checked destination remains under artifacts.
    $resolvedContent = (Resolve-Path -LiteralPath $content).Path
    if ($resolvedContent -cne [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'Content'))) { throw 'Unexpected generated Content target.' }
    Move-Item -LiteralPath $resolvedContent -Destination (Join-Path $output 'previous-generated')
}
$ddc = Join-Path $PSScriptRoot '.ddc'
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $ddc, 'Process')
$null = New-Item -ItemType Directory -Path $ddc -Force
Invoke-NativeStage 'generate' $editor @($project,'-run=GenerateHarpoonInputs','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-unreal.log')))
$packages = @('/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookYaw','/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookPitch','/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonExit','/Game/Mods/HarpoonCannon/Inputs/IMC_HarpoonKeyboard','/Game/Mods/HarpoonCannon/Inputs/DA_HarpoonInputContext')
if ($StationPrototype) {
    Invoke-NativeStage 'generate-station' $editor @($project,'-run=GenerateHarpoonProbe','-DedicatedStation','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-station-unreal.log')))
    $packages += @('/Game/Mods/HarpoonCannon/Station/BP_HarpoonOperator','/Game/Mods/HarpoonCannon/Station/WBP_HarpoonHUD','/Game/Mods/HarpoonCannonLifecycleProbe/ModActor','/Game/Mods/HarpoonCannonLifecycleProbe/ProbeHUD')
}
$cookArguments = @($project,'-run=cook','-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs',('-Package=' + ($packages -join '+')),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'cook-unreal.log')))
if ($StationInputsOnly) { $cookArguments += '-unversioned' }
Invoke-NativeStage 'cook' $editor $cookArguments
if ($StationPrototype) {
    Invoke-NativeStage 'verify-tagged' $editor @($project,'-run=GenerateHarpoonProbe','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-tagged-unreal.log')))
}
$original = & (Join-Path $repo 'tools/Extract-VoyagePackage.ps1') -Filter 'Blueprints/BP_FirstPersonCharacter_New.uasset' -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $output 'original')
$extraction = Get-Content -LiteralPath $original.manifestPath -Raw | ConvertFrom-Json
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
if ((Get-FileHash -LiteralPath $retoc -Algorithm SHA256).Hash -cne $extraction.retocSha256) { throw 'retoc identity changed after extraction.' }
$loose = Join-Path $output 'loose'
$assetRelatives = @($packages | ForEach-Object { $_ -creplace '^/Game/', 'Voyage/Content/' })
foreach ($assetRelative in $assetRelatives) {
    $looseAsset = Join-Path $loose $assetRelative
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $looseAsset) -Force
    foreach ($extension in @('.uasset','.uexp')) {
        $cooked = Join-Path (Join-Path $PSScriptRoot 'Saved/Cooked/Windows') ($assetRelative + $extension)
        Copy-Item -LiteralPath $cooked -Destination ($looseAsset + $extension)
    }
}
Copy-Item -LiteralPath (Join-Path $original.outputPath 'scriptobjects.bin') -Destination (Join-Path $loose 'scriptobjects.bin')
$payload = Join-Path $output 'payload'
$null = New-Item -ItemType Directory -Path $payload
$modName = if ($StationPrototype) { 'HarpoonCannonLifecycleProbe' } else { 'HarpoonCannonInputPrototype' }
$stem = $modName + '_P'
$container = Join-Path $payload ($stem + '.utoc')
$pack = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') -Executable $retoc -Arguments @('to-zen',$loose,$container,'--version','UE5_8') -TimeoutSeconds 60 -MemoryLimitMB 1024
if ($pack.status -ne 'passed') { throw 'Packaging failed.' }
$expected = Join-Path $output 'expected-packages.txt'
[IO.File]::WriteAllLines($expected, @($assetRelatives | ForEach-Object { $_ + '.uasset' }))
$verify = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') -Container $container -ExpectedPackageList $expected
if ($verify.status -ne 'passed' -or -not $verify.packageSetMatches) { throw 'Container verification failed.' }
if ($StationPrototype) {
    # Cheap release-blocking gate: editor defaults must not erase this channel delta.
    $queryEvidence = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') -Query '/Game/Mods/HarpoonCannon/Station/BP_HarpoonOperator' -Source Mod -ModContainer $container -AsJson) | ConvertFrom-Json
    $stationExports = Get-Content -LiteralPath $queryEvidence.jsonPath -Raw | ConvertFrom-Json
    $classes = @($stationExports | Where-Object { $_.Name -ceq 'BP_HarpoonOperator_C' -and $_.Type -ceq 'BlueprintGeneratedClass' })
    if ($classes.Count -ne 1) { throw 'HC32: missing/ambiguous operator class.' }
    $entryInterfaces = @($classes[0].Interfaces | Where-Object { $_.Class.ObjectName -ceq "Class'InteractiveInterface'" -and $_.Class.ObjectPath -ceq '/Script/Voyage' -and $_.bImplementedByK2 -eq $true })
    if ($entryInterfaces.Count -ne 1) { throw 'HC32: cooked operator must explicitly implement InteractiveInterface in Blueprint.' }
    # Explicit interface implementation has no inherited UFunction SuperStruct
    # (same as our validated own HUD interface). Gate membership + exact signature.
    $entryFunctions = @($stationExports | Where-Object { $_.Name -ceq 'GetInteractiveProvidedActions' -and $_.Type -ceq 'Function' })
    if ($entryFunctions.Count -ne 1) { throw 'HC32: missing/ambiguous interface implementation function.' }
    $entryParams = @($entryFunctions[0].ChildProperties | Where-Object { $_.PSObject.Properties['PropertyFlags'] -and $_.PropertyFlags -match '(^| \| )Parm( \| |$)' })
    if (($entryParams.Name -join ',') -cne 'MyCharacter,Component,OutActions,ReturnValue' -or
        ($entryParams.Type -join ',') -cne 'ObjectProperty,ObjectProperty,ArrayProperty,BoolProperty' -or
        $entryParams[0].PropertyClass.ObjectName -cne "Class'Pawn'" -or $entryParams[0].PropertyClass.ObjectPath -cne '/Script/Engine' -or
        $entryParams[1].PropertyClass.ObjectName -cne "Class'SceneComponent'" -or $entryParams[1].PropertyClass.ObjectPath -cne '/Script/Engine' -or
        $entryParams[2].Inner.Struct.ObjectName -cne "Class'PlayerInputInterfaceAction'" -or $entryParams[2].Inner.Struct.ObjectPath -cne '/Script/Voyage' -or
        $entryParams[2].PropertyFlags -notmatch 'OutParm' -or $entryParams[3].PropertyFlags -notmatch 'ReturnParm' -or
        $entryFunctions[0].FunctionFlags -notmatch 'FUNC_BlueprintEvent') { throw 'HC32: explicit interface implementation signature mismatch.' }
    $queryBoxes = @($stationExports | Where-Object { $_.Name -ceq 'HarpoonEntryQuery_GEN_VARIABLE' -and $_.Type -ceq 'BoxComponent' })
    if ($queryBoxes.Count -ne 1) { throw 'HC31: missing/ambiguous cooked query box.' }
    $responses = @($queryBoxes[0].Properties.BodyInstance.CollisionResponses.ResponseArray | Where-Object { $_.Channel -ceq 'Interact' })
    if ($responses.Count -ne 1 -or $responses[0].Response -notmatch '(^|::)ECR_Block$') { throw 'HC31: cooked query must explicitly serialize Interact=Block.' }
    $queryEvidence | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'collision-json-evidence.json') -Encoding UTF8
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'HarpoonCannonLifecycleProbe_P.autoload') -Destination $payload
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'StationPrototype-README.txt') -Destination (Join-Path $payload 'README.txt')
} else {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'InputPrototype-README.txt') -Destination (Join-Path $payload 'README.txt')
}
$version = Split-Path -Leaf $output
$archivePath = Join-Path $output ($modName + '_' + $version + '.zip')
Compress-Archive -Path (Join-Path $payload '*') -DestinationPath $archivePath
$sourceAfter = @(& git -C $repo status --porcelain -- $sourcePaths)
if (($sourceAfter -join "`n") -cne ($sourceStatus -join "`n")) { throw 'Source status changed during preparation.' }
if ((@(Get-ProbeSourceHashes) | ConvertTo-Json -Compress) -cne ($sourceHashes | ConvertTo-Json -Compress)) { throw 'Source content changed during preparation.' }
if ((& git -C $repo rev-parse HEAD).Trim() -cne $sourceCommit) { throw 'Repository HEAD changed during preparation.' }
$experiment = if ($StationPrototype) { 'HC33-all-hit-names-mouse80' } else { 'HC25-input-authoring' }
$architecture = if ($StationPrototype) { 'Nine tagged packages. Own common VehiclePawn child, native-selected own HUD, Enhanced Input E/mouse handlers and owned camera. No Forklift runtime dependency. Native inherited property deltas must pass independent audit; runtime pending.' } else { 'Five standalone Harpoon input assets only; no actor, widget, autoload, Forklift references or installation.' }
$provenance = [ordered]@{
    schemaVersion=1;mod=$modName;version=$version;experiment=$experiment;createdAtUtc=[DateTime]::UtcNow.ToString('o');
    sourceCommit=$sourceCommit;dirtySource=($sourceStatus.Count -gt 0);sourceStatus=$sourceStatus;sourceHashes=$sourceHashes;
    gameEngineVersion='5.8';gameEngineVersionBasis='Reviewed mapping/parser target; game patch version not independently established';editorEngineVersion='5.8.2';retocCompatibilityVersion='UE5_8';retocSha256=$extraction.retocSha256;
    gameFingerprint=@{steamBuildId=[string]$fingerprint.steam.buildId;executableSha256=$fingerprint.executable.sha256};
    validation='static-only; runtime unvalidated';runtimeArchitecture=$architecture;
    mappingSha256=$mapping.sha256;verificationReport=$verify.reportPath;packagingReport=$pack.reportPath;
}
$provenance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-provenance.json') -Encoding UTF8
$release = (& (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod $modName -Version $version -Container $container -Archive $archivePath -SourcePath $sourcePaths -AllowDirtySource -AsJson) | ConvertFrom-Json
$manifestPath = Join-Path $output 'release-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Manifest producer did not publish the candidate.' }
[pscustomobject]@{status='prepared-not-installed';releaseManifestPath=$manifestPath;archivePath=$archivePath;verificationReport=$verify.reportPath} | ConvertTo-Json -Compress
