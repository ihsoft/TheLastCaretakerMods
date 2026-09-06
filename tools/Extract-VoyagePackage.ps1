param(
    [Parameter(Mandatory = $true)]
    [string]$Filter,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc,

    [string]$RetocEngineVersion = 'UE5_8',

    [string]$OutputRoot,

    [ValidateSet('Game', 'Mod')]
    [string]$Source = 'Game',

    [string]$ModContainer,

    [switch]$AllowAdditionalContainers
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $PSScriptRoot '..\.tools\bin\retoc.exe'
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $PSScriptRoot '..\artifacts\extracted'
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$retocSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $retocPath).Hash
$paks = Join-Path $root 'Voyage\Content\Paks'
$exe = Join-Path $root 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
if (-not (Test-Path -LiteralPath $paks -PathType Container)) {
    throw "Voyage Paks directory not found: $paks"
}
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Voyage executable not found: $exe"
}

function Test-StockContainer([string]$Name) {
    $Name -ieq 'global.utoc' -or $Name -match '^(?i:pakchunk\d+(?:optional)?-Windows\.utoc)$'
}
if ($Source -eq 'Game' -and $ModContainer) { throw '-ModContainer requires -Source Mod.' }
if ($Source -eq 'Mod' -and -not $ModContainer) { throw '-Source Mod requires one exact -ModContainer.' }
if ($AllowAdditionalContainers -and ($Source -ne 'Game' -or $PSBoundParameters.ContainsKey('Source'))) {
    throw 'Legacy -AllowAdditionalContainers cannot be combined with an explicit source.'
}
$allContainers = @(Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc')
$stock = @($allContainers | Where-Object { Test-StockContainer $_.Name } | Sort-Object Name -Descending)
$additionalContainers = @($allContainers | Where-Object { -not (Test-StockContainer $_.Name) })
$global = @($stock | Where-Object Name -ieq 'global.utoc')
if ($global.Count -ne 1 -or $stock.Count -lt 2) { throw 'Stock global and package containers are required.' }
$selected = @($global[0])
$modPackages = @()
if ($Source -eq 'Mod') {
    $mod = Get-Item -LiteralPath $ModContainer
    if ($mod.PSIsContainer -or $mod.Extension -ine '.utoc' -or (Test-StockContainer $mod.Name)) {
        throw 'Select a mod .utoc, not a stock container or directory.'
    }
    $selected += $mod
    # Ownership is independent of the parser's combined-view precedence.
    $inventory = & (Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1') `
        -GameRoot $root -ListPackages -Source Mod -ModContainer $mod.FullName
    $modPackages = @(Get-Content -LiteralPath $inventory.packageListPath)
    if (@($modPackages | Where-Object { $_.IndexOf($Filter, [StringComparison]::OrdinalIgnoreCase) -ge 0 }).Count -eq 0) {
        throw "Filter matches no package owned by the selected mod: $Filter"
    }
}
$selected += @($stock | Where-Object Name -ine 'global.utoc')
$helpText = (& $retocPath to-legacy --help) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'retoc to-legacy capability check failed.' }
$supportsSelection = $helpText.Contains('--include-container')
if ($AllowAdditionalContainers) {
    Write-Warning 'Legacy combined installed view requested. Prefer Game or one explicit Mod source.'
    $selected = $allContainers
}
elseif (-not $supportsSelection -and ($Source -eq 'Mod' -or $additionalContainers.Count -gt 0)) {
    throw 'This retoc cannot isolate containers. A validated retoc with --include-container is required; no files were extracted. Do not remove installed mods to work around this gate.'
}
$selectedRecords = @($selected | ForEach-Object {
    [ordered]@{ path = $_.FullName; length = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
})

$version = (Get-Item -LiteralPath $exe).VersionInfo.ProductVersion
$exeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash
$steamApps = Split-Path -Parent (Split-Path -Parent $root)
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
$steamBuildId = 'unknown'
if (Test-Path -LiteralPath $steamManifest -PathType Leaf) {
    $buildMatch = [regex]::Match([IO.File]::ReadAllText($steamManifest), '"buildid"\s+"(?<id>\d+)"')
    if ($buildMatch.Success) {
        $steamBuildId = $buildMatch.Groups['id'].Value
    }
}
$buildId = "steam-$steamBuildId-$($exeHash.Substring(0, 12))"
$filterSafe = ($Filter -replace '[^A-Za-z0-9._-]', '_').Trim('_')
if (-not $filterSafe) {
    $filterSafe = 'query'
}
$output = [IO.Path]::GetFullPath((Join-Path $OutputRoot "$buildId\$filterSafe"))
if (Test-Path -LiteralPath $output) {
    throw "Output already exists; use a new query or remove the local artifact explicitly: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

$arguments = @('to-legacy', '--version', $RetocEngineVersion, '--filter', $Filter, '--no-shaders')
if ($supportsSelection -and -not $AllowAdditionalContainers) {
    foreach ($container in $selected) { $arguments += @('--include-container', $container.FullName) }
}
$arguments += @($paks, $output)
$logPath = Join-Path $output 'retoc.log'
$previousPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $retocPath @arguments *> $logPath
    $retocExit = $LASTEXITCODE
}
finally { $ErrorActionPreference = $previousPreference }
if ($retocExit -ne 0) {
    throw "retoc to-legacy failed with exit code $retocExit. Log: $logPath"
}
$extractedAssets = @(Get-ChildItem -LiteralPath $output -File -Recurse -Filter '*.uasset')
if ($extractedAssets.Count -eq 0) {
    throw "Filter extracted no assets: $Filter"
}
if ($Source -eq 'Mod') {
    foreach ($asset in $extractedAssets) {
        $relative = $asset.FullName.Substring($output.Length + 1).Replace('\', '/')
        if ($modPackages -inotcontains $relative) { throw "Extracted package is not owned by the selected mod: $relative" }
    }
}
foreach ($record in $selectedRecords) {
    if ((Get-FileHash -LiteralPath $record.path -Algorithm SHA256).Hash -cne $record.sha256) {
        throw "Container changed during extraction: $($record.path)"
    }
}

$manifest = [ordered]@{
    schemaVersion = 2
    steamAppId = '1783560'
    steamBuildId = $steamBuildId
    gameVersion = $version
    executableSha256 = $exeHash
    retocEngineVersion = $RetocEngineVersion
    retocPath = $retocPath
    retocSha256 = $retocSha256
    filter = $Filter
    source = $(if ($AllowAdditionalContainers) { 'InstalledLegacy' } else { $Source })
    selectedContainers = $selectedRecords
    containerSelection = $(if ($supportsSelection -and -not $AllowAdditionalContainers) { 'explicit-first-wins' } else { 'directory' })
    assets = @($extractedAssets | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($output.Length + 1).Replace('\', '/'); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    allowAdditionalContainers = [bool]$AllowAdditionalContainers
    additionalContainers = @(
        $additionalContainers | ForEach-Object { $_.Name } | Sort-Object
    )
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'extraction-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 6) + [Environment]::NewLine))

Write-Host "Extracted current-game package data to: $output"
[pscustomobject]@{ source = $manifest.source; outputPath = $output; manifestPath = (Join-Path $output 'extraction-manifest.json'); assetCount = $extractedAssets.Count }
