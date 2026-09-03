# HAND-WRITTEN BUILD TOOL: extracts the base-game asset required by the
# DonkLift package while temporarily disabling and hash-restoring every
# additional IoStore container that could shadow the original.

param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
$retocCompatibilityVersion = 'UE5_7'
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($Retoc)) {
    $Retoc = Join-Path $repoRoot '.tools\bin\retoc.exe'
}

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    $processes = ($running | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ', '
    throw "Voyage must be closed before preparing original assets: $processes"
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$paks = Join-Path $root 'Voyage\Content\Paks'
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$additionalContainers = @(
    Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc' |
        Where-Object { $_.Name -ne 'global.utoc' -and $_.Name -notlike 'pakchunk*.utoc' }
)
$leftovers = @(Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc.disabled-for-donklift-extraction-*')
if ($leftovers.Count -gt 0) {
    $names = ($leftovers.Name | Sort-Object) -join ', '
    throw "Unrestored UTOC files from an earlier interrupted extraction require manual recovery: $names"
}

$token = [Guid]::NewGuid().ToString('N')
$containerMoves = @(
    $additionalContainers | ForEach-Object {
        [pscustomobject]@{
            Original = $_.FullName
            Disabled = $_.FullName + ".disabled-for-donklift-extraction-$token"
            Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
            Moved = $false
        }
    }
)

$extractor = Join-Path $PSScriptRoot '..\..\tools\Extract-VoyagePackage.ps1'
try {
    foreach ($entry in $containerMoves) {
        if (Test-Path -LiteralPath $entry.Disabled) {
            throw "Temporary UTOC path already exists: $($entry.Disabled)"
        }
        Move-Item -LiteralPath $entry.Original -Destination $entry.Disabled
        $entry.Moved = $true
    }

    & $extractor `
        -Filter 'Vehicles/BP_Forklift_Possesable' `
        -GameRoot $root `
        -Retoc $Retoc `
        -RetocEngineVersion $retocCompatibilityVersion `
        -OutputRoot $output
}
finally {
    $restoreErrors = @()
    foreach ($entry in $containerMoves) {
        if (-not $entry.Moved) {
            continue
        }
        if (-not (Test-Path -LiteralPath $entry.Disabled -PathType Leaf)) {
            $restoreErrors += "Temporary file is missing: $($entry.Disabled)"
            continue
        }
        if (Test-Path -LiteralPath $entry.Original) {
            $restoreErrors += "Restore target already exists: $($entry.Original)"
            continue
        }
        try {
            Move-Item -LiteralPath $entry.Disabled -Destination $entry.Original
            $restoredHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Original).Hash
            if ($restoredHash -cne $entry.Hash) {
                $restoreErrors += "Restored hash mismatch: $($entry.Original)"
            }
        }
        catch {
            $restoreErrors += "Failed to restore $($entry.Original): $($_.Exception.Message)"
        }
    }
    if ($restoreErrors.Count -gt 0) {
        throw "One or more additional UTOCs were not restored correctly:`n$($restoreErrors -join "`n")"
    }
}

$manifests = @(Get-ChildItem -LiteralPath $output -File -Recurse -Filter 'extraction-manifest.json')
if ($manifests.Count -ne 1) {
    throw "Expected exactly one extraction manifest; found $($manifests.Count)."
}

Write-Host 'Prepared fresh DonkLift base-game inputs:'
$manifests | Sort-Object FullName | Select-Object -ExpandProperty DirectoryName
if ($containerMoves.Count -gt 0) {
    Write-Host "Restored $($containerMoves.Count) additional IoStore container(s) with matching hashes."
}
