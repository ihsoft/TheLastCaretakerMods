Set-StrictMode -Version Latest

function Get-StableGyroMooringMeshPlan {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId,
        [switch]$Reimport
    )

    if ($Reimport) {
        return [pscustomobject]@{ refresh = $true; reason = 'explicit-reimport'; steamBuildId = $SteamBuildId }
    }
    try {
        $state = Get-Content -LiteralPath $StatePath -Raw -ErrorAction Stop | ConvertFrom-Json
        if ($state.schemaVersion -ne 1 -or $state.status -cne 'ready' -or
            [string]$state.steamBuildId -cnotmatch '^[0-9]+$') {
            throw 'Missing, invalid or incomplete mesh source state.'
        }
    } catch {
        throw "Cannot reuse maintained mooring meshes: $($_.Exception.Message) Restore their source state from Git or request -ReimportMooringMeshes."
    }

    $refresh = [string]$state.steamBuildId -cne $SteamBuildId
    return [pscustomobject]@{
        refresh = $refresh
        reason = $(if ($refresh) { 'game-build-changed' } else { 'same-game-build' })
        steamBuildId = $SteamBuildId
    }
}

function Set-StableGyroMooringMeshState {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId,
        [Parameter(Mandatory = $true)] [ValidateSet('pending', 'ready')] [string]$Status
    )

    # Pending is saved before replacing either mesh. Only final-container validation
    # may mark the pair ready, so an interrupted refresh cannot silently be reused.
    $state = [ordered]@{ schemaVersion = 1; steamBuildId = $SteamBuildId; status = $Status }
    $json = ($state | ConvertTo-Json) + [Environment]::NewLine
    [IO.File]::WriteAllText($StatePath, $json, (New-Object Text.UTF8Encoding($false)))
}

Export-ModuleMember -Function Get-StableGyroMooringMeshPlan, Set-StableGyroMooringMeshState
