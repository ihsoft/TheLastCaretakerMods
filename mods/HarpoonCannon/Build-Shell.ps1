[CmdletBinding()]
param([string]$OutputRoot = '', [switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $PSScriptRoot 'Voyage.uproject'
$engine = 'K:\Epic Games\UE_5.8\Engine'
$editor = Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not $OutputRoot) { $OutputRoot = Join-Path $repo ('artifacts/harpoon-cannon/shell-return-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputRoot)
$artifactBoundary = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($artifactBoundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be under repository artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Output already exists; use a new candidate identity.' }
$null = New-Item -ItemType Directory -Path $output
$sourcePaths = @('mods/HarpoonCannon/Source','mods/HarpoonCannon/SourceAssets','mods/HarpoonCannon/Config','mods/HarpoonCannon/Voyage.uproject','mods/HarpoonCannon/Build-Shell.ps1','mods/HarpoonCannon/Validate-Shell.ps1','mods/HarpoonCannon/Shell-README.txt','mods/HarpoonCannon/GAME_DERIVED_SOURCES.md','tools/UnrealEditorGeneratorCommon/Public/BlueprintGraphNames.h')
$sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
$modelDirectory = Join-Path $repo 'models/HarpoonCannon'
$modelPath = Join-Path $modelDirectory 'runtime-model.json'
$model = Get-Content -LiteralPath $modelPath -Raw | ConvertFrom-Json
if (-not ($model.PSObject.Properties.Name -contains 'sight') -or $model.sight.parentRole -cne 'pitch' -or $model.sight.locationRelativeToParentCm.Count -ne 3) { throw 'Optical shell requires model sight under pitch.' }
if ($model.schemaVersion -ne 1 -or $model.units -cne 'centimeters' -or $model.uniformScale -ne 1 -or $model.axes.forward -cne '+X' -or $model.axes.right -cne '+Y' -or $model.axes.up -cne '+Z') { throw 'Unsupported model coordinate contract.' }
$sourcePaths += @('mods/HarpoonCannon/HarpoonModelContract.h','models/HarpoonCannon/runtime-model.json')
$sourcePaths += @('models/HarpoonCannon/material-palette.json','models/HarpoonCannon/prepare_materials.py','models/HarpoonCannon/Unreal/HarpoonPaletteData.h','models/HarpoonCannon/Unreal/HarpoonPaletteMaterials.h')
foreach ($inputSource in @($model.sources.obj, $model.sources.mtl)) {
    if ([IO.Path]::IsPathRooted($inputSource.path)) { throw 'Model source must be relative.' }
    $inputPath = [IO.Path]::GetFullPath((Join-Path $modelDirectory $inputSource.path))
    if (-not $inputPath.StartsWith($modelDirectory + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Model source escapes its directory.' }
    if ((Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash -cne $inputSource.sha256) { throw 'Model source hash differs from reviewed descriptor.' }
    $sourcePaths += $inputPath.Substring($repo.Length + 1).Replace('\','/')
}
$objectNames = @(Get-Content -LiteralPath (Join-Path $modelDirectory $model.sources.obj.path) | Where-Object { $_ -cmatch '^o ' } | ForEach-Object { $_.Substring(2).Trim() })
$selected = @($model.meshes | ForEach-Object { $_.objectNames }) + @($model.ammo.instances | ForEach-Object { $_.sourceObjectName })
if ($selected.Count -ne @($selected | Sort-Object -Unique).Count -or @(Compare-Object ($objectNames | Sort-Object) ($selected | Sort-Object)).Count -ne 0) { throw 'Model selections must cover every OBJ object exactly once.' }
if ((@($model.meshes.role | Sort-Object) -join ',') -cne 'base,pitch,yaw') { throw 'Expected base/yaw/pitch mesh roles.' }
if (($model.hierarchy.role -join ',') -cne 'base,yaw,pitch' -or $null -ne $model.hierarchy[0].parentRole -or $model.hierarchy[1].parentRole -cne 'base' -or $model.hierarchy[2].parentRole -cne 'yaw') { throw 'Unsupported model hierarchy.' }
$origin = @(0.0,0.0,0.0)
foreach ($part in $model.hierarchy) {
    if ($part.locationRelativeToParentCm.Count -ne 3) { throw 'Invalid model translation.' }
    for ($axis=0; $axis -lt 3; $axis++) { $origin[$axis] += $part.locationRelativeToParentCm[$axis] }
    $mesh = @($model.meshes | Where-Object { $_.role -ceq $part.role })[0]
    if ($mesh.meshOriginInSourceCm.Count -ne 3) { throw 'Invalid mesh origin.' }
    for ($axis=0; $axis -lt 3; $axis++) { if ([Math]::Abs($origin[$axis] - $mesh.meshOriginInSourceCm[$axis]) -gt 0.000001) { throw 'Mesh origin and hierarchy do not reconstruct neutral pose.' } }
}
if (@($model.hierarchy[0].locationRelativeToParentCm | Where-Object { [Math]::Abs([double]$_) -gt 0.000001 }).Count -ne 0) { throw 'Base must retain the physical installation origin.' }
$instanceNames = @($model.ammo.instances.name)
if ($instanceNames.Count -ne @($instanceNames | Sort-Object -Unique).Count) { throw 'Duplicate ammo component names.' }
foreach ($instance in $model.ammo.instances) {
    if ($instance.parentRole -cne 'yaw' -or $instance.name -cnotmatch '^ammo[0-9]+$' -or $instance.locationRelativeToParentCm.Count -ne 3) { throw 'Unsupported independent ammo component contract.' }
}
$sourceStatus = @(& git -C $repo status --porcelain -- $sourcePaths)
function Get-ShellSourceHashes {
    @(& git -C $repo ls-files --cached --others --exclude-standard -- $sourcePaths | Sort-Object -Unique | ForEach-Object {
        [pscustomobject]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_) -Algorithm SHA256).Hash}
    })
}
$sourceHashes = @(Get-ShellSourceHashes)
$fingerprint = (& (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -OutputPath (Join-Path $output 'fingerprint.json')) | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne '25191271' -or $fingerprint.executable.sha256 -cne '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B') { throw 'Shell-return game provenance mismatch.' }
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1')
$engineVersion = Get-Content -LiteralPath (Join-Path $engine 'Build/Build.version') -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or $engineVersion.PatchVersion -ne 2) { throw 'Shell-return requires reviewed editor 5.8.2.' }

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
Invoke-NativeStage 'generate' $editor @($project,'-run=GenerateHarpoonCannon','-ShellOnly','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-unreal.log')))
$packages = @('/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New','/Game/Mods/HarpoonCannon/SM_HarpoonCannonBase','/Game/Mods/HarpoonCannon/SM_HarpoonCannonYawAssembly','/Game/Mods/HarpoonCannon/SM_HarpoonCannonPitchAssembly')
$packages += '/Game/Mods/HarpoonCannon/SM_HarpoonCannonAmmo'
# Keep new material shader code inline in owned packages. Do not change shared
# project config or require a game-global ShaderArchive-Voyage library override.
Invoke-NativeStage 'cook' $editor @($project,'-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore','-CookSinglePackageNoRefs',('-Package=' + ($packages -join '+')),'-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False','-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6,D3D11TargetedShaderFormats=PCD3D_SM5','-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'cook-unreal.log')))
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
}
Copy-Item -LiteralPath (Join-Path $original.outputPath 'scriptobjects.bin') -Destination (Join-Path $loose 'scriptobjects.bin')
$payload = Join-Path $output 'payload'
$null = New-Item -ItemType Directory -Path $payload
$stem = 'CannonPlacementProbe_P'
$container = Join-Path $payload ($stem + '.utoc')
$pack = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') -Executable $retoc -Arguments @('to-zen',$loose,$container,'--version','UE5_8') -TimeoutSeconds 60 -MemoryLimitMB 1024
if ($pack.status -ne 'passed') { throw 'Packaging failed.' }
$expected = Join-Path $output 'expected-packages.txt'
[IO.File]::WriteAllLines($expected, @($assetRelatives | ForEach-Object { $_ + '.uasset' }))
$verify = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') -Container $container -ExpectedPackageList $expected
if ($verify.status -ne 'passed' -or -not $verify.packageSetMatches) { throw 'Container verification failed.' }
$semantic = & (Join-Path $PSScriptRoot 'Validate-Shell.ps1') -Container $container -OutputRoot (Join-Path $output 'semantic')
if ($semantic.status -ne 'passed') { throw 'Shell semantic validation failed.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Shell-README.txt') -Destination (Join-Path $payload 'README.txt')
$version = Split-Path -Leaf $output
$archivePath = Join-Path $output ('HarpoonCannonShellProbe_' + $version + '.zip')
Compress-Archive -Path (Join-Path $payload '*') -DestinationPath $archivePath
$sourceAfter = @(& git -C $repo status --porcelain -- $sourcePaths)
if (($sourceAfter -join "`n") -cne ($sourceStatus -join "`n")) { throw 'Source status changed during preparation.' }
if ((@(Get-ShellSourceHashes) | ConvertTo-Json -Compress) -cne ($sourceHashes | ConvertTo-Json -Compress)) { throw 'Source content changed during preparation.' }
if ((& git -C $repo rev-parse HEAD).Trim() -cne $sourceCommit) { throw 'Repository HEAD changed during preparation.' }
$provenance = [ordered]@{
    schemaVersion=1;mod='HarpoonCannonShellProbe';version=$version;experiment='shell-return';createdAtUtc=[DateTime]::UtcNow.ToString('o');
    sourceCommit=$sourceCommit;dirtySource=($sourceStatus.Count -gt 0);sourceStatus=$sourceStatus;sourceHashes=$sourceHashes;
    gameEngineVersion='5.8';gameEngineVersionBasis='Reviewed mapping/parser target; game patch version not independently established';editorEngineVersion='5.8.2';retocCompatibilityVersion='UE5_8';retocSha256=$extraction.retocSha256;
    gameFingerprint=@{steamBuildId=[string]$fingerprint.steam.buildId;executableSha256=$fingerprint.executable.sha256};
    validation='static-only; runtime pending';runtimeArchitecture='Cyclone leaf VoyageModuleActor shell; descriptor-driven base/yaw/pitch and shared ammo mesh with separate instances; no operator event graph';
    mappingSha256=$mapping.sha256;verificationReport=$verify.reportPath;packagingReport=$pack.reportPath;semanticReport=$semantic.reportPath;
}
$provenance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-provenance.json') -Encoding UTF8
$release = (& (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') -ReleaseRoot $output -Mod HarpoonCannonShellProbe -Version $version -Container $container -Archive $archivePath -SourcePath $sourcePaths -AllowDirtySource -AsJson) | ConvertFrom-Json
$manifestPath = Join-Path $output 'release-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Manifest producer did not publish the candidate.' }
[pscustomobject]@{status='prepared-not-installed';releaseManifestPath=$manifestPath;archivePath=$archivePath;verificationReport=$verify.reportPath} | ConvertTo-Json -Compress
