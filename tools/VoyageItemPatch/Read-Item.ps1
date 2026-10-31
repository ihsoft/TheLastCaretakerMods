# Internal reader: use the canonical API to expose every serialized export.
param(
    [Parameter(Mandatory)][string]$AssetPath,
    [Parameter(Mandatory)][string]$MappingsPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$SchemaPath
)
$ErrorActionPreference='Stop'
$repository=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$bundle=Join-Path $repository '.tools/bin/UAssetAPI'
$manifest=Get-Content (Join-Path $bundle 'publish-manifest.json') -Raw | ConvertFrom-Json
if($manifest.kind -cne 'Voyage canonical UAssetAPI bundle' -or $manifest.targetFramework -cne 'net10.0') { throw 'Unexpected canonical UAssetAPI bundle.' }
foreach($name in @('Newtonsoft.Json.dll','ZstdSharp.dll','UAssetAPI.dll')) { [Reflection.Assembly]::LoadFrom((Join-Path $bundle $name)) | Out-Null }
$mappings=[UAssetAPI.Unversioned.Usmap]::new((Resolve-Path $MappingsPath).Path)
$asset=[UAssetAPI.UAsset]::new((Resolve-Path $AssetPath).Path,[UAssetAPI.UnrealTypes.EngineVersion]::VER_UE5_8,$mappings)
if($asset.ExportParseFailures.Count -ne 0 -or @($asset.Exports | Where-Object { $_.GetType().Name -ceq 'RawExport' }).Count) { throw 'DataAsset contains unparsed exports.' }
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputPath),$asset.SerializeJson($true))
if($SchemaPath) {
    $schema=$mappings.Schemas['BP_CraftingLocationGenerator_Default_C']
    if($schema.SuperType -cne 'VoyageCraftingLocationGenerator') { throw 'Unexpected crafting generator parent.' }
    $variables=[ordered]@{}
    foreach($p in @($schema.Properties.Values | Sort-Object SchemaIndex)) {
        if($p.ArraySize -ne 1) { throw 'Unsupported fixed property array.' }
        $variables[$p.Name]=$p.PropertyData.ToString()
    }
    [IO.File]::WriteAllText([IO.Path]::GetFullPath($SchemaPath),($variables | ConvertTo-Json -Depth 5))
}
Write-Output "Read complete DataAsset: $($asset.Exports.Count) exports"
