[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RepositoryRoot,
    [Parameter(Mandatory = $true)][string]$PatchedJsonPath,
    [Parameter(Mandatory = $true)][string]$OutputAssetPath,
    [Parameter(Mandatory = $true)][string]$MappingsPath,
    [Parameter(Mandatory = $true)][string]$OwnedClassReadbackPath,
    [Parameter(Mandatory = $true)][string]$ExpectedOwnedClassReadbackSha256,
    [Parameter(Mandatory = $true)][string]$ReadbackJsonPath,
    [Parameter(Mandatory = $true)][string]$EvidencePath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ownedClassName = 'BP_CraftingLocationGenerator_PalletContainerSnap_C'
$ownedClassPackage = '/Game/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_PalletContainerSnap'
$parentClassName = 'BP_CraftingLocationGenerator_Default_C'
$nativeBaseClassName = 'VoyageCraftingLocationGenerator'
$ownedPropertyName = 'SavedIgnoreNoFabrication'
$expectedOwnedPropertyFlags = 'Edit | BlueprintVisible | DisableEditOnInstance'

function Get-JsonSha256([object]$Value) {
    $json = $Value | ConvertTo-Json -Depth 20 -Compress
    $bytes = [Text.Encoding]::UTF8.GetBytes($json)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '') }
    finally { $sha.Dispose() }
}

function Get-SchemaSnapshot([UAssetAPI.Unversioned.Usmap]$Mappings, [string]$Name) {
    if (-not $Mappings.Schemas.ContainsKey($Name)) { throw "Required reviewed schema is missing: $Name" }
    $schema = $Mappings.Schemas[$Name]
    $properties = @($schema.Properties.GetEnumerator() | Sort-Object Key | ForEach-Object {
        [ordered]@{
            key = [int]$_.Key
            name = [string]$_.Value.Name
            schemaIndex = [int]$_.Value.SchemaIndex
            arrayIndex = [int]$_.Value.ArrayIndex
            arraySize = [int]$_.Value.ArraySize
            type = [string]$_.Value.PropertyData.Type
            flags = [string]$_.Value.PropertyFlags
        }
    })
    return [ordered]@{
        name = [string]$schema.Name
        modulePath = $schema.ModulePath
        superType = [string]$schema.SuperType
        superTypeModulePath = $schema.SuperTypeModulePath
        propCount = [int]$schema.PropCount
        properties = $properties
    }
}

function Add-ExactOwnedSchema([UAssetAPI.Unversioned.Usmap]$Mappings) {
    if ($Mappings.Schemas.ContainsKey($ownedClassName)) {
        throw 'Owned class schema unexpectedly exists in the reviewed game mapping.'
    }
    $parent = Get-SchemaSnapshot $Mappings $parentClassName
    $nativeBase = Get-SchemaSnapshot $Mappings $nativeBaseClassName
    if ($parent.propCount -ne 19 -or $parent.superType -cne $nativeBaseClassName -or
        $null -ne $parent.modulePath -or $null -ne $parent.superTypeModulePath -or
        $nativeBase.propCount -ne 1 -or $nativeBase.superType -cne 'Object' -or
        $null -ne $nativeBase.modulePath -or $null -ne $nativeBase.superTypeModulePath) {
        throw 'Reviewed generator parent schema chain does not match the accepted 19/1 contract.'
    }

    $propertyData = [UAssetAPI.Unversioned.UsmapPropertyData]::new(
        [UAssetAPI.Unversioned.UsmapPropertyType]::BoolProperty)
    $property = [UAssetAPI.Unversioned.UsmapProperty]::new(
        $ownedPropertyName, 0, 0, 1, $propertyData)
    $property.PropertyFlags =
        [UAssetAPI.UnrealTypes.EPropertyFlags]::CPF_Edit -bor
        [UAssetAPI.UnrealTypes.EPropertyFlags]::CPF_BlueprintVisible -bor
        [UAssetAPI.UnrealTypes.EPropertyFlags]::CPF_DisableEditOnInstance
    $properties = [Collections.Concurrent.ConcurrentDictionary[int,UAssetAPI.Unversioned.UsmapProperty]]::new()
    if (-not $properties.TryAdd(0, $property)) { throw 'Could not add the exact owned Bool schema property.' }
    $schema = [UAssetAPI.Unversioned.UsmapSchema]::new(
        $ownedClassName, $parentClassName, 1, $properties,
        $Mappings.AreFNamesCaseInsensitive, $null, $false)
    $schema.ModulePath = $null
    $schema.StructKind = [UAssetAPI.Unversioned.UsmapStructKind]::UClass
    if (-not $Mappings.Schemas.TryAdd($ownedClassName, $schema)) {
        throw 'Could not add the exact owned class schema supplement.'
    }
}

$repository = [IO.Path]::GetFullPath($RepositoryRoot)
$bundle = Join-Path $repository '.tools/bin/UAssetAPI'
$manifestPath = Join-Path $bundle 'publish-manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([string]$manifest.kind -cne 'Voyage canonical UAssetAPI bundle' -or
    [string]$manifest.sourceCommit -cne '21c982fa8f04e12d5d216fdf330a2f206e81156f' -or
    [string]$manifest.targetFramework -cne 'net10.0') {
    throw 'Published UAssetAPI manifest does not match the accepted runtime contract.'
}
$publishedFiles = @{}
foreach ($file in @($manifest.files)) { $publishedFiles[[string]$file.path] = [string]$file.sha256 }
foreach ($name in @('Newtonsoft.Json.dll', 'ZstdSharp.dll', 'UAssetAPI.dll')) {
    $path = Join-Path $bundle $name
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -cne $publishedFiles[$name]) {
        throw "Published dependency hash mismatch: $name"
    }
    [Reflection.Assembly]::LoadFrom($path) | Out-Null
}

$actualClassReadbackSha = (Get-FileHash -LiteralPath $OwnedClassReadbackPath -Algorithm SHA256).Hash
if ($actualClassReadbackSha -cne $ExpectedOwnedClassReadbackSha256) {
    throw 'Owned generator class readback differs from the accepted installed 0.6 identity.'
}
$classReadback = [object[]](Get-Content -LiteralPath $OwnedClassReadbackPath -Raw | ConvertFrom-Json)
$ownedClass = @($classReadback | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass' -and $_.Name -ceq $ownedClassName
})
if ($ownedClass.Count -ne 1 -or
    [string]$ownedClass[0].Super.ObjectName -cne "BlueprintGeneratedClass'$parentClassName'" -or
    [string]$ownedClass[0].Super.ObjectPath -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.0') {
    throw 'Owned generator class/super identity is not the accepted 0.6 contract.'
}
$ownedProperties = @($ownedClass[0].ChildProperties)
if ($ownedProperties.Count -ne 1 -or
    [string]$ownedProperties[0].Name -cne $ownedPropertyName -or
    [string]$ownedProperties[0].Type -cne 'BoolProperty' -or
    [int]$ownedProperties[0].ElementSize -ne 1 -or
    [int]$ownedProperties[0].FieldSize -ne 1 -or
    [int]$ownedProperties[0].ByteOffset -ne 0 -or
    [int]$ownedProperties[0].ByteMask -ne 1 -or
    [int]$ownedProperties[0].FieldMask -ne 255 -or
    [int]$ownedProperties[0].BoolSize -ne 1 -or
    -not [bool]$ownedProperties[0].bIsNativeBool -or
    [string]$ownedProperties[0].PropertyFlags -cne $expectedOwnedPropertyFlags) {
    throw 'Owned generator declared-property schema is not the accepted one-Bool contract.'
}

$patchedPackage = Get-Content -LiteralPath $PatchedJsonPath -Raw | ConvertFrom-Json
$nestedExports = @($patchedPackage.Exports | Where-Object { $_.ObjectName -ceq ($ownedClassName + '_0') })
if ($nestedExports.Count -ne 1 -or @($nestedExports[0].Data).Count -ne 0) {
    throw 'Patched input does not contain exactly one empty owned generator instance export.'
}

$mappings = [UAssetAPI.Unversioned.Usmap]::new((Resolve-Path -LiteralPath $MappingsPath).Path)
if ($mappings.Schemas.ContainsKey($ownedClassName)) { throw 'Owned schema must be absent from the reviewed mapping.' }
$parentBefore = Get-SchemaSnapshot $mappings $parentClassName
$nativeBefore = Get-SchemaSnapshot $mappings $nativeBaseClassName
$parentBeforeHash = Get-JsonSha256 $parentBefore
$nativeBeforeHash = Get-JsonSha256 $nativeBefore
$schemaCountBefore = $mappings.Schemas.Count
Add-ExactOwnedSchema $mappings
$parentAfterHash = Get-JsonSha256 (Get-SchemaSnapshot $mappings $parentClassName)
$nativeAfterHash = Get-JsonSha256 (Get-SchemaSnapshot $mappings $nativeBaseClassName)
if ($parentAfterHash -cne $parentBeforeHash -or $nativeAfterHash -cne $nativeBeforeHash -or
    $mappings.Schemas.Count -ne ($schemaCountBefore + 1)) {
    throw 'Owned schema supplement changed a reviewed stock schema.'
}

$asset = [UAssetAPI.UAsset]::DeserializeJson([IO.File]::ReadAllText((Resolve-Path -LiteralPath $PatchedJsonPath).Path))
$asset.Mappings = $mappings
$asset.SetSerializationEngineVersion([UAssetAPI.UnrealTypes.EngineVersion]::VER_UE5_8)
$asset.FilePath = (Resolve-Path -LiteralPath $PatchedJsonPath).Path
$outputFullPath = [IO.Path]::GetFullPath($OutputAssetPath)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($outputFullPath)) | Out-Null
$asset.Write($outputFullPath)

$reopenMappings = [UAssetAPI.Unversioned.Usmap]::new((Resolve-Path -LiteralPath $MappingsPath).Path)
if ($reopenMappings.Schemas.ContainsKey($ownedClassName)) { throw 'Owned schema unexpectedly exists before reopen supplement.' }
Add-ExactOwnedSchema $reopenMappings
$reopened = [UAssetAPI.UAsset]::new(
    $outputFullPath,
    [UAssetAPI.UnrealTypes.EngineVersion]::VER_UE5_8,
    $reopenMappings,
    [UAssetAPI.CustomSerializationFlags]::None,
    [UAssetAPI.UnrealTypes.GameSpecificOverride]::None)
$readbackFullPath = [IO.Path]::GetFullPath($ReadbackJsonPath)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($readbackFullPath)) | Out-Null
[IO.File]::WriteAllText(
    $readbackFullPath,
    $reopened.SerializeJson([Newtonsoft.Json.Formatting]::Indented),
    (New-Object Text.UTF8Encoding($false)))

$outputFiles = @(Get-ChildItem -LiteralPath ([IO.Path]::GetDirectoryName($outputFullPath)) -File | Where-Object {
    $_.BaseName -ceq [IO.Path]::GetFileNameWithoutExtension($outputFullPath)
} | Sort-Object Extension | ForEach-Object {
    [ordered]@{ path = $_.FullName; length = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
})
$evidence = [ordered]@{
    schemaVersion = 1
    status = 'passed'
    writer = 'published-UAssetAPI-with-exact-owned-class-in-memory-schema-supplement'
    uassetApiSourceCommit = [string]$manifest.sourceCommit
    uassetApiSha256 = [string]$publishedFiles['UAssetAPI.dll']
    mappingsPath = (Resolve-Path -LiteralPath $MappingsPath).Path
    mappingsSha256 = (Get-FileHash -LiteralPath $MappingsPath -Algorithm SHA256).Hash
    ownedClassReadbackPath = (Resolve-Path -LiteralPath $OwnedClassReadbackPath).Path
    ownedClassReadbackSha256 = $actualClassReadbackSha
    ownedClassPackage = $ownedClassPackage
    ownedClass = $ownedClassName
    parentClass = $parentClassName
    localProperty = [ordered]@{ index = 0; name = $ownedPropertyName; type = 'BoolProperty'; arrayIndex = 0; arraySize = 1; flags = $expectedOwnedPropertyFlags }
    schemaCountBefore = $schemaCountBefore
    schemaCountAfter = $mappings.Schemas.Count
    parentSchemaBeforeSha256 = $parentBeforeHash
    parentSchemaAfterSha256 = $parentAfterHash
    nativeBaseSchemaBeforeSha256 = $nativeBeforeHash
    nativeBaseSchemaAfterSha256 = $nativeAfterHash
    patchedJsonSha256 = (Get-FileHash -LiteralPath $PatchedJsonPath -Algorithm SHA256).Hash
    readbackJsonPath = $readbackFullPath
    readbackJsonSha256 = (Get-FileHash -LiteralPath $readbackFullPath -Algorithm SHA256).Hash
    outputs = $outputFiles
}
$evidenceFullPath = [IO.Path]::GetFullPath($EvidencePath)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($evidenceFullPath)) | Out-Null
[IO.File]::WriteAllText(
    $evidenceFullPath,
    (($evidence | ConvertTo-Json -Depth 12) + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))
$evidence | ConvertTo-Json -Depth 12
