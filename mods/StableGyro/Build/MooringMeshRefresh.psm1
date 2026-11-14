Set-StrictMode -Version Latest

function Get-StableGyroMooringMeshPlan {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId
    )

    try {
        $state = Get-Content -LiteralPath $StatePath -Raw -ErrorAction Stop | ConvertFrom-Json
        if ($state.schemaVersion -ne 2 -or $state.recipe -cne 'single-positive-y-v1' -or
            $state.status -cne 'ready' -or
            [string]$state.steamBuildId -cnotmatch '^[0-9]+$') {
            throw 'Missing, invalid or incomplete mesh source state.'
        }
        if ([string]$state.steamBuildId -cne $SteamBuildId) {
            throw "Plate source build $($state.steamBuildId) differs from installed build $SteamBuildId."
        }
    } catch {
        throw "Cannot reuse the maintained mooring plate: $($_.Exception.Message) Ask the user whether to run Update-StableGyroMooringPlate.ps1; no automatic refresh was performed."
    }

    return [pscustomobject]@{
        refresh = $false
        reason = 'same-game-build'
        steamBuildId = $SteamBuildId
    }
}

function Set-StableGyroMooringMeshState {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId,
        [Parameter(Mandatory = $true)] [ValidateSet('pending', 'ready')] [string]$Status
    )

    # Only the explicit updater writes this state. Ready requires full geometry
    # comparison after cooking the mesh in the maintained project.
    $state = [ordered]@{
        schemaVersion = 2; recipe = 'single-positive-y-v1'
        steamBuildId = $SteamBuildId; status = $Status
    }
    $json = ($state | ConvertTo-Json) + [Environment]::NewLine
    [IO.File]::WriteAllText($StatePath, $json, (New-Object Text.UTF8Encoding($false)))
}

Export-ModuleMember -Function Get-StableGyroMooringMeshPlan, Set-StableGyroMooringMeshState
