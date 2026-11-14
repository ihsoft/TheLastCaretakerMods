# Internal implementation; invoke the public command at the mod root.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$EngineRoot,
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion
)
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    throw 'Use the root Update-StableGyroItemAsset.ps1 command to acquire the Unreal queue.'
}
$modRoot = Split-Path -Parent $PSScriptRoot
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $modRoot '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
Import-Module (Join-Path $modRoot 'Build/ItemAssetRefresh.psm1') -Force
Import-Module (Join-Path $modRoot 'Build/SourceUpdate.psm1') -Force
$run = New-VoyageModRun -ModRoot $modRoot -OutputRoot $OutputRoot -Kind update-item
$fingerprint = & (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot | ConvertFrom-Json
$build = [string]$fingerprint.steam.buildId
& (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') -ModRoot $modRoot -ActualSteamBuildId $build -IgnorePinnedGameVersion:$IgnorePinnedGameVersion | Out-Null
$paths = @('Project/Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset',
    'Project/Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset','Build/Item.source.json')
$snapshot = Get-StableGyroSourceSnapshot -ModRoot $modRoot -Paths ($paths + 'Build/Item.patch.json')
$project = & (Join-Path $modRoot 'New-StableGyroProjectCopy.ps1') -Destination (Join-Path $run 'Project')
$projectRoot = Split-Path -Parent $project
$contentBefore = Get-StableGyroContentSnapshot -ProjectRoot $projectRoot
$recipe = Join-Path $run 'Item.patch.json'
Copy-Item -LiteralPath (Join-Path $modRoot 'Build/Item.patch.json') -Destination $recipe
& pwsh -NoProfile -File (Join-Path $repo 'tools/Update-VoyageItemAsset.ps1') `
    -Project $project -Recipe $recipe -GameRoot $GameRoot -EngineRoot $EngineRoot -OutputRoot (Join-Path $run 'item-update') > (Join-Path $run 'update.log')
if ($LASTEXITCODE -ne 0) { throw "Item update failed; inspect $run" }
Assert-StableGyroItemUpdateScope -ProjectRoot $projectRoot -Before $contentBefore `
    -AllowedPaths @('Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset',
        'Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset')
$null = New-Item -ItemType Directory -Path (Join-Path $run 'Build')
Set-StableGyroItemSourceState -StatePath (Join-Path $run 'Build/Item.source.json') -SteamBuildId $build `
    -Recipe (Get-Content -LiteralPath $recipe -Raw | ConvertFrom-Json) -Status ready
Save-StableGyroSourceUpdate -ModRoot $modRoot -RunRoot $run -Paths $paths -Snapshot $snapshot
Remove-VoyageModScratch -ModRoot $modRoot -RunRoot $run -Path (Join-Path $run 'Project')
[pscustomobject]@{status='updated-and-validated';outputRoot=$run;sourceState=(Join-Path $modRoot 'Build/Item.source.json')} | ConvertTo-Json -Compress
