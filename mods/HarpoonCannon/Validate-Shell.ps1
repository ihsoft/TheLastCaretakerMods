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
foreach ($meshName in @('SM_HarpoonCannonBase','SM_HarpoonCannonYawAssembly','SM_HarpoonCannonPitchAssembly')) {
    $mesh = @(Read-Candidate ('/Game/Mods/HarpoonCannon/' + $meshName))
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
[ordered]@{status='passed';runtime='pending';containerSha256=(Get-FileHash -LiteralPath $Container -Algorithm SHA256).Hash;assetEvidence=$evidence;assertions='shell-only native parent, zero Blueprint functions, exact ItemAsset, no unreviewed native template values, auto-weld, direct mesh components, no operator references, simple collision preserved'} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
