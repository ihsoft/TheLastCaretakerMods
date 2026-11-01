Set-StrictMode -Version Latest

function ConvertTo-StableItemRecipeValue {
    param([AllowNull()]$Value)
    if ($Value -is [pscustomobject]) {
        $result = [ordered]@{}
        foreach ($name in @($Value.PSObject.Properties.Name | Sort-Object -CaseSensitive)) {
            $result[$name] = ConvertTo-StableItemRecipeValue $Value.$name
        }
        return $result
    }
    if ($Value -is [array]) {
        $items = @(foreach ($entry in $Value) { ConvertTo-StableItemRecipeValue $entry })
        return ,$items
    }
    return $Value
}

function Get-StableGyroItemRefreshPlan {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [string]$RecipePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId,
        [switch]$Reimport
    )

    $recipe = Get-Content -LiteralPath $RecipePath -Raw -ErrorAction Stop | ConvertFrom-Json
    if ($recipe -isnot [pscustomobject]) { throw 'Item recipe must be a JSON object.' }
    $reason = 'explicit-reimport'
    if (-not $Reimport) {
        try {
            $state = Get-Content -LiteralPath $StatePath -Raw -ErrorAction Stop | ConvertFrom-Json
            if ($state.schemaVersion -ne 1 -or $state.status -cne 'ready' -or
                [string]$state.steamBuildId -cnotmatch '^[0-9]+$' -or
                $state.recipe -isnot [pscustomobject]) {
                throw 'Missing, invalid or incomplete item source state.'
            }
        } catch {
            throw "Cannot reuse maintained item assets: $($_.Exception.Message) Restore their source state from Git or request -ReimportItemAsset."
        }
        $expected = ConvertTo-Json -InputObject (ConvertTo-StableItemRecipeValue $state.recipe) -Depth 32 -Compress
        $actual = ConvertTo-Json -InputObject (ConvertTo-StableItemRecipeValue $recipe) -Depth 32 -Compress
        $reason = if ([string]$state.steamBuildId -cne $SteamBuildId) { 'game-build-changed' }
            elseif ($expected -cne $actual) { 'recipe-changed' }
            else { 'same-game-build-and-recipe' }
    }
    return [pscustomobject]@{
        refresh = $reason -cne 'same-game-build-and-recipe'
        reason = $reason
        steamBuildId = $SteamBuildId
        recipe = $recipe
    }
}

function Set-StableGyroItemSourceState {
    param(
        [Parameter(Mandatory = $true)] [string]$StatePath,
        [Parameter(Mandatory = $true)] [ValidatePattern('^[0-9]+$')] [string]$SteamBuildId,
        [Parameter(Mandatory = $true)] [pscustomobject]$Recipe,
        [Parameter(Mandatory = $true)] [ValidateSet('pending', 'ready')] [string]$Status
    )

    $state = [ordered]@{ schemaVersion = 1; steamBuildId = $SteamBuildId; status = $Status; recipe = $Recipe }
    $json = ($state | ConvertTo-Json -Depth 32) + [Environment]::NewLine
    [IO.File]::WriteAllText($StatePath, $json, (New-Object Text.UTF8Encoding($false)))
}

Export-ModuleMember -Function Get-StableGyroItemRefreshPlan, Set-StableGyroItemSourceState
