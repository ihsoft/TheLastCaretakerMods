$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'MooringMeshRefresh.psm1') -Force
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$testRoot = Join-Path $repoRoot ('Tmp\StableGyro\test-mesh-refresh-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$statePath = Join-Path $testRoot 'source.json'

function Assert-Plan {
    param([string]$Build, [bool]$Refresh, [string]$Reason, [switch]$Reimport)
    $plan = Get-StableGyroMooringMeshPlan -StatePath $statePath -SteamBuildId $Build -Reimport:$Reimport
    if ($plan.refresh -ne $Refresh -or $plan.reason -cne $Reason -or $plan.steamBuildId -cne $Build) {
        throw "Unexpected refresh plan: $($plan | ConvertTo-Json -Compress)"
    }
}

function Assert-ReuseRejected {
    $rejected = $false
    try { Get-StableGyroMooringMeshPlan -StatePath $statePath -SteamBuildId '100' | Out-Null }
    catch {
        if ($_.Exception.Message -notlike '*-ReimportMooringMeshes*') { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw 'Unproven mesh state was accepted for reuse.' }
}

# Missing provenance never silently becomes a third automatic refresh trigger.
Assert-ReuseRejected
Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
Set-StableGyroMooringMeshState -StatePath $statePath -SteamBuildId '100' -Status ready
$before = [IO.File]::ReadAllBytes($statePath)
$writeTime = (Get-Item -LiteralPath $statePath).LastWriteTimeUtc
Assert-Plan -Build '100' -Refresh $false -Reason 'same-game-build'
Assert-Plan -Build '101' -Refresh $true -Reason 'game-build-changed'
Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
if ([Convert]::ToBase64String($before) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($statePath)) -or
    $writeTime -ne (Get-Item -LiteralPath $statePath).LastWriteTimeUtc) {
    throw 'Planning rewrote maintained source state.'
}

# An interrupted same-build repair must not inherit the previous ready state.
Set-StableGyroMooringMeshState -StatePath $statePath -SteamBuildId '100' -Status pending
Assert-ReuseRejected
Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
Set-StableGyroMooringMeshState -StatePath $statePath -SteamBuildId '101' -Status ready
Assert-Plan -Build '101' -Refresh $false -Reason 'same-game-build'
Assert-Plan -Build '100' -Refresh $true -Reason 'game-build-changed'

foreach ($invalid in @('{', '{}', '{"schemaVersion":2,"steamBuildId":"100","status":"ready"}',
        '{"schemaVersion":1,"steamBuildId":"","status":"ready"}')) {
    [IO.File]::WriteAllText($statePath, $invalid)
    Assert-ReuseRejected
    Assert-Plan -Build '100' -Refresh $true -Reason 'explicit-reimport' -Reimport
}

Remove-Item -LiteralPath $statePath
Remove-Item -LiteralPath $testRoot
Write-Output 'Mooring mesh refresh tests passed: same build, changed build, explicit repair, invalid state, interrupted refresh, and no-write planning.'
