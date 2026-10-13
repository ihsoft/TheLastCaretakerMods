# HAND-WRITTEN RELEASE PRODUCER. Builds the editor generator, cooks the exact
# replacement, relocates a freshly extracted stock parent, packages the two
# runtime assets, and publishes a manifest-validated release. It never installs.

param(
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]*$')]
    [string]$Version = 'v1-test',
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8',
    [string]$OutputRoot,
    [switch]$AllowDirtySource,
    [switch]$FinalizeExisting
)

$ErrorActionPreference = 'Stop'
$expectedBuild = '25880952'
$expectedExecutableHash = '16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44'
$retocVersion = 'UE5_8'
$containerName = 'ContainerLidLabel_P'
$sourcePackage = '/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large'
$relocatedPackage = '/Game/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large'
$assetName = 'BP_Inventory_PlayerBox_Large'
$assetFilter = 'Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large'

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$tmpRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'Tmp\ContainerLidLabel'))
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $stamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
    $OutputRoot = Join-Path $tmpRoot "releases\ContainerLidLabel-$Version-$stamp"
}
$releaseRoot = [IO.Path]::GetFullPath($OutputRoot)
$tmpPrefix = $tmpRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $releaseRoot.StartsWith($tmpPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must stay below $tmpRoot"
}
if ($FinalizeExisting) {
    Write-Host 'Finalize 1/4 Loading prior validation evidence'
    if (-not (Test-Path -LiteralPath $releaseRoot -PathType Container)) {
        throw "FinalizeExisting requires an existing OutputRoot: $releaseRoot"
    }
}
else {
    if (Test-Path -LiteralPath $releaseRoot) { throw "OutputRoot already exists: $releaseRoot" }
    New-Item -ItemType Directory -Path $releaseRoot | Out-Null
}

$project = Join-Path $modRoot 'Voyage.uproject'
$buildBatch = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
$fingerprinter = Join-Path $repoRoot 'tools\Get-VoyageBuildFingerprint.ps1'
$extractor = Join-Path $repoRoot 'tools\Extract-VoyagePackage.ps1'
$containerTester = Join-Path $repoRoot 'tools\Test-VoyageContainer.ps1'
$manifestProducer = Join-Path $repoRoot 'tools\New-VoyageReleaseManifest.ps1'
foreach ($required in @($project, $buildBatch, $editor, $retoc, $fingerprinter, $extractor, $containerTester, $manifestProducer)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Required file was not found: $required" }
}
if ($sourcePackage.Length -ne $relocatedPackage.Length) { throw 'Relocation paths must have equal byte lengths.' }

function Invoke-LoggedProcess {
    param([string]$FilePath, [string[]]$Arguments, [string]$Name)
    $logRoot = Join-Path $releaseRoot 'logs'
    New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
    $stdout = Join-Path $logRoot "$Name.stdout.log"
    $stderr = Join-Path $logRoot "$Name.stderr.log"
    $process = Start-Process -FilePath $FilePath -ArgumentList $Arguments -Wait -PassThru `
        -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    if ($process.ExitCode -ne 0) {
        Get-Content -LiteralPath $stdout -Tail 50 -ErrorAction SilentlyContinue
        Get-Content -LiteralPath $stderr -Tail 50 -ErrorAction SilentlyContinue
        throw "$Name failed with exit code $($process.ExitCode). Logs: $logRoot"
    }
}

function Find-Offsets {
    param([byte[]]$Bytes, [byte[]]$Sequence)
    $matches = [Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le $Bytes.Length - $Sequence.Length; $offset++) {
        $match = $true
        for ($index = 0; $index -lt $Sequence.Length; $index++) {
            if ($Bytes[$offset + $index] -ne $Sequence[$index]) { $match = $false; break }
        }
        if ($match) { $matches.Add($offset); $offset += $Sequence.Length - 1 }
    }
    return $matches
}

if ($FinalizeExisting) {
    $packageRoot = Join-Path $releaseRoot 'package'
    $utoc = Join-Path $packageRoot "$containerName.utoc"
    $archive = Join-Path $releaseRoot "ContainerLidLabel-$Version.zip"
    $releaseManifestPath = Join-Path $releaseRoot 'release-manifest.json'
    $expectedList = Join-Path $releaseRoot 'expected-packages.txt'
    $verification = Get-ChildItem -LiteralPath (Join-Path $releaseRoot 'container-check') `
        -Recurse -File -Filter 'verification.json' | Select-Object -First 1
    $stockUexp = Get-ChildItem -LiteralPath (Join-Path $releaseRoot 'work\stock-extraction') `
        -Recurse -File -Filter "$assetName.uexp" | Select-Object -First 1
    $relocatedUexp = Join-Path $releaseRoot "work\stage\Voyage\Content\Mods\ContainerLidLabel\StockOrigin\$assetName.uexp"
    foreach ($required in @($utoc, $archive, $releaseManifestPath, $expectedList, $verification.FullName, $stockUexp.FullName, $relocatedUexp)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "FinalizeExisting input missing: $required" }
    }
    $verificationRecord = Get-Content -LiteralPath $verification.FullName -Raw | ConvertFrom-Json
    if ([string]$verificationRecord.status -cne 'passed' -or
        [int]$verificationRecord.packageCount -ne 2 -or
        $verificationRecord.packageSetMatches -ne $true -or
        @($verificationRecord.missingPackages).Count -ne 0 -or
        @($verificationRecord.unexpectedPackages).Count -ne 0 -or
        [string]$verificationRecord.expectedPackageListSha256 -cne (Get-FileHash -Algorithm SHA256 -LiteralPath $expectedList).Hash) {
        throw "FinalizeExisting rejected the container verification report: $($verification.FullName)"
    }
    foreach ($fileRecord in @($verificationRecord.files)) {
        $livePath = Join-Path $packageRoot ([string]$fileRecord.name)
        if (-not (Test-Path -LiteralPath $livePath -PathType Leaf) -or
            [int64]$fileRecord.size -ne (Get-Item -LiteralPath $livePath).Length -or
            [string]$fileRecord.sha256 -cne (Get-FileHash -Algorithm SHA256 -LiteralPath $livePath).Hash) {
            throw "FinalizeExisting rejected changed container file: $livePath"
        }
    }
    Write-Host 'Finalize 2/4 Validating immutable release manifest'
    $releaseRecord = Get-Content -LiteralPath $releaseManifestPath -Raw | ConvertFrom-Json
    if ([int]$releaseRecord.schemaVersion -ne 2 -or
        [string]$releaseRecord.mod -cne 'ContainerLidLabel' -or
        [string]$releaseRecord.version -cne $Version -or
        [string]$releaseRecord.gameFingerprint.steamBuildId -cne $expectedBuild -or
        [string]$releaseRecord.gameFingerprint.executableSha256 -cne $expectedExecutableHash) {
        throw "FinalizeExisting rejected release identity: $releaseManifestPath"
    }
    foreach ($record in @($releaseRecord.payload) + @($releaseRecord.archive)) {
        $livePath = Join-Path $releaseRoot ([string]$record.path)
        if (-not (Test-Path -LiteralPath $livePath -PathType Leaf) -or
            [int64]$record.size -ne (Get-Item -LiteralPath $livePath).Length -or
            [string]$record.sha256 -cne (Get-FileHash -Algorithm SHA256 -LiteralPath $livePath).Hash) {
            throw "FinalizeExisting rejected changed release file: $livePath"
        }
    }
    $stockHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $stockUexp.FullName).Hash
    if ($stockHash -cne (Get-FileHash -Algorithm SHA256 -LiteralPath $relocatedUexp).Hash) {
        throw 'FinalizeExisting found a changed relocated stock export payload.'
    }
    Write-Host 'Finalize 3/4 Hashing authored source and artifacts'
    $sourceFiles = @(
        Get-Item -LiteralPath (Join-Path $modRoot '.gitignore')
        Get-Item -LiteralPath (Join-Path $modRoot 'Voyage.uproject')
        Get-Item -LiteralPath (Join-Path $modRoot 'Build-ContainerLidLabel.ps1')
        Get-ChildItem -LiteralPath (Join-Path $modRoot 'Source') -Recurse -File
        Get-ChildItem -LiteralPath $modRoot -File -Filter '*.md'
    ) | Sort-Object FullName -Unique
    Write-Host "Finalize 3a/4 Enumerated $($sourceFiles.Count) source files"
    $payload = @(
        (Join-Path $packageRoot "$containerName.pak"),
        (Join-Path $packageRoot "$containerName.ucas"),
        $utoc,
        $archive,
        $releaseManifestPath)
    $buildManifest = [ordered]@{
        schemaVersion = 1; status = 'succeeded'; mod = 'ContainerLidLabel'; version = $Version
        createdUtc = [DateTime]::UtcNow.ToString('o'); steamBuildId = $expectedBuild
        executableSha256 = $expectedExecutableHash; engine = '5.8.2'; retocEngineVersion = $retocVersion
        runtimePackages = [IO.File]::ReadAllLines($expectedList)
        relocation = [ordered]@{ source = $sourcePackage; destination = $relocatedPackage; replacementCount = 2; sourceUexpSha256 = $stockHash; relocatedUexpSha256 = $stockHash }
        containerCheckReport = $verification.FullName; releaseManifest = $releaseManifestPath
        sourceFiles = @($sourceFiles | ForEach-Object { [ordered]@{ path = $_.FullName.Substring($repoRoot.Length + 1).Replace('\', '/'); length = $_.Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash } })
        files = @($payload | ForEach-Object { $item = Get-Item -LiteralPath $_; [ordered]@{ path = $item.FullName; length = $item.Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_).Hash } })
    }
    Write-Host 'Finalize 3b/4 Hash records assembled'
    $buildManifestPath = Join-Path $releaseRoot 'build-manifest.json'
    Write-Host 'Finalize 3c/4 Serializing build identity'
    [IO.File]::WriteAllText($buildManifestPath, ($buildManifest | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
    Write-Host 'Finalize 4/4 Build identity recorded'
    [pscustomobject]@{ status = 'succeeded'; releaseRoot = $releaseRoot; releaseManifest = $releaseManifestPath; buildManifest = $buildManifestPath; container = $utoc; archive = $archive; packageCount = 2 }
    exit 0
}

Write-Host '1/8 Validating game fingerprint'
$fingerprintPath = Join-Path $releaseRoot 'game-fingerprint.json'
& $fingerprinter -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne $expectedBuild -or
    [string]$fingerprint.executable.sha256 -cne $expectedExecutableHash) {
    throw "Installed game does not match the ContainerLidLabel source contract. See $fingerprintPath"
}

Write-Host '2/8 Building editor generator'
Invoke-LoggedProcess -FilePath $buildBatch -Name 'editor-build' -Arguments @(
    'VoyageEditor', 'Win64', 'Development', ('-Project="{0}"' -f $project), '-WaitMutex', '-NoHotReload')

Write-Host '3/8 Generating Blueprint assets'
$content = [IO.Path]::GetFullPath((Join-Path $modRoot 'Content'))
$contentPrefix = $modRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $content.StartsWith($contentPrefix, [StringComparison]::OrdinalIgnoreCase) -or
    (Split-Path -Leaf $content) -cne 'Content') { throw "Unsafe generated Content path: $content" }
if (Test-Path -LiteralPath $content) {
    $item = Get-Item -LiteralPath $content -Force
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing reparse point: $content" }
    Remove-Item -LiteralPath $content -Recurse -Force
}
Invoke-LoggedProcess -FilePath $editor -Name 'generate' -Arguments @(
    ('"{0}"' -f $project), '-run=GenerateContainerLidLabel', '-unattended', '-nop4', '-nosplash', '-nullrhi', '-nosound')

Write-Host '4/8 Cooking exact replacement package'
Invoke-LoggedProcess -FilePath $editor -Name 'cook' -Arguments @(
    ('"{0}"' -f $project), '-run=cook', '-targetplatform=Windows', '-SkipZenStore',
    ('-Package={0}' -f $sourcePackage), '-CookSinglePackageNoRefs', '-unattended', '-nop4', '-nosplash', '-nullrhi',
    '-ddc=NoZenLocalFallback', ('-LocalDataCachePath="{0}"' -f (Join-Path $modRoot 'DerivedDataCache')))

Write-Host '5/8 Extracting and relocating the exact stock parent'
$work = Join-Path $releaseRoot 'work'
$extractRoot = Join-Path $work 'stock-extraction'
$extraction = & $extractor -Filter $assetFilter -GameRoot $GameRoot -RetocEngineVersion $retocVersion -OutputRoot $extractRoot
$originalRoot = $extraction.outputPath
$originalDir = Join-Path $originalRoot 'Voyage\Content\Blueprints\Interactives\Containers'
$sourceUasset = Join-Path $originalDir "$assetName.uasset"
$sourceUexp = Join-Path $originalDir "$assetName.uexp"
$scriptObjects = Join-Path $originalRoot 'scriptobjects.bin'
foreach ($required in @($sourceUasset, $sourceUexp, $scriptObjects)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Extraction output missing: $required" }
}

$stage = Join-Path $work 'stage'
$childStage = Join-Path $stage 'Voyage\Content\Blueprints\Interactives\Containers'
$parentStage = Join-Path $stage 'Voyage\Content\Mods\ContainerLidLabel\StockOrigin'
$packageRoot = Join-Path $releaseRoot 'package'
New-Item -ItemType Directory -Path $childStage, $parentStage, $packageRoot -Force | Out-Null
$cookedDir = Join-Path $modRoot 'Saved\Cooked\Windows\Voyage\Content\Blueprints\Interactives\Containers'
Copy-Item -LiteralPath (Join-Path $cookedDir "$assetName.uasset") -Destination $childStage
Copy-Item -LiteralPath (Join-Path $cookedDir "$assetName.uexp") -Destination $childStage

$bytes = [IO.File]::ReadAllBytes($sourceUasset)
$originalBytes = [byte[]]$bytes.Clone()
$find = [Text.Encoding]::ASCII.GetBytes($sourcePackage)
$replace = [Text.Encoding]::ASCII.GetBytes($relocatedPackage)
$matches = @(Find-Offsets -Bytes $bytes -Sequence $find)
if ($matches.Count -ne 2 -or @(Find-Offsets -Bytes $bytes -Sequence $replace).Count -ne 0) {
    throw "Stock relocation precondition failed: source=$($matches.Count), destination=$(@(Find-Offsets -Bytes $bytes -Sequence $replace).Count)"
}
foreach ($offset in $matches) { [Array]::Copy($replace, 0, $bytes, $offset, $replace.Length) }
if ($bytes.Length -ne $originalBytes.Length -or
    @(Find-Offsets -Bytes $bytes -Sequence $find).Count -ne 0 -or
    @(Find-Offsets -Bytes $bytes -Sequence $replace).Count -ne 2) {
    throw 'Stock relocation postcondition failed.'
}
$relocatedUasset = Join-Path $parentStage "$assetName.uasset"
$relocatedUexp = Join-Path $parentStage "$assetName.uexp"
[IO.File]::WriteAllBytes($relocatedUasset, $bytes)
Copy-Item -LiteralPath $sourceUexp -Destination $relocatedUexp
if ((Get-FileHash $sourceUexp).Hash -cne (Get-FileHash $relocatedUexp).Hash) { throw 'Relocation changed the stock export payload.' }
Copy-Item -LiteralPath $scriptObjects -Destination (Join-Path $stage 'scriptobjects.bin')

Write-Host '6/8 Packaging and verifying exact two-package container'
$utoc = Join-Path $packageRoot "$containerName.utoc"
& $retoc to-zen --version $retocVersion $stage $utoc
if ($LASTEXITCODE -ne 0) { throw "retoc to-zen failed with exit code $LASTEXITCODE" }
$expectedList = Join-Path $releaseRoot 'expected-packages.txt'
[IO.File]::WriteAllLines($expectedList, @(
    '../../../Voyage/Content/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large.uasset',
    '../../../Voyage/Content/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large.uasset'))
$containerCheckOutput = @(& $containerTester -Container $utoc -ExpectedPackageList $expectedList -OutputRoot (Join-Path $releaseRoot 'container-check'))
$containerCheck = @($containerCheckOutput | Where-Object { $null -ne $_.PSObject.Properties['status'] }) | Select-Object -Last 1
if ($null -eq $containerCheck) { throw 'Container validator did not return a structured result.' }
if ($containerCheck.status -cne 'passed' -or $containerCheck.packageCount -ne 2 -or $containerCheck.packageSetMatches -ne $true) {
    throw "Container validation failed. Report: $($containerCheck.reportPath)"
}

Write-Host '7/8 Creating ZIP and validated release manifest'
$archive = Join-Path $releaseRoot "ContainerLidLabel-$Version.zip"
Compress-Archive -LiteralPath @(
    (Join-Path $packageRoot "$containerName.pak"),
    (Join-Path $packageRoot "$containerName.ucas"),
    $utoc) -DestinationPath $archive -CompressionLevel Optimal
$manifestArgs = @{
    ReleaseRoot = $releaseRoot
    Mod = 'ContainerLidLabel'
    Version = $Version
    ModVersion = $Version
    Container = $utoc
    Archive = $archive
    SourcePath = @($modRoot)
    GameRoot = $GameRoot
    AsJson = $true
}
if ($AllowDirtySource) { $manifestArgs.AllowDirtySource = $true }
& $manifestProducer @manifestArgs | Out-Null
$releaseManifestPath = Join-Path $releaseRoot 'release-manifest.json'
if (-not (Test-Path -LiteralPath $releaseManifestPath -PathType Leaf)) {
    throw "Validated release manifest was not produced: $releaseManifestPath"
}

Write-Host '8/8 Recording build identity'
$payload = @(
    (Join-Path $packageRoot "$containerName.pak"),
    (Join-Path $packageRoot "$containerName.ucas"),
    $utoc,
    $archive,
    $releaseManifestPath)
$sourceFiles = @(
    Get-Item -LiteralPath (Join-Path $modRoot '.gitignore')
    Get-Item -LiteralPath (Join-Path $modRoot 'Voyage.uproject')
    Get-Item -LiteralPath (Join-Path $modRoot 'Build-ContainerLidLabel.ps1')
    Get-ChildItem -LiteralPath (Join-Path $modRoot 'Source') -Recurse -File
    Get-ChildItem -LiteralPath $modRoot -File -Filter '*.md'
) | Sort-Object FullName -Unique
$buildManifest = [ordered]@{
    schemaVersion = 1
    mod = 'ContainerLidLabel'
    version = $Version
    createdUtc = [DateTime]::UtcNow.ToString('o')
    steamBuildId = $expectedBuild
    executableSha256 = $expectedExecutableHash
    engine = '5.8.2'
    retocEngineVersion = $retocVersion
    runtimePackages = [IO.File]::ReadAllLines($expectedList)
    relocation = [ordered]@{
        source = $sourcePackage
        destination = $relocatedPackage
        replacementCount = 2
        sourceLength = $originalBytes.Length
        relocatedLength = $bytes.Length
        sourceUexpSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourceUexp).Hash
        relocatedUexpSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $relocatedUexp).Hash
    }
    containerCheckReport = $containerCheck.reportPath
    releaseManifest = $releaseManifestPath
    sourceFiles = @($sourceFiles | ForEach-Object {
        [ordered]@{
            path = $_.FullName.Substring($repoRoot.Length + 1).Replace('\', '/')
            length = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    })
    files = @($payload | ForEach-Object {
        $item = Get-Item -LiteralPath $_
        [ordered]@{ path = $item.FullName; length = $item.Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_).Hash }
    })
}
$buildManifestPath = Join-Path $releaseRoot 'build-manifest.json'
[IO.File]::WriteAllText($buildManifestPath, ($buildManifest | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
[pscustomobject]@{
    status = 'succeeded'
    releaseRoot = $releaseRoot
    releaseManifest = $releaseManifestPath
    buildManifest = $buildManifestPath
    container = $utoc
    archive = $archive
    packageCount = $containerCheck.packageCount
}
