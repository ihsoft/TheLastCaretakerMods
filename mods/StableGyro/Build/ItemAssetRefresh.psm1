Set-StrictMode -Version Latest

function Get-StableGyroItemRefreshPlan {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId
    )

    try {
        $state = Get-Content -LiteralPath $StatePath -Raw -ErrorAction Stop | ConvertFrom-Json
        if ($state.schemaVersion -ne 1 -or
            [string]$state.steamBuildId -cnotmatch '^[0-9]+$') {
            throw 'Missing, invalid or incomplete item source state.'
        }
    } catch {
        throw "Cannot reuse maintained item assets: $($_.Exception.Message) Restore their source state from Git or run Build/Update-StableGyroItemAsset.ps1."
    }
    $reason = if ([string]$state.steamBuildId -cne $SteamBuildId) { 'game-build-changed' }
        else { 'same-game-build' }
    return [pscustomobject]@{
        refresh = $reason -cne 'same-game-build'
        reason = $reason
        steamBuildId = $SteamBuildId
        sourceSteamBuildId = [string]$state.steamBuildId
    }
}

function Set-StableGyroItemSourceState {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId
    )

    $state = [ordered]@{ schemaVersion = 1; steamBuildId = $SteamBuildId }
    $json = ($state | ConvertTo-Json) + [Environment]::NewLine
    [IO.File]::WriteAllText($StatePath, $json, (New-Object Text.UTF8Encoding($false)))
}

Export-ModuleMember -Function Get-StableGyroItemRefreshPlan, Set-StableGyroItemSourceState
