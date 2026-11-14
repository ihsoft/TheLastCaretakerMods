# INTERNAL BUILD STAGE: cooks the maintained production packages in narrow
# Unreal invocations. It contains no extracted game data; its output is generated
# and not committed.

param(
    [Parameter(Mandatory = $true)]
    [string]$UnrealEditor,

    [Parameter(Mandatory = $true)][string]$Project,

    [Parameter(Mandatory = $true)][string]$LocalDataCachePath,

    [string]$LogPath,

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
    '/Game/Data/Assets/Modules/DA_Item_Module_GyroCopter',
    '/Game/Mods/StableGyro/ModActor',
    '/Game/Mods/StableGyro/PitchInputActor',
    '/Game/Mods/StableGyro/Settings/BP_IniFileParser',
    '/Game/Mods/StableGyro/IAV_GyroPitchReset',
    '/Game/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCompassHUD',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCameraCompass',
    '/Game/Mods/StableGyro/UI/WBP_StableGyroCompassDisplay',
    '/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard',
    '/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY'
)
$compassDisplay = '/Game/Mods/StableGyro/UI/WBP_StableGyroCompassDisplay'
$ordinaryPackages = @($packages | Where-Object { $_ -ne $compassDisplay })
$requiredCookedExtensions = @('.uasset', '.uexp')
$optionalCookedExtensions = @('.ubulk', '.uptnl', '.m.ubulk')
$cookedExtensions = @($requiredCookedExtensions + $optionalCookedExtensions)

foreach ($package in $packages) {
    $relativeAsset = $package.Substring('/Game/'.Length)
    $sourceBase = Join-Path $cookedContent $relativeAsset
    foreach ($extension in $cookedExtensions) {
        $staleSource = $sourceBase + $extension
        if (Test-Path -LiteralPath $staleSource -PathType Leaf) {
            Remove-Item -LiteralPath $staleSource -Force
        }
    }
}

Write-Host "Cooking $($packages.Count) StableGyro packages"
$arguments = @(
    ('"{0}"' -f $projectPath),
    '-run=cook',
    '-targetplatform=Windows',
    '-unversioned',
    '-SkipZenStore',
    ('-Package={0}' -f ($ordinaryPackages -join '+')),
    '-CookSinglePackageNoRefs',
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nullrhi',
    '-ddc=NoZenLocalFallback',
    ('-LocalDataCachePath="{0}"' -f $ddc)
)
if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
    $log = [IO.Path]::GetFullPath($LogPath)
    New-Item -ItemType Directory -Path (Split-Path $log -Parent) -Force | Out-Null
    $arguments += ('-abslog="{0}"' -f $log)
}
$process = Start-Process `
    -FilePath $editor `
    -ArgumentList $arguments `
    -Wait `
    -PassThru `
    -WindowStyle Hidden
if ($process.ExitCode -ne 0) {
    throw "Cook failed with exit code $($process.ExitCode)."
}
$badMeshDescriptionErrors = @()
if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
    $badMeshDescriptionErrors = @(
        Select-String -LiteralPath $log -SimpleMatch 'LogStaticMesh: Error: Bad MeshDescription' `
            -ErrorAction SilentlyContinue
    )
}
if ($badMeshDescriptionErrors.Count -ne 0) {
    throw "Cook reported invalid reference StaticMesh packages despite exit code 0: $($badMeshDescriptionErrors.Line -join '; ')"
}

function Copy-CookedPackageFiles {
    param([string[]]$PackageNames)
    foreach ($package in $PackageNames) {
        $relativeAsset = $package.Substring('/Game/'.Length)
        $sourceBase = Join-Path $cookedContent $relativeAsset
        $destinationBase = Join-Path $outputContent $relativeAsset
        New-Item -ItemType Directory -Path (Split-Path $destinationBase -Parent) -Force | Out-Null
        foreach ($extension in $requiredCookedExtensions) {
            $source = $sourceBase + $extension
            if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
                throw "Expected cooked file was not produced: $source"
            }
            Copy-Item -LiteralPath $source -Destination ($destinationBase + $extension)
        }
        foreach ($extension in $optionalCookedExtensions) {
            $source = $sourceBase + $extension
            if (Test-Path -LiteralPath $source -PathType Leaf) {
                Copy-Item -LiteralPath $source -Destination ($destinationBase + $extension)
            }
        }
    }
}

# A subsequent cook cleans the project cook directory; stage each batch first.
Copy-CookedPackageFiles -PackageNames $ordinaryPackages

if ($packages -contains $compassDisplay) {
    # This child writes an inherited UUserWidget property through partial native
    # editor mirrors. Named property tags preserve TickFrequency's real identity;
    # an unversioned field index would resolve against the game's larger schema.
    $displayArguments = @($arguments | Where-Object { $_ -ne '-unversioned' -and $_ -notlike '-Package=*' -and $_ -notlike '-abslog=*' })
    $displayArguments += ('-Package={0}' -f $compassDisplay)
    if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
        $displayArguments += ('-abslog="{0}"' -f ($log + '.compass-display.log'))
    }
    $displayProcess = Start-Process -FilePath $editor -ArgumentList $displayArguments -Wait -PassThru -WindowStyle Hidden
    if ($displayProcess.ExitCode -ne 0) {
        throw "Compass display cook failed with exit code $($displayProcess.ExitCode)."
    }
}


if ($packages -contains $compassDisplay) {
    Copy-CookedPackageFiles -PackageNames @($compassDisplay)
}

$files = @(Get-ChildItem -LiteralPath $outputContent -Recurse -File)
$minimumFileCount = $packages.Count * $requiredCookedExtensions.Count
if ($files.Count -lt $minimumFileCount) {
    throw "Expected at least $minimumFileCount staged cooked files; found $($files.Count)."
}
$unexpectedFiles = @($files | Where-Object {
    $name = $_.Name
    -not ($cookedExtensions | Where-Object { $name.EndsWith($_, [StringComparison]::OrdinalIgnoreCase) })
})
if ($unexpectedFiles.Count -ne 0) {
    throw "Unexpected cooked companion files were staged: $($unexpectedFiles.FullName -join ', ')"
}

Write-Host "Cooked StableGyro production assets: $output"
$files | Select-Object FullName, Length | Format-Table -AutoSize
