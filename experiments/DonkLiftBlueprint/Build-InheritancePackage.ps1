param(
    [string]$CookedRoot = "$PSScriptRoot\Saved\Cooked\Windows\Voyage",

    [Parameter(Mandatory = $true)]
    [string]$OriginalForkliftDirectory,

    [Parameter(Mandatory = $true)]
    [string]$ScriptObjects,

    [string]$Retoc = "R:\Codex\ToolCache\rust-retoc-master\source\target\release\retoc.exe",

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$ContainerName = 'DonkLiftKeyboardControl_P'
)

$ErrorActionPreference = 'Stop'

$originalPackage = '/Game/Blueprints/Vehicles/BP_Forklift_Possesable'
$renamedPackage = '/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original'
if ($originalPackage.Length -ne $renamedPackage.Length) {
    throw 'The original and renamed package paths must have identical byte lengths.'
}

$cooked = (Resolve-Path -LiteralPath $CookedRoot).Path
$originalDirectory = (Resolve-Path -LiteralPath $OriginalForkliftDirectory).Path
$scriptObjectsPath = (Resolve-Path -LiteralPath $ScriptObjects).Path
$retocPath = (Resolve-Path -LiteralPath $Retoc).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$stage = Join-Path $output 'stage'
$package = Join-Path $output 'package'
$childStage = Join-Path $stage 'Voyage\Content\Blueprints\Vehicles'
$parentStage = Join-Path $stage 'Voyage\Content\Mods\DonkLiftKeyboard'
$helperStage = Join-Path $stage 'Voyage\Content\Mods\DonkLiftKeyboardControl'
New-Item -ItemType Directory -Path $childStage, $parentStage, $helperStage, $package -Force | Out-Null

$childSource = Join-Path $cooked 'Content\Blueprints\Vehicles'
$helperSource = Join-Path $cooked 'Content\Mods\DonkLiftKeyboardControl'
foreach ($assetName in @('BP_Forklift_Possesable.uasset', 'BP_Forklift_Possesable.uexp')) {
    Copy-Item -LiteralPath (Join-Path $childSource $assetName) -Destination $childStage
}
foreach ($assetName in @('ModActor.uasset', 'ModActor.uexp')) {
    Copy-Item -LiteralPath (Join-Path $helperSource $assetName) -Destination $helperStage
}

$originalUasset = Join-Path $originalDirectory 'BP_Forklift_Possesable.uasset'
$originalUexp = Join-Path $originalDirectory 'BP_Forklift_Possesable.uexp'
if (-not (Test-Path -LiteralPath $originalUasset -PathType Leaf) -or
    -not (Test-Path -LiteralPath $originalUexp -PathType Leaf)) {
    throw "Original cooked forklift asset is incomplete: $originalDirectory"
}

$bytes = [IO.File]::ReadAllBytes($originalUasset)
$find = [Text.Encoding]::ASCII.GetBytes($originalPackage)
$replace = [Text.Encoding]::ASCII.GetBytes($renamedPackage)
$matches = [Collections.Generic.List[int]]::new()
for ($offset = 0; $offset -le $bytes.Length - $find.Length; $offset++) {
    $isMatch = $true
    for ($index = 0; $index -lt $find.Length; $index++) {
        if ($bytes[$offset + $index] -ne $find[$index]) {
            $isMatch = $false
            break
        }
    }
    if ($isMatch) {
        $matches.Add($offset)
        $offset += $find.Length - 1
    }
}
if ($matches.Count -ne 2) {
    throw "Expected exactly two embedded original package paths; found $($matches.Count)."
}
foreach ($offset in $matches) {
    [Array]::Copy($replace, 0, $bytes, $offset, $replace.Length)
}
[IO.File]::WriteAllBytes((Join-Path $parentStage 'BP_Forklift_Original.uasset'), $bytes)
Copy-Item -LiteralPath $originalUexp -Destination (Join-Path $parentStage 'BP_Forklift_Original.uexp')
Copy-Item -LiteralPath $scriptObjectsPath -Destination (Join-Path $stage 'scriptobjects.bin')

$utoc = Join-Path $package ($ContainerName + '.utoc')
& $retocPath to-zen --version UE5_7 $stage $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc to-zen failed with exit code $LASTEXITCODE"
}
& $retocPath verify $utoc
if ($LASTEXITCODE -ne 0) {
    throw "retoc verify failed with exit code $LASTEXITCODE"
}

$outputs = @(
    Join-Path $package ($ContainerName + '.pak')
    Join-Path $package ($ContainerName + '.ucas')
    $utoc
)
foreach ($path in $outputs) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Expected package file was not produced: $path"
    }
}

Write-Host 'Autonomous DonkLift inheritance package built successfully.'
Get-FileHash -Algorithm SHA256 -LiteralPath $outputs |
    Select-Object Path, Hash |
    Format-Table -AutoSize
