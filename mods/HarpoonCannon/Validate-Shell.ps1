[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Container, [Parameter(Mandatory=$true)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
$boundary = [IO.Path]::GetFullPath((Join-Path $repo 'artifacts')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($boundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence must be below artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Semantic output exists; use a fresh candidate.' }
$null = New-Item -ItemType Directory -Path $output
$evidence = @()
$model = Get-Content -LiteralPath (Join-Path $repo 'models/HarpoonCannon/runtime-model.json') -Raw | ConvertFrom-Json
$palette = Get-Content -LiteralPath (Join-Path $repo 'models/HarpoonCannon/material-palette.json') -Raw | ConvertFrom-Json
$objectMaterials = @{}
$currentObject = $null
foreach ($line in Get-Content -LiteralPath (Join-Path (Join-Path $repo 'models/HarpoonCannon') $model.sources.obj.path)) {
    if ($line.StartsWith('o ')) { $currentObject = $line.Substring(2).Trim() }
    if ($line.StartsWith('usemtl ') -and $currentObject) { $objectMaterials[$currentObject] = $line.Substring(7).Trim() }
}
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
foreach ($part in @('HarpoonBaseBody_GEN_VARIABLE','HarpoonTurretBody_GEN_VARIABLE','HarpoonBarrelBody_GEN_VARIABLE')) {
    Require (@($shell | Where-Object { $_.Name -ceq $part -and $_.Type -ceq 'StaticMeshComponent' }).Count -eq 1) ('Missing direct mesh: ' + $part)
}
$serialized = $shell | ConvertTo-Json -Depth 100 -Compress
Require (-not ($serialized -match 'BP_CameraDrone|BP_HarpoonCannon|WBP_HarpoonCannon|SpawnActor|SetHiddenInGame|InteractiveInterface')) 'Operator/Drone/marker dependency leaked into shell.'
foreach ($instance in $model.ammo.instances) {
    $component = @($shell | Where-Object { $_.Name -ceq ($instance.name + '_GEN_VARIABLE') -and $_.Type -ceq 'StaticMeshComponent' })
    Require ($component.Count -eq 1) ('Missing independent ammo component: ' + $instance.name)
    Require ($component[0].Properties.StaticMesh.ObjectPath -like '/Game/Mods/HarpoonCannon/SM_HarpoonCannonAmmo.*') 'Ammo must reference shared authored mesh.'
    Require ($component[0].Properties.BodyInstance.CollisionEnabled -ceq 'ECollisionEnabled::NoCollision') 'Ammo must not affect physics.'
    foreach ($axis in @('X','Y','Z')) {
        $index = @('X','Y','Z').IndexOf($axis)
        Require ([Math]::Abs($component[0].Properties.RelativeLocation.$axis - $instance.locationRelativeToParentCm[$index]) -lt 0.000001) ('Ammo offset mismatch: ' + $instance.name)
    }
}
function Require-Child([string]$ParentName, [string]$ChildName) {
    $parent = @($shell | Where-Object { $_.Type -ceq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq $ParentName })
    $child = @($shell | Where-Object { $_.Type -ceq 'SCS_Node' -and $_.Properties.InternalVariableName -ceq $ChildName })
    Require ($parent.Count -eq 1 -and $child.Count -eq 1) 'Missing/ambiguous model hierarchy node.'
    $suffix = '.' + $child[0].Name + "'"
    Require (@($parent[0].Properties.ChildNodes | Where-Object { $_.ObjectName.EndsWith($suffix) }).Count -eq 1) ('Model parent mismatch: ' + $ChildName)
}
Require-Child 'ModuleMountCollision' 'YawPivot'
Require-Child 'YawPivot' 'PitchPivot'
Require-Child 'YawPivot' 'HarpoonTurretBody'
Require-Child 'PitchPivot' 'HarpoonBarrelBody'
Require-Child 'PitchPivot' 'HarpoonSight'
$sight = @($shell | Where-Object { $_.Name -ceq 'HarpoonSight_GEN_VARIABLE' })
Require ($sight.Count -eq 1 -and @($sight[0].Properties.ComponentTags) -ccontains 'Harpoon.Model.Sight') 'Missing sight role.'
foreach ($axis in @('X','Y','Z')) {
    Require ([Math]::Abs($sight[0].Properties.RelativeLocation.$axis - $model.sight.locationRelativeToParentCm[@('X','Y','Z').IndexOf($axis)]) -lt 0.000001) 'Sight origin mismatch.'
}
foreach ($instance in $model.ammo.instances) { Require-Child 'YawPivot' $instance.name }
foreach ($role in @('yaw','pitch')) {
    $name = if ($role -ceq 'yaw') { 'YawPivot_GEN_VARIABLE' } else { 'PitchPivot_GEN_VARIABLE' }
    $component = @($shell | Where-Object { $_.Name -ceq $name })
    $tag = if ($role -ceq 'yaw') { 'Harpoon.Model.Yaw' } else { 'Harpoon.Model.Pitch' }
    Require ($component.Count -eq 1 -and @($component[0].Properties.ComponentTags) -ccontains $tag) ('Missing role tag: ' + $role)
    $descriptorNode = @($model.hierarchy | Where-Object { $_.role -ceq $role })[0]
    foreach ($axis in @('X','Y','Z')) {
        $actual = 0.0
        if ((PropertyNames $component[0]) -contains 'RelativeLocation') { $actual = $component[0].Properties.RelativeLocation.$axis }
        Require ([Math]::Abs($actual - $descriptorNode.locationRelativeToParentCm[@('X','Y','Z').IndexOf($axis)]) -lt 0.000001) ('Pivot offset mismatch: ' + $role)
    }
}
foreach ($meshName in @('SM_HarpoonCannonBase','SM_HarpoonCannonYawAssembly','SM_HarpoonCannonPitchAssembly','SM_HarpoonCannonAmmo')) {
    $mesh = @(Read-Candidate ('/Game/Mods/HarpoonCannon/' + $meshName))
    $role = switch ($meshName) { 'SM_HarpoonCannonBase' { 'base' }; 'SM_HarpoonCannonYawAssembly' { 'yaw' }; 'SM_HarpoonCannonPitchAssembly' { 'pitch' }; 'SM_HarpoonCannonAmmo' { 'ammo' } }
    $part = if ($role -ceq 'ammo') { $model.ammo.prototype } else { @($model.meshes | Where-Object { $_.role -ceq $role })[0] }
    $expectedSlots = @($part.objectNames | ForEach-Object { $objectMaterials[$_] } | Sort-Object -Unique)
    $masters = @($mesh | Where-Object { $_.Type -ceq 'Material' -and $_.Name -ceq 'M_HarpoonPaletteSurface' })
    $instances = @($mesh | Where-Object { $_.Type -ceq 'MaterialInstanceConstant' })
    Require ($masters.Count -eq 1) ('Missing owned palette master: ' + $meshName)
    Require ($instances.Count -eq $expectedSlots.Count) ('Wrong palette instance count: ' + $meshName)
    $renderMesh = @($mesh | Where-Object { $_.Type -ceq 'StaticMesh' -and $_.Name -ceq $meshName })
    Require ($renderMesh.Count -eq 1) ('Missing render mesh: ' + $meshName)
    $renderSlots = @($renderMesh[0].Properties.StaticMaterials)
    Require ($renderSlots.Count -eq $expectedSlots.Count) ('Wrong render material slot count: ' + $meshName)
    foreach ($slot in $expectedSlots) {
        $instance = @($instances | Where-Object { $_.Name -ceq ('MI_HarpoonPalette_' + $slot) })
        $expected = @($palette.slots | Where-Object { $_.name -ceq $slot })
        Require ($instance.Count -eq 1 -and $expected.Count -eq 1) ('Palette slot missing/ambiguous: ' + $slot)
        Require ($instance[0].Properties.Parent.ObjectName -ceq "Material'M_HarpoonPaletteSurface'") ('Palette parent mismatch: ' + $slot)
        Require ($instance[0].Properties.Parent.ObjectPath -clike ('/Game/Mods/HarpoonCannon/' + $meshName + '.*')) ('Palette parent escaped mesh package: ' + $slot)
        $renderSlot = @($renderSlots | Where-Object { $_.MaterialSlotName -ceq $slot })
        Require ($renderSlot.Count -eq 1) ('Render slot missing: ' + $slot)
        Require ($renderSlot[0].MaterialInterface.ObjectName -ceq ("MaterialInstanceConstant'MI_HarpoonPalette_" + $slot + "'")) ('Mesh still has default/wrong material: ' + $slot)
        Require ($renderSlot[0].MaterialInterface.ObjectPath -clike ('/Game/Mods/HarpoonCannon/' + $meshName + '.*')) ('Palette instance escaped mesh package: ' + $slot)
        foreach ($field in @('BaseColor','Emissive')) {
            $vector = @($instance[0].Properties.VectorParameterValues | Where-Object { $_.ParameterInfo.Name -ceq $field })
            Require ($vector.Count -eq 1) ('Missing material color parameter: ' + $slot + '/' + $field)
            $expectedRgb = if ($field -ceq 'BaseColor') { $expected[0].baseColorLinear } else { $expected[0].emissiveColorLinear }
            foreach ($channel in @('R','G','B')) {
                Require ([Math]::Abs($vector[0].ParameterValue.$channel - $expectedRgb[@('R','G','B').IndexOf($channel)]) -lt 0.000001) ('Cooked palette color mismatch: ' + $slot + '/' + $field)
            }
        }
        foreach ($field in @('Roughness','Metallic')) {
            $scalar = @($instance[0].Properties.ScalarParameterValues | Where-Object { $_.ParameterInfo.Name -ceq $field })
            Require ($scalar.Count -eq 1 -and [Math]::Abs($scalar[0].ParameterValue - $expected[0].($field.ToLowerInvariant())) -lt 0.000001) ('Cooked material scalar mismatch: ' + $slot + '/' + $field)
        }
    }
    $materialEvidence += [pscustomobject]@{mesh=$meshName;slots=$expectedSlots;master='M_HarpoonPaletteSurface';parameterValues='passed'}
    $body = @($mesh | Where-Object { $_.Type -ceq 'BodySetup' })
    Require ($body.Count -eq 1) ('Missing BodySetup: ' + $meshName)
    $boxes = @()
    if ((PropertyNames $body[0]) -contains 'AggGeom') {
        if ($body[0].Properties.AggGeom.PSObject.Properties.Name -contains 'BoxElems') { $boxes = @($body[0].Properties.AggGeom.BoxElems) }
    }
    if ($meshName -ceq 'SM_HarpoonCannonBase') {
        Require ($boxes.Count -eq 1) 'Base must preserve one simple collision box.'
        Require ($boxes[0].X -eq 164 -and $boxes[0].Y -eq 164 -and $boxes[0].Z -eq 12) 'Base collision dimensions changed.'
        Require ($boxes[0].Center.X -eq 0 -and $boxes[0].Center.Y -eq 0 -and $boxes[0].Center.Z -eq 6) 'Base collision center changed.'
        Require ($body[0].Properties.CollisionTraceFlag -ceq 'ECollisionTraceFlag::CTF_UseSimpleAsComplex') 'Base simple collision trace mode changed.'
    } else { Require ($boxes.Count -eq 0) ('Unexpected moving-mesh collision: ' + $meshName) }
}
$reportPath = Join-Path $output 'validation.json'
[ordered]@{status='passed';runtime='pending';containerSha256=(Get-FileHash -LiteralPath $Container -Algorithm SHA256).Hash;assetEvidence=$evidence;materialEvidence=$materialEvidence;assertions='shell-only native parent, zero Blueprint functions, exact ItemAsset, no unreviewed native template values, auto-weld, direct mesh components, no operator references, simple collision preserved; owned palette masters and per-slot cooked color/emissive/roughness/metallic values'} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
