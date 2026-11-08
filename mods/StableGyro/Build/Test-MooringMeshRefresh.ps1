$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'MooringMeshRefresh.psm1') -Force
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$testRoot = Join-Path $repoRoot ('Tmp\StableGyro\test-mesh-refresh-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$statePath = Join-Path $testRoot 'source.json'

function Assert-Plan {
    param([string]$Build, [bool]$Refresh, [string]$Reason)
    $plan = Get-StableGyroMooringMeshPlan -StatePath $statePath -SteamBuildId $Build
    if ($plan.refresh -ne $Refresh -or $plan.reason -cne $Reason -or $plan.steamBuildId -cne $Build) {
        throw "Unexpected refresh plan: $($plan | ConvertTo-Json -Compress)"
    }
}

function Assert-ReuseRejected {
    param([string]$Build = "100")
    $rejected = $false
    try { Get-StableGyroMooringMeshPlan -StatePath $statePath -SteamBuildId $Build | Out-Null }
    catch {
        if ($_.Exception.Message -notlike '*Update-StableGyroMooringPlate.ps1*') { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw 'Unproven mesh state was accepted for reuse.' }
}

# Missing provenance must not trigger an implicit extraction or import.
Assert-ReuseRejected
Set-StableGyroMooringMeshState -StatePath $statePath -SteamBuildId '100' -Status ready
$before = [IO.File]::ReadAllBytes($statePath)
$writeTime = (Get-Item -LiteralPath $statePath).LastWriteTimeUtc
Assert-Plan -Build '100' -Refresh $false -Reason 'same-game-build'
Assert-ReuseRejected -Build '101'
if ([Convert]::ToBase64String($before) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($statePath)) -or
    $writeTime -ne (Get-Item -LiteralPath $statePath).LastWriteTimeUtc) {
    throw 'Planning rewrote maintained source state.'
}

# An interrupted same-build repair must not inherit the previous ready state.
Set-StableGyroMooringMeshState -StatePath $statePath -SteamBuildId '100' -Status pending
Assert-ReuseRejected
Set-StableGyroMooringMeshState -StatePath $statePath -SteamBuildId '101' -Status ready
Assert-Plan -Build '101' -Refresh $false -Reason 'same-game-build'
Assert-ReuseRejected -Build '100'

foreach ($invalid in @('{', '{}', '{"schemaVersion":1,"steamBuildId":"100","status":"ready"}',
        '{"schemaVersion":2,"recipe":"wrong","steamBuildId":"100","status":"ready"}',
        '{"schemaVersion":2,"recipe":"single-positive-y-v1","steamBuildId":"","status":"ready"}')) {
    [IO.File]::WriteAllText($statePath, $invalid)
    Assert-ReuseRejected
}

Remove-Item -LiteralPath $statePath
Remove-Item -LiteralPath $testRoot
Write-Output 'Mooring mesh refresh tests passed: same build, stale build rejection, invalid state, interrupted refresh, and no-write planning.'
