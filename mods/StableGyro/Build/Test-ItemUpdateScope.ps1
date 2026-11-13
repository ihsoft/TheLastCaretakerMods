$ErrorActionPreference='Stop'
$mod=Split-Path -Parent $PSScriptRoot
$repo=[IO.Path]::GetFullPath((Join-Path $mod '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run=New-VoyageModRun -ModRoot $mod -Kind test-item-scope
$fixture=Join-Path $run 'fixture'
$fixtureMod=Join-Path $fixture 'mods/StableGyro'
foreach ($relative in @('tools','mods/StableGyro/Build','mods/StableGyro/Project/Content/Data/Assets/Modules','mods/StableGyro/Project/Content/Blueprints/Fabrication','mods/StableGyro/Project/Content/LocalizationStringTables')) {
    $null=New-Item -ItemType Directory -Path (Join-Path $fixture $relative) -Force
}
foreach ($relative in @('Update-StableGyroItemAsset.ps1','New-StableGyroProjectCopy.ps1','Build/SourceUpdate.psm1','Build/ItemAssetRefresh.psm1')) {
    Copy-Item -LiteralPath (Join-Path $mod $relative) -Destination (Join-Path $fixtureMod $relative)
}
Copy-Item -LiteralPath (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Destination (Join-Path $fixture 'tools')
@'
'{"steam":{"buildId":"25982825"}}'
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/Get-VoyageBuildFingerprint.ps1')
'param($ModRoot,$ActualSteamBuildId,[switch]$IgnorePinnedGameVersion)' | Set-Content -LiteralPath (Join-Path $fixture 'tools/Assert-VoyageModGameBuild.ps1')
'{}' | Set-Content -LiteralPath (Join-Path $fixtureMod 'Project/Voyage.uproject')
'{"schema":1,"plugins":[]}' | Set-Content -LiteralPath (Join-Path $fixtureMod 'Project/EditorSupport.json')
'{"schemaVersion":1}' | Set-Content -LiteralPath (Join-Path $fixtureMod 'Build/Item.patch.json')
$item='Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset'
$generator='Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset'
$dependency='Content/LocalizationStringTables/ST_Modules.uasset'
foreach ($relative in @($item,$generator)) { [IO.File]::WriteAllText((Join-Path $fixtureMod ('Project/'+$relative)),'uncommitted original') }
@'
param($Project,$Recipe,$GameRoot,$EngineRoot,$OutputRoot)
$root=Split-Path -Parent $Project
foreach ($relative in @('Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset','Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset')) {
    [IO.File]::WriteAllText((Join-Path $root $relative),'updated pair')
}
$dependency=Join-Path $root 'Content/LocalizationStringTables/ST_Modules.uasset'
if (-not (Test-Path -LiteralPath $dependency)) { [IO.File]::WriteAllText($dependency,'created dependency') }
'{"status":"passed"}'
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/Update-VoyageItemAsset.ps1')
$lease=$env:CODEX_UNREAL_QUEUE_LEASE
function Invoke-Wrapper([string]$Output,[string]$LogName) {
    $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+(Join-Path $fixtureMod 'Update-StableGyroItemAsset.ps1')+'"'),'-OutputRoot',('"'+$Output+'"'))
    $process=Start-Process powershell.exe -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru `
        -RedirectStandardOutput (Join-Path $run ($LogName+'.log')) -RedirectStandardError (Join-Path $run ($LogName+'-error.log'))
    $process.ExitCode
}
try {
    # Only synthetic tools run in this fixture; no engine or queue child exists.
    $env:CODEX_UNREAL_QUEUE_LEASE='offline-fixture'
    $output=Join-Path $fixture 'Tmp/StableGyro/missing-dependency'
    if ((Invoke-Wrapper $output 'missing-dependency') -eq 0) { throw 'Missing dependency creation was silently accepted.' }
    foreach ($relative in @($item,$generator)) {
        if ([IO.File]::ReadAllText((Join-Path $fixtureMod ('Project/'+$relative))) -cne 'uncommitted original') { throw 'Rejected dependency update changed maintained source.' }
    }
    if (Test-Path -LiteralPath (Join-Path $fixtureMod 'Build/Item.source.json')) { throw 'Rejected update accepted ready state.' }
    if (-not (Test-Path -LiteralPath (Join-Path $output ('Project/'+$dependency)))) { throw 'Rejected working copy was discarded.' }
    if (-not (Select-String -LiteralPath (Join-Path $run 'missing-dependency-error.log') -SimpleMatch 'outside the accepted pair' -Quiet)) { throw 'Rejection did not identify the dependency boundary.' }
    [IO.File]::WriteAllText((Join-Path $fixtureMod ('Project/'+$dependency)),'existing dependency')
    if ((Invoke-Wrapper (Join-Path $fixture 'Tmp/StableGyro/ready-dependencies') 'ready-dependencies') -ne 0) { throw "Valid pair update failed; inspect $run" }
    if ([IO.File]::ReadAllText((Join-Path $fixtureMod ('Project/'+$dependency))) -cne 'existing dependency') { throw 'Existing dependency changed.' }
    foreach ($relative in @($item,$generator)) {
        if ([IO.File]::ReadAllText((Join-Path $fixtureMod ('Project/'+$relative))) -cne 'updated pair') { throw 'Valid pair not accepted.' }
    }
} finally { $env:CODEX_UNREAL_QUEUE_LEASE=$lease }
@{status='passed';checks=@('missing string-table creation rejected before acceptance','source bytes and state preserved','working copy retained','existing-dependency pair accepted');boundary='Real public wrapper with synthetic item updater; no Unreal.'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $run 'report.json')
"Item dependency acceptance report: $run/report.json"
