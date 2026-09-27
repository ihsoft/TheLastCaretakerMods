[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Container, [Parameter(Mandatory=$true)][string]$OutputRoot, [Parameter(Mandatory=$true)][string]$ModelInventory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
$boundary = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($boundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence must be below artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Semantic output exists; use a fresh candidate.' }
$null = New-Item -ItemType Directory -Path $output
$evidence = @()
$materialEvidence = @()
$itemDiscoveryRoot = '/Game/Data/Assets'
$skillDiscoveryRoot = '/Game/Data/Assets/Skill'
$gunItemPackage = '/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$skillPackage = '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun'
function Read-Candidate([string]$Query) {
    $result = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') -Query $Query -Source Mod -ModContainer $Container -AsJson) | ConvertFrom-Json
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
Require (@($shell | Where-Object { $_.Type -eq 'Function' }).Count -eq 0) 'Shell must not contain executable Blueprint functions.'
$class = @($shell | Where-Object { $_.Type -eq 'BlueprintGeneratedClass' })
Require ($class.Count -eq 1) 'Expected one generated class.'
Require ($class[0].SuperStruct.ObjectName -ceq "Class'VoyageModuleActor'") 'Shell native parent mismatch.'
$module = @($shell | Where-Object { $_.Name -ceq 'ModuleComponent' })
Require ($module.Count -eq 1 -and $module[0].Type -ceq 'VoyageModuleComponent') 'Native buffer module missing.'
Require (@(PropertyNames $module[0]) -contains 'ItemAsset') 'Module ItemAsset missing.'
Require ($module[0].Properties.ItemAsset.ObjectPath -ceq ($gunItemPackage + '.0')) 'Railgun ItemAsset mismatch.'
Require ($gunItemPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) 'Railgun item is outside the confirmed Item AssetManager discovery root.'
Require ($skillPackage.StartsWith($skillDiscoveryRoot + '/', [StringComparison]::Ordinal)) 'Railgun skill is outside the confirmed Skill AssetManager discovery root.'
$energy = $module[0].Properties.ConfigData
Require ([Math]::Abs($energy.ResourceConsumptionOn - 1000) -lt 0.001) 'Idle ON consumption mismatch.'
Require ([Math]::Abs($energy.ResourceConsumptionStandby - 1000) -lt 0.001) 'Idle standby consumption mismatch.'
Require ([Math]::Abs($energy.ResourceBandwidthInput - 1000) -lt 0.001) 'Idle input bandwidth mismatch.'
Require ([Math]::Abs($energy.MaxResourceAmount - 1.0) -lt 0.000001) 'Idle buffer mismatch.'
Require ($energy.bAutoStartModule -and $energy.bAcceptResourceOffer -and $energy.bAcceptResourceOfferProduction) 'Native receiver disabled.'
Require ($energy.bAcceptResourceOfferOff) 'Empty/unpowered receiver cannot recover.'
Require ($module[0].Properties.SocketCustomTarget.ComponentProperty -ceq 'ElectricSocket') 'Electric socket target mismatch.'
Require ($module[0].Properties.bUseSocketCustomTarget -eq $true) 'Custom socket target disabled.'
foreach ($name in @('Electric_GEN_VARIABLE','ElectricSocket_GEN_VARIABLE')) {
    Require (@($shell | Where-Object { $_.Type -eq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq ($name -replace '_GEN_VARIABLE$','') }).Count -eq 1) ('Missing power component: ' + $name)
}
$electric = @($shell | Where-Object { $_.Name -ceq 'ElectricSocket_GEN_VARIABLE' })
Require ($electric[0].Type -ceq 'VoyageModuleSocketViewComponent') 'Power socket native class mismatch.'
Require ($electric[0].Properties.DataAsset.AssetPathName -like '*/DA_Socket_ElectricData.DA_Socket_ElectricData') 'Power socket data asset mismatch.'
Require ($electric[0].Properties.Port.DefaultDirection -ceq 'EModuleSocketType::ST_Input') 'Power socket must default to input.'
Require ($electric[0].Properties.SocketID -eq 2236302826 -and $electric[0].Properties.bAutoInitialize -eq $false) 'Power socket identity/initialization mismatch.'
foreach ($name in @('PersistentComponent','DestructibleObjectComponent')) {
    $component = @($shell | Where-Object { $_.Name -ceq $name })
    Require ($component.Count -eq 1) ('Missing native template: ' + $name)
    Require (@(PropertyNames $component[0]).Count -eq 0) ('Unreviewed native template property: ' + $name)
}
$dynamic = @($shell | Where-Object { $_.Type -ceq 'VoyageDynamicCollisionComponent' })
Require ($dynamic.Count -eq 1 -and $dynamic[0].Properties.bAutoWeld -eq $true) 'Missing auto-weld.'
$inventory = Get-Content -LiteralPath $ModelInventory -Raw | ConvertFrom-Json
function Require-Child([string]$ParentName, [string]$ChildName) {
    $parent = @($shell | Where-Object { $_.Type -ceq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq $ParentName })
    $child = @($shell | Where-Object { $_.Type -ceq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq $ChildName })
    Require ($parent.Count -eq 1 -and $child.Count -eq 1) ('Missing hierarchy nodes: ' + $ChildName)
    $suffix = '.' + $child[0].Name + "'"
    Require (@($parent[0].Properties.ChildNodes | Where-Object { $_.ObjectName.EndsWith($suffix) }).Count -eq 1) ('Parent mismatch: ' + $ChildName)
}
$entryConfig = $inventory.entryInteraction
Require-Child $entryConfig.node 'RailgunEntryReference'
$entry = @($shell | Where-Object { $_.Name -ceq 'RailgunEntryReference_GEN_VARIABLE' })
Require ($entry.Count -eq 1 -and $entry[0].Type -ceq 'BoxComponent') 'Missing entry reference box.'
Require (@($entry[0].Properties.ComponentTags) -ccontains 'Railgun.Model.Entry') 'Entry reference tag missing.'
Require ($entry[0].Properties.BodyInstance.CollisionEnabled -ceq 'ECollisionEnabled::NoCollision') 'Entry reference must not collide.'
for ($i=0; $i -lt 3; $i++) {
    $axis = @('X','Y','Z')[$i]
    $center = 0.0
    if ((PropertyNames $entry[0]) -contains 'RelativeLocation') { $center = $entry[0].Properties.RelativeLocation.$axis }
    Require ([Math]::Abs($center - $entryConfig.centerCm[$i]) -lt 0.001) 'Entry reference center mismatch.'
    Require ([Math]::Abs($entry[0].Properties.BoxExtent.$axis * 2 - $entryConfig.sizeCm[$i]) -lt 0.001) 'Entry reference dimensions mismatch.'
}
foreach ($expected in $inventory.components) {
    $component = @($shell | Where-Object { $_.Name -ceq ($expected.name + '_GEN_VARIABLE') })
    Require ($component.Count -eq 1) ('Missing GLB component: ' + $expected.name)
    Require-Child $expected.parent $expected.name
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
Require-Child $inventory.roles.powerSocketAnchor 'Electric'
Require-Child 'Electric' 'ElectricSocket'
foreach ($name in @('Electric_GEN_VARIABLE','ElectricSocket_GEN_VARIABLE')) {
    $component = @($shell | Where-Object { $_.Name -ceq $name })[0]
    foreach ($field in @('RelativeLocation','RelativeRotation','RelativeScale3D')) {
        if ((PropertyNames $component) -contains $field) {
            $axes = if ($field -ceq 'RelativeRotation') { @('Pitch','Yaw','Roll') } else { @('X','Y','Z') }
            $expectedValue = if ($field -ceq 'RelativeScale3D') { 1.0 } else { 0.0 }
            foreach ($axis in $axes) {
                Require ([Math]::Abs($component.Properties.$field.$axis - $expectedValue) -lt 0.001) ('Unexpected power anchor offset: ' + $name)
            }
        }
    }
}
foreach ($role in @('yaw','pitch','sight','muzzle')) {
    $component = @($shell | Where-Object { $_.Name -ceq ($inventory.roles.$role + '_GEN_VARIABLE') })
    $tag = @{yaw='Railgun.Model.Yaw';pitch='Railgun.Model.Pitch';sight='Railgun.Model.Sight';muzzle='Railgun.Model.Muzzle'}[$role]
    Require (@($component[0].Properties.ComponentTags) -ccontains $tag) ('Missing tag: ' + $role)
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
foreach ($package in @($inventory.packages | Where-Object { $_ -like '*/Materials/*' })) {
    $exports = @(Read-Candidate $package)
    $material = @($exports | Where-Object { $_.Type -ceq 'MaterialInstanceConstant' })
    Require ($material.Count -eq 1) ('Expected imported material: ' + $package)
    $materialPropertyNames = @(PropertyNames $material[0])
    $textureParameterNames = @()
    if ($materialPropertyNames -contains 'TextureParameterValues') {
        $textureParameterNames = @($material[0].Properties.TextureParameterValues |
            ForEach-Object { [string]$_.ParameterInfo.Name })
    }
    $hasColorData = $materialPropertyNames -contains 'VectorParameterValues' -or
        $textureParameterNames -ccontains 'BaseColorTexture'
    $hasPbrData = $materialPropertyNames -contains 'ScalarParameterValues' -or
        $textureParameterNames -ccontains 'MetallicRoughnessTexture'
    Require $hasColorData ('Importer lost material colors: ' + $package)
    Require $hasPbrData ('Importer lost PBR parameters: ' + $package)
    Require ($material[0].Properties.Parent.ObjectPath -like '/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque.*') 'Unreviewed material parent; check against stock and source before shipping.'
    $materialEvidence += [pscustomobject]@{package=$package;parent=$material[0].Properties.Parent.ObjectPath}
}
$gun = @(Read-Candidate $gunItemPackage)
$gunItem = @($gun | Where-Object { $_.Type -ceq 'VoyageItem' -and $_.Name -ceq 'DA_Item_Module_RailgunCannonMk01' })
Require ($gun.Count -eq 4 -and $gunItem.Count -eq 1) 'Expected one complete four-export Railgun item.'
$gunProperties = $gunItem[0].Properties
Require ($gunProperties.Icon.ObjectPath -ceq '/Game/Mods/Railgun/Fabricator/T_RailgunIcon.0') 'Railgun icon mismatch.'
Require ($gunProperties.Category -ceq 'EVoyageItemCategory::Module') 'Railgun category mismatch.'
Require ($gunProperties.CategoryAsset.ObjectPath -ceq '/Game/Data/Assets/ItemCategories/DA_ItemCategory_Module.0') 'Railgun category asset mismatch.'
Require ($gunProperties.Quality -ceq 'EVoyageItemQuality::Uncommon') 'Railgun quality mismatch.'
Require ([Math]::Abs($gunProperties.Weight - 89.7) -lt 0.000001) 'Railgun mass mismatch.'
Require ([Math]::Abs($gunProperties.CraftTime - 10.0) -lt 0.000001) 'Railgun craft time mismatch.'
Require ([Math]::Abs($gunProperties.CraftElectricityCost - 5.0) -lt 0.000001) 'Railgun craft energy mismatch.'
Require ($gunProperties.CraftFilter -eq 9) 'Railgun fabrication filter mismatch.'
Require (@($gunProperties.Components).Count -eq 1 -and
    [string]$gunProperties.Components[0].Key -match 'DA_Part_AlloyFrame' -and
    $gunProperties.Components[0].Value -eq 1) 'Railgun recipe must be exactly one Alloy Frame.'
Require ($gunProperties.bIsDismantlable -and $gunProperties.bIsRepairable) 'Railgun dismantle/repair defaults changed.'
Require (@($gunProperties.DropVariations).Count -eq 1) 'Railgun drop variation mismatch.'
Require ($gunProperties.DroppedActor.AssetPathName -ceq '/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun_C') 'Railgun dropped actor mismatch.'
Require ($gunProperties.bIsDestructible -and
    [Math]::Abs($gunProperties.DestructibleObjectProperties.Health - 400.0) -lt 0.000001 -and
    [Math]::Abs($gunProperties.DestructibleObjectProperties.DestructionDamageThreshold - 100.0) -lt 0.000001) 'Railgun destructible defaults changed.'
Require (@($gunProperties.properties).Count -eq 0) 'Railgun still carries donor production metadata.'
Require ($gunProperties.Name.SourceString -ceq 'Railgun') 'Railgun name mismatch.'
Require ($gunProperties.Description.SourceString -ceq 'A long-range electromagnetic cannon. Payload selection is your responsibility.') 'Railgun description mismatch.'
$gunIcon = @(Read-Candidate '/Game/Mods/Railgun/Fabricator/T_RailgunIcon')
$gunTexture = @($gunIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunIcon' })
Require ($gunTexture.Count -eq 1 -and $gunTexture[0].SizeX -eq 256 -and $gunTexture[0].SizeY -eq 256) 'Railgun icon must be 256x256.'
$ammo = @(Read-Candidate '/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod')
$ammoItem = @($ammo | Where-Object { $_.Type -ceq 'VoyageItemAmmo' -and $_.Name -ceq 'DA_Ammo_Railgun_FullRod' })
Require ($ammo.Count -eq 1 -and $ammoItem.Count -eq 1) 'Expected exactly one complete VoyageItemAmmo export.'
$ammoProperties = $ammoItem[0].Properties
$ammoPropertyNames = @(PropertyNames $ammoItem[0])
$expectedAmmoPropertyNames = @(
    'Caliber','Icon','Category','CategoryAsset','Quality','Weight','CraftTime',
    'CraftElectricityCost','CraftAmount','CraftFilter','Components','DropVariations',
    'DroppedActor','MaxDropCount','Name','Description'
)
Require (@(Compare-Object -ReferenceObject $expectedAmmoPropertyNames -DifferenceObject $ammoPropertyNames -CaseSensitive).Count -eq 0) 'Ammo serialized top-level property set differs from the approved Railgun contract.'
Require ($ammoPropertyNames -cnotcontains 'WeaponData') 'Removed ammo weapon data was serialized.'
Require ($ammoPropertyNames -cnotcontains 'ScalePerItem') 'Removed ammo scale-per-item was serialized.'
Require ($ammoProperties.MaxDropCount -eq 50) 'Ammo max-drop count mismatch.'
Require ([Math]::Abs($ammoProperties.Caliber - 45.0) -lt 0.000001) 'Ammo caliber mismatch.'
Require ($ammoPropertyNames -cnotcontains 'SecondaryIcon') 'Removed ammo secondary icon was serialized.'
Require ($ammoPropertyNames -cnotcontains 'MaxStackCount') 'Authored ammo must retain the native MaxStackCount default.'
Require ($ammoProperties.Icon.ObjectPath -ceq '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon.0') 'Ammo icon mismatch.'
Require ($ammoProperties.Category -ceq 'EVoyageItemCategory::Ammo') 'Ammo category mismatch.'
Require ($ammoProperties.CategoryAsset.ObjectPath -ceq '/Game/Data/Assets/ItemCategories/DA_ItemCategory_Ammo.0') 'Ammo category asset mismatch.'
Require ($ammoProperties.Quality -ceq 'EVoyageItemQuality::Common') 'Ammo quality mismatch.'
Require ([Math]::Abs($ammoProperties.Weight - 3.9) -lt 0.000001) 'Ammo mass mismatch.'
Require ([Math]::Abs($ammoProperties.CraftTime - 6.0) -lt 0.000001) 'Ammo craft time mismatch.'
Require ([Math]::Abs($ammoProperties.CraftElectricityCost - 5.0) -lt 0.000001) 'Ammo craft energy mismatch.'
Require ($ammoProperties.CraftAmount -eq 6 -and $ammoProperties.CraftFilter -eq 3) 'Ammo fabrication contract mismatch.'
$expectedComponents = @{
    "/Game/Data/Assets/Materials/DA_Material_Iron.DA_Material_Iron" = 2
    "/Game/Data/Assets/Materials/DA_Material_Copper.DA_Material_Copper" = 2
    "/Game/Data/Assets/Materials/DA_Material_Plastic.DA_Material_Plastic" = 1
}
Require (@($ammoProperties.Components).Count -eq $expectedComponents.Count) 'Ammo component count mismatch.'
foreach ($component in @($ammoProperties.Components)) {
    $key = [string]$component.Key
    if ($key -match "^VoyageItemMaterial'(.+)'$") { $key = $matches[1] }
    Require ($expectedComponents.ContainsKey($key)) ('Unexpected ammo component: ' + $key)
    Require ($component.Value -eq $expectedComponents[$key]) ('Ammo component amount mismatch: ' + $key)
}
Require (@($ammoProperties.DropVariations).Count -eq 1) 'Ammo drop variation mismatch.'
Require ($ammoProperties.DropVariations[0].RenderAsset.AssetPathName -ceq '/Game/AssetSets/Items/Ammobox/SM_Ammobox_03.SM_Ammobox_03') 'Ammo drop mesh mismatch.'
Require ($ammoProperties.DroppedActor.AssetPathName -ceq '/Game/Blueprints/BP_DynamicMeshActor.BP_DynamicMeshActor_C') 'Ammo dropped actor mismatch.'
Require ($ammoProperties.Name.SourceString -ceq 'Railgun Kinetic Rounds') 'Ammo name mismatch.'
Require ($ammoProperties.Description.SourceString -ceq
    'Armor-piercing kinetic rounds. No explosives, just mass and velocity.') 'Ammo description mismatch.'
$ammoIcon = @(Read-Candidate '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon')
$ammoTexture = @($ammoIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunAmmoIcon' })
Require ($ammoTexture.Count -eq 1 -and $ammoTexture[0].SizeX -eq 256 -and $ammoTexture[0].SizeY -eq 256) 'Ammo icon must be 256x256.'
$skillIcon = @(Read-Candidate '/Game/Mods/Railgun/Research/T_RailgunSkill')
$skillTexture = @($skillIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunSkill' })
Require ($skillTexture.Count -eq 1 -and $skillTexture[0].SizeX -eq 256 -and $skillTexture[0].SizeY -eq 256) 'Railgun skill icon must be 256x256.'
$reportPath = Join-Path $output 'validation.json'
[ordered]@{status='passed';runtime='pending';containerSha256=(Get-FileHash -LiteralPath $Container -Algorithm SHA256).Hash;assetEvidence=$evidence;materialEvidence=$materialEvidence;assertions='owned shell-only native parent, zero Blueprint functions, exact discovered ItemAsset, confirmed Item and Skill AssetManager scan roots, no unreviewed native template values, auto-weld, inventory-matched component hierarchy and transforms, no operator references, simple collision preserved; material parameter presence and reviewed stock parent; complete Railgun item with Alloy Frame recipe, owned actor/icon and cleared production metadata; one complete VoyageItemAmmo export with the stock Rod item fields except its nested-export ProjectileTemplate; distinct 256x256 research icon'} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
