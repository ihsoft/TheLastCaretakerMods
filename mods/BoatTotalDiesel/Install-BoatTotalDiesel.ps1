# HAND-WRITTEN INSTALL TOOL: installs only the BoatTotalDiesel container while Voyage
# is closed and records enough evidence for exact hash-guarded removal.

param(
    [Parameter(Mandatory = $true)]
    [string]$PackageRoot,

    [Parameter(Mandatory = $true)]
    [string]$EvidenceRoot,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$ContainerName = 'BoatTotalDiesel_P'
)

$ErrorActionPreference = 'Stop'

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    $processes = ($running | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ', '
    throw "Voyage must be closed before installing BoatTotalDiesel: $processes"
}

$package = (Resolve-Path -LiteralPath $PackageRoot).Path
$buildManifestPath = Join-Path $package 'build-manifest.json'
if (-not (Test-Path -LiteralPath $buildManifestPath -PathType Leaf)) {
    throw "BoatTotalDiesel build manifest is missing: $buildManifestPath"
}
$buildManifest = Get-Content -LiteralPath $buildManifestPath -Raw | ConvertFrom-Json
if ([string]$buildManifest.kind -cne 'BoatTotalDiesel mod' -or
    [string]$buildManifest.mountEvent -cne 'UUserWidget::PreConstruct override' -or
    [string]$buildManifest.initialText -cne '-- L' -or
    [string]$buildManifest.stockPetrolVisibility -cne 'Collapsed' -or
    [string]$buildManifest.replacementPackage -cne
        '/Game/Mods/BoatTotalDiesel/WBP_BoatTotalDieselValue' -or
    [string]$buildManifest.aggregationResource -cne 'Diesel' -or
    [int]$buildManifest.aggregationResourceValue -ne 16 -or
    [string]$buildManifest.engineVersion -cne '5.8.1' -or
    [string]$buildManifest.editorEngineVersion -cne '5.8.2' -or
    [string]$buildManifest.retocCompatibilityVersion -cne 'UE5_7' -or
    [string]$buildManifest.legacyImportPackageNameLayout -cne
        'SerializedForFilteredCookedPackages' -or
    [string]$buildManifest.retocSha256 -notmatch '^[0-9A-F]{64}$' -or
    [string]$buildManifest.cookStorage -cne 'LooseCookedPackageWriter' -or
    [string]$buildManifest.steamBuildId -cne '24990438' -or
    [string]$buildManifest.executableSha256 -cne
        'D9BF4C9624C60615198E62C87DA7792A9888AB02F7905AAAF1C9B02C7A9E524F') {
    throw 'Only the statically checked BoatTotalDiesel build is allowed.'
}
$root = (Resolve-Path -LiteralPath $GameRoot).Path
$exe = (Resolve-Path -LiteralPath (
    Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe')).Path
$actualExecutableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash
$steamApps = Split-Path -Parent (Split-Path -Parent $root)
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
if (-not (Test-Path -LiteralPath $steamManifest -PathType Leaf)) {
    throw "Steam manifest not found: $steamManifest"
}
$buildIdMatch = [regex]::Match(
    [IO.File]::ReadAllText($steamManifest),
    '"buildid"\s+"(?<id>\d+)"')
if (-not $buildIdMatch.Success) {
    throw "Steam build ID not found in: $steamManifest"
}
$actualSteamBuildId = $buildIdMatch.Groups['id'].Value
if ($actualSteamBuildId -cne [string]$buildManifest.steamBuildId -or
    $actualExecutableSha256 -cne [string]$buildManifest.executableSha256) {
    throw 'Installed Voyage fingerprint does not match the BoatTotalDiesel build.'
}

$paks = (Resolve-Path -LiteralPath (Join-Path $root 'Voyage\Content\Paks')).Path
$evidence = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $evidence) {
    throw "EvidenceRoot already exists: $evidence"
}
[IO.Directory]::CreateDirectory($evidence) | Out-Null
$backup = Join-Path $evidence 'previous-container'

$extensions = @('.pak', '.ucas', '.utoc')
$records = @()
foreach ($extension in $extensions) {
    $name = $ContainerName + $extension
    $source = Join-Path $package $name
    $destination = Join-Path $paks $name
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "BoatTotalDiesel package file is missing: $source"
    }
    $manifestRecord = @($buildManifest.containerFiles | Where-Object {
        [IO.Path]::GetFileName([string]$_.path) -ceq $name
    })
    if ($manifestRecord.Count -ne 1) {
        throw "Build manifest does not contain exactly one record for $name"
    }
    $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
    if ($sourceHash -cne [string]$manifestRecord[0].sha256) {
        throw "BoatTotalDiesel package hash does not match its build manifest: $source"
    }

    $hadExisting = Test-Path -LiteralPath $destination -PathType Leaf
    $previousHash = $null
    $backupPath = $null
    if ($hadExisting) {
        [IO.Directory]::CreateDirectory($backup) | Out-Null
        $previousHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
        $backupPath = Join-Path $backup $name
        Copy-Item -LiteralPath $destination -Destination $backupPath
    }

    Copy-Item -LiteralPath $source -Destination $destination -Force
    $installedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
    if ($installedHash -cne $sourceHash) {
        throw "Installed BoatTotalDiesel hash mismatch: $destination"
    }
    $records += [pscustomobject][ordered]@{
        name = $name
        sourceSha256 = $sourceHash
        installedSha256 = $installedHash
        hadExisting = $hadExisting
        previousSha256 = $previousHash
        backupPath = $backupPath
    }
}

$manifest = [ordered]@{
    kind = 'BoatTotalDiesel installation'
    gameRoot = $root
    steamBuildId = $actualSteamBuildId
    executableSha256 = $actualExecutableSha256
    paksDirectory = $paks
    containerName = $ContainerName
    installedAtUtc = [DateTime]::UtcNow.ToString('o')
    files = $records
}
$manifestPath = Join-Path $evidence 'install-manifest.json'
[IO.File]::WriteAllText(
    $manifestPath,
    (($manifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine))

Write-Host "BoatTotalDiesel installed. Removal manifest: $manifestPath"
$records | Select-Object name, installedSha256, hadExisting | Format-Table -AutoSize
