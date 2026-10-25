# INTERNAL BUILD STAGE. Validates that the six runtime-created mooring component
# templates survived cook outside the unconditional SCS node list. It performs
# a byte-identical writer roundtrip but does not patch the staged package.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RepositoryRoot,
    [Parameter(Mandatory = $true)][string]$AssetPath,
    [Parameter(Mandatory = $true)][string]$GeneratedParentAssetPath,
    [Parameter(Mandatory = $true)][string]$PitchHelperAssetPath,
    [Parameter(Mandatory = $true)][string]$StabilizationHelperAssetPath,
    [Parameter(Mandatory = $true)][string]$MappingsPath,
    [Parameter(Mandatory = $true)][string]$EvidencePath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$targetNames = @(
    'StableGyroMooringSocketRearLeft',
    'StableGyroMooringSocketRearRight',
    'StableGyroMooringPlateFrontPositiveY',
    'StableGyroMooringPlateFrontNegativeY',
    'StableGyroMooringPlateRearPositiveY',
    'StableGyroMooringPlateRearNegativeY'
)

function Get-Sha256([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

$repository = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$bundle = Join-Path $repository '.tools\bin\UAssetAPI'
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
    if ((Get-Sha256 $path) -cne $publishedFiles[$name]) {
        throw "Published dependency hash mismatch: $name"
    }
    [Reflection.Assembly]::LoadFrom($path) | Out-Null
}
$uassetApiAssembly = [AppDomain]::CurrentDomain.GetAssemblies() |
    Where-Object { $_.GetName().Name -ceq 'UAssetAPI' } |
    Select-Object -First 1
$newtonsoftAssembly = [AppDomain]::CurrentDomain.GetAssemblies() |
    Where-Object { $_.GetName().Name -ceq 'Newtonsoft.Json' } |
    Select-Object -First 1
$usmapType = $uassetApiAssembly.GetType('UAssetAPI.Unversioned.Usmap', $true)
$usmapPropertyType = $uassetApiAssembly.GetType('UAssetAPI.Unversioned.UsmapProperty', $true)
$usmapPropertyDataType = $uassetApiAssembly.GetType('UAssetAPI.Unversioned.UsmapPropertyData', $true)
$usmapPropertyKindType = $uassetApiAssembly.GetType('UAssetAPI.Unversioned.UsmapPropertyType', $true)
$usmapSchemaType = $uassetApiAssembly.GetType('UAssetAPI.Unversioned.UsmapSchema', $true)
$usmapStructKindType = $uassetApiAssembly.GetType('UAssetAPI.Unversioned.UsmapStructKind', $true)
$propertyFlagsType = $uassetApiAssembly.GetType('UAssetAPI.UnrealTypes.EPropertyFlags', $true)
$uassetType = $uassetApiAssembly.GetType('UAssetAPI.UAsset', $true)
$engineVersionType = $uassetApiAssembly.GetType('UAssetAPI.UnrealTypes.EngineVersion', $true)
$jsonFormattingType = $newtonsoftAssembly.GetType('Newtonsoft.Json.Formatting', $true)

function Assert-NoRawExports([object]$Package, [string]$Label) {
    $raw = @($Package.Exports | Where-Object { $_.'$type' -like '*RawExport*' })
    if ($raw.Count -ne 0) { throw "$Label contains $($raw.Count) RawExport records: $(@($raw.ObjectName) -join ', ')." }
}

function Get-ClassContract([object]$Package, [string]$Name, [string]$Super) {
    $classes = @($Package.Exports | Where-Object {
        $_.'$type' -like '*ClassExport*' -and $_.ObjectName -ceq $Name
    })
    if ($classes.Count -ne 1) { throw "Expected exactly one ClassExport named $Name." }
    $class = $classes[0]
    $superImport = $Package.Imports[[Math]::Abs([int]$class.SuperIndex) - 1]
    if ([string]$superImport.ObjectName -cne $Super) {
        throw "$Name has unexpected superclass '$($superImport.ObjectName)'."
    }
    [pscustomobject]@{
        Name = $Name
        Super = $Super
        Fields = @($class.LoadedProperties | ForEach-Object {
            [pscustomobject]@{
                Name = [string]$_.Name
                Type = [string]$_.SerializedType
                Flags = [string]$_.PropertyFlags
            }
        })
    }
}

function Set-ExactSchema(
    [object]$Mappings,
    [object]$Contract
) {
    $dictionaryType = [Collections.Concurrent.ConcurrentDictionary``2].MakeGenericType(
        [int], $usmapPropertyType)
    $properties = $dictionaryType::new()
    $index = 0
    foreach ($field in @($Contract.Fields)) {
        $type = [Enum]::Parse($usmapPropertyKindType, [string]$field.Type)
        $data = $usmapPropertyDataType::new($type)
        $property = $usmapPropertyType::new(
            [string]$field.Name, $index, 0, 1, $data)
        if (-not [string]::IsNullOrWhiteSpace([string]$field.Flags)) {
            $property.PropertyFlags = [Enum]::Parse($propertyFlagsType, [string]$field.Flags)
        }
        if (-not $properties.TryAdd($index, $property)) {
            throw "Could not add schema property $($Contract.Name)[$index]."
        }
        $index++
    }
    $schema = $usmapSchemaType::new(
        [string]$Contract.Name, [string]$Contract.Super, $index, $properties,
        $Mappings.AreFNamesCaseInsensitive, $null, $false)
    $schema.StructKind = [Enum]::Parse($usmapStructKindType, 'UClass')
    if ($Mappings.Schemas.ContainsKey([string]$Contract.Name)) {
        $removed = $null
        if (-not $Mappings.Schemas.TryRemove([string]$Contract.Name, [ref]$removed)) {
            throw "Could not replace colliding schema $($Contract.Name)."
        }
    }
    if (-not $Mappings.Schemas.TryAdd([string]$Contract.Name, $schema)) {
        throw "Could not add schema $($Contract.Name)."
    }
}

function New-SupplementedMappings([object[]]$Contracts) {
    $result = $usmapType::new((Resolve-Path -LiteralPath $MappingsPath).Path)
    foreach ($contract in $Contracts) { Set-ExactSchema $result $contract }
    $result
}

function Read-PackageJson([string]$Path, [object]$Mappings) {
    $asset = $uassetType::new(
        (Resolve-Path -LiteralPath $Path).Path,
        [Enum]::Parse($engineVersionType, 'VER_UE5_8'),
        $Mappings)
    $asset.SerializeJson([Enum]::Parse($jsonFormattingType, 'Indented')) | ConvertFrom-Json
}

function Convert-PackageToJson([string]$Path, [string]$Label) {
    $jsonPath = Join-Path $work ($Label + '.json')
    & $boundedTool -Executable $guiPath -Arguments @(
        '--portable', 'tojson', (Resolve-Path -LiteralPath $Path).Path,
        $jsonPath, '5.8', (Resolve-Path -LiteralPath $MappingsPath).Path
    ) -MemoryLimitMB 2048 -TimeoutSeconds 45 | Out-Null
    Get-Content -LiteralPath $jsonPath -Raw | ConvertFrom-Json
}

function Write-PackageJson(
    [object]$Package,
    [string]$OutputPath,
    [object]$Mappings
) {
    $json = $Package | ConvertTo-Json -Depth 100
    $asset = $uassetType::DeserializeJson($json)
    $asset.Mappings = $Mappings
    $asset.SetSerializationEngineVersion([Enum]::Parse($engineVersionType, 'VER_UE5_8'))
    $asset.FilePath = $OutputPath + '.json'
    $asset.Write($OutputPath)
}

$assetFullPath = (Resolve-Path -LiteralPath $AssetPath).Path
$assetExportPath = [IO.Path]::ChangeExtension($assetFullPath, '.uexp')
foreach ($required in @(
    $assetFullPath, $assetExportPath, $GeneratedParentAssetPath,
    $PitchHelperAssetPath, $StabilizationHelperAssetPath, $MappingsPath
)) { Resolve-Path -LiteralPath $required | Out-Null }

$workRoot = [IO.Path]::GetFullPath([IO.Path]::GetDirectoryName($assetFullPath))
$work = [IO.Path]::GetFullPath((Join-Path $workRoot ('.stablegyro-parent-patch-' + [guid]::NewGuid().ToString('N'))))
if (-not $work.StartsWith($workRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not (Split-Path -Leaf $work).StartsWith('.stablegyro-parent-patch-', [StringComparison]::Ordinal)) {
    throw "Refusing unsafe patch work directory: $work"
}
[IO.Directory]::CreateDirectory($work) | Out-Null
try {
    $guiPath = Join-Path $repository '.tools\bin\UAssetGUI.exe'
    $guiManifest = Get-Content -LiteralPath (Join-Path $repository '.tools\bin\UAssetGUI.manifest.json') -Raw | ConvertFrom-Json
    if ([string]$guiManifest.kind -cne 'Voyage canonical UAssetGUI binary' -or
        [string]$guiManifest.apiCommit -cne [string]$manifest.sourceCommit -or
        (Get-Sha256 $guiPath) -cne [string]$guiManifest.executableSha256) {
        throw 'Published UAssetGUI binary does not match its accepted manifest.'
    }
    $boundedTool = Join-Path $repository 'tools\Invoke-VoyageBoundedTool.ps1'
    $childDiscovery = Convert-PackageToJson $assetFullPath 'child-discovery'
    $pitch = Convert-PackageToJson $PitchHelperAssetPath 'pitch-helper'
    $stabilization = Convert-PackageToJson $StabilizationHelperAssetPath 'stabilization-helper'
    $parent = Convert-PackageToJson $GeneratedParentAssetPath 'generated-parent'
    Assert-NoRawExports $pitch 'Pitch helper schema source'
    Assert-NoRawExports $stabilization 'Stabilization helper schema source'
    $discoveryRaw = @($childDiscovery.Exports | Where-Object { $_.'$type' -like '*RawExport*' })
    if ($discoveryRaw.Count -gt 1 -or
        ($discoveryRaw.Count -eq 1 -and
         $discoveryRaw[0].ObjectName -cne 'StableGyroPitchInputHelper_GEN_VARIABLE_PitchInputActor_C_CAT')) {
        throw 'Child schema discovery produced an unexpected RawExport set.'
    }

    $parentContract = Get-ClassContract $parent 'BP_GyroCopter_Possessable_C' 'VoyageVehicleGyroCopter'
    if ($parentContract.Fields.Count -ne 1 -or
        $parentContract.Fields[0].Name -cne 'DefaultSceneRoot' -or
        $parentContract.Fields[0].Type -cne 'ObjectProperty') {
        throw 'Generated parent schema is not the expected one-DefaultSceneRoot contract.'
    }
    $contracts = @(
        [pscustomobject]@{ Name = 'VoyageVehiclePawn'; Super = 'Pawn'; Fields = @() },
        [pscustomobject]@{ Name = 'VoyageVehicleGyroCopter'; Super = 'VoyageVehiclePawn'; Fields = @(
            [pscustomobject]@{Name='TiltForwardInput';Type='FloatProperty';Flags=''},
            [pscustomobject]@{Name='TiltInput';Type='FloatProperty';Flags=''},
            [pscustomobject]@{Name='bAutoHoverCollective';Type='BoolProperty';Flags=''},
            [pscustomobject]@{Name='bAltitudeAssist';Type='BoolProperty';Flags=''},
            [pscustomobject]@{Name='Collective';Type='FloatProperty';Flags=''},
            [pscustomobject]@{Name='HoverPitchScale';Type='FloatProperty';Flags=''},
            [pscustomobject]@{Name='CurrentThrottle';Type='FloatProperty';Flags=''},
            [pscustomobject]@{Name='bEngineRunning';Type='BoolProperty';Flags=''},
            [pscustomobject]@{Name='PropellerVelocity';Type='FloatProperty';Flags=''},
            [pscustomobject]@{Name='DropCargoTriggerTime';Type='FloatProperty';Flags=''}
        )},
        $parentContract,
        (Get-ClassContract $pitch 'PitchInputActor_C' 'Actor'),
        (Get-ClassContract $stabilization 'ModActor_C' 'Actor'),
        (Get-ClassContract $childDiscovery 'BP_GyroCopter_StablePilot_C' 'BP_GyroCopter_Possessable_C')
    )
    $original = Read-PackageJson $assetFullPath (New-SupplementedMappings $contracts)
    Assert-NoRawExports $original 'Cooked child input with exact source-cook schema overlay'
    $sourceHashes = [ordered]@{
        uasset = Get-Sha256 $assetFullPath
        uexp = Get-Sha256 $assetExportPath
    }
    $unchangedPath = Join-Path $work 'unchanged.uasset'
    Write-PackageJson $original $unchangedPath (New-SupplementedMappings $contracts)
    $unchanged = Read-PackageJson $unchangedPath (New-SupplementedMappings $contracts)
    Assert-NoRawExports $unchanged 'Unchanged reopen'
    if ((Get-Sha256 $unchangedPath) -cne $sourceHashes.uasset -or
        (Get-Sha256 ([IO.Path]::ChangeExtension($unchangedPath, '.uexp'))) -cne $sourceHashes.uexp) {
        throw 'Published UAssetAPI unchanged roundtrip was not byte-identical.'
    }
    foreach ($field in @('ObjectVersion', 'ObjectVersionUE5', 'CustomVersionContainer')) {
        if (($original.$field | ConvertTo-Json -Depth 100 -Compress) -cne
            ($unchanged.$field | ConvertTo-Json -Depth 100 -Compress)) {
            throw "Unchanged roundtrip changed $field."
        }
    }

    $generatedClasses = @($original.Exports | Where-Object {
        $_.'$type' -like '*ClassExport*' -and
        [string]$_.ObjectName -ceq 'BP_GyroCopter_StablePilot_C'
    })
    if ($generatedClasses.Count -ne 1) {
        throw 'Expected exactly one replacement BlueprintGeneratedClass export.'
    }
    $componentTemplateProperties = @($generatedClasses[0].Data | Where-Object {
        [string]$_.Name -ceq 'ComponentTemplates'
    })
    if ($componentTemplateProperties.Count -ne 1 -or
        @($componentTemplateProperties[0].Value).Count -ne $targetNames.Count) {
        throw 'Generated class does not reference exactly six component templates.'
    }
    $templateReferenceNames = @($componentTemplateProperties[0].Value | ForEach-Object {
        $packageIndex = [int]$_.Value
        if ($packageIndex -le 0 -or $packageIndex -gt @($original.Exports).Count) {
            throw "Invalid component-template package index $packageIndex."
        }
        [string]$original.Exports[$packageIndex - 1].ObjectName
    })
    $actualTemplateNames = @($templateReferenceNames | ForEach-Object { [string]$_ } | Sort-Object)
    $expectedTemplateNames = @($targetNames | ForEach-Object { [string]$_ } | Sort-Object)
    if (($actualTemplateNames -join "`n") -cne ($expectedTemplateNames -join "`n")) {
        throw "Generated class component-template references differ from the exact six mooring templates: $($templateReferenceNames -join ', ')."
    }

    $templateSummaries = [ordered]@{}
    foreach ($target in $targetNames) {
        $scsNodes = @($original.Exports | Where-Object {
            @($_.Data | Where-Object {
                $_.Name -ceq 'InternalVariableName' -and [string]$_.Value -ceq $target
            }).Count -eq 1
        })
        if ($scsNodes.Count -ne 0) {
            throw "$target is still serialized as an unconditional SCS node."
        }
        $templateExports = @($original.Exports | Where-Object {
            [string]$_.ObjectName -ceq $target
        })
        if ($templateExports.Count -ne 1 -or
            $templateExports[0].'$type' -notlike '*NormalExport*') {
            throw "Expected exactly one normal component-template export for $target."
        }
        $templateSummaries[$target] = [ordered]@{
            objectName = [string]$templateExports[0].ObjectName
            exportType = [string]$templateExports[0].'$type'
            referencedByGeneratedClass = $true
        }
    }
    $outputHashes = [ordered]@{
        uasset = Get-Sha256 $assetFullPath
        uexp = Get-Sha256 $assetExportPath
    }
    if ($outputHashes.uasset -cne $sourceHashes.uasset -or
        $outputHashes.uexp -cne $sourceHashes.uexp) {
        throw 'Validation-only mooring stage changed the staged child package.'
    }

    $evidence = [ordered]@{
        schemaVersion = 2; status = 'passed'
        writer = 'published-UAssetAPI-validation-only-with-source-cook-schema-overlay'
        uassetApiSourceCommit = [string]$manifest.sourceCommit
        uassetApiSha256 = [string]$publishedFiles['UAssetAPI.dll']
        mappingsPath = (Resolve-Path -LiteralPath $MappingsPath).Path
        mappingsSha256 = Get-Sha256 $MappingsPath
        unchangedRoundtripByteIdentical = $true
        rawExports = 0
        sourceHashes = $sourceHashes
        outputHashes = $outputHashes
        exports = @($original.Exports).Count
        imports = @($original.Imports).Count
        unconditionalMooringScsNodes = 0
        generatedClassComponentTemplateCount = $templateReferenceNames.Count
        runtimeCreatedComponentTemplates = $templateSummaries
        semanticDifference = 'none; validation-only stage for runtime-created component templates'
    }
    $evidenceFullPath = [IO.Path]::GetFullPath($EvidencePath)
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($evidenceFullPath)) | Out-Null
    [IO.File]::WriteAllText($evidenceFullPath, (($evidence | ConvertTo-Json -Depth 10) + [Environment]::NewLine), (New-Object Text.UTF8Encoding($false)))
    [pscustomobject]$evidence
}
finally {
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($workRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
        -not (Split-Path -Leaf $resolvedWork).StartsWith('.stablegyro-parent-patch-', [StringComparison]::Ordinal)) {
        throw "Refusing unsafe recursive cleanup target: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
