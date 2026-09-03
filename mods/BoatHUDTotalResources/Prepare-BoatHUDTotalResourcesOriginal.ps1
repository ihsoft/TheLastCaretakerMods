# HAND-WRITTEN BUILD TOOL: extracts the current base-game Boat HUD while
# temporarily disabling and hash-restoring additional IoStore containers.

param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$Retoc = "$PSScriptRoot\..\..\.tools\bin\retoc.exe",

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$running = @(Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    $processes = ($running | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ', '
    throw "Voyage must be closed before preparing original assets: $processes"
}

$root = (Resolve-Path -LiteralPath $GameRoot).Path
$paks = (Resolve-Path -LiteralPath (Join-Path $root 'Voyage\Content\Paks')).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$additionalContainers = @(
    Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc' |
        Where-Object { $_.Name -ne 'global.utoc' -and $_.Name -notlike 'pakchunk*.utoc' }
)
$leftovers = @(
    Get-ChildItem -LiteralPath $paks -File -Filter '*.utoc.disabled-for-boat-hud-extraction-*'
)
if ($leftovers.Count -gt 0) {
    $names = ($leftovers.Name | Sort-Object) -join ', '
    throw "Unrestored UTOC files require manual recovery: $names"
}

$token = [Guid]::NewGuid().ToString('N')
$containerMoves = @(
    $additionalContainers | ForEach-Object {
        [pscustomobject]@{
            Original = $_.FullName
            Disabled = $_.FullName + ".disabled-for-boat-hud-extraction-$token"
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
        -Filter 'UI/Game/HUD/BP_VoyageIngameBoatHud' `
        -GameRoot $root `
        -Retoc $Retoc `
        -RetocEngineVersion 'UE5_7' `
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

Write-Host 'Prepared fresh base-game Boat HUD input.'
