# HAND-WRITTEN BUILD TOOL: performs the five narrow Unreal cooks for this mod.
# It contains no extracted game data; its output is generated and not committed.

param(
    [Parameter(Mandatory = $true)]
    [string]$UnrealEditor,

    [string]$Project = "$PSScriptRoot\Voyage.uproject",

    [string]$LocalDataCachePath = "$PSScriptRoot\DerivedDataCache",

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$editor = (Resolve-Path -LiteralPath $UnrealEditor).Path
$projectPath = (Resolve-Path -LiteralPath $Project).Path
$ddc = [IO.Path]::GetFullPath($LocalDataCachePath)
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$cookedContent = Join-Path (Split-Path $projectPath -Parent) 'Saved\Cooked\Windows\Voyage\Content'
$outputContent = Join-Path $output 'Content'
New-Item -ItemType Directory -Path $outputContent -Force | Out-Null

$packages = @(
    '/Game/Mods/DonkLiftKeyboardControl/ModActor',
    '/Game/Mods/DonkLiftKeyboardControl/IAV_DonkLiftBrake',
    '/Game/Mods/DonkLiftKeyboardControl/IAV_DonkLiftCenterSteering',
    '/Game/Game/Input/Vehicle/IMC_Forklift_Keyboard',
    '/Game/Blueprints/Vehicles/BP_Forklift_Possesable'
)

foreach ($package in $packages) {
    Write-Host "Cooking $package"
    $arguments = @(
        ('"{0}"' -f $projectPath),
        '-run=cook',
        '-targetplatform=Windows',
        '-unversioned',
        "-Package=$package",
        '-CookSinglePackageNoRefs',
        '-unattended',
        '-nop4',
        '-nosplash',
        '-nullrhi',
        '-ddc=NoZenLocalFallback',
        ('-LocalDataCachePath="{0}"' -f $ddc)
    )
    $process = Start-Process `
        -FilePath $editor `
        -ArgumentList $arguments `
        -Wait `
        -PassThru `
        -WindowStyle Hidden
    if ($process.ExitCode -ne 0) {
        throw "Cook failed for $package with exit code $($process.ExitCode)."
    }

    $relativeAsset = $package.Substring('/Game/'.Length)
    $sourceBase = Join-Path $cookedContent $relativeAsset
    $destinationBase = Join-Path $outputContent $relativeAsset
    New-Item -ItemType Directory -Path (Split-Path $destinationBase -Parent) -Force | Out-Null
    foreach ($extension in @('.uasset', '.uexp')) {
        $source = $sourceBase + $extension
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Expected cooked file was not produced: $source"
        }
        Copy-Item -LiteralPath $source -Destination ($destinationBase + $extension)
    }
}

$files = @(Get-ChildItem -LiteralPath $outputContent -Recurse -File)
if ($files.Count -ne $packages.Count * 2) {
    throw "Expected 10 staged cooked files; found $($files.Count)."
}

Write-Host "Cooked DonkLift production assets: $output"
Get-FileHash -Algorithm SHA256 -LiteralPath $files.FullName |
    Select-Object Path, Hash |
    Format-Table -AutoSize
