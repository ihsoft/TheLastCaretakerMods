Set-StrictMode -Version Latest

function Assert-StableGyroMooringMeshSource {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId
    )

    try {
        $state = Get-Content -LiteralPath $StatePath -Raw -ErrorAction Stop | ConvertFrom-Json
        if ($state.schemaVersion -ne 2 -or
            [string]$state.steamBuildId -cnotmatch '^[0-9]+$') {
            throw 'Missing, invalid or incomplete mesh source state.'
        }
        if ([string]$state.steamBuildId -cne $SteamBuildId) {
            throw "Plate source build $($state.steamBuildId) differs from installed build $SteamBuildId."
        }
    } catch {
        throw "Cannot reuse the maintained mooring plate: $($_.Exception.Message) Ask the user whether to run Build/Update-StableGyroMooringPlate.ps1; no automatic refresh was performed."
    }
}

function Set-StableGyroMooringMeshState {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId
    )

    # Only the explicit updater writes this state, after extraction, splitting
    # and the shared importer's package/material reopen checks succeed.
    $state = [ordered]@{
        schemaVersion = 2; steamBuildId = $SteamBuildId
    }
    $json = ($state | ConvertTo-Json) + [Environment]::NewLine
    [IO.File]::WriteAllText($StatePath, $json, (New-Object Text.UTF8Encoding($false)))
}

Export-ModuleMember -Function Assert-StableGyroMooringMeshSource, Set-StableGyroMooringMeshState
