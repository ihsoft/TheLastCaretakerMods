[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ContractPath,
    [Parameter(Mandatory=$true)][string[]]$AssetJsonPath,
    [Parameter(Mandatory=$true)][string]$OutputPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Get-ExactProperty($Object, [string]$Name) {
    @($Object.PSObject.Properties | Where-Object { $_.Name -ceq $Name })
}

function Require-ExactFields($Object, [string[]]$Expected, [string]$Label) {
    $actual = @($Object.PSObject.Properties.Name | Sort-Object)
    if (Compare-Object $actual @($Expected | Sort-Object)) {
        throw "$Label has an unsupported schema."
    }
}

function Read-BoolValue($Value, [string]$Label) {
    if ($Value -isnot [bool]) { throw "$Label must be a JSON boolean." }
    if ([bool]$Value) { 'True' } else { 'False' }
}

function Read-PackageFlags($Value, [string]$Label) {
    $known = @{
        'PKG_None' = [uint32]0
        'PKG_Cooked' = [uint32]0x00000200
        'PKG_UnversionedProperties' = [uint32]0x00002000
        'PKG_RequiresLocalizationGather' = [uint32]0x00040000
        'PKG_FilterEditorOnly' = [uint32]2147483648
    }
    $result = [uint32]0
    $names = @(([string]$Value).Split(',') | ForEach-Object { $_.Trim() } |
        Where-Object { $_ })
    if ($names.Count -eq 0) { throw "$Label has no package flags." }
    foreach ($name in $names) {
        if (-not $known.ContainsKey($name)) {
            throw "$Label uses unsupported package flag $name."
        }
        $result = $result -bor $known[$name]
    }
    [uint32]$result
}

function Get-PropertyMap($Export, [string]$Label) {
    $map = @{}
    foreach ($property in @($Export.Data)) {
        $name = [string]$property.Name
        if ([string]::IsNullOrWhiteSpace($name) -or $map.ContainsKey($name)) {
            throw "$Label has an unnamed or duplicate serialized property."
        }
        $map[$name] = $property
    }
    $map
}

function Get-ValueOrFallback($Properties, $Fallbacks, [string]$Name,
    [string]$Label) {
    if ($Properties.ContainsKey($Name)) { return $Properties[$Name].Value }
    $fallback = @(Get-ExactProperty $Fallbacks $Name)
    if ($fallback.Count -eq 1) { return $fallback[0].Value }
    throw "$Label is missing required registry value $Name."
}

$contract = Get-Content -LiteralPath $ContractPath -Raw | ConvertFrom-Json
Require-ExactFields $contract @(
    'assets','registry','revalidateWhen','schemaVersion','serialization') 'Contract'
if ([int]$contract.schemaVersion -ne 2) { throw 'Unsupported contract version.' }
Require-ExactFields $contract.registry @(
    'chunkIds','classPolicies','commonFallbacks','filterEditorOnly',
    'formatVersion','versionRange') 'Registry policy'
if ([int]$contract.registry.formatVersion -ne 24 -or
    [bool]$contract.registry.filterEditorOnly -ne $true -or
    @($contract.registry.chunkIds).Count -ne 1 -or
    [int]$contract.registry.chunkIds[0] -ne 0) {
    throw 'Unsupported registry writer/deployment policy.'
}
$assets = @($contract.assets)
if ($assets.Count -ne 3 -or $AssetJsonPath.Count -ne $assets.Count) {
    throw 'Registry metadata requires exactly three contracted assets.'
}

$records = @()
for ($index = 0; $index -lt $assets.Count; $index++) {
    $asset = $assets[$index]
    Require-ExactFields $asset @('class','object','package','sourceFile') "Asset[$index]"
    $source = Get-Content -LiteralPath $AssetJsonPath[$index] -Raw | ConvertFrom-Json
    $primary = @($source.Exports | Where-Object {
        [string]$_.ObjectName -ceq [string]$asset.object
    })
    if ([string]$source.FolderName -cne [string]$asset.package -or
        $primary.Count -ne 1 -or
        @($source.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count -ne 0) {
        throw "Asset[$index] has the wrong package or an ambiguous primary export."
    }
    $classIndex = [int]$primary[0].ClassIndex
    $importIndex = -$classIndex - 1
    if ($classIndex -ge 0 -or $importIndex -lt 0 -or
        $importIndex -ge @($source.Imports).Count) {
        throw "Asset[$index] has an invalid native class reference."
    }
    $classImport = $source.Imports[$importIndex]
    if ([string]$classImport.ObjectName -cne [string]$asset.class -or
        [string]$classImport.ClassName -cne 'Class' -or
        [string]$classImport.ClassPackage -cne '/Script/CoreUObject') {
        throw "Asset[$index] has the wrong native class."
    }
    $voyagePackage = @($source.Imports | Where-Object {
        [string]$_.ObjectName -ceq '/Script/Voyage' -and
        [string]$_.ClassName -ceq 'Package' -and [int]$_.OuterIndex -eq 0
    })
    if ($voyagePackage.Count -ne 1 -or
        [int]$classImport.OuterIndex -ne -([array]::IndexOf(@($source.Imports), $voyagePackage[0]) + 1)) {
        throw "Asset[$index] native class is not owned by /Script/Voyage."
    }
    $policyProperty = @(Get-ExactProperty $contract.registry.classPolicies ([string]$asset.class))
    if ($policyProperty.Count -ne 1) {
        throw "Asset[$index] has no registry class policy."
    }
    $policy = $policyProperty[0].Value
    Require-ExactFields $policy @('fallbacks','primaryAssetType') "Policy $($asset.class)"
    $properties = Get-PropertyMap $primary[0] "Asset[$index]"
    $primaryType = [string]$policy.primaryAssetType
    if ($properties.ContainsKey('Type')) {
        $typeProperty = $properties['Type']
        $typeName = @($typeProperty.Value | Where-Object {
            [string]$_.Name -ceq 'Name'
        })
        if ([string]$typeProperty.StructType -cne 'PrimaryAssetType' -or
            $typeName.Count -ne 1 -or [string]$typeName[0].Value -cne $primaryType) {
            throw "Asset[$index] serialized primary asset type is invalid."
        }
    }
    $packageName = [string]$asset.package
    $lastSlash = $packageName.LastIndexOf('/')
    if ($lastSlash -le 0 -or $lastSlash -eq $packageName.Length - 1) {
        throw "Asset[$index] package identity is invalid."
    }
    $packagePath = $packageName.Substring(0, $lastSlash)
    $assetName = [string]$asset.object
    $tags = [ordered]@{
        bExcludeFromDemo = Read-BoolValue (
            Get-ValueOrFallback $properties $contract.registry.commonFallbacks `
                'bExcludeFromDemo' "Asset[$index]") "Asset[$index].bExcludeFromDemo"
        bExcludeFromDistribution = Read-BoolValue (
            Get-ValueOrFallback $properties $contract.registry.commonFallbacks `
                'bExcludeFromDistribution' "Asset[$index]") "Asset[$index].bExcludeFromDistribution"
        NativeClass = "/Script/CoreUObject.Class'/Script/Voyage.$($asset.class)'"
        PrimaryAssetName = $assetName
        PrimaryAssetType = $primaryType
        VersionRange = [string]$contract.registry.versionRange
    }
    if ($primaryType -ceq 'Item') {
        foreach ($name in @('bAllowFabricationOnTop','bCanGainLootExp',
                'bIsDestructible','bIsDismantlable','bIsRecyclable','bIsRepairable')) {
            $tags[$name] = Read-BoolValue (
                Get-ValueOrFallback $properties $policy.fallbacks $name "Asset[$index]") `
                "Asset[$index].$name"
        }
        $tags['CanGainLootExp'] = $tags['bCanGainLootExp']
        $tags['IsDestructible'] = $tags['bIsDestructible']
        $tags['IsDismantlable'] = $tags['bIsDismantlable']
        $tags['IsRecyclable'] = $tags['bIsRecyclable']
        $tags['IsRepairable'] = $tags['bIsRepairable']
        $category = [string](Get-ValueOrFallback $properties $policy.fallbacks `
            'Category' "Asset[$index]")
        $quality = [string](Get-ValueOrFallback $properties $policy.fallbacks `
            'Quality' "Asset[$index]")
        if ($category.Contains('::') -or $quality.Contains('::')) {
            throw "Asset[$index] item enum values must be unqualified serialized values."
        }
        $craftFilter = Get-ValueOrFallback $properties $policy.fallbacks `
            'CraftFilter' "Asset[$index]"
        $weight = Get-ValueOrFallback $properties $policy.fallbacks `
            'Weight' "Asset[$index]"
        $tags['Category'] = 'EVoyageItemCategory::' + $category
        $tags['CraftFilter'] = ([int64]$craftFilter).ToString($invariant)
        $tags['Quality'] = 'EVoyageItemQuality::' + $quality
        $tags['Tag'] = [string](Get-ValueOrFallback $properties $policy.fallbacks `
            'Tag' "Asset[$index]")
        $tags['Weight'] = ([single]$weight).ToString('G9', $invariant)
    }
    $records += [ordered]@{
        packageName = $packageName
        packagePath = $packagePath
        assetName = $assetName
        objectPath = $packageName + '.' + $assetName
        assetClassPath = '/Script/Voyage.' + [string]$asset.class
        optionalOuterPath = 'None'
        chunkIds = @($contract.registry.chunkIds | ForEach-Object { [int]$_ })
        packageFlags = Read-PackageFlags $source.PackageFlags "Asset[$index]"
        tags = $tags
        assetBundles = @()
    }
}

$metadata = [ordered]@{
    schemaVersion = 1
    formatVersion = [int]$contract.registry.formatVersion
    filterEditorOnly = [bool]$contract.registry.filterEditorOnly
    assets = $records
}
$parent = Split-Path -Parent ([IO.Path]::GetFullPath($OutputPath))
if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $parent -Force
}
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputPath),
    ($metadata | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
[pscustomobject]@{
    status = 'passed'
    outputPath = [IO.Path]::GetFullPath($OutputPath)
    assetCount = $records.Count
}
