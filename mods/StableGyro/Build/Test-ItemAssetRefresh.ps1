$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'ItemAssetRefresh.psm1') -Force
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$testRoot = Join-Path $repoRoot ('Tmp\StableGyro\test-item-refresh-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$statePath = Join-Path $testRoot 'source.json'
$recipePath = Join-Path $testRoot 'recipe.json'
$originalRecipe = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Item.patch.json') -Raw
[IO.File]::WriteAllText($recipePath, $originalRecipe)
$recipe = $originalRecipe | ConvertFrom-Json

function Assert-Plan {
    param([string]$Build, [bool]$Refresh, [string]$Reason, [switch]$Reimport)
    $plan = Get-StableGyroItemRefreshPlan -StatePath $statePath -RecipePath $recipePath -SteamBuildId $Build -Reimport:$Reimport
    if ($plan.refresh -ne $Refresh -or $plan.reason -cne $Reason -or $plan.steamBuildId -cne $Build) {
        throw "Unexpected item refresh plan: $($plan | ConvertTo-Json -Depth 10 -Compress)"
    }
}

function Assert-ReuseRejected {
    $rejected = $false
    try { Get-StableGyroItemRefreshPlan -StatePath $statePath -RecipePath $recipePath -SteamBuildId '100' | Out-Null }
    catch {
        if ($_.Exception.Message -notlike '*-ReimportItemAsset*') { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw 'Unproven item state was accepted for reuse.' }
}

Assert-ReuseRejected
Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
Set-StableGyroItemSourceState -StatePath $statePath -SteamBuildId '100' -Recipe $recipe -Status ready
$before = [IO.File]::ReadAllBytes($statePath)
$writeTime = (Get-Item -LiteralPath $statePath).LastWriteTimeUtc
Assert-Plan -Build '100' -Refresh $false -Reason 'same-game-build-and-recipe'
Assert-Plan -Build '101' -Refresh $true -Reason 'game-build-changed'
Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport

# Formatting and object key order are not recipe edits; array contents and
# case-sensitive reference changes are. Exercise nested patch fields too.
$patch = $recipe.patches[0]
$reordered = [ordered]@{
    patches = @([ordered]@{ value = $patch.value; expect = $patch.expect; property = $patch.property })
    asset = $recipe.asset; profile = $recipe.profile; schemaVersion = $recipe.schemaVersion
}
[IO.File]::WriteAllText($recipePath, ($reordered | ConvertTo-Json -Depth 10 -Compress))
Assert-Plan -Build '100' -Refresh $false -Reason 'same-game-build-and-recipe'
foreach ($field in @('value', 'expect', 'property')) {
    $changed = $originalRecipe | ConvertFrom-Json
    $changed.patches[0].$field = ([string]$changed.patches[0].$field).ToUpperInvariant()
    [IO.File]::WriteAllText($recipePath, ($changed | ConvertTo-Json -Depth 10))
    Assert-Plan -Build '100' -Refresh $true -Reason 'recipe-changed'
}
$changed = $originalRecipe | ConvertFrom-Json
$changed.patches = @()
[IO.File]::WriteAllText($recipePath, ($changed | ConvertTo-Json -Depth 10))
Assert-Plan -Build '100' -Refresh $true -Reason 'recipe-changed'
[IO.File]::WriteAllText($recipePath, $originalRecipe)
if ([Convert]::ToBase64String($before) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($statePath)) -or
    $writeTime -ne (Get-Item -LiteralPath $statePath).LastWriteTimeUtc) {
    throw 'Planning rewrote maintained item state.'
}

Set-StableGyroItemSourceState -StatePath $statePath -SteamBuildId '100' -Recipe $recipe -Status pending
Assert-ReuseRejected
Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
Set-StableGyroItemSourceState -StatePath $statePath -SteamBuildId '101' -Recipe $recipe -Status ready
Assert-Plan -Build '101' -Refresh $false -Reason 'same-game-build-and-recipe'
foreach ($invalid in @('{', '{}', '{"schemaVersion":1,"steamBuildId":"100","status":"ready"}')) {
    [IO.File]::WriteAllText($statePath, $invalid)
    Assert-ReuseRejected
    Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
}

Remove-Item -LiteralPath $statePath, $recipePath
Remove-Item -LiteralPath $testRoot
Write-Output 'Item refresh tests passed: version, recipe semantics, explicit repair, invalid/interrupted state, and no-write planning.'
