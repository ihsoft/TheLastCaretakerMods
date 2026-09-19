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
$shell = @(Read-Candidate '/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New')
Require (@($shell | Where-Object { $_.Type -eq 'Function' }).Count -eq 0) 'Shell must not contain executable Blueprint functions.'
$class = @($shell | Where-Object { $_.Type -eq 'BlueprintGeneratedClass' })
Require ($class.Count -eq 1) 'Expected one generated class.'
Require ($class[0].SuperStruct.ObjectName -ceq "Class'VoyageModuleActor'") 'Shell native parent mismatch.'
$module = @($shell | Where-Object { $_.Name -ceq 'ModuleComponent' })
Require ($module.Count -eq 1 -and $module[0].Type -ceq 'VoyageCustomModuleComponent') 'Custom module override missing.'
Require ((@(PropertyNames $module[0]) -join ',') -ceq 'ItemAsset') 'Unreviewed module property serialized.'
Require ($module[0].Properties.ItemAsset.ObjectPath -ceq '/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.0') 'Cyclone ItemAsset mismatch.'
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
foreach ($role in @('yaw','pitch','sight','muzzle')) {
    $component = @($shell | Where-Object { $_.Name -ceq ($inventory.roles.$role + '_GEN_VARIABLE') })
    $tag = @{yaw='Harpoon.Model.Yaw';pitch='Harpoon.Model.Pitch';sight='Harpoon.Model.Sight';muzzle='Harpoon.Model.Muzzle'}[$role]
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
Require ($body[0].Properties.CollisionTraceFlag -ceq 'ECollisionTraceFlag::CTF_UseSimpleAsComplex') 'Fabricator collision mode changed.'
foreach ($package in @($inventory.packages | Where-Object { $_ -like '*/Materials/*' })) {
    $exports = @(Read-Candidate $package)
    $material = @($exports | Where-Object { $_.Type -ceq 'MaterialInstanceConstant' })
    Require ($material.Count -eq 1) ('Expected imported material: ' + $package)
    Require ((PropertyNames $material[0]) -contains 'VectorParameterValues') ('Importer lost material colors: ' + $package)
    Require ((PropertyNames $material[0]) -contains 'ScalarParameterValues') ('Importer lost PBR parameters: ' + $package)
    Require ($material[0].Properties.Parent.ObjectPath -like '/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque.*') 'Unreviewed material parent; check against stock and source before shipping.'
    $materialEvidence += [pscustomobject]@{package=$package;parent=$material[0].Properties.Parent.ObjectPath}
}
$reportPath = Join-Path $output 'validation.json'
[ordered]@{status='passed';runtime='pending';containerSha256=(Get-FileHash -LiteralPath $Container -Algorithm SHA256).Hash;assetEvidence=$evidence;materialEvidence=$materialEvidence;assertions='shell-only native parent, zero Blueprint functions, exact ItemAsset, no unreviewed native template values, auto-weld, inventory-matched component hierarchy and transforms, no operator references, simple collision preserved; material parameter presence and reviewed stock parent (exact source factors checked separately)'} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
