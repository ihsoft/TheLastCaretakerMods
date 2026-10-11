[CmdletBinding()]
param(
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc,

    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedSteamBuildId = '25191271'
$expectedExecutableSha256 = '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B'
$expectedEditorVersion = '5.8.2'
$retocCompatibilityVersion = 'UE5_8'
$containerName = 'ElectrifiedBoat'
$modActorPackage = '/Game/Mods/ElectrifiedBoat/ModActor'
$entryClass = '/Game/Mods/ElectrifiedBoat/ModActor.ModActor_C'
$expectedPackage = '../../../Voyage/Content/Mods/ElectrifiedBoat/ModActor.uasset'
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$ownerTmp = [IO.Path]::GetFullPath((Join-Path $repoRoot 'Tmp/ElectrifiedBoat'))
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $ownerTmp ('build-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
}
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not ($output + [IO.Path]::DirectorySeparatorChar).StartsWith(
        ($ownerTmp + [IO.Path]::DirectorySeparatorChar),
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must be a fresh directory below $ownerTmp"
}
if (Test-Path -LiteralPath $output) {
    throw "Use a fresh OutputRoot identity: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools/bin/retoc.exe'
}
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$retocSha256 = (Get-FileHash -LiteralPath $retocPath -Algorithm SHA256).Hash
$project = Join-Path $modRoot 'Voyage.uproject'
$buildBatch = (Resolve-Path -LiteralPath (
    Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat')).Path
$editor = (Resolve-Path -LiteralPath (
    Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe')).Path
$engineVersionPath = (Resolve-Path -LiteralPath (
    Join-Path $EngineRoot 'Engine/Build/Build.version')).Path
$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$editorVersion = '{0}.{1}.{2}' -f @(
    $engineVersion.MajorVersion,
    $engineVersion.MinorVersion,
    $engineVersion.PatchVersion)
if ($editorVersion -cne $expectedEditorVersion) {
    throw "Unreal Editor version must be $expectedEditorVersion; found $editorVersion."
}

$fingerprintPath = Join-Path $output 'game-fingerprint.json'
& (Join-Path $repoRoot 'tools/Get-VoyageBuildFingerprint.ps1') `
    -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne $expectedSteamBuildId -or
    [string]$fingerprint.executable.sha256 -cne $expectedExecutableSha256) {
    throw 'Installed game fingerprint does not match the generator contracts.'
}

$generatedContent = Join-Path $modRoot 'Content'
if (Test-Path -LiteralPath $generatedContent) {
    $resolvedContent = (Resolve-Path -LiteralPath $generatedContent).Path
    if (-not $resolvedContent.StartsWith(
            $modRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean generated Content outside this mod: $resolvedContent"
    }
    Remove-Item -LiteralPath $resolvedContent -Recurse -Force
}

$buildLog = Join-Path $output 'editor-build.log'
$buildArguments = @(
    'VoyageEditor',
    'Win64',
    'Development',
    ('-Project="{0}"' -f $project),
    ('-Log="{0}"' -f (Join-Path $output 'unreal-build-tool.log')),
    '-WaitMutex',
    '-NoHotReload',
    '-NoUBA'
)
$buildProcess = Start-Process `
    -FilePath $buildBatch `
    -ArgumentList $buildArguments `
    -Wait `
    -PassThru `
    -WindowStyle Hidden `
    -RedirectStandardOutput $buildLog `
    -RedirectStandardError (Join-Path $output 'editor-build-errors.log')
if ($buildProcess.ExitCode -ne 0) {
    throw "VoyageEditor build failed with exit code $($buildProcess.ExitCode); see $buildLog"
}

$ddc = Join-Path $modRoot 'DerivedDataCache'
$generateLog = Join-Path $output 'generate.log'
$generateArguments = @(
    ('"{0}"' -f $project),
    '-run=GenerateElectrifiedBoat',
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nullrhi',
    '-ddc=NoZenLocalFallback',
    ('-LocalDataCachePath="{0}"' -f $ddc),
    ('-abslog="{0}"' -f $generateLog)
)
$generateProcess = Start-Process `
    -FilePath $editor `
    -ArgumentList $generateArguments `
    -Wait `
    -PassThru `
    -WindowStyle Hidden
if ($generateProcess.ExitCode -ne 0) {
    throw "Asset generation failed with exit code $($generateProcess.ExitCode); see $generateLog"
}
$ownedActorCanaryMessage = 'OwnedActors Blueprint by-reference read canary passed'
if (-not ([IO.File]::ReadAllText($generateLog).Contains($ownedActorCanaryMessage))) {
    throw "OwnedActors Blueprint by-reference read canary did not pass; see $generateLog"
}

$cookedPlatform = [IO.Path]::GetFullPath((Join-Path $modRoot 'Saved/Cooked/Windows'))
$expectedCookedParent = [IO.Path]::GetFullPath((Join-Path $modRoot 'Saved/Cooked')) + `
    [IO.Path]::DirectorySeparatorChar
if (-not $cookedPlatform.StartsWith(
        $expectedCookedParent,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing unexpected cooked-output path: $cookedPlatform"
}
if (Test-Path -LiteralPath $cookedPlatform) {
    Remove-Item -LiteralPath $cookedPlatform -Recurse -Force
}

$cookPackages = @($modActorPackage)
$cookRelatives = @('Voyage/Content/Mods/ElectrifiedBoat/ModActor')
$cookedAccumulated = Join-Path $output 'cooked-accumulated'
for ($cookIndex = 0; $cookIndex -lt $cookPackages.Count; $cookIndex++) {
    $cookLog = Join-Path $output ('cook-' + ($cookIndex + 1) + '.log')
    $cookArguments = @(
        ('"{0}"' -f $project), '-run=cook', '-targetplatform=Windows',
        '-unversioned', '-SkipZenStore',
        ('-Package={0}' -f $cookPackages[$cookIndex]),
        '-CookSinglePackageNoRefs', '-unattended', '-nop4', '-nosplash',
        '-nullrhi', '-ddc=NoZenLocalFallback',
        ('-LocalDataCachePath="{0}"' -f $ddc),
        ('-abslog="{0}"' -f $cookLog)
    )
    $cookProcess = Start-Process -FilePath $editor -ArgumentList $cookArguments `
        -Wait -PassThru -WindowStyle Hidden
    if ($cookProcess.ExitCode -ne 0) {
        throw "Cook failed with exit code $($cookProcess.ExitCode); see $cookLog"
    }
    $sourceBase = Join-Path $cookedPlatform $cookRelatives[$cookIndex]
    $destinationBase = Join-Path $cookedAccumulated $cookRelatives[$cookIndex]
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destinationBase)) | Out-Null
    foreach ($extension in @('.uasset', '.uexp')) {
        if (-not (Test-Path -LiteralPath ($sourceBase + $extension) -PathType Leaf)) {
            throw "Expected cooked file was not produced: $sourceBase$extension"
        }
        Copy-Item -LiteralPath ($sourceBase + $extension) -Destination ($destinationBase + $extension)
    }
}

$cookedBase = Join-Path $cookedAccumulated $cookRelatives[0]
foreach ($extension in @('.uasset', '.uexp')) {
    if (-not (Test-Path -LiteralPath ($cookedBase + $extension) -PathType Leaf)) {
        throw "Expected cooked ModActor file was not produced: $cookedBase$extension"
    }
}
$nameTable = [Text.Encoding]::ASCII.GetString(
    [IO.File]::ReadAllBytes($cookedBase + '.uasset'))
foreach ($requiredName in @(
    'OnActorAttached',
    'OnEndPlay',
    'HandleActorAttached',
    'HandleRegisteredActorEndPlay',
    'GetRootPrimitiveComponent',
    'RemoveSecondaryGroup',
    'VoyageLevelInstanceComponent',
    'OwnedActors',
    'Boat_Electric',
    'BP_AttachmentVirtualSocketActor_Electricity_C',
    'SetSocketID',
    'Port',
    'AddExternalSocket',
    'RemoveExternalSocket',
    'RegisteredExternalActors',
    'RegisteredExternalSockets',
    'RegisteredExpectedOwners',
    'SetActorTickEnabled',
    'Array_Contains',
    'PairedModule',
    'BP_WallSocket_Electric_C'
)) {
    if (-not $nameTable.Contains($requiredName)) {
        throw "Cooked ModActor name table is missing: $requiredName"
    }
}

$stage = Join-Path $output 'stage'
$stageMod = Join-Path $stage 'Voyage/Content/Mods/ElectrifiedBoat'
$package = Join-Path $output 'package'
[IO.Directory]::CreateDirectory($stageMod) | Out-Null
[IO.Directory]::CreateDirectory($package) | Out-Null
Copy-Item -LiteralPath ($cookedBase + '.uasset'), ($cookedBase + '.uexp') `
    -Destination $stageMod

$utoc = Join-Path $package ($containerName + '.utoc')
& $retocPath to-zen --version $retocCompatibilityVersion $stage $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc verify failed with exit code $LASTEXITCODE"
}

$inventory = @(& $retocPath list --path --size --hash --package $utoc)
if ($LASTEXITCODE -ne 0) {
    throw "retoc list failed with exit code $LASTEXITCODE"
}
$inventoryRecords = @(
    $inventory | ForEach-Object {
        if ($_ -match '^\S+\s+(?<chunk>\S+)\s+(?<hash>\S+)\s+(?<package>\S+)\s+' +
            '\S+\s+(?<size>\d+)\s+(?<path>\.\./\.\./\.\./Voyage/Content/.+\.uasset)$') {
            [pscustomobject]@{
                path = $Matches.path
                size = [int64]$Matches.size
                hash = $Matches.hash
                packageId = $Matches.package
                chunkId = $Matches.chunk
            }
        }
    }
)
$actualInventorySet = (@($inventoryRecords.path | Sort-Object) -join "`n")
$expectedInventorySet = $expectedPackage
if ($inventoryRecords.Count -ne 1 -or
    $actualInventorySet -cne $expectedInventorySet) {
    throw 'Packaged inventory is not the exact one-package additive set.'
}
$inventoryPath = Join-Path $package ($containerName + '.inventory.txt')
$inventoryRecords | ForEach-Object {
    '{0}|{1}|{2}|{3}|{4}' -f `
        $_.path, $_.size, $_.hash, $_.packageId, $_.chunkId
} | Set-Content -LiteralPath $inventoryPath
$expectedListPath = Join-Path $package 'expected-packages.txt'
[IO.File]::WriteAllText(
    $expectedListPath,
    $expectedPackage + [Environment]::NewLine,
    (New-Object Text.UTF8Encoding($false)))

$autoloadPath = Join-Path $package ($containerName + '.autoload')
[IO.File]::WriteAllText(
    $autoloadPath,
    ('entryClass: ' + $entryClass + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))

$containerFiles = @(
    (Join-Path $package ($containerName + '.pak')),
    (Join-Path $package ($containerName + '.ucas')),
    $utoc
)
foreach ($containerFile in $containerFiles) {
    if (-not (Test-Path -LiteralPath $containerFile -PathType Leaf)) {
        throw "Expected container file was not produced: $containerFile"
    }
}
$containerHashes = @(
    Get-FileHash -LiteralPath $containerFiles -Algorithm SHA256 |
        ForEach-Object {
            [ordered]@{ path = $_.Path; sha256 = $_.Hash }
        }
)
$buildManifest = [ordered]@{
    kind = 'ElectrifiedBoat candidate'
    artifactVersion = '0.1-candidate'
    containerName = $containerName
    architecture = 'additive-owned-package-only'
    entryClass = $entryClass
    ownedPackages = @($modActorPackage)
    stockPackages = @()
    startupRecovery = 'none; OnActorAttached handles new and restored wall sockets'
    lifecycle = @(
        'OnActorAttached synchronous external battery-port registration',
        'wall actor OnEndPlay guarded RemoveExternalSocket cleanup'
    )
    steadyState = 'no actor Tick, timers, polling, startup scan, or recurring discovery'
    diagnostic = 'none'
    electricityDiscriminator = 'exact stock class equality'
    boatGroup = 'none; external registration uses the first suitable Boat-owned stock electric port, resolving its parent through stock GetParentModule and native GetModuleFromActor'
    socketId = 'stock SetSocketID(Conv_StringToName(GetObjectName(WallActor))); no invented collision scan'
    externalRegistration = 'copy complete Port plus five live reference booleans, call AddExternalSocket, then register guarded EndPlay cleanup'
    ownedActorRead = 'exact native weak array; Blueprint helper uses standard by-reference array get and passed weak/null/weak ProcessEvent canary'
    pairedModuleWrite = 'clear only; never Boat master or mod object'
    steamBuildId = $expectedSteamBuildId
    executableSha256 = $expectedExecutableSha256
    editorVersion = $editorVersion
    retocCompatibilityVersion = $retocCompatibilityVersion
    retocSha256 = $retocSha256
    containerFiles = $containerHashes
    gameplayValidated = $false
    saveReloadValidated = $false
    removalValidated = $false
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$manifestPath = Join-Path $package 'build-manifest.json'
[IO.File]::WriteAllText(
    $manifestPath,
    (($buildManifest | ConvertTo-Json -Depth 8) + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))

$validation = & (Join-Path $modRoot 'Test-ElectrifiedBoatCandidate.ps1') `
    -PackageRoot $package `
    -GameRoot $GameRoot

[pscustomobject]@{
    status = 'passed'
    outputRoot = $output
    packageRoot = $package
    manifestPath = $manifestPath
    validationResult = $validation
    gameplayValidated = $false
}
