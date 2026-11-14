[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Container, [Parameter(Mandatory=$true)][string]$OutputRoot, [Parameter(Mandatory=$true)][string]$ModelInventory, [Parameter(Mandatory=$true)][string]$AmmoSourceJson, [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
$boundary = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp\Railgun')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($boundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Semantic scratch output must be below Tmp/Railgun.' }
if (Test-Path -LiteralPath $output) { throw 'Semantic output exists; use a fresh candidate.' }
$null = New-Item -ItemType Directory -Path $output
# Validate the produced package against this run's authored inputs. Gameplay
# graph structure and presentation choices are not package invariants.
$evidence = @()
$materialEvidence = @()
$gunItemPackage = '/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$ammoPackage = '/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod'
function Read-Candidate([string]$Query) {
    $result = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
        -Query $Query -Source Mod -ModContainer $Container -GameRoot $GameRoot `
        -ModInspectionRoot (Join-Path $output 'asset-inspections') `
        -AsJson) | ConvertFrom-Json
    $script:evidence += $result
    # Windows PowerShell5.1 ConvertFrom-Json returns a top-level array as one
    # pipeline value. Emit each export explicitly before caller filtering.
    $exports = Get-Content -LiteralPath $result.jsonPath -Raw | ConvertFrom-Json
    foreach ($export in $exports) { $export }
}
function Require([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function PropertyNames($Value) {
    if ($Value.PSObject.Properties.Name -contains 'Properties') { @($Value.Properties.PSObject.Properties.Name) }
}
$shell = @(Read-Candidate '/Game/Mods/Railgun/Module/BP_Module_Railgun')
$class = @($shell | Where-Object { $_.Type -ceq 'BlueprintGeneratedClass' })
Require ($class.Count -eq 1 -and $class[0].SuperStruct.ObjectName -ceq "Class'VoyageModuleActor'") 'Shell native parent mismatch.'
$module = @($shell | Where-Object { $_.Name -ceq 'ModuleComponent' })
Require ($module.Count -eq 1 -and $module[0].Properties.ItemAsset.ObjectPath -ceq ($gunItemPackage + '.0')) 'Native module ItemAsset mismatch.'
$inventory = Get-Content -LiteralPath $ModelInventory -Raw | ConvertFrom-Json
$constructionScript = @($shell | Where-Object {
    $_.Type -ceq 'SimpleConstructionScript'
})
Require ($constructionScript.Count -eq 1) `
    'Expected one shell construction script.'
$modelRootName = [string]$inventory.roles.root
$modelRootNode = @($shell | Where-Object {
    $_.Type -ceq 'SCS_Node' -and
    $_.Properties.InternalVariableName -ceq $modelRootName
})
Require ($modelRootNode.Count -eq 1) 'Model root SCS node is missing.'
$modelRootSuffix = '.' + $modelRootNode[0].Name + "'"
$rootNodes = @($constructionScript[0].Properties.RootNodes)
Require ($rootNodes.Count -ge 1 -and
    $rootNodes[0].ObjectName.EndsWith($modelRootSuffix)) `
    'Model registry root is not the shell scene root.'
$modelRootComponent = @($shell | Where-Object {
    $_.Name -ceq ($modelRootName + '_GEN_VARIABLE')
})
$modelRootInventory = @($inventory.components | Where-Object {
    $_.name -ceq $modelRootName
})
Require ($modelRootInventory.Count -eq 1 -and
    $modelRootComponent.Count -eq 1) `
    'Model registry root is missing or duplicated.'
$expectedModelRootType = if ([string]::IsNullOrWhiteSpace(
    [string]$modelRootInventory[0].mesh)) {
    'SceneComponent'
} else {
    'StaticMeshComponent'
}
Require ($modelRootComponent[0].Type -ceq $expectedModelRootType) `
    'Model registry root type does not match the imported root.'
Require (@($modelRootComponent[0].Properties.ComponentTags) -ccontains
    'Railgun.Model.Root') 'Model root role tag is missing.'
$rootInventoryEntries = @($inventory.components | Where-Object {
    [string]::IsNullOrEmpty([string]$_.parent)
})
Require ($rootInventoryEntries.Count -eq 1 -and
    $rootInventoryEntries[0].name -ceq $modelRootName) `
    'Model inventory does not identify exactly one root.'
$ammoPickupInventory = $inventory.ammoPickup
$ammoPickupPackage = '/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette'
$ammoPickupClass = $ammoPickupPackage + '.BP_RailgunAmmoCassette_C'
$ammoSource = Get-Content -LiteralPath $AmmoSourceJson -Raw | ConvertFrom-Json
$ammoSourcePrimary = @($ammoSource.Exports | Where-Object {
    $_.ObjectName -ceq 'DA_Ammo_Railgun_FullRod'
})
Require ($ammoSourcePrimary.Count -eq 1) `
    'Authored ammo JSON primary export is missing or duplicated.'
$ammoSourceDropProperties = @($ammoSourcePrimary[0].Data | Where-Object {
    $_.Name -ceq 'DropVariations'
})
Require ($ammoSourceDropProperties.Count -eq 1) `
    'Authored ammo JSON has no single DropVariations property.'
$ammoSourceRenderAssets = @($ammoSourceDropProperties[0].Value.Value |
    Where-Object { $_.Name -ceq 'RenderAsset' })
Require ($ammoSourceRenderAssets.Count -eq 1) `
    'Authored ammo JSON has no single supported RenderAsset.'
$ammoSourceRenderIdentity = $ammoSourceRenderAssets[0].Value.AssetPath
$ammoPickupCarrierMesh = [string]$ammoSourceRenderIdentity.PackageName + '.' +
    [string]$ammoSourceRenderIdentity.AssetName
Require ($null -ne $ammoPickupInventory) `
    'Shared composite ammo pickup evidence is missing.'
Require ($ammoPickupInventory.root -ceq $inventory.roles.ammoPickupRoot -and
    $ammoPickupInventory.carrier -ceq $inventory.roles.ammoPickupCarrier) `
    'Ammo pickup role evidence differs from the model role registry.'
Require ($ammoPickupInventory.package -ceq $ammoPickupPackage -and
    $ammoPickupInventory.classObjectPath -ceq $ammoPickupClass -and
    $ammoPickupInventory.carrierMesh -ceq $ammoPickupCarrierMesh) `
    'Ammo pickup owned actor or shared carrier identity mismatch.'
Require (@($ammoPickupInventory.parts).Count -gt 0) `
    'Ammo pickup has no non-carrier render descendants.'
Require (-not (@($ammoPickupInventory.parts.sourceName) -ccontains
    [string]$ammoPickupInventory.carrier)) `
    'Ammo pickup duplicated its native carrier as an SCS child.'
$ammoPickupCollisionSize = @($ammoPickupInventory.collisionSizeCm)
$ammoPickupCollisionCenter = @($ammoPickupInventory.collisionCenterCm)
Require ($ammoPickupCollisionSize.Count -eq 3 -and
    $ammoPickupCollisionCenter.Count -eq 3) `
    'Ammo pickup collision bounds are incomplete.'
foreach ($extent in $ammoPickupCollisionSize) {
    $value = [double]$extent
    Require ($value -gt 0.0 -and -not [double]::IsNaN($value) -and
        -not [double]::IsInfinity($value)) `
        'Ammo pickup collision size is not finite and nondegenerate.'
}
foreach ($center in $ammoPickupCollisionCenter) {
    $value = [double]$center
    Require (-not [double]::IsNaN($value) -and
        -not [double]::IsInfinity($value)) `
        'Ammo pickup collision center is not finite.'
}
foreach ($part in @($ammoPickupInventory.parts)) {
    foreach ($field in @('location','rotation','scale')) {
        $values = @($part.$field)
        Require ($values.Count -eq 3) `
            ('Ammo pickup transform is incomplete: ' + $part.sourceName + '/' + $field)
        foreach ($coordinate in $values) {
            $value = [double]$coordinate
            Require (-not [double]::IsNaN($value) -and
                -not [double]::IsInfinity($value)) `
                ('Ammo pickup transform is not finite: ' + $part.sourceName + '/' + $field)
        }
    }
}
$pickupAssetPaths = @($ammoPickupInventory.carrierMesh) +
    @($ammoPickupInventory.carrierMaterials) +
    @($ammoPickupInventory.parts | ForEach-Object {
        @($_.mesh) + @($_.materials)
    })
foreach ($pickupAssetPath in $pickupAssetPaths) {
    if ([string]::IsNullOrWhiteSpace([string]$pickupAssetPath)) { continue }
    $separator = ([string]$pickupAssetPath).LastIndexOf('.')
    Require ($separator -gt 0 -and @($inventory.packages) -ccontains
        ([string]$pickupAssetPath).Substring(0, $separator)) `
        ('Ammo pickup shared dependency is absent: ' + $pickupAssetPath)
}
function Require-Child([string]$ParentName, [string]$ChildName) {
    $parent = @($shell | Where-Object { $_.Type -ceq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq $ParentName })
    $child = @($shell | Where-Object { $_.Type -ceq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq $ChildName })
    Require ($parent.Count -eq 1 -and $child.Count -eq 1) ('Missing hierarchy nodes: ' + $ChildName)
    $suffix = '.' + $child[0].Name + "'"
    Require (@($parent[0].Properties.ChildNodes | Where-Object { $_.ObjectName.EndsWith($suffix) }).Count -eq 1) ('Parent mismatch: ' + $ChildName)
}
foreach ($expected in $inventory.components) {
    $component = @($shell | Where-Object { $_.Name -ceq ($expected.name + '_GEN_VARIABLE') })
    Require ($component.Count -eq 1) ('Missing GLB component: ' + $expected.name)
    if ([string]::IsNullOrEmpty([string]$expected.parent)) {
        Require ($expected.name -ceq $modelRootName) `
            ('Unexpected root component: ' + $expected.name)
    } else {
        Require-Child $expected.parent $expected.name
    }
    foreach ($field in @('location','rotation','scale')) {
        $property = @{location='RelativeLocation';rotation='RelativeRotation';scale='RelativeScale3D'}[$field]
        $axes = if ($field -ceq 'rotation') { @('Pitch','Yaw','Roll') } else { @('X','Y','Z') }
        for ($i=0; $i -lt 3; $i++) {
            $actual = if ($field -ceq 'scale') { 1.0 } else { 0.0 }
            if ((PropertyNames $component[0]) -contains $property) { $actual = $component[0].Properties.$property.($axes[$i]) }
            Require ([Math]::Abs($actual - $expected.$field[$i]) -lt 0.001) ('GLB transform mismatch: ' + $expected.name + '/' + $field)
        }
    }
    if ($expected.mesh) {
        Require ($component[0].Type -ceq 'StaticMeshComponent') 'Mesh type mismatch.'
        $package = $expected.mesh.Substring(0,$expected.mesh.LastIndexOf('.'))
        Require ($component[0].Properties.StaticMesh.ObjectPath.StartsWith($package + '.')) ('Mesh mismatch: ' + $expected.name)
        if ($package -cne $inventory.collisionMesh) {
            Require ($component[0].Properties.BodyInstance.CollisionEnabled -ceq 'ECollisionEnabled::NoCollision') ('Moving collision: ' + $expected.name)
        }
    }
}
$mesh = @(Read-Candidate $inventory.collisionMesh)
$body = @($mesh | Where-Object { $_.Type -ceq 'BodySetup' })
Require ($body.Count -eq 1) 'Missing collision BodySetup.'
$boxes = @($body[0].Properties.AggGeom.BoxElems)
Require ($boxes.Count -eq 1) 'Expected one fabricator collision box.'
$config = $inventory.fabricatorCollision
for ($i=0; $i -lt 3; $i++) {
    $axis = @('X','Y','Z')[$i]
    Require ([Math]::Abs($boxes[0].$axis - $config.sizeCm[$i]) -lt 0.001) 'Fabricator collision dimensions mismatch.'
    Require ([Math]::Abs($boxes[0].Center.$axis - $config.centerCm[$i]) -lt 0.001) 'Fabricator collision center mismatch.'
}
Require ($body[0].Properties.CollisionTraceFlag -cin @('ECollisionTraceFlag::CTF_UseSimpleAsComplex','CTF_UseSimpleAsComplex')) 'Fabricator collision mode changed.'
$ammoPickupCarrierPackage = $ammoPickupCarrierMesh.Substring(0,
    $ammoPickupCarrierMesh.LastIndexOf('.'))
$ammoPickupCarrier = @(Read-Candidate $ammoPickupCarrierPackage)
$ammoPickupCarrierStaticMesh = @($ammoPickupCarrier | Where-Object {
    $_.Type -ceq 'StaticMesh'
})
$ammoPickupCarrierBody = @($ammoPickupCarrier | Where-Object {
    $_.Type -ceq 'BodySetup'
})
Require ($ammoPickupCarrierStaticMesh.Count -eq 1 -and
    $ammoPickupCarrierBody.Count -eq 1) `
    'Cooked shared ammo carrier mesh or BodySetup is missing.'
$ammoPickupBoxes = @($ammoPickupCarrierBody[0].Properties.AggGeom.BoxElems)
Require ($ammoPickupBoxes.Count -eq 1) `
    'Shared ammo carrier must own exactly one composite simple box.'
for ($i=0; $i -lt 3; $i++) {
    $axis = @('X','Y','Z')[$i]
    Require ([Math]::Abs([double]$ammoPickupBoxes[0].$axis -
        [double]$ammoPickupCollisionSize[$i]) -lt 0.001) `
        ('Ammo pickup collision size mismatch: ' + $axis)
    Require ([Math]::Abs([double]$ammoPickupBoxes[0].Center.$axis -
        [double]$ammoPickupCollisionCenter[$i]) -lt 0.001) `
        ('Ammo pickup collision center mismatch: ' + $axis)
}
Require ($ammoPickupCarrierBody[0].Properties.CollisionTraceFlag -cin @(
    'ECollisionTraceFlag::CTF_UseSimpleAsComplex','CTF_UseSimpleAsComplex')) `
    'Shared ammo carrier collision mode changed.'
$ammoPickup = @(Read-Candidate $ammoPickupPackage)
$ammoPickupClasses = @($ammoPickup | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass' -and
    $_.Name -ceq 'BP_RailgunAmmoCassette_C'
})
Require ($ammoPickupClasses.Count -eq 1 -and
    $ammoPickupClasses[0].Super.ObjectPath -ceq
        '/Game/Blueprints/BP_DynamicMeshActor.0') `
    'Ammo pickup lost its exact stock Blueprint parent.'
$ammoPickupNodes = @($ammoPickup | Where-Object {
    if ($_.Type -cne 'SCS_Node') { return $false }
    $propertyNames = @($_.Properties.PSObject.Properties.Name)
    ($propertyNames -ccontains 'ParentComponentOrVariableName') -and
        $_.Properties.ParentComponentOrVariableName -ceq 'MeshComponent' -and
        ($propertyNames -ccontains 'bIsParentComponentNative') -and
        $_.Properties.bIsParentComponentNative -eq $true
})
$ammoPickupMeshes = @($ammoPickup | Where-Object {
    $_.Type -ceq 'StaticMeshComponent'
})
$expectedPickupParts = @()
foreach ($pickupPart in $ammoPickupInventory.parts) {
    $expectedPickupParts += $pickupPart
}
Require ($ammoPickupNodes.Count -eq $expectedPickupParts.Count -and
    $ammoPickupMeshes.Count -eq $expectedPickupParts.Count) `
    'Cooked ammo pickup does not preserve its complete native-parent render subtree.'
function Read-PickupTransformCoordinate($Component, [string]$Property,
    [string]$Axis, [double]$DefaultValue) {
    if (@(PropertyNames $Component) -ccontains $Property) {
        return [double]$Component.Properties.$Property.$Axis
    }
    return $DefaultValue
}
foreach ($pickupPart in $expectedPickupParts) {
    $componentName = [string]$pickupPart.componentName
    $pickupNode = @($ammoPickupNodes | Where-Object {
        $_.Properties.InternalVariableName -ceq $componentName
    })
    $pickupMesh = @($ammoPickupMeshes | Where-Object {
        $_.Name -ceq ($componentName + '_GEN_VARIABLE')
    })
    Require ($pickupNode.Count -eq 1 -and $pickupMesh.Count -eq 1) `
        ('Cooked ammo pickup child is missing or duplicated: ' + $componentName)
    $expectedMeshPackage = ([string]$pickupPart.mesh).Substring(0,
        ([string]$pickupPart.mesh).LastIndexOf('.'))
    Require ($pickupMesh[0].Properties.StaticMesh.ObjectPath.StartsWith(
        $expectedMeshPackage + '.', [StringComparison]::Ordinal)) `
        ('Cooked ammo pickup child changed shared mesh: ' + $componentName)
    Require ($pickupMesh[0].Properties.BodyInstance.CollisionEnabled -ceq
        'ECollisionEnabled::NoCollision') `
        ('Cooked ammo pickup child gained collision: ' + $componentName)
    $transformContracts = @(
        [pscustomobject]@{property='RelativeLocation';evidence='location';axes=@('X','Y','Z');default=0.0},
        [pscustomobject]@{property='RelativeRotation';evidence='rotation';axes=@('Pitch','Yaw','Roll');default=0.0},
        [pscustomobject]@{property='RelativeScale3D';evidence='scale';axes=@('X','Y','Z');default=1.0}
    )
    foreach ($contract in $transformContracts) {
        $expectedCoordinates = @($pickupPart.($contract.evidence))
        for ($i=0; $i -lt 3; $i++) {
            $actualCoordinate = Read-PickupTransformCoordinate $pickupMesh[0] `
                $contract.property $contract.axes[$i] $contract.default
            Require (-not [double]::IsNaN($actualCoordinate) -and
                -not [double]::IsInfinity($actualCoordinate) -and
                [Math]::Abs($actualCoordinate -
                    [double]$expectedCoordinates[$i]) -lt 0.001) `
                ('Cooked ammo pickup transform mismatch: ' + $componentName +
                    '/' + $contract.evidence + '/' + $contract.axes[$i])
        }
    }
}
foreach ($package in @($inventory.packages | Where-Object {
    $_ -like '*/Materials/*'
} | Sort-Object -Unique)) {
    $exports = @(Read-Candidate $package)
    $parents = @($exports | ForEach-Object {
        if ((PropertyNames $_) -contains 'Parent' -and
            $null -ne $_.Properties.Parent.ObjectPath) {
            [string]$_.Properties.Parent.ObjectPath
        }
    })
    $materialEvidence += [pscustomobject]@{
        package = $package
        exportTypes = @($exports | ForEach-Object { [string]$_.Type })
        parents = $parents
    }
}
$gun = @(Read-Candidate $gunItemPackage)
$gunItem = @($gun | Where-Object { $_.Type -ceq 'VoyageItem' -and $_.Name -ceq 'DA_Item_Module_RailgunCannonMk01' })
Require ($gunItem.Count -eq 1) 'Railgun item primary export is missing or duplicated.'
$gunProperties = $gunItem[0].Properties
Require ($gunProperties.Category -ceq 'EVoyageItemCategory::Module') 'Railgun category mismatch.'
Require ($gunProperties.DroppedActor.AssetPathName -ceq '/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun_C') 'Railgun dropped actor mismatch.'
Require ($null -ne $gunProperties.Components) 'Railgun fabrication components are absent.'
$ammo = @(Read-Candidate $ammoPackage)
$ammoItem = @($ammo | Where-Object { $_.Type -ceq 'VoyageItemAmmo' -and $_.Name -ceq 'DA_Ammo_Railgun_FullRod' })
Require ($ammoItem.Count -eq 1) 'Railgun ammo primary export is missing or duplicated.'
$ammoProperties = $ammoItem[0].Properties
Require ($ammoProperties.Category -ceq 'EVoyageItemCategory::Ammo') 'Ammo category mismatch.'
Require ([double]$ammoProperties.Weight -gt 0.0) 'Ammo mass must be positive.'
Require ($null -ne $ammoProperties.Components) 'Ammo fabrication components are absent.'
Require (@($ammoProperties.DropVariations | Where-Object {
    $_.RenderAsset.AssetPathName -ceq
        $ammoPickupCarrierMesh
}).Count -ge 1) `
    'Ammo drop mesh mismatch.'
Require ($ammoProperties.DroppedActor.AssetPathName -ceq $ammoPickupClass) `
    'Ammo dropped actor mismatch.'
$reportPath = Join-Path $output 'validation.json'
[ordered]@{
    status='passed'
    runtime='pending'
    assetEvidence=$evidence
    materialEvidence=$materialEvidence
    assertions='Native module/item and pickup identities; model hierarchy, transforms, mesh references and collision match the current project inventory; converted item references remain readable in the final container. Gameplay, presentation and settings-loader regressions are separate acceptance.'
} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
