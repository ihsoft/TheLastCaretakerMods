Set-StrictMode -Version Latest

function Assert-StableGyroItemSource {
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
        if ([string]$state.steamBuildId -cne $SteamBuildId) {
            throw "Item source build $($state.steamBuildId) differs from installed build $SteamBuildId."
        }
    } catch {
        throw "Cannot reuse maintained item assets: $($_.Exception.Message) Restore their source state from Git if damaged; ask the user whether to run Build/Update-StableGyroItemAsset.ps1 to refresh inputs. No automatic refresh was performed."
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

Export-ModuleMember -Function Assert-StableGyroItemSource, Set-StableGyroItemSourceState
