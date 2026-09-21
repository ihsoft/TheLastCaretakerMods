[CmdletBinding()]
param([string]$OutputRoot = '', [switch]$SkipBuild, [switch]$Install, [string]$CacheRoot = 'P:\UnrealCache\TheLastCaretakerMods\UE5.8')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $PSScriptRoot 'Voyage.uproject'
$engine = 'K:\Epic Games\UE_5.8\Engine'
$editor = Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not $OutputRoot) { $OutputRoot = Join-Path $repo ('artifacts/railgun/build-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
$artifactBoundary = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifactBoundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be under repository artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Output already exists; use a new build identity.' }
$null = New-Item -ItemType Directory -Path $output
$sourcePaths = @('mods/Railgun','tools/UnrealEditorGeneratorCommon/Public')
$sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
$modelDirectory = Join-Path $PSScriptRoot 'Assets/Model'
$modelPath = Join-Path $modelDirectory 'model-source.json'
$model = Get-Content -LiteralPath $modelPath -Raw | ConvertFrom-Json
$modelFields = @($model.PSObject.Properties.Name | Sort-Object)
$expectedModelFields = @('entryInteraction','fabricatorCollision','nodes','schemaVersion')
if ($model.schemaVersion -ne 1 -or (Compare-Object $modelFields $expectedModelFields)) { throw 'Unsupported Railgun model registry.' }
$glbPath = [IO.Path]::GetFullPath((Join-Path $modelDirectory 'Railgun.glb'))
if (-not (Test-Path -LiteralPath $glbPath -PathType Leaf)) { throw "GLB not found: $glbPath" }
# The stable filename makes model replacement independent of revision names.
# Build provenance hashes the actual file and rejects edits during a build.
$sourcePaths += @($glbPath.Substring($repo.Length + 1).Replace('\','/'))
$sourceStatus = @(& git -C $repo status --porcelain -- $sourcePaths)
function Get-RailgunSourceHashes {
    @(& git -C $repo ls-files --cached --others --exclude-standard -- $sourcePaths | Sort-Object -Unique | Where-Object {
        Test-Path -LiteralPath (Join-Path $repo $_) -PathType Leaf
    } | ForEach-Object {
        [pscustomobject]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_) -Algorithm SHA256).Hash}
    })
}
$sourceHashes = @(Get-RailgunSourceHashes)
$fingerprint = (& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -OutputPath (Join-Path $output 'fingerprint.json')) | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne '25191271' -or $fingerprint.executable.sha256 -cne '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B') { throw 'Railgun game provenance mismatch.' }
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1')
$engineVersion = Get-Content -LiteralPath (Join-Path $engine 'Build/Build.version') -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or $engineVersion.PatchVersion -ne 2) { throw 'Railgun requires reviewed editor 5.8.2.' }

function Invoke-NativeStage([string]$Name, [string]$Executable, [string[]]$NativeArguments) {
    if ($Executable -eq $editor) { $NativeArguments += ('-ZenDataPath=' + (Join-Path $ddc 'Zen')) }
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
$ddc = [IO.Path]::GetFullPath($CacheRoot)
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $ddc, 'Process')
$null = New-Item -ItemType Directory -Path $ddc -Force
Invoke-NativeStage 'generate' $editor @($project,'-run=GenerateRailgun','-ShellOnly','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-unreal.log')))
$inventoryPath = Join-Path $output 'model-inventory.json'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Saved/RailgunGlbInventory.json') -Destination $inventoryPath
$inventory = Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
$packages = @($inventory.packages)
if ($packages.Count -lt 2 -or @($packages | Where-Object { -not $_.StartsWith('/Game/Mods/Railgun/Visual/') -and $_ -cne '/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New' }).Count) { throw 'GLB cook inventory escaped owned packages.' }
$shotSound = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/Railgun_Shot_Blast.wav')).Path
$scopeOverlay = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ScopeOverlay.png')).Path
$chargingStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ChargingStatusIcon.png')).Path
$offlineStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/OfflineStatusIcon.png')).Path
$readyStatusIcon = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'Assets/ScopeOverlay/ReadyStatusIcon.png')).Path
Invoke-NativeStage 'generate-inputs' $editor @($project,'-run=GenerateRailgunInputs','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-inputs-unreal.log')))
Invoke-NativeStage 'generate-runtime' $editor @($project,'-run=GenerateRailgunRuntime','-DedicatedStation',('-ShotSound=' + $shotSound),('-ScopeOverlay=' + $scopeOverlay),('-ChargingStatusIcon=' + $chargingStatusIcon),('-OfflineStatusIcon=' + $offlineStatusIcon),('-ReadyStatusIcon=' + $readyStatusIcon),'-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-runtime-unreal.log')))
$packages += @(
    '/Game/Mods/Railgun/Inputs/IA_RailgunLookYaw',
    '/Game/Mods/Railgun/Inputs/IA_RailgunLookPitch',
    '/Game/Mods/Railgun/Inputs/IA_RailgunExit',
    '/Game/Mods/Railgun/Inputs/IA_RailgunZoom',
    '/Game/Mods/Railgun/Inputs/IA_RailgunFire',
    '/Game/Mods/Railgun/Inputs/IMC_RailgunKeyboard',
    '/Game/Mods/Railgun/Inputs/DA_RailgunInputContext',
    '/Game/Mods/Railgun/Station/BP_RailgunOperator',
    '/Game/Mods/Railgun/Station/WBP_RailgunHUD',
    '/Game/Mods/Railgun/Station/T_RailgunOpticalMask',
    '/Game/Mods/Railgun/Station/T_RailgunStatusCharging',
    '/Game/Mods/Railgun/Station/T_RailgunStatusOffline',
    '/Game/Mods/Railgun/Station/T_RailgunStatusReady',
    '/Game/Mods/Railgun/Station/BP_RailgunTestShot',
    '/Game/Mods/Railgun/Station/S_RailgunShotBlast',
    '/Game/Mods/Railgun/Runtime/BP_RailgunCoordinator',
    '/Game/Mods/Railgun/Runtime/WBP_RailgunDiagnostics'
)
$packages = @($packages | Sort-Object -Unique)
# Keep new material shader code inline in owned packages. Do not change shared
# project config or require a game-global ShaderArchive-Voyage library override.
# Partial native mirrors serialize named property tags, never positional indices.
Invoke-NativeStage 'cook' $editor @($project,'-run=cook','-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs',('-Package=' + ($packages -join '+')),'-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False','-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6,D3D11TargetedShaderFormats=PCD3D_SM5','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'cook-unreal.log')))
Invoke-NativeStage 'verify-tagged' $editor @($project,'-run=GenerateRailgun','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-tagged-unreal.log')))
Invoke-NativeStage 'verify-runtime-tagged' $editor @($project,'-run=GenerateRailgunRuntime','-VerifyTagged','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-runtime-tagged-unreal.log')))
$original = & (Join-Path $repo 'tools/Extract-VoyagePackage.ps1') -Filter 'Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New.uasset' -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $output 'original')
$extraction = Get-Content -LiteralPath $original.manifestPath -Raw | ConvertFrom-Json
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
if ((Get-FileHash -LiteralPath $retoc -Algorithm SHA256).Hash -cne $extraction.retocSha256) { throw 'retoc identity changed after extraction.' }
$loose = Join-Path $output 'loose'
$assetRelatives = @($packages | ForEach-Object { $_.Replace('/Game/', 'Voyage/Content/') })
foreach ($assetRelative in $assetRelatives) {
    $looseAsset = Join-Path $loose $assetRelative
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $looseAsset) -Force
    foreach ($extension in @('.uasset','.uexp')) {
        $cooked = Join-Path (Join-Path $PSScriptRoot 'Saved/Cooked/Windows') ($assetRelative + $extension)
        Copy-Item -LiteralPath $cooked -Destination ($looseAsset + $extension)
    }
    foreach ($extension in @('.ubulk','.uptnl')) {
        $cooked = Join-Path (Join-Path $PSScriptRoot 'Saved/Cooked/Windows') ($assetRelative + $extension)
        if (Test-Path -LiteralPath $cooked -PathType Leaf) { Copy-Item -LiteralPath $cooked -Destination ($looseAsset + $extension) }
    }
}
Copy-Item -LiteralPath (Join-Path $original.outputPath 'scriptobjects.bin') -Destination (Join-Path $loose 'scriptobjects.bin')
$payload = Join-Path $output 'payload'
$null = New-Item -ItemType Directory -Path $payload
$stem = 'Railgun_P'
$container = Join-Path $payload ($stem + '.utoc')
$pack = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') -Executable $retoc -Arguments @('to-zen',$loose,$container,'--version','UE5_8') -TimeoutSeconds 60 -MemoryLimitMB 1024
if ($pack.status -ne 'passed') { throw 'Packaging failed.' }
$expected = Join-Path $output 'expected-packages.txt'
[IO.File]::WriteAllLines($expected, @($assetRelatives | ForEach-Object { $_ + '.uasset' }))
$verify = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') -Container $container -ExpectedPackageList $expected
if ($verify.status -ne 'passed' -or -not $verify.packageSetMatches) { throw 'Container verification failed.' }
$semantic = & (Join-Path $PSScriptRoot 'Validate-Railgun.ps1') -Container $container -OutputRoot (Join-Path $output 'semantic') -ModelInventory $inventoryPath
if ($semantic.status -ne 'passed') { throw 'Railgun semantic validation failed.' }
$containerReport = Get-Content -LiteralPath $verify.reportPath -Raw | ConvertFrom-Json
$bulkChunks = @($containerReport.chunkTypes | Where-Object { $_.type -ceq 'BulkData' } | ForEach-Object { $_.count } | Measure-Object -Sum).Sum
if ($null -eq $bulkChunks -or $bulkChunks -lt 1) { throw 'Cooked shot sound bulk data is absent from the container.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Railgun_P.autoload') -Destination $payload
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Assets/Railgun.ini') -Destination $payload
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'README.txt') -Destination $payload
$version = Split-Path -Leaf $output
$archivePath = Join-Path $output ('Railgun_' + $version + '.zip')
Compress-Archive -Path (Join-Path $payload '*') -DestinationPath $archivePath
$sourceAfter = @(& git -C $repo status --porcelain -- $sourcePaths)
if (($sourceAfter -join "`n") -cne ($sourceStatus -join "`n")) { throw 'Source status changed during preparation.' }
if ((@(Get-RailgunSourceHashes) | ConvertTo-Json -Compress) -cne ($sourceHashes | ConvertTo-Json -Compress)) { throw 'Source content changed during preparation.' }
if ((& git -C $repo rev-parse HEAD).Trim() -cne $sourceCommit) { throw 'Repository HEAD changed during preparation.' }
$provenance = [ordered]@{
    schemaVersion=1;mod='Railgun';version=$version;createdAtUtc=[DateTime]::UtcNow.ToString('o');
    sourceCommit=$sourceCommit;dirtySource=($sourceStatus.Count -gt 0);sourceStatus=$sourceStatus;sourceHashes=$sourceHashes;
    gameEngineVersion='5.8';gameEngineVersionBasis='Reviewed mapping/parser target; game patch version not independently established';editorEngineVersion='5.8.2';retocCompatibilityVersion='UE5_8';retocSha256=$extraction.retocSha256;
    gameFingerprint=@{steamBuildId=[string]$fingerprint.steam.buildId;executableSha256=$fingerprint.executable.sha256};
    validation='build and static verification; gameplay validation is a separate gate';runtimeArchitecture='Single Railgun container with construction override, GLB visual hierarchy, operator, inputs, HUD, shot audio and autoload coordinator';
    mappingSha256=$mapping.sha256;verificationReport=$verify.reportPath;packagingReport=$pack.reportPath;semanticReport=$semantic.reportPath;
}
$provenance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-provenance.json') -Encoding UTF8
$releaseSourcePaths = @($sourcePaths | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $repo $_)) })
$release = (& (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod Railgun -Version $version -Container $container -Archive $archivePath -SourcePath $releaseSourcePaths -AllowDirtySource -AsJson) | ConvertFrom-Json
$manifestPath = Join-Path $output 'release-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Manifest producer did not publish the candidate.' }
$installation = $null
$settingsInstallation = $null
if ($Install) {
    $installation = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') -ReleaseManifest $manifestPath -AllowDirtySource
    $settingsPath = Join-Path $installation.paksDirectory 'Railgun.ini'
    if (-not (Test-Path -LiteralPath $settingsPath)) {
        if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' -ErrorAction SilentlyContinue).Count -gt 0) { throw 'Game started; settings installation refused.' }
        Copy-Item -LiteralPath (Join-Path $payload 'Railgun.ini') -Destination $settingsPath
        $settingsInstallation = [pscustomobject]@{path=$settingsPath;created=$true;sha256=(Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash}
    } else {
        $settingsInstallation = [pscustomobject]@{path=$settingsPath;created=$false;sha256=(Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash}
    }
    Write-Host 'Railgun installed successfully; container hashes verified and settings are present.'
}
[pscustomobject]@{status=$(if ($Install) { 'installed' } else { 'prepared-not-installed' });releaseManifestPath=$manifestPath;archivePath=$archivePath;verificationReport=$verify.reportPath;installation=$installation;settingsInstallation=$settingsInstallation} | ConvertTo-Json -Depth 8 -Compress
