param(
    [string]$CookedRoot = "$PSScriptRoot\Saved\CookedModActor\Voyage",
    [string]$ScriptObjects = "$PSScriptRoot\..\..\artifacts\raw\iostore-throttle\persistent-throttle-stage-v2\scriptobjects.bin",
    [string]$Retoc = "R:\Codex\ToolCache\rust-retoc-master\source\target\release\retoc.exe",
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$assetRelativeDirectory = 'Voyage\Content\Mods\DonkLiftKeyboardControl'
$assetNames = @('ModActor.uasset', 'ModActor.uexp')
$stageRoot = Join-Path $OutputRoot 'stage'
$packageRoot = Join-Path $OutputRoot 'package'
$outputUtoc = Join-Path $packageRoot 'DonkLiftKeyboardControl_P.utoc'

if (Test-Path -LiteralPath $OutputRoot) {
    throw "OutputRoot already exists: $OutputRoot"
}
if (-not (Test-Path -LiteralPath $Retoc -PathType Leaf)) {
    throw "retoc was not found: $Retoc"
}
if (-not (Test-Path -LiteralPath $ScriptObjects -PathType Leaf)) {
    throw "Target-game scriptobjects.bin was not found: $ScriptObjects"
}

$sourceAssetDirectory = Join-Path $CookedRoot 'Content\Mods\DonkLiftKeyboardControl'
foreach ($assetName in $assetNames) {
    $sourceAsset = Join-Path $sourceAssetDirectory $assetName
    if (-not (Test-Path -LiteralPath $sourceAsset -PathType Leaf)) {
        throw "Cooked asset was not found: $sourceAsset"
    }
}

New-Item -ItemType Directory -Path (Join-Path $stageRoot $assetRelativeDirectory) -Force | Out-Null
New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null
Copy-Item -LiteralPath $ScriptObjects -Destination (Join-Path $stageRoot 'scriptobjects.bin')
foreach ($assetName in $assetNames) {
    Copy-Item -LiteralPath (Join-Path $sourceAssetDirectory $assetName) -Destination (Join-Path (Join-Path $stageRoot $assetRelativeDirectory) $assetName)
}

& $Retoc to-zen --version UE5_7 $stageRoot $outputUtoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc failed with exit code $LASTEXITCODE"
}

$expectedOutputs = @(
    $outputUtoc,
    [System.IO.Path]::ChangeExtension($outputUtoc, '.ucas'),
    [System.IO.Path]::ChangeExtension($outputUtoc, '.pak')
)
foreach ($output in $expectedOutputs) {
    if (-not (Test-Path -LiteralPath $output -PathType Leaf)) {
        throw "Expected package file was not created: $output"
    }
}

$expectedOutputs | ForEach-Object {
    $file = Get-Item -LiteralPath $_
    [pscustomobject]@{
        Name = $file.Name
        Length = $file.Length
        SHA256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash
    }
}
