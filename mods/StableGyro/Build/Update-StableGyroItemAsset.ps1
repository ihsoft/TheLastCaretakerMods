# Explicit source refresh; never called by the release build.
[CmdletBinding()]
param(
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [string]$EngineRoot = 'P:/UnrealEngine/UE_5.8',
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion
)
$ErrorActionPreference = 'Stop'
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Import-Module (Join-Path $PSScriptRoot '../../../tools/unreal-coordination.psm1') -Force
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Update StableGyro item'
    return
}
$modRoot = Split-Path -Parent $PSScriptRoot
$repo = [IO.Path]::GetFullPath((Join-Path $modRoot '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot -Kind update-item
$fingerprint = & (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot | ConvertFrom-Json
$build = [string]$fingerprint.steam.buildId
& (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot -ActualSteamBuildId $build -IgnorePinnedGameVersion:$IgnorePinnedGameVersion | Out-Null
$paths = @('Project/Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset',
    'Project/Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset',
    'Project/BlueprintText/Data/Assets/Modules/DA_Item_Module_GyroCopter.json','Build/Item.source.json')
$snapshot = Get-VoyageModSourceSnapshot -ModRoot $modRoot -Paths ($paths + 'Build/Item.patch.json')
$project = & (Join-Path $modRoot 'New-StableGyroProjectCopy.ps1') -Destination (Join-Path $run 'Project')
$projectRoot = Split-Path -Parent $project
$contentBefore = Get-VoyageProjectContentSnapshot -ProjectRoot $projectRoot
$recipe = Join-Path $run 'Item.patch.json'
Copy-Item -LiteralPath (Join-Path $modRoot 'Build/Item.patch.json') -Destination $recipe
& pwsh -NoProfile -File (Join-Path $repo 'tools/Update-VoyageItemAsset.ps1') `
    -Project $project -Recipe $recipe -GameRoot $GameRoot -EngineRoot $EngineRoot -OutputRoot (Join-Path $run 'item-update') > (Join-Path $run 'update.log')
if ($LASTEXITCODE -ne 0) { throw "Item update failed; inspect $run" }
Assert-VoyageProjectContentUpdateScope -ProjectRoot $projectRoot -Before $contentBefore `
    -AllowedPaths @('Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset',
        'Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset')
$workspace = Join-Path $repo ('Tmp/BlueprintText/stablegyro-item-refresh-' + [guid]::NewGuid().ToString('N'))
$export = & (Join-Path $repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') `
    -Action ExportDataAsset -ProjectFile $project -WorkspaceRoot $workspace `
    -Asset '/Game/Data/Assets/Modules/DA_Item_Module_GyroCopter' -EngineRoot $EngineRoot | ConvertFrom-Json
$manifest = Get-Content -LiteralPath $export.manifest -Raw | ConvertFrom-Json
$textPath = Join-Path $projectRoot 'BlueprintText/Data/Assets/Modules/DA_Item_Module_GyroCopter.json'
$null = New-Item -ItemType Directory -Path (Split-Path -Parent $textPath) -Force
Copy-Item -LiteralPath (Join-Path $workspace $manifest.assets[0].source) -Destination $textPath
$null = New-Item -ItemType Directory -Path (Join-Path $run 'Build')
Set-VoyageModAssetSourceBuild -StatePath (Join-Path $run 'Build/Item.source.json') -SteamBuildId $build
Save-VoyageModSourceUpdate -ModRoot $modRoot -RunRoot $run -Paths $paths -Snapshot $snapshot
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $run -Path (Join-Path $run 'Project')
[pscustomobject]@{status='updated-and-validated';outputRoot=$run;sourceState=(Join-Path $modRoot 'Build/Item.source.json')} | ConvertTo-Json -Compress
