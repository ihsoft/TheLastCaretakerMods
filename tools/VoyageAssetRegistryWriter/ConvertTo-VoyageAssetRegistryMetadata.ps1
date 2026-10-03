[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string[]]$AssetJsonPath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [string]$PolicyPath = '',
    [int[]]$ChunkId = @()
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$invariant = [Globalization.CultureInfo]::InvariantCulture
if (-not $PolicyPath) { $PolicyPath = Join-Path $PSScriptRoot 'voyage-policy.json' }

function Get-ExactProperty($Object, [string]$Name) {
    @($Object.PSObject.Properties | Where-Object { $_.Name -ceq $Name })
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

function Get-ImportPath($Source, [int]$ClassIndex, [string]$Label) {
    if ($ClassIndex -ge 0) { throw "$Label does not reference a native imported class." }
    $imports = @($Source.Imports)
    $index = -$ClassIndex - 1
    if ($index -lt 0 -or $index -ge $imports.Count) {
        throw "$Label has an invalid class import index."
    }
    $classImport = $imports[$index]
    if ([string]$classImport.ClassPackage -cne '/Script/CoreUObject' -or
        [string]$classImport.ClassName -cne 'Class') {
        throw "$Label class import is not a native class."
    }
    $ownerIndex = [int]$classImport.OuterIndex
    if ($ownerIndex -ge 0) { throw "$Label class import has no native package owner." }
    $owner = -$ownerIndex - 1
    if ($owner -lt 0 -or $owner -ge $imports.Count) {
        throw "$Label class owner import index is invalid."
    }
    $packageImport = $imports[$owner]
    if ([int]$packageImport.OuterIndex -ne 0 -or
        [string]$packageImport.ClassPackage -cne '/Script/CoreUObject' -or
        [string]$packageImport.ClassName -cne 'Package') {
        throw "$Label class owner is not a top-level script package."
    }
    [string]$packageImport.ObjectName + '.' + [string]$classImport.ObjectName
}

$policy = Get-Content -LiteralPath $PolicyPath -Raw | ConvertFrom-Json
if ([int]$policy.schemaVersion -ne 1 -or [int]$policy.formatVersion -ne 24 -or
    [bool]$policy.filterEditorOnly -ne $true) {
    throw 'Unsupported Voyage asset-registry policy.'
}
$effectiveChunkIds = if ($ChunkId.Count -gt 0) {
    @($ChunkId)
} else {
    @($policy.defaultChunkIds | ForEach-Object { [int]$_ })
}
$classPolicies = @{}
foreach ($entry in $policy.classPolicies.PSObject.Properties) {
    $classPolicies[[string]$entry.Name] = $entry.Value
}
if ($AssetJsonPath.Count -lt 1) { throw 'At least one asset JSON path is required.' }

$records = @()
$objectPaths = New-Object 'System.Collections.Generic.HashSet[string]' `
    ([StringComparer]::OrdinalIgnoreCase)
$primaryIds = New-Object 'System.Collections.Generic.HashSet[string]' `
    ([StringComparer]::OrdinalIgnoreCase)
for ($index = 0; $index -lt $AssetJsonPath.Count; $index++) {
    $label = "Asset[$index]"
    $sourcePath = [IO.Path]::GetFullPath($AssetJsonPath[$index])
    $source = Get-Content -LiteralPath $sourcePath -Raw | ConvertFrom-Json
    $packageName = [string]$source.FolderName
    $lastSlash = $packageName.LastIndexOf('/')
    if ($lastSlash -le 0 -or $lastSlash -eq $packageName.Length - 1) {
        throw "$label has an invalid FolderName package identity."
    }
    if (@($source.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count -ne 0) {
        throw "$label contains a RawExport and is not safe to publish."
    }
    $candidates = @()
    foreach ($export in @($source.Exports)) {
        $classIndex = [int]$export.ClassIndex
        if ($classIndex -lt 0 -and [int]$export.OuterIndex -eq 0) {
            $classPath = Get-ImportPath $source $classIndex "$label export $($export.ObjectName)"
            if ($classPolicies.ContainsKey($classPath)) {
                $candidates += [pscustomobject]@{
                    export = $export
                    classPath = $classPath
                    policy = $classPolicies[$classPath]
                }
            }
        }
    }
    if ($candidates.Count -ne 1) {
        throw "$label must contain exactly one supported Voyage primary export; found $($candidates.Count)."
    }
    $candidate = $candidates[0]
    $primary = $candidate.export
    $assetName = [string]$primary.ObjectName
    if ([string]::IsNullOrWhiteSpace($assetName)) { throw "$label has no primary object name." }
    $classPolicy = $candidate.policy
    $properties = Get-PropertyMap $primary $label
    $primaryType = [string]$classPolicy.primaryAssetType
    if ($properties.ContainsKey('Type')) {
        $typeProperty = $properties['Type']
        $typeName = @($typeProperty.Value | Where-Object { [string]$_.Name -ceq 'Name' })
        if ([string]$typeProperty.StructType -cne 'PrimaryAssetType' -or
            $typeName.Count -ne 1 -or [string]$typeName[0].Value -cne $primaryType) {
            throw "$label serialized primary asset type is invalid."
        }
    }
    $objectPath = $packageName + '.' + $assetName
    $primaryId = $primaryType + ':' + $assetName
    if (-not $objectPaths.Add($objectPath)) { throw "Duplicate object path: $objectPath" }
    if (-not $primaryIds.Add($primaryId)) { throw "Duplicate primary asset ID: $primaryId" }

    $tags = [ordered]@{
        bExcludeFromDemo = Read-BoolValue (
            Get-ValueOrFallback $properties $policy.commonFallbacks `
                'bExcludeFromDemo' $label) "$label.bExcludeFromDemo"
        bExcludeFromDistribution = Read-BoolValue (
            Get-ValueOrFallback $properties $policy.commonFallbacks `
                'bExcludeFromDistribution' $label) "$label.bExcludeFromDistribution"
        NativeClass = "/Script/CoreUObject.Class'$($candidate.classPath)'"
        PrimaryAssetName = $assetName
        PrimaryAssetType = $primaryType
        VersionRange = [string]$policy.versionRange
    }
    if ($primaryType -ceq 'Item') {
        foreach ($name in @('bAllowFabricationOnTop','bCanGainLootExp',
                'bIsDestructible','bIsDismantlable','bIsRecyclable','bIsRepairable')) {
            $tags[$name] = Read-BoolValue (
                Get-ValueOrFallback $properties $classPolicy.fallbacks $name $label) `
                "$label.$name"
        }
        $tags['CanGainLootExp'] = $tags['bCanGainLootExp']
        $tags['IsDestructible'] = $tags['bIsDestructible']
        $tags['IsDismantlable'] = $tags['bIsDismantlable']
        $tags['IsRecyclable'] = $tags['bIsRecyclable']
        $tags['IsRepairable'] = $tags['bIsRepairable']
        $category = [string](Get-ValueOrFallback $properties $classPolicy.fallbacks `
            'Category' $label)
        $quality = [string](Get-ValueOrFallback $properties $classPolicy.fallbacks `
            'Quality' $label)
        if ($category.Contains('::') -or $quality.Contains('::')) {
            throw "$label item enum values must be unqualified serialized values."
        }
        $tags['Category'] = 'EVoyageItemCategory::' + $category
        $tags['CraftFilter'] = ([int64](Get-ValueOrFallback $properties `
            $classPolicy.fallbacks 'CraftFilter' $label)).ToString($invariant)
        $tags['Quality'] = 'EVoyageItemQuality::' + $quality
        $tags['Tag'] = [string](Get-ValueOrFallback $properties `
            $classPolicy.fallbacks 'Tag' $label)
        $tags['Weight'] = ([single](Get-ValueOrFallback $properties `
            $classPolicy.fallbacks 'Weight' $label)).ToString('G9', $invariant)
    }
    $records += [ordered]@{
        packageName = $packageName
        packagePath = $packageName.Substring(0, $lastSlash)
        assetName = $assetName
        objectPath = $objectPath
        assetClassPath = [string]$candidate.classPath
        optionalOuterPath = 'None'
        chunkIds = @($effectiveChunkIds)
        packageFlags = Read-PackageFlags $source.PackageFlags $label
        tags = $tags
        assetBundles = @()
    }
}

$metadata = [ordered]@{
    schemaVersion = 1
    formatVersion = [int]$policy.formatVersion
    filterEditorOnly = [bool]$policy.filterEditorOnly
    assets = $records
}
$fullOutput = [IO.Path]::GetFullPath($OutputPath)
$parent = Split-Path -Parent $fullOutput
if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $parent -Force
}
[IO.File]::WriteAllText($fullOutput, ($metadata | ConvertTo-Json -Depth 12),
    (New-Object Text.UTF8Encoding($false)))
[pscustomobject]@{
    status = 'passed'
    outputPath = $fullOutput
    assetCount = $records.Count
    primaryAssetIds = @($records | ForEach-Object {
        $_.tags.PrimaryAssetType + ':' + $_.tags.PrimaryAssetName
    })
}
