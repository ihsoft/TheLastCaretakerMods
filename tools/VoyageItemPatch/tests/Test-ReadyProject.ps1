# Integration regression: copy maintained assets into a consumer without project
# sources/binaries, bootstrap through ordinary BPT Export, then refresh twice.
param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$Recipe,
    [Parameter(Mandatory)][string[]]$BlueprintAsset,
    [string]$BlueprintSourceRoot,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8',
    [string]$GameRoot='P:/SteamLibrary/steamapps/common/Voyage'
)
$ErrorActionPreference='Stop'
$repository=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$sourceProject=(Resolve-Path -LiteralPath $Project).Path
$sourceRoot=Split-Path $sourceProject -Parent
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith((Join-Path $repository 'Tmp')+'\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)) { throw 'Use a fresh test directory below repository Tmp.' }
$descriptor=Get-Content -Raw -LiteralPath $sourceProject | ConvertFrom-Json
if($descriptor.Modules) { throw 'Test requires the migrated project descriptor without native project modules.' }
$configuration=Get-Content -Raw -LiteralPath (Join-Path $sourceRoot 'EditorSupport.json') | ConvertFrom-Json
foreach($provider in $configuration.plugins) { $provider.source=[IO.Path]::GetFullPath((Join-Path $sourceRoot $provider.source)) }
$consumer=Join-Path $output 'consumer'
New-Item -ItemType Directory -Path $consumer | Out-Null
foreach($folder in @('Content','Config')) { Copy-Item -LiteralPath (Join-Path $sourceRoot $folder) -Destination (Join-Path $consumer $folder) -Recurse }
$testProject=Join-Path $consumer 'Voyage.uproject'
$descriptor | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $testProject -Encoding utf8
$configuration | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $consumer 'EditorSupport.json') -Encoding utf8
$before=@{}
Get-ChildItem -LiteralPath (Join-Path $consumer 'Content') -Recurse -File | ForEach-Object { $before[$_.FullName]=[Convert]::ToBase64String([IO.File]::ReadAllBytes($_.FullName)) }
$workspace=Join-Path $repository ('Tmp/BlueprintText/item-ready-test-'+[guid]::NewGuid().ToString('N').Substring(0,8))
$bpt=Join-Path $repository 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1'
$export=& $bpt -Action Export -ProjectFile $testProject -WorkspaceRoot $workspace -Asset $BlueprintAsset -EngineRoot $EngineRoot | ConvertFrom-Json
if($export.status -cne 'exported') { throw 'Cold-start BPT Export did not succeed.' }
if($BlueprintSourceRoot) {
    $python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    $expected=@{}
    Get-ChildItem -LiteralPath $BlueprintSourceRoot -Filter '*.json' -File -Recurse | ForEach-Object {
        $source=Get-Content -Raw -LiteralPath $_.FullName | ConvertFrom-Json -AsHashtable
        $expected[$source.asset]=$_.FullName
    }
    foreach($asset in $BlueprintAsset) {
        if(-not $expected.ContainsKey($asset)) { throw "Missing expected BPT representation: $asset" }
        & $python -B (Join-Path $repository 'tools/BlueprintText/asset_text_codec.py') compare $expected[$asset] --actual (Join-Path $workspace ('blueprints/'+$asset.Substring(6)+'.json')) --output (Join-Path $output ('comparison-'+($asset -split '/')[-1]+'.json')) | Out-Null
        if($LASTEXITCODE -ne 0) { throw "Native provider changed the BPT representation: $asset" }
    }
}
foreach($run in @('first','repeat')) {
    & (Join-Path $repository 'tools/Update-VoyageItemAsset.ps1') -Project $testProject -Recipe $Recipe -EngineRoot $EngineRoot -GameRoot $GameRoot -OutputRoot (Join-Path $output $run) | Out-Null
    $support=Get-Content -Raw -LiteralPath (Join-Path $output "$run/editor-support/result.json") | ConvertFrom-Json
    if(@($support.providers | Where-Object built).Count) { throw 'Unchanged native inputs were recompiled after BPT preparation.' }
}
$preview=& $bpt -Action Preview -WorkspaceRoot $workspace -EngineRoot $EngineRoot | ConvertFrom-Json
if($preview.status -cne 'previewed') { throw 'Preview failed.' }
$spec=Get-Content -Raw -LiteralPath (Join-Path $output 'repeat/import.json') | ConvertFrom-Json
$owned=@($spec.package,$spec.blueprint) | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $consumer ('Content/'+([string]$_).Substring(6)+'.uasset'))) }
$after=@(Get-ChildItem -LiteralPath (Join-Path $consumer 'Content') -Recurse -File)
if($after.Count -ne $before.Count) { throw 'Repeated update changed the asset inventory.' }
foreach($file in $after) {
    if(-not $before.ContainsKey($file.FullName)) { throw "Unexpected asset: $($file.FullName)" }
    if($file.FullName -notin $owned -and $before[$file.FullName] -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($file.FullName))) { throw "Unrelated asset changed: $($file.FullName)" }
}
$sourceFiles=@(Get-ChildItem -LiteralPath $consumer -Recurse -File | Where-Object Extension -in @('.h','.cpp','.cs'))
if($sourceFiles.Count) { throw 'Consumer contains native or build sources.' }
$result=[ordered]@{status='passed';project=$testProject;assets=$after.Count;sourceFiles=0;repeatedUpdate='passed';unchangedInputsRecompiled=$false;exportedAssets=$export.assets;graphs=$preview.graphs;preview=$preview.preview;evidence=$output}
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding utf8
$result | ConvertTo-Json -Depth 10 -Compress
