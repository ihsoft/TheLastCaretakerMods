[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Container, [Parameter(Mandatory=$true)][string]$OutputRoot, [Parameter(Mandatory=$true)][string]$ModelInventory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
$boundary = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp')) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($boundary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Semantic scratch output must be below repository Tmp.' }
if (Test-Path -LiteralPath $output) { throw 'Semantic output exists; use a fresh candidate.' }
$null = New-Item -ItemType Directory -Path $output
$evidence = @()
$materialEvidence = @()
$itemDiscoveryRoot = '/Game/Data/Assets'
$skillDiscoveryRoot = '/Game/Data/Assets/Skill'
$gunItemPackage = '/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01'
$skillPackage = '/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun'
$ammoPackage = '/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod'
$operatorPackage = '/Game/Mods/Railgun/Station/BP_RailgunOperator'
$hudPackage = '/Game/Mods/Railgun/Station/WBP_RailgunHUD'
$ammoIndicatorPackage = '/Game/Mods/Railgun/Station/T_RailgunAmmoIndicator'
function Read-Candidate([string]$Query) {
    $result = (& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
        -Query $Query -Source Mod -ModContainer $Container `
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
function ArrayPropertyCount($Value, [string]$Name) {
    if ($null -ne $Value -and
        @($Value.PSObject.Properties.Name) -ccontains $Name) {
        return @($Value.$Name).Count
    }
    return 0
}
function JsonStringLeaves($Value) {
    if ($null -eq $Value) { return }
    if ($Value -is [string]) { $Value; return }
    if ($Value -is [array]) {
        foreach ($entry in $Value) { JsonStringLeaves $entry }
        return
    }
    if ($Value -is [pscustomobject]) {
        foreach ($property in $Value.PSObject.Properties) {
            JsonStringLeaves $property.Value
        }
    }
}
function VisibilityTargets($Value) {
    if ($null -eq $Value) { return }
    if ($Value -is [array]) {
        foreach ($entry in $Value) { VisibilityTargets $entry }
        return
    }
    if ($Value -isnot [pscustomobject]) { return }
    $names = @($Value.PSObject.Properties.Name)
    if (($names -ccontains 'Token') -and $Value.Token -ceq 'EX_Context' -and
        ($names -ccontains 'ContextExpression') -and
        $null -ne $Value.ContextExpression -and
        (@($Value.ContextExpression.PSObject.Properties.Name) -ccontains 'Function') -and
        $Value.ContextExpression.Function -ceq 'SetVisibility' -and
        ($names -ccontains 'ObjectExpression') -and
        $null -ne $Value.ObjectExpression -and
        (@($Value.ObjectExpression.PSObject.Properties.Name) -ccontains 'Variable') -and
        $null -ne $Value.ObjectExpression.Variable -and
        $null -ne $Value.ObjectExpression.Variable.Property) {
        [string]$Value.ObjectExpression.Variable.Property.Name
    }
    foreach ($property in $Value.PSObject.Properties) {
        VisibilityTargets $property.Value
    }
}
function AmmoActivationThresholds($Value) {
    if ($null -eq $Value) { return }
    if ($Value -is [array]) {
        foreach ($entry in $Value) { AmmoActivationThresholds $entry }
        return
    }
    if ($Value -isnot [pscustomobject]) { return }
    $names = @($Value.PSObject.Properties.Name)
    if (($names -ccontains 'Expression') -and
        $null -ne $Value.Expression -and
        (@(JsonStringLeaves $Value.Expression) -ccontains
            'RailgunAmmoLastVisualCount') -and
        (@(JsonStringLeaves $Value.Expression) -ccontains
            "Class'KismetMathLibrary:Greater_IntInt'")) {
        $thresholds = @($Value.Expression.Parameters | Where-Object {
            $_.Token -ceq 'EX_IntConst'
        })
        Require ($thresholds.Count -eq 1) `
            'Ammo indicator comparison has an unexpected threshold shape.'
        [int]$thresholds[0].Value
        return
    }
    foreach ($property in $Value.PSObject.Properties) {
        AmmoActivationThresholds $property.Value
    }
}
function EmptyAmmoTintAssignments($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject]) { continue }
        $statementNames = @($statement.PSObject.Properties.Name)
        if (-not ($statementNames -ccontains 'Token') -or
            -not ($statementNames -ccontains 'Variable') -or
            -not ($statementNames -ccontains 'Expression')) {
            continue
        }
        $variableLeaves = @(JsonStringLeaves $statement.Variable)
        $expressionLeaves = @(JsonStringLeaves $statement.Expression)
        if ($statement.Token -cne 'EX_Let' -or
            $null -eq $statement.Variable -or
            $null -eq $statement.Expression -or
            @($variableLeaves | Where-Object {
                $_.StartsWith('CallFunc_SelectColor_ReturnValue',
                    [StringComparison]::Ordinal)
            }).Count -ne 1 -or
            -not ($expressionLeaves -ccontains
                "Class'KismetMathLibrary:SelectColor'")) {
            continue
        }
        $parameters = @($statement.Expression.Parameters)
        if ($parameters.Count -eq 3) {
            $tint = $parameters[0]
            $tintValues = @($tint.Properties | ForEach-Object { [double]$_.Value })
            $fallbackLeaves = @(JsonStringLeaves $parameters[1])
            $conditionLeaves = @(JsonStringLeaves $parameters[2])
            if ($tint.Token -ceq 'EX_StructConst' -and
                $tint.Struct.ObjectName -ceq "Class'LinearColor'" -and
                $tintValues.Count -eq 4 -and
                $tintValues[0] -eq 1.0 -and
                $tintValues[1] -eq 0.25 -and
                $tintValues[2] -eq 0.25 -and
                $tintValues[3] -eq 0.3 -and
                $fallbackLeaves -ccontains 'RailgunChargeRadial' -and
                $fallbackLeaves -ccontains 'SliderBarColor' -and
                @($conditionLeaves | Where-Object {
                    $_.StartsWith('CallFunc_EqualEqual_IntInt_ReturnValue',
                        [StringComparison]::Ordinal)
                }).Count -eq 1) {
                $statement
            }
        }
    }
}
function EmptyAmmoZeroComparisons($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject]) { continue }
        $statementNames = @($statement.PSObject.Properties.Name)
        if (-not ($statementNames -ccontains 'Token') -or
            -not ($statementNames -ccontains 'Variable') -or
            -not ($statementNames -ccontains 'Expression')) {
            continue
        }
        $variableLeaves = @(JsonStringLeaves $statement.Variable)
        $expressionLeaves = @(JsonStringLeaves $statement.Expression)
        if ($statement.Token -cne 'EX_LetBool' -or
            $null -eq $statement.Variable -or
            $null -eq $statement.Expression -or
            @($variableLeaves | Where-Object {
                $_.StartsWith('CallFunc_EqualEqual_IntInt_ReturnValue',
                    [StringComparison]::Ordinal)
            }).Count -ne 1 -or
            -not ($expressionLeaves -ccontains
                "Class'KismetMathLibrary:EqualEqual_IntInt'")) {
            continue
        }
        $parameters = @($statement.Expression.Parameters)
        if ($parameters.Count -eq 2 -and
            (@(JsonStringLeaves $parameters[0]) -ccontains
                'RailgunAmmoLastVisualCount') -and
            $parameters[1].Token -ceq 'EX_IntConst' -and
            [int]$parameters[1].Value -eq 0) {
            $statement
        }
    }
}
function ScopeAmmoTintAssignments($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject]) { continue }
        $statementNames = @($statement.PSObject.Properties.Name)
        if (-not ($statementNames -ccontains 'Token') -or
            -not ($statementNames -ccontains 'Expression') -or
            $statement.Token -cne 'EX_Let' -or
            $null -eq $statement.Expression -or
            -not (@(JsonStringLeaves $statement.Expression) -ccontains
                "Class'KismetMathLibrary:SelectColor'")) {
            continue
        }
        $parameters = @($statement.Expression.Parameters)
        if ($parameters.Count -ne 3 -or
            $parameters[0].Token -cne 'EX_StructConst') {
            continue
        }
        $active = @($parameters[0].Properties | ForEach-Object {
            [double]$_.Value
        })
        $inactiveLeaves = @(JsonStringLeaves $parameters[1])
        $conditionLeaves = @(JsonStringLeaves $parameters[2])
        if ($active.Count -eq 4 -and
            [Math]::Abs($active[0] - 0.65) -lt 0.000001 -and
            [Math]::Abs($active[1] - 0.95) -lt 0.000001 -and
            [Math]::Abs($active[2] - 1.0) -lt 0.000001 -and
            [Math]::Abs($active[3] - 0.65) -lt 0.000001 -and
            @($inactiveLeaves | Where-Object {
                $_.StartsWith('CallFunc_SelectColor_ReturnValue',
                    [StringComparison]::Ordinal)
            }).Count -eq 1 -and
            @($conditionLeaves | Where-Object {
                $_.StartsWith('CallFunc_Greater_IntInt_ReturnValue',
                    [StringComparison]::Ordinal)
            }).Count -eq 1) {
            $statement
        }
    }
}
function ScopeEmptyAmmoTintAssignments($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            $statement.Token -cne 'EX_Let' -or
            $null -eq $statement.Expression -or
            -not (@(JsonStringLeaves $statement.Expression) -ccontains
                "Class'KismetMathLibrary:SelectColor'")) {
            continue
        }
        $parameters = @($statement.Expression.Parameters)
        if ($parameters.Count -ne 3 -or
            $parameters[0].Token -cne 'EX_StructConst' -or
            $parameters[1].Token -cne 'EX_StructConst') {
            continue
        }
        $empty = @($parameters[0].Properties | ForEach-Object {
            [double]$_.Value
        })
        $inactive = @($parameters[1].Properties | ForEach-Object {
            [double]$_.Value
        })
        $conditionLeaves = @(JsonStringLeaves $parameters[2])
        if ($empty.Count -eq 4 -and $inactive.Count -eq 4 -and
            [Math]::Abs($empty[0] - 1.0) -lt 0.000001 -and
            [Math]::Abs($empty[1] - 0.25) -lt 0.000001 -and
            [Math]::Abs($empty[2] - 0.25) -lt 0.000001 -and
            [Math]::Abs($empty[3] - 0.3) -lt 0.000001 -and
            [Math]::Abs($inactive[0] - 0.65) -lt 0.000001 -and
            [Math]::Abs($inactive[1] - 0.95) -lt 0.000001 -and
            [Math]::Abs($inactive[2] - 1.0) -lt 0.000001 -and
            [Math]::Abs($inactive[3] - 0.22) -lt 0.000001 -and
            @($conditionLeaves | Where-Object {
                $_.StartsWith('CallFunc_EqualEqual_IntInt_ReturnValue',
                    [StringComparison]::Ordinal)
            }).Count -eq 1) {
            $statement
        }
    }
}
function ChargeTextColorAssignments($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            $statement.Token -cne 'EX_Context') {
            continue
        }
        $targetLeaves = @(JsonStringLeaves $statement.ObjectExpression)
        $callLeaves = @(JsonStringLeaves $statement.ContextExpression)
        if (-not ($targetLeaves -ccontains 'RailgunChargeText') -or
            -not ($callLeaves -ccontains
                "Class'TextBlock:SetColorAndOpacity'")) {
            continue
        }
        $parameters = @($statement.ContextExpression.Parameters)
        Require ($parameters.Count -eq 1 -and
            $parameters[0].Token -ceq 'EX_StructConst' -and
            $parameters[0].Struct.ObjectName -ceq "Class'SlateColor'") `
            'Charge text color call has an unexpected SlateColor shape.'
        $linearColor = $parameters[0].Properties[0]
        Require ($linearColor.Token -ceq 'EX_StructConst' -and
            $linearColor.Struct.ObjectName -ceq "Class'LinearColor'") `
            'Charge text tint does not contain a LinearColor.'
        [pscustomobject]@{
            StatementIndex = [int]$statement.StatementIndex
            Values = @($linearColor.Properties | ForEach-Object {
                [double]$_.Value
            })
        }
    }
}
function ChargeRadialProgressColorAssignments($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            $statement.Token -cne 'EX_Context') {
            continue
        }
        $targetLeaves = @(JsonStringLeaves $statement.ObjectExpression)
        $callLeaves = @(JsonStringLeaves $statement.ContextExpression)
        if (-not ($targetLeaves -ccontains 'RailgunChargeRadial') -or
            -not ($callLeaves -ccontains
                "Class'RadialSlider:SetSliderProgressColor'")) {
            continue
        }
        $parameters = @($statement.ContextExpression.Parameters)
        Require ($parameters.Count -eq 1 -and
            $parameters[0].Token -ceq 'EX_StructConst' -and
            $parameters[0].Struct.ObjectName -ceq "Class'LinearColor'") `
            'Charge radial progress color call has an unexpected shape.'
        [pscustomobject]@{
            StatementIndex = [int]$statement.StatementIndex
            Values = @($parameters[0].Properties | ForEach-Object {
                [double]$_.Value
            })
        }
    }
}
function InsufficientChargeComparisons($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            $statement.Token -cne 'EX_LetBool') {
            continue
        }
        $expressionLeaves = @(JsonStringLeaves $statement.Expression)
        if (($expressionLeaves -ccontains
                "Class'KismetMathLibrary:Less_DoubleDouble'") -and
            ($expressionLeaves -ccontains 'RailgunChargeAmount')) {
            $statement
        }
    }
}
function NativeContextCallIndexes($Statements, [string]$ObjectName) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            -not ($statement.PSObject.Properties.Name -ccontains 'Expression') -or
            $null -eq $statement.Expression -or
            $statement.Expression.Token -cne 'EX_Context' -or
            -not ($statement.Expression.PSObject.Properties.Name -ccontains
                'ContextExpression')) {
            continue
        }
        $context = $statement.Expression.ContextExpression
        if ($null -eq $context -or
            -not ($context.PSObject.Properties.Name -ccontains 'Function')) {
            continue
        }
        $function = $context.Function
        if ($function -is [pscustomobject] -and
            $function.ObjectName -ceq $ObjectName) {
            [int]$statement.StatementIndex
        }
    }
}
function DirectFunctionCalls($Statements, [string]$ObjectName) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject]) { continue }
        foreach ($containerName in @('Expression', 'ContextExpression')) {
            if (-not ($statement.PSObject.Properties.Name -ccontains
                $containerName)) {
                continue
            }
            $container = $statement.$containerName
            if ($null -eq $container -or
                -not ($container.PSObject.Properties.Name -ccontains
                    'Function')) {
                continue
            }
            $function = $container.Function
            if ($function -is [pscustomobject] -and
                $function.ObjectName -ceq $ObjectName) {
                $statement
            }
        }
    }
}
function StatementIndexesContaining($Statements, [string]$Value) {
    foreach ($statement in @($Statements)) {
        if (@(JsonStringLeaves $statement) -ccontains $Value) {
            [int]$statement.StatementIndex
        }
    }
}
function RemoveAmmoResultGates($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            $statement.Token -cne 'EX_LetBool') {
            continue
        }
        $expressionProperty = $statement.PSObject.Properties['Expression']
        if ($null -eq $expressionProperty -or
            $null -eq $expressionProperty.Value) {
            continue
        }
        $expression = $expressionProperty.Value
        if ((@(JsonStringLeaves $expression) -ccontains
                "Class'KismetMathLibrary:EqualEqual_IntInt'") -and
            (@(JsonStringLeaves $expression) -ccontains
                'CallFunc_RemoveItem_ReturnValue') -and
            @($expression.Parameters | Where-Object {
                $_.Token -ceq 'EX_IntConst' -and [int]$_.Value -eq 1
            }).Count -eq 1) {
            $statement
        }
    }
}
function ShotClaimAssignments($Statements) {
    foreach ($statement in @($Statements)) {
        if ($statement -isnot [pscustomobject] -or
            $statement.Token -cne 'EX_LetBool') {
            continue
        }
        $expressionProperty = $statement.PSObject.Properties['Expression']
        if ($null -eq $expressionProperty -or
            $null -eq $expressionProperty.Value) {
            continue
        }
        if ((@(JsonStringLeaves $statement.Variable) -ccontains
                'ShotSpawnedThisPress') -and
            $expressionProperty.Value.Token -ceq 'EX_True') {
            $statement
        }
    }
}
$shell = @(Read-Candidate '/Game/Mods/Railgun/Module/BP_Module_Railgun')
$shellFunctions = @($shell | Where-Object { $_.Type -eq 'Function' })
$expectedShellFunctions = @(
    'ExecuteUbergraph_BP_Module_Railgun','InteractGetInventory',
    'InitializeRailgunStation',
    'OnPersistentActorPostLoad','OnRailgunAmmoInventoryChanged',
    'ReceiveBeginPlay','SyncRailgunAmmoVisuals','ValidateItem'
)
Require (@(Compare-Object -ReferenceObject $expectedShellFunctions `
    -DifferenceObject @($shellFunctions.Name) -CaseSensitive).Count -eq 0) `
    'Shell inventory function set differs from the approved contract.'
$class = @($shell | Where-Object { $_.Type -eq 'BlueprintGeneratedClass' })
Require ($class.Count -eq 1) 'Expected one generated class.'
Require ($class[0].SuperStruct.ObjectName -ceq "Class'VoyageModuleActor'") 'Shell native parent mismatch.'
$interactiveInterface = @($class[0].Interfaces | Where-Object {
    $_.Class.ObjectName -ceq "Class'InteractiveInterface'"
})
Require ($interactiveInterface.Count -eq 0) `
    'InteractiveInterface must be inherited from VoyageModuleActor, not reimplemented.'
$validatorInterface = @($class[0].Interfaces | Where-Object {
    $_.Class.ObjectName -ceq "Class'VoyageInventoryItemValidatorInterface'"
})
Require ($validatorInterface.Count -eq 1 -and $validatorInterface[0].bImplementedByK2) `
    'Railgun validator interface binding mismatch.'
$validateItem = @($shellFunctions | Where-Object { $_.Name -ceq 'ValidateItem' })
Require ($validateItem.Count -eq 1) 'Expected one Railgun item validator.'
$validateItemStrings = @(JsonStringLeaves $validateItem[0])
foreach ($requiredValidatorReference in @(
    "Class'KismetSystemLibrary:IsValid'",
    "Class'KismetMathLibrary:EqualEqual_ObjectObject'",
    "Class'KismetMathLibrary:BooleanAND'",
    'AcceptedRailgunAmmo'
)) {
    Require ($validateItemStrings -ccontains $requiredValidatorReference) `
        ('Railgun exact-item validator reference missing: ' + $requiredValidatorReference)
}
Require (-not ($validateItemStrings -ccontains 'RailgunAmmoInventory')) `
    'Railgun item validator must not require one inventory instance.'
$interactGetInventory = @($shellFunctions | Where-Object {
    $_.Name -ceq 'InteractGetInventory'
})
Require ($interactGetInventory.Count -eq 1 -and
    $interactGetInventory[0].SuperStruct.ObjectName -ceq
        "Class'InteractiveInterface:InteractGetInventory'") `
    'InteractGetInventory must override the exact inherited Voyage interface function.'
$interactGetInventoryStrings = @(JsonStringLeaves $interactGetInventory[0])
Require ($interactGetInventoryStrings -ccontains
    "Class'VoyageModuleComponent:GetInternalInventory'") `
    'InteractGetInventory must return the native module inventory.'
$beginPlay = @($shellFunctions | Where-Object { $_.Name -ceq 'ReceiveBeginPlay' })
Require ($beginPlay.Count -eq 1 -and
    $beginPlay[0].SuperStruct.ObjectName -ceq "Class'Actor:ReceiveBeginPlay'") `
    'Inventory limit initialization must use the exact Actor BeginPlay event.'
$ubergraph = @($shellFunctions | Where-Object {
    $_.Name -ceq 'ExecuteUbergraph_BP_Module_Railgun'
})
Require ($ubergraph.Count -eq 1) 'Expected one Railgun module ubergraph.'
$ubergraphStrings = @(JsonStringLeaves $ubergraph[0])
Require ($ubergraphStrings -ccontains
    "Class'VoyageInventoryWeightLimitedComponent:SetMaxWeightLimit'") `
    'BeginPlay must call the native weight-limit setter.'
Require ($ubergraphStrings -ccontains 'MaxWeightLimit') `
    'Native weight-limit setter must receive the authored component limit.'
Require ($ubergraphStrings -ccontains 'SyncRailgunAmmoVisuals') `
    'Inventory lifecycle must invoke the owned ammo-visual sync function.'
Require ($ubergraphStrings -ccontains 'InitializeRailgunStation') `
    'Shell lifecycle must invoke its owned station initializer.'
foreach ($requiredDelegateReference in @(
    'OnInventoryChanged','OnRailgunAmmoInventoryChanged',
    "Class'InventoryDelegate__DelegateSignature'",
    'EX_AddMulticastDelegate','EX_RemoveMulticastDelegate',
    "Class'KismetSystemLibrary:DelayUntilNextTick'"
)) {
    Require ($ubergraphStrings -ccontains $requiredDelegateReference) `
        ('Ammo-visual event reference missing: ' + $requiredDelegateReference)
}
$inventoryChanged = @($shellFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunAmmoInventoryChanged'
})
Require ($inventoryChanged.Count -eq 1 -and
    -not ($inventoryChanged[0].PSObject.Properties.Name -ccontains
        'ChildProperties') -and
    (@(JsonStringLeaves $inventoryChanged[0]) -ccontains
        'SyncRailgunAmmoVisuals')) `
    'Ammo-visual callback must be parameterless and invoke the owned sync function.'
$postLoad = @($shellFunctions | Where-Object {
    $_.Name -ceq 'OnPersistentActorPostLoad'
})
Require ($postLoad.Count -eq 1 -and
    $postLoad[0].SuperStruct.ObjectName -ceq
        "Class'PersistentInterface:OnPersistentActorPostLoad'") `
    'Post-load refresh must override the exact inherited Voyage interface event.'
$initializeStation = @($shellFunctions | Where-Object {
    $_.Name -ceq 'InitializeRailgunStation'
})
Require ($initializeStation.Count -eq 1) `
    'Expected one shell-owned station initialization function.'
$initializeStationStrings = @(JsonStringLeaves $initializeStation[0])
foreach ($requiredInitializationReference in @(
    'NativeStation','RailgunModelEntryReference','RailgunEntryAction',
    'ExpectedVehicleInputContext','Railgun.Model.Root',
    "Class'Actor:HasAuthority'",
    "Class'Actor:GetComponentsByTag'",
    "Class'GameplayStatics:BeginDeferredActorSpawnFromClass'",
    "Class'GameplayStatics:FinishSpawningActor'",
    "Class'Actor:K2_AttachToComponent'",
    "Class'KismetSystemLibrary:LoadAsset_Blocking'",
    'BindRailgunChargeIndicator'
)) {
    Require ($initializeStationStrings -ccontains
        $requiredInitializationReference) `
        ('Station initialization reference missing: ' +
            $requiredInitializationReference)
}
foreach ($forbiddenDiscoveryReference in @(
    "Class'GameplayStatics:GetAllActorsOfClass'",
    "Class'GameplayStatics:GetPlayerController'"
)) {
    Require (-not ($initializeStationStrings -ccontains
        $forbiddenDiscoveryReference)) `
        ('Station initialization retained global discovery: ' +
            $forbiddenDiscoveryReference)
}
$shellStrings = @(JsonStringLeaves $shell)
Require (-not ($shellStrings -ccontains 'BP_RailgunCoordinator')) `
    'Shell retained the removed global coordinator identity.'
Require (-not ($shellStrings -ccontains 'ModuleMountCollision')) `
    'Removed module-mount collision identity was serialized.'
Require (-not ($shellStrings -ccontains 'RailgunAmmoVisualSyncElapsed')) `
    'Removed ammo-visual polling accumulator was serialized.'
$syncVisuals = @($shellFunctions | Where-Object {
    $_.Name -ceq 'SyncRailgunAmmoVisuals'
})
Require ($syncVisuals.Count -eq 1) 'Expected one ammo-visual sync function.'
$syncVisualStrings = @(JsonStringLeaves $syncVisuals[0])
foreach ($requiredVisualReference in @(
    'RailgunAmmoInventory','AcceptedRailgunAmmo','Items','ItemCount',
    "Class'BlueprintMapLibrary:Map_Values'",
    "Class'KismetMathLibrary:Add_IntInt'",
    "Class'KismetMathLibrary:Clamp'",
    "Class'KismetMathLibrary:NotEqual_IntInt'",
    "Class'SceneComponent:SetHiddenInGame'"
)) {
    Require ($syncVisualStrings -ccontains $requiredVisualReference) `
        ('Ammo-visual sync reference missing: ' + $requiredVisualReference)
}
$operator = @(Read-Candidate $operatorPackage)
$operatorFunctions = @($operator | Where-Object { $_.Type -ceq 'Function' })
$operatorUbergraph = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ExecuteUbergraph_BP_RailgunOperator'
})
Require ($operatorUbergraph.Count -eq 1) 'Expected one Railgun operator ubergraph.'
$operatorStatements = @($operatorUbergraph[0].ScriptBytecode)
$operatorStrings = @(JsonStringLeaves $operatorUbergraph[0])
foreach ($requiredFireReference in @(
    'ShotSpawnedThisPress','ShotRefundFaulted','ShotEnergyBeforeDebit',
    'ShotAmmoSlot','AcceptedRailgunAmmo','ItemCount',
    "Class'VoyageModuleComponent:GetInternalInventory'",
    "Class'VoyageBaseInventoryComponent:GetLastOccupiedSlot'",
    "Class'VoyageBaseInventoryComponent:GetSlot'",
    "Class'VoyageModuleComponent:RemoveResource'",
    "Class'VoyageBaseInventoryComponent:RemoveItem'",
    "Class'VoyageModuleComponent:AddResource'",
    "Class'GameplayStatics:BeginDeferredActorSpawnFromClass'",
    "Class'GameplayStatics:FinishSpawningActor'",
    "Class'GameplayStatics:PlaySoundAtLocation'",
    'K2_DestroyActor',
    "Class'KismetSystemLibrary:PrintString'"
)) {
    Require ($operatorStrings -ccontains $requiredFireReference) `
        ('Railgun fire contract reference missing: ' + $requiredFireReference)
}
foreach ($forbiddenFireReference in @(
    'Items','RailgunAmmoLastVisualCount','AddItem'
)) {
    Require (-not ($operatorStrings -ccontains $forbiddenFireReference)) `
        ('Railgun fire must not use presentation state or direct mutation: ' +
            $forbiddenFireReference)
}
$chargeIndicatorBind = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'BindRailgunChargeIndicator'
})
Require ($chargeIndicatorBind.Count -eq 1) `
    'Expected one event-driven charge-indicator binding function.'
$chargeIndicatorBindStrings = @(JsonStringLeaves $chargeIndicatorBind[0])
foreach ($requiredChargeIndicatorReference in @(
    'RailgunChargeIndicatorOwner',
    'RailgunChargeIndicatorComponent',
    'RailgunChargeIndicatorMaterial',
    'RailgunChargeIndicatorLastLevel',
    'RailgunChargeIndicatorModule',
    'OnModuleValueChanged',
    'OnRailgunChargeIndicatorModuleValueChanged',
    "Class'VoyageModuleComponent:GetResourceAmount'",
    'EX_AddMulticastDelegate',
    'EX_RemoveMulticastDelegate',
    'Railgun.Model.ChargeIndicator',
    'ProgressLevel',
    'CreateDynamicMaterialInstance',
    "Class'MaterialInstanceDynamic:SetScalarParameterValue'",
    '/Game/Materials/Modules/MI_PogressBar_Basic_LED.MI_PogressBar_Basic_LED'
)) {
    Require ($chargeIndicatorBindStrings -ccontains
        $requiredChargeIndicatorReference) `
        ('Railgun charge-indicator contract reference missing: ' +
            $requiredChargeIndicatorReference)
}
$chargeIndicatorCallback = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunChargeIndicatorModuleValueChanged'
})
Require ($chargeIndicatorCallback.Count -eq 1) `
    'Expected one charge-indicator callback.'
$chargeIndicatorParameters = @($chargeIndicatorCallback[0].ChildProperties |
    Where-Object {
        $_.PSObject.Properties.Name -ccontains 'PropertyFlags' -and
        $_.PropertyFlags -match '(^| \| )Parm($| \| )'
    })
Require ($chargeIndicatorParameters.Count -eq 1 -and
    $chargeIndicatorParameters[0].Name -ceq 'Module' -and
    $chargeIndicatorParameters[0].Type -ceq 'ObjectProperty' -and
    $chargeIndicatorParameters[0].PropertyClass.ObjectName -ceq
        "Class'VoyageModuleComponent'" -and
    $chargeIndicatorParameters[0].PropertyClass.ObjectPath -ceq '/Script/Voyage') `
    'Charge-indicator callback must own the exact Voyage module delegate signature.'
$chargeIndicatorCallbackStrings = @(
    JsonStringLeaves $chargeIndicatorCallback[0])
foreach ($requiredCallbackReference in @(
    'Module','RailgunChargeIndicatorModule',
    "Class'VoyageModuleComponent:GetResourceAmount'",
    "Class'MaterialInstanceDynamic:SetScalarParameterValue'",
    'ProgressLevel'
)) {
    Require ($chargeIndicatorCallbackStrings -ccontains
        $requiredCallbackReference) `
        ('Charge-indicator callback reference missing: ' +
            $requiredCallbackReference)
}
foreach ($forbiddenCallbackReference in @(
    "Class'VoyageModuleComponent:AddResource'",
    "Class'VoyageModuleComponent:RemoveResource'",
    "Class'VoyageModuleComponent:SetCustomConsumption'",
    "Class'KismetSystemLibrary:Delay",
    "Class'GameplayStatics:GetAllActorsOfClass'"
)) {
    Require (-not ($chargeIndicatorCallbackStrings -ccontains
        $forbiddenCallbackReference)) `
        ('Charge-indicator callback must remain visual-only: ' +
            $forbiddenCallbackReference)
}
foreach ($requiredChargeTeardownReference in @(
    'RailgunChargeIndicatorModule',
    'OnRailgunChargeIndicatorModuleValueChanged',
    'EX_RemoveMulticastDelegate'
)) {
    Require ($operatorStrings -ccontains $requiredChargeTeardownReference) `
        ('Charge-indicator teardown reference missing: ' +
            $requiredChargeTeardownReference)
}
Require (-not ($operatorStrings -ccontains
    "Class'MaterialInstanceDynamic:SetScalarParameterValue'")) `
    'Operator tick/fire/end-play graph must not poll or write the charge indicator.'
Require (-not ($operatorStrings -ccontains "Class'GameplayStatics:GetAllActorsOfClass'")) `
    'Railgun charge indicator must not add a world actor scan.'
$removeEnergyCallIndexes = @(NativeContextCallIndexes $operatorStatements `
    "Class'VoyageModuleComponent:RemoveResource'")
$removeAmmoCallIndexes = @(NativeContextCallIndexes $operatorStatements `
    "Class'VoyageBaseInventoryComponent:RemoveItem'")
$refundCallIndexes = @(NativeContextCallIndexes $operatorStatements `
    "Class'VoyageModuleComponent:AddResource'")
Require ($removeEnergyCallIndexes.Count -eq 2) `
    'Railgun operator must contain one shot debit and one offline idle drain.'
$offlineDrainStatements = @($operatorStatements | Where-Object {
    if ($_ -isnot [pscustomobject] -or
        -not ($_.PSObject.Properties.Name -ccontains 'Expression') -or
        $null -eq $_.Expression -or
        $_.Expression.Token -cne 'EX_Context' -or
        -not ($_.Expression.PSObject.Properties.Name -ccontains
            'ContextExpression')) {
        return $false
    }
    $context = $_.Expression.ContextExpression
    if ($null -eq $context -or
        -not ($context.PSObject.Properties.Name -ccontains 'Function') -or
        $context.Function -isnot [pscustomobject] -or
        $context.Function.ObjectName -cne
            "Class'VoyageModuleComponent:RemoveResource'" -or
        @($context.Parameters).Count -ne 3) {
        return $false
    }
    $amount = $context.Parameters[1]
    $amount.Token -ceq 'EX_LocalVariable' -and
        $amount.Variable.Property.Name -clike
            'CallFunc_FMin_ReturnValue*'
})
Require ($offlineDrainStatements.Count -eq 1) `
    'Offline idle drain must debit the amount capped by stored energy.'
$offlineDrainMath = @($operatorStatements | Where-Object {
    if ($_ -isnot [pscustomobject] -or
        -not ($_.PSObject.Properties.Name -ccontains 'Expression') -or
        $null -eq $_.Expression -or
        $_.Expression.Token -cne 'EX_CallMath' -or
        -not ($_.Expression.PSObject.Properties.Name -ccontains
            'Function') -or
        $_.Expression.Function -isnot [pscustomobject] -or
        $_.Expression.Function.ObjectName -cne
            "Class'KismetMathLibrary:Divide_DoubleDouble'") {
        return $false
    }
    @($_.Expression.Parameters | Where-Object {
        $_.Token -ceq 'EX_DoubleConst' -and
        [Math]::Abs([double]$_.Value - 3600.0) -lt 0.000001
    }).Count -eq 1
})
Require ($offlineDrainMath.Count -eq 1 -and
    ($operatorStrings -ccontains
        "Class'VoyageModuleComponent:HasSocketConnection'") -and
    ($operatorStrings -ccontains "Class'VoyageModuleComponent:HasPower'") -and
    ($operatorStrings -ccontains 'RailgunOfflineDischargeKW')) `
    'Offline idle drain must use supply state and convert W-seconds to stored energy.'
Require ($removeAmmoCallIndexes.Count -eq 1) `
    'Railgun fire must contain exactly one native ammo debit.'
Require ($refundCallIndexes.Count -eq 1) `
    'Railgun fire must contain exactly one bounded energy refund path.'
$removeAmmoStatement = @($operatorStatements | Where-Object {
    [int]$_.StatementIndex -eq [int]$removeAmmoCallIndexes[0]
})
Require ($removeAmmoStatement.Count -eq 1) `
    'Native ammo debit statement could not be resolved.'
$removeAmmoParameters = @(
    $removeAmmoStatement[0].Expression.ContextExpression.Parameters
)
Require ($removeAmmoParameters.Count -eq 4 -and
    (@(JsonStringLeaves $removeAmmoParameters[0]) -ccontains
        'AcceptedRailgunAmmo') -and
    $removeAmmoParameters[1].Token -ceq 'EX_IntConst' -and
    [int]$removeAmmoParameters[1].Value -eq 1 -and
    (@(JsonStringLeaves $removeAmmoParameters[2]) -ccontains
        'ShotAmmoSlot') -and
    $removeAmmoParameters[3].Token -ceq 'EX_True') `
    'Native ammo debit must use exact owned ammo, quantity one, captured slot and notifications.'
$removeAmmoResultGates = @(RemoveAmmoResultGates $operatorStatements)
Require ($removeAmmoResultGates.Count -eq 1) `
    'Railgun fire must gate success on RemoveItem returning exactly one.'
$shotDeferredCalls = @(DirectFunctionCalls $operatorStatements `
    "Class'GameplayStatics:BeginDeferredActorSpawnFromClass'" | Where-Object {
        @(JsonStringLeaves $_) -ccontains
            "BlueprintGeneratedClass'BP_RailgunTestShot_C'"
    })
Require ($shotDeferredCalls.Count -eq 1) `
    'Railgun fire must preflight exactly one deferred shot actor.'
$claimAssignments = @(ShotClaimAssignments $operatorStatements)
Require ($claimAssignments.Count -eq 1) `
    'Railgun fire must claim each input request exactly once.'
$finishIndexes = @(StatementIndexesContaining $operatorStatements `
    "Class'GameplayStatics:FinishSpawningActor'" | Where-Object {
        $_ -gt [int]$removeAmmoResultGates[0].StatementIndex
    } | Sort-Object)
$audioIndexes = @(StatementIndexesContaining $operatorStatements `
    "Class'GameplayStatics:PlaySoundAtLocation'" | Where-Object {
        $_ -gt [int]$removeAmmoResultGates[0].StatementIndex
    } | Sort-Object)
$getSlotIndexes = @(StatementIndexesContaining $operatorStatements `
    "Class'VoyageBaseInventoryComponent:GetSlot'")
Require ($finishIndexes.Count -ge 1 -and $audioIndexes.Count -ge 1 -and
    $getSlotIndexes.Count -ge 1) `
    'Railgun fire ordering evidence is incomplete.'
$shotBeginIndex = [int]$shotDeferredCalls[0].StatementIndex
$claimIndex = [int]$claimAssignments[0].StatementIndex
$removeAmmoIndex = [int]$removeAmmoCallIndexes[0]
$shotEnergyCallIndexes = @($removeEnergyCallIndexes | Where-Object {
    [int]$_ -gt $claimIndex -and [int]$_ -lt $removeAmmoIndex
})
Require ($shotEnergyCallIndexes.Count -eq 1) `
    'Railgun fire must contain exactly one native energy debit after its claim.'
$removeEnergyIndex = [int]$shotEnergyCallIndexes[0]
$offlineDrainCallIndexes = @(
    $offlineDrainStatements | ForEach-Object { [int]$_.StatementIndex }
)
Require ($offlineDrainCallIndexes.Count -eq 1 -and
    [int]$offlineDrainCallIndexes[0] -ne $removeEnergyIndex) `
    'Offline idle drain and shot energy debit must remain distinct.'
$removeAmmoGateIndex = [int]$removeAmmoResultGates[0].StatementIndex
$shotFinishIndex = [int]$finishIndexes[0]
$shotAudioIndex = [int]$audioIndexes[0]
$refundIndex = [int]$refundCallIndexes[0]
Require ((($getSlotIndexes | Measure-Object -Maximum).Maximum) -lt
        $shotBeginIndex -and
    $shotBeginIndex -lt $claimIndex -and
    $claimIndex -lt $removeEnergyIndex -and
    $removeEnergyIndex -lt $removeAmmoIndex -and
    $removeAmmoIndex -lt $removeAmmoGateIndex -and
    $removeAmmoGateIndex -lt $shotFinishIndex -and
    $shotFinishIndex -lt $shotAudioIndex -and
    $shotAudioIndex -lt $refundIndex) `
    'Railgun fire must preflight, claim, debit both resources and only then activate the shot.'
$shotDestroyIndexes = @(StatementIndexesContaining $operatorStatements `
    'K2_DestroyActor' | Where-Object {
        $_ -gt $shotBeginIndex -and $_ -lt ($refundIndex + 1000)
    })
Require ($shotDestroyIndexes.Count -eq 3) `
    'Railgun fire must destroy the deferred shot on cast, energy and ammo failure paths.'
$refundBalanceReads = @(StatementIndexesContaining $operatorStatements `
    "Class'VoyageModuleComponent:GetResourceAmount'" | Where-Object {
        $_ -gt $refundIndex -and $_ -lt ($refundIndex + 1000)
    })
Require ($refundBalanceReads.Count -ge 1 -and
    @($operatorStrings | Where-Object { $_ -ceq 'ShotEnergyBeforeDebit' }).Count -ge 2 -and
    @($operatorStrings | Where-Object { $_ -ceq 'ShotRefundFaulted' }).Count -ge 2) `
    'Energy compensation must verify the restored live balance and fail closed.'
$hud = @(Read-Candidate $hudPackage)
$hudFunctions = @($hud | Where-Object { $_.Type -ceq 'Function' })
$hudUbergraph = @($hudFunctions | Where-Object {
    $_.Name -ceq 'ExecuteUbergraph_WBP_RailgunHUD'
})
Require ($hudUbergraph.Count -eq 1) 'Expected one Railgun HUD ubergraph.'
$hudUbergraphStrings = @(JsonStringLeaves $hudUbergraph[0])
Require (@($hudUbergraphStrings | Where-Object {
    $_ -ceq 'SyncRailgunAmmoVisuals'
}).Count -eq 1) 'HUD must perform exactly one guarded initial ammo sync.'
Require ($hudUbergraphStrings -ccontains 'RailgunAmmoHudInitialized') `
    'HUD initial ammo-sync guard is missing.'
Require ($hudUbergraphStrings -ccontains 'RailgunAmmoLastVisualCount') `
    'HUD does not read the event-maintained ammo count cache.'
Require (@($hudUbergraphStrings | Where-Object {
    $_ -ceq "Class'Image:SetColorAndOpacity'"
}).Count -eq 12) 'HUD must tint exactly six wide and six scope ammo indicators.'
$emptyAmmoTintAssignments = @(
    EmptyAmmoTintAssignments $hudUbergraph[0].ScriptBytecode
)
Require ($emptyAmmoTintAssignments.Count -eq 6) `
    'HUD must apply the subtle red empty-magazine tint to all six indicators.'
$emptyAmmoZeroComparisons = @(
    EmptyAmmoZeroComparisons $hudUbergraph[0].ScriptBytecode
)
Require ($emptyAmmoZeroComparisons.Count -eq 12) `
    'Every HUD empty-magazine tint must use an exact zero-count comparison.'
$scopeAmmoTintAssignments = @(
    ScopeAmmoTintAssignments $hudUbergraph[0].ScriptBytecode
)
Require ($scopeAmmoTintAssignments.Count -eq 6) `
    'Scope ammo indicators must use the independent blue active/inactive tints.'
$scopeEmptyAmmoTintAssignments = @(
    ScopeEmptyAmmoTintAssignments $hudUbergraph[0].ScriptBytecode
)
Require ($scopeEmptyAmmoTintAssignments.Count -eq 6) `
    'Scope ammo indicators must use the wide HUD red tint only at zero rounds.'
$chargeTextColors = @(ChargeTextColorAssignments $hudUbergraph[0].ScriptBytecode)
Require ($chargeTextColors.Count -eq 3) `
    'HUD must set charge text colors for invalid, charging and ready paths.'
$insufficientChargeTextColors = @($chargeTextColors | Where-Object {
    $_.Values.Count -eq 4 -and $_.Values[0] -eq 1.0 -and
    $_.Values[1] -eq 0.25 -and $_.Values[2] -eq 0.25 -and
    $_.Values[3] -eq 0.3
})
Require ($insufficientChargeTextColors.Count -eq 1) `
    'HUD must tint insufficient charge text subtle red.'
$readyChargeTextColors = @($chargeTextColors | Where-Object {
    $_.Values.Count -eq 4 -and $_.Values[0] -eq 1.0 -and
    $_.Values[1] -eq 1.0 -and $_.Values[2] -eq 1.0 -and
    $_.Values[3] -eq 1.0
})
Require ($readyChargeTextColors.Count -eq 2) `
    'HUD must keep invalid and sufficient charge text opaque white.'
$chargeRadialProgressColors = @(
    ChargeRadialProgressColorAssignments $hudUbergraph[0].ScriptBytecode
)
Require ($chargeRadialProgressColors.Count -eq 3) `
    'HUD must set charge ring colors for invalid, charging and ready paths.'
$insufficientChargeRingColors = @($chargeRadialProgressColors | Where-Object {
    $_.Values.Count -eq 4 -and $_.Values[0] -eq 1.0 -and
    $_.Values[1] -eq 0.25 -and $_.Values[2] -eq 0.25 -and
    $_.Values[3] -eq 0.3
})
Require ($insufficientChargeRingColors.Count -eq 1) `
    'HUD must tint the insufficient charge ring subtle red.'
$readyChargeRingColors = @($chargeRadialProgressColors | Where-Object {
    $_.Values.Count -eq 4 -and $_.Values[0] -eq 1.0 -and
    $_.Values[1] -eq 1.0 -and $_.Values[2] -eq 1.0 -and
    $_.Values[3] -eq 1.0
})
Require ($readyChargeRingColors.Count -eq 2) `
    'HUD must keep invalid and sufficient charge rings opaque white.'
$insufficientChargeComparisons = @(
    InsufficientChargeComparisons $hudUbergraph[0].ScriptBytecode
)
Require ($insufficientChargeComparisons.Count -eq 1) `
    'Charge text tint must use one exact current-charge threshold comparison.'
Require (-not ($hudUbergraphStrings -ccontains
    "Class'RadialSlider:SetSliderBarColor'")) `
    'Insufficient-charge tint must not alter the radial gauge background.'
Require (-not ($hudUbergraphStrings -ccontains 'SliderProgressColor')) `
    'Ammo indicators must not inherit the dynamic charge ring color.'
$ammoActivationThresholds = @(AmmoActivationThresholds $hudUbergraph[0])
Require (@(Compare-Object -ReferenceObject @(5,4,3,2,1,0,5,4,3,2,1,0) `
    -DifferenceObject $ammoActivationThresholds -SyncWindow 0).Count -eq 0) `
    'Wide and scope ammo indicators must activate from right to left.'
foreach ($forbiddenHudInventoryReference in @(
    'Items', "Class'BlueprintMapLibrary:Map_Values'", 'OnInventoryChanged'
)) {
    Require (-not ($hudUbergraphStrings -ccontains
        $forbiddenHudInventoryReference)) `
        ('HUD must not poll or bind inventory data directly: ' +
            $forbiddenHudInventoryReference)
}
$visibilityTargets = @(VisibilityTargets $hudUbergraph[0])
Require (@($visibilityTargets | Where-Object {
    $_ -ceq 'RailgunChargeRadial'
}).Count -eq 2) 'Wide/optics visibility gate does not own the charge radial.'
Require (@($visibilityTargets | Where-Object {
    $_ -ceq 'RailgunChargeBlock'
}).Count -eq 2) 'Wide/optics visibility gate does not own the charge block.'
Require (@($visibilityTargets | Where-Object {
    $_ -ceq 'RailgunChargeText'
}).Count -eq 0) 'Charge text has a duplicate direct visibility gate.'
Require (@($visibilityTargets | Where-Object {
    $_ -ceq 'RailgunScopeAmmoIndicatorRow'
}).Count -eq 2) 'Wide/optics visibility gate does not own the scope ammo row.'
$indicatorNames = @(
    'RailgunAmmoIndicator01','RailgunAmmoIndicator02',
    'RailgunAmmoIndicator03','RailgunAmmoIndicator04',
    'RailgunAmmoIndicator05','RailgunAmmoIndicator06'
)
$indicators = @($hud | Where-Object {
    $_.Type -ceq 'Image' -and $indicatorNames -ccontains $_.Name
})
Require (@(Compare-Object -ReferenceObject $indicatorNames `
    -DifferenceObject @($indicators.Name) -CaseSensitive).Count -eq 0) `
    'HUD ammo-indicator identity set differs from the six-slot contract.'
$chargeRadial = @($hud | Where-Object {
    $_.Type -ceq 'RadialSlider' -and $_.Name -ceq 'RailgunChargeRadial'
})
Require ($chargeRadial.Count -eq 1) 'HUD charge radial is missing or duplicated.'
$faintColor = $chargeRadial[0].Properties.SliderBarColor
foreach ($indicatorName in $indicatorNames) {
    $indicator = @($indicators | Where-Object { $_.Name -ceq $indicatorName })
    Require ($indicator.Count -eq 1 -and
        $indicator[0].Properties.Visibility -ceq
            'ESlateVisibility::HitTestInvisible') `
        ('Ammo indicator is not persistent: ' + $indicatorName)
    $brush = $indicator[0].Properties.Brush
    Require ($brush.ResourceObject.ObjectPath -ceq
        ($ammoIndicatorPackage + '.0')) `
        ('Ammo indicator texture mismatch: ' + $indicatorName)
    Require ([Math]::Abs([double]$brush.ImageSize.Y - 30.0) -lt 0.0001 -and
        [Math]::Abs([double]$brush.UVRegion.Min.X - (548.0 / 1254.0)) -lt
            0.000001 -and
        [Math]::Abs([double]$brush.UVRegion.Min.Y - (372.0 / 1254.0)) -lt
            0.000001 -and
        [Math]::Abs([double]$brush.UVRegion.Max.X - (706.0 / 1254.0)) -lt
            0.000001 -and
        [Math]::Abs([double]$brush.UVRegion.Max.Y - (895.0 / 1254.0)) -lt
            0.000001) ('Ammo indicator crop/size mismatch: ' + $indicatorName)
    $color = $indicator[0].Properties.ColorAndOpacity
    Require ([Math]::Abs([double]$color.R - [double]$faintColor.R) -lt 0.000001 -and
        [Math]::Abs([double]$color.G - [double]$faintColor.G) -lt 0.000001 -and
        [Math]::Abs([double]$color.B - [double]$faintColor.B) -lt 0.000001 -and
        [Math]::Abs([double]$color.A - [double]$faintColor.A) -lt 0.000001) `
        ('Ammo indicator faint color differs from radial bar: ' + $indicatorName)
}
$scopeIndicatorNames = @(
    'RailgunScopeAmmoIndicator01','RailgunScopeAmmoIndicator02',
    'RailgunScopeAmmoIndicator03','RailgunScopeAmmoIndicator04',
    'RailgunScopeAmmoIndicator05','RailgunScopeAmmoIndicator06'
)
$scopeIndicators = @($hud | Where-Object {
    $_.Type -ceq 'Image' -and $scopeIndicatorNames -ccontains $_.Name
})
Require (@(Compare-Object -ReferenceObject $scopeIndicatorNames `
    -DifferenceObject @($scopeIndicators.Name) -CaseSensitive).Count -eq 0) `
    'Scope ammo-indicator identity set differs from the six-slot contract.'
foreach ($indicatorName in $scopeIndicatorNames) {
    $indicator = @($scopeIndicators | Where-Object {
        $_.Name -ceq $indicatorName
    })
    Require ($indicator.Count -eq 1 -and
        $indicator[0].Properties.Visibility -ceq
            'ESlateVisibility::HitTestInvisible') `
        ('Scope ammo indicator is not persistent: ' + $indicatorName)
    $brush = $indicator[0].Properties.Brush
    Require ($brush.ResourceObject.ObjectPath -ceq
        ($ammoIndicatorPackage + '.0') -and
        [Math]::Abs([double]$brush.ImageSize.Y - 38.0) -lt 0.0001 -and
        [Math]::Abs([double]$brush.UVRegion.Min.X - (548.0 / 1254.0)) -lt
            0.000001 -and
        [Math]::Abs([double]$brush.UVRegion.Min.Y - (372.0 / 1254.0)) -lt
            0.000001 -and
        [Math]::Abs([double]$brush.UVRegion.Max.X - (706.0 / 1254.0)) -lt
            0.000001 -and
        [Math]::Abs([double]$brush.UVRegion.Max.Y - (895.0 / 1254.0)) -lt
            0.000001) `
        ('Scope ammo indicator crop/size mismatch: ' + $indicatorName)
    $color = $indicator[0].Properties.ColorAndOpacity
    Require ([Math]::Abs([double]$color.R - 0.65) -lt 0.000001 -and
        [Math]::Abs([double]$color.G - 0.95) -lt 0.000001 -and
        [Math]::Abs([double]$color.B - 1.0) -lt 0.000001 -and
        [Math]::Abs([double]$color.A - 0.22) -lt 0.000001) `
        ('Scope ammo indicator initial tint mismatch: ' + $indicatorName)
}
$ammoRowSlots = @($hud | Where-Object {
    $_.Type -ceq 'HorizontalBoxSlot' -and
    $_.Outer.ObjectName -ceq
        "HorizontalBox'WBP_RailgunHUD_C:WidgetTree.RailgunAmmoIndicatorRow'"
} | Sort-Object Name)
Require ($ammoRowSlots.Count -eq 6) 'Ammo indicator row must retain six slots.'
for ($index = 0; $index -lt $indicatorNames.Count; $index++) {
    Require ($ammoRowSlots[$index].Properties.Content.ObjectName.EndsWith(
        '.' + $indicatorNames[$index] + "'", [StringComparison]::Ordinal) -and
        [Math]::Abs([double]$ammoRowSlots[$index].Properties.Padding.Left - 3.0) -lt
            0.0001 -and
        [Math]::Abs([double]$ammoRowSlots[$index].Properties.Padding.Right - 3.0) -lt
            0.0001) 'Ammo indicator row order/gap mismatch.'
}
$scopeAmmoRowSlots = @($hud | Where-Object {
    $_.Type -ceq 'HorizontalBoxSlot' -and
    $_.Outer.ObjectName -ceq
        "HorizontalBox'WBP_RailgunHUD_C:WidgetTree.RailgunScopeAmmoIndicatorRow'"
} | Sort-Object Name)
Require ($scopeAmmoRowSlots.Count -eq 6) `
    'Scope ammo indicator row must retain six slots.'
for ($index = 0; $index -lt $scopeIndicatorNames.Count; $index++) {
    Require ($scopeAmmoRowSlots[$index].Properties.Content.ObjectName.EndsWith(
        '.' + $scopeIndicatorNames[$index] + "'", [StringComparison]::Ordinal) -and
        [Math]::Abs([double]$scopeAmmoRowSlots[$index].Properties.Padding.Left - 7.0) -lt
            0.0001 -and
        [Math]::Abs([double]$scopeAmmoRowSlots[$index].Properties.Padding.Right - 7.0) -lt
        0.0001) 'Scope ammo indicator row order/gap mismatch.'
}
$scopeAmmoRow = @($hud | Where-Object {
    $_.Type -ceq 'HorizontalBox' -and
    $_.Name -ceq 'RailgunScopeAmmoIndicatorRow'
})
Require ($scopeAmmoRow.Count -eq 1 -and
    $scopeAmmoRow[0].Properties.Visibility -ceq
        'ESlateVisibility::Collapsed') `
    'Scope ammo row must start hidden until the optics gate shows it.'
$scopeAmmoCanvasSlot = @($hud | Where-Object {
    $_.Type -ceq 'CanvasPanelSlot' -and
    $_.Properties.Content.ObjectName.EndsWith(
        ".RailgunScopeAmmoIndicatorRow'", [StringComparison]::Ordinal)
})
Require ($scopeAmmoCanvasSlot.Count -eq 1) `
    'Scope ammo row must have exactly one canvas slot.'
$scopeAmmoLayout = $scopeAmmoCanvasSlot[0].Properties.LayoutData
Require ($scopeAmmoCanvasSlot[0].Properties.bAutoSize -and
    [Math]::Abs([double]$scopeAmmoLayout.Offsets.Top - -252.0) -lt 0.0001 -and
    [Math]::Abs([double]$scopeAmmoLayout.Anchors.Minimum.X - 0.5) -lt 0.0001 -and
    [Math]::Abs([double]$scopeAmmoLayout.Anchors.Minimum.Y - 0.5) -lt 0.0001 -and
    [Math]::Abs([double]$scopeAmmoLayout.Anchors.Maximum.X - 0.5) -lt 0.0001 -and
    [Math]::Abs([double]$scopeAmmoLayout.Anchors.Maximum.Y - 0.5) -lt 0.0001 -and
    [Math]::Abs([double]$scopeAmmoLayout.Alignment.X - 0.5) -lt 0.0001 -and
    [Math]::Abs([double]$scopeAmmoLayout.Alignment.Y - 0.5) -lt 0.0001) `
    'Scope ammo row is not mathematically centered in the scope coordinate system.'
$scopeStatusIcons = @($hud | Where-Object {
    $_.Type -ceq 'Image' -and $_.Name -in @(
        'RailgunStatusCharging','RailgunStatusOffline','RailgunStatusReady')
})
Require ($scopeStatusIcons.Count -eq 3) `
    'Scope energy-status icon set differs from the approved contract.'
foreach ($statusIcon in $scopeStatusIcons) {
    Require ([Math]::Abs([double]$statusIcon.Properties.Brush.ImageSize.Y - 54.0) -lt
        0.0001) 'Existing scope energy indicator height changed.'
}
$statusCanvasSlots = @($hud | Where-Object {
    $_.Type -ceq 'CanvasPanelSlot' -and
    $_.Properties.Content.ObjectName -match
        '\.RailgunStatus(Charging|Offline|Ready)''$'
})
Require ($statusCanvasSlots.Count -eq 3) `
    'Scope energy-status canvas layout is incomplete.'
foreach ($statusSlot in $statusCanvasSlots) {
    $statusLayout = $statusSlot.Properties.LayoutData
    Require ([Math]::Abs([double]$statusLayout.Offsets.Top - -190.0) -lt 0.0001 -and
        [Math]::Abs([double]$statusLayout.Anchors.Minimum.X - 0.5) -lt 0.0001 -and
        [Math]::Abs([double]$statusLayout.Anchors.Minimum.Y - 0.5) -lt 0.0001 -and
        [Math]::Abs([double]$statusLayout.Alignment.X - 0.5) -lt 0.0001 -and
        [Math]::Abs([double]$statusLayout.Alignment.Y - 0.5) -lt 0.0001) `
        'Existing scope energy-status layout changed.'
}
$visibleScopeGap = (-190.0 - 54.0 * 0.5) - (-252.0 + 38.0 * 0.5)
Require ([Math]::Abs($visibleScopeGap - 16.0) -lt 0.0001) `
    'Scope ammo/energy visible gap differs from the fixed 70-percent layout.'
$chargeBlockSlots = @($hud | Where-Object {
    $_.Type -ceq 'VerticalBoxSlot' -and
    $_.Outer.ObjectName -ceq
        "VerticalBox'WBP_RailgunHUD_C:WidgetTree.RailgunChargeBlock'"
} | Sort-Object Name)
Require ($chargeBlockSlots.Count -eq 2 -and
    $chargeBlockSlots[0].Properties.Content.ObjectName.EndsWith(
        ".RailgunAmmoIndicatorRow'", [StringComparison]::Ordinal) -and
    $chargeBlockSlots[1].Properties.Content.ObjectName.EndsWith(
        ".RailgunChargeText'", [StringComparison]::Ordinal) -and
    [Math]::Abs([double]$chargeBlockSlots[0].Properties.Padding.Bottom - 5.0) -lt
        0.0001) 'Ammo row and charge text vertical layout mismatch.'
$chargeText = @($hud | Where-Object {
    $_.Type -ceq 'TextBlock' -and $_.Name -ceq 'RailgunChargeText'
})
Require ($chargeText.Count -eq 1 -and
    $chargeText[0].Properties.Text.SourceString -ceq '0.0 KWh' -and
    [Math]::Abs([double]$chargeText[0].Properties.Font.Size - 14.0) -lt 0.0001) `
    'Existing charge text presentation changed.'
$ammoIndicatorTexture = @(Read-Candidate $ammoIndicatorPackage)
Require ($ammoIndicatorTexture.Count -eq 1 -and
    $ammoIndicatorTexture[0].Type -ceq 'Texture2D' -and
    [int]$ammoIndicatorTexture[0].SizeX -eq 1254 -and
    [int]$ammoIndicatorTexture[0].SizeY -eq 1254 -and
    $ammoIndicatorTexture[0].PixelFormat -ceq 'PF_B8G8R8A8') `
    'Ammo indicator texture dimensions or format changed.'
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
$ammoInventory = @($shell | Where-Object {
    $_.Name -ceq 'RailgunAmmoInventory_GEN_VARIABLE' -and
    $_.Type -ceq 'VoyageInventoryWeightLimitedComponent'
})
Require ($ammoInventory.Count -eq 1) 'Railgun weight-limited ammo inventory missing.'
$ammoInventoryProperties = $ammoInventory[0].Properties
Require ($ammoInventoryProperties.Type -ceq 'EVoyageInventoryType::Container') `
    'Railgun ammo inventory is not a container.'
Require (@($ammoInventoryProperties.AcceptedItemCategories).Count -eq 1 -and
    @($ammoInventoryProperties.AcceptedItemCategories)[0] -ceq 'EVoyageItemCategory::Ammo') `
    'Railgun ammo inventory category prefilter mismatch.'
Require ($ammoInventoryProperties.Access -ceq 'EVoyageInventoryAccessType::ReadWrite') `
    'Railgun ammo inventory must be read/write.'
Require (-not $ammoInventoryProperties.bAllowFiltering -and
    -not $ammoInventoryProperties.bAllowNearbyQueries -and
    -not $ammoInventoryProperties.bAutoCloseHudWhenEmpty) `
    'Railgun ammo inventory UI/query defaults mismatch.'
Require (@($ammoInventoryProperties.DepositAllCategoryFilter).Count -eq 1 -and
    $ammoInventoryProperties.DepositAllCategoryFilter[0].ObjectPath -ceq
        '/Game/Data/Assets/ItemCategories/DA_ItemCategory_Ammo.0') `
    'Railgun ammo inventory deposit-all category mismatch.'
Require (-not $ammoInventoryProperties.bAllowBeyondWeightLimit) `
    'Railgun ammo inventory must enforce its JSON-derived six-round mass budget.'
$ammoInteraction = @($shell | Where-Object {
    $_.Name -ceq 'RailgunAmmoInventoryInteraction_GEN_VARIABLE' -and
    $_.Type -ceq 'InteractiveObjectComponent'
})
Require ($ammoInteraction.Count -eq 1) 'Railgun ammo inventory interaction missing.'
Require ($ammoInteraction[0].Properties.InteractType -ceq 'FVoyageInteractType::WidgetOverlay' -and
    $ammoInteraction[0].Properties.PartId -eq 100 -and
    $ammoInteraction[0].Properties.OverlayWidget.ObjectPath -ceq
        '/Game/Data/UI/OverlayWidgets/DA_Widget_Container.0') `
    'Railgun ammo inventory interaction contract mismatch.'
$ammoQuery = @($shell | Where-Object {
    $_.Name -ceq 'RailgunAmmoInventoryQuery_GEN_VARIABLE' -and
    $_.Type -ceq 'BoxComponent'
})
Require ($ammoQuery.Count -eq 1) 'Railgun ammo inventory query box missing.'
$ammoQueryProperties = $ammoQuery[0].Properties
Require ($ammoQueryProperties.BodyInstance.CollisionProfileName -ceq 'Interactive' -and
    $ammoQueryProperties.BodyInstance.ObjectType -ceq 'ECC_GameTraceChannel2') `
    'Railgun ammo inventory query profile mismatch.'
Require (-not $ammoQueryProperties.bGenerateOverlapEvents) `
    'Railgun ammo inventory query must not generate overlaps.'
$expectedInteractionResponses = @(
    'WorldStatic','WorldDynamic','Pawn','Visibility','Camera','PhysicsBody',
    'Vehicle','Destructible','Interact','Interactive','LocatorVolume',
    'LocationVolume','WaterBody','Tentacle','TentacleOverlap'
)
$queryResponses = @($ammoQueryProperties.BodyInstance.CollisionResponses.ResponseArray)
Require ($queryResponses.Count -eq $expectedInteractionResponses.Count) `
    'Railgun ammo inventory query response count mismatch.'
foreach ($channel in $expectedInteractionResponses) {
    $response = @($queryResponses | Where-Object { $_.Channel -ceq $channel })
    $expectedResponse = if ($channel -ceq 'Interact') { 'ECR_Block' } else { 'ECR_Ignore' }
    Require ($response.Count -eq 1 -and $response[0].Response -ceq $expectedResponse) `
        ('Railgun ammo inventory query response mismatch: ' + $channel)
}
$shellDefault = @($shell | Where-Object { $_.Name -ceq 'Default__BP_Module_Railgun_C' })
Require ($shellDefault.Count -eq 1 -and
    $shellDefault[0].Properties.AcceptedRailgunAmmo.ObjectPath -ceq ($ammoPackage + '.0')) `
    'Railgun validator does not bind the exact owned ammo asset.'
$diagnosticFields = @(
    'RailgunInventoryValidateCount','RailgunInventoryProbeHud',
    'RailgunInventoryLastOpenSummary','RailgunInventoryLastOpenDetails',
    'RailgunInventoryLastValidateSummary','RailgunInventoryLastValidateDetails'
)
$shellDefaultFields = @($shellDefault[0].Properties.PSObject.Properties.Name)
foreach ($diagnosticField in $diagnosticFields) {
    Require (-not ($shellDefaultFields -ccontains $diagnosticField)) `
        ('Temporary inventory diagnostic field remains: ' + $diagnosticField)
}
$shellStrings = @(JsonStringLeaves $shell)
Require (-not ($shellStrings -ccontains
    '/Game/Mods/Railgun/Diagnostics/WBP_RailgunInventoryProbe')) `
    'Temporary inventory diagnostic widget reference remains.'
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
$ammoCassetteInventory = $inventory.ammoCassette
Require ($null -ne $ammoCassetteInventory) `
    'Ammo cassette import evidence is missing.'
Require ([IO.Path]::GetFullPath([string]$ammoCassetteInventory.sourceFile) -ceq `
    [IO.Path]::GetFullPath((Join-Path $PSScriptRoot `
        'Assets/Fabricator/RailgunAmmoCassette.glb'))) `
    'Ammo cassette import used an unexpected source file.'
Require ($ammoCassetteInventory.meshPackage -ceq `
    '/Game/Mods/Railgun/Fabricator/AmmoCassette/SM_RailgunAmmoCassette') `
    'Ammo cassette mesh package mismatch.'
Require ($ammoCassetteInventory.objectPath -ceq `
    '/Game/Mods/Railgun/Fabricator/AmmoCassette/SM_RailgunAmmoCassette.SM_RailgunAmmoCassette') `
    'Ammo cassette object path mismatch.'
Require ([int]$ammoCassetteInventory.triangles -gt 0) `
    'Ammo cassette has no render geometry.'
Require ([int]$ammoCassetteInventory.collisionPrimitives -gt 0) `
    'Ammo cassette has no simple collision.'
$ammoCassetteBounds = @($ammoCassetteInventory.boundsCm)
Require ($ammoCassetteBounds.Count -eq 3) `
    'Ammo cassette bounds are incomplete.'
foreach ($extent in $ammoCassetteBounds) {
    $value = [double]$extent
    Require ($value -gt 0.0 -and -not [double]::IsNaN($value) -and
        -not [double]::IsInfinity($value)) `
        'Ammo cassette bounds are not finite and nondegenerate.'
}
foreach ($package in @($ammoCassetteInventory.meshPackage) +
    @($ammoCassetteInventory.materialPackages) +
    @($ammoCassetteInventory.texturePackages)) {
    Require (@($inventory.packages) -ccontains $package) `
        ('Ammo cassette cook dependency is absent: ' + $package)
}
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
$inventoryInteractionConfig = $inventory.inventoryInteraction
Require ($inventoryInteractionConfig.node -ceq 'AmmoMagazine_6Slot') `
    'Inventory interaction must be anchored to the magazine node.'
Require-Child $inventoryInteractionConfig.node 'RailgunInventoryReference'
Require-Child 'RailgunInventoryReference' 'RailgunAmmoInventoryInteraction'
Require-Child 'RailgunAmmoInventoryInteraction' 'RailgunAmmoInventoryQuery'
$inventoryReference = @($shell | Where-Object {
    $_.Name -ceq 'RailgunInventoryReference_GEN_VARIABLE'
})
Require ($inventoryReference.Count -eq 1 -and
    $inventoryReference[0].Type -ceq 'BoxComponent') `
    'Missing inventory interaction reference box.'
Require (@($inventoryReference[0].Properties.ComponentTags) -ccontains
    'Railgun.Model.Inventory') 'Inventory interaction reference tag missing.'
Require ($inventoryReference[0].Properties.BodyInstance.CollisionEnabled -ceq
    'ECollisionEnabled::NoCollision') 'Inventory interaction reference must not collide.'
$expectedAmmoRoots = @(
    'Slot_01_AmmoCassette','Slot_02_AmmoCassette','Slot_03_AmmoCassette',
    'Slot_04_AmmoCassette','Slot_05_AmmoCassette','Slot_06_AmmoCassette'
)
function Get-InventoryComponent([string]$Name) {
    return @($inventory.components | Where-Object { $_.name -ceq $Name })
}
function Test-InventoryDescendant([object]$Component, [string]$AncestorName) {
    $parentName = [string]$Component.parent
    $visited = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    while (-not [string]::IsNullOrEmpty($parentName)) {
        if ($parentName -ceq $AncestorName) { return $true }
        if (-not $visited.Add($parentName)) { return $false }
        $parent = @(Get-InventoryComponent $parentName)
        if ($parent.Count -ne 1) { return $false }
        $parentName = [string]$parent[0].parent
    }
    return $false
}
Require (@(Compare-Object -ReferenceObject $expectedAmmoRoots `
    -DifferenceObject @($inventory.roles.ammoInstances) `
    -CaseSensitive -SyncWindow 0).Count -eq 0) `
    'Ordered ammo cassette role binding mismatch.'
$ammoRenderOwners = @{}
foreach ($ammoRoot in $expectedAmmoRoots) {
    $slot = $ammoRoot.Substring(0,7)
    $bin = $slot + '_AmmoBin'
    Require-Child $slot $ammoRoot
    Require-Child $slot $bin
    $binComponent = @($shell | Where-Object {
        $_.Name -ceq ($bin + '_GEN_VARIABLE')
    })
    Require ($binComponent.Count -eq 1 -and
        $binComponent[0].Type -ceq 'StaticMeshComponent') `
        ('Ammo bin was removed with its cassette: ' + $bin)
    Require (-not ((PropertyNames $binComponent[0]) -contains 'bHiddenInGame') -or
        -not $binComponent[0].Properties.bHiddenInGame) `
        ('Ammo bin must stay visible: ' + $bin)
    $descendants = @($inventory.components | Where-Object {
        Test-InventoryDescendant $_ $ammoRoot
    })
    Require (-not (@($descendants.name) -ccontains $slot) -and
        -not (@($descendants.name) -ccontains $bin)) `
        ('Ammo holder entered hidden cassette subtree: ' + $ammoRoot)
    foreach ($otherRoot in $expectedAmmoRoots) {
        if ($otherRoot -ceq $ammoRoot) { continue }
        Require (-not (@($descendants.name) -ccontains $otherRoot)) `
            ('Ammo cassette roots overlap: ' + $ammoRoot + ' / ' + $otherRoot)
    }
    $renderDescendants = @($descendants | Where-Object { $_.mesh })
    Require ($renderDescendants.Count -gt 0) `
        ('Ammo cassette has no render descendants: ' + $ammoRoot)
    foreach ($descendant in $renderDescendants) {
        Require (-not $ammoRenderOwners.ContainsKey([string]$descendant.name)) `
            ('Ammo render descendant belongs to multiple cassette roots: ' +
                $descendant.name)
        $ammoRenderOwners[[string]$descendant.name] = $ammoRoot
        Require ($descendant.hiddenInGame) `
            ('Ammo render descendant is not default-hidden: ' + $descendant.name)
        $component = @($shell | Where-Object {
            $_.Name -ceq ($descendant.name + '_GEN_VARIABLE')
        })
        Require ($component.Count -eq 1 -and
            $component[0].Properties.bHiddenInGame) `
            ('Cooked ammo render descendant is not default-hidden: ' +
                $descendant.name)
    }
}
for ($i=0; $i -lt 3; $i++) {
    $axis = @('X','Y','Z')[$i]
    Require ([Math]::Abs($inventoryReference[0].Properties.RelativeLocation.$axis -
        $inventoryInteractionConfig.centerCm[$i]) -lt 0.0001) `
        ('Inventory interaction center mismatch: ' + $axis)
    Require ([Math]::Abs($inventoryReference[0].Properties.BoxExtent.$axis * 2.0 -
        $inventoryInteractionConfig.sizeCm[$i]) -lt 0.0001) `
        ('Inventory interaction size mismatch: ' + $axis)
    Require ([Math]::Abs($ammoQueryProperties.BoxExtent.$axis -
        $inventoryReference[0].Properties.BoxExtent.$axis) -lt 0.0001) `
        ('Inventory query/reference extent mismatch: ' + $axis)
}
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
$chargeIndicatorName = [string]$inventory.roles.chargeIndicatorMesh
Require (-not [string]::IsNullOrWhiteSpace($chargeIndicatorName)) `
    'Charge-indicator model role is missing.'
$chargeIndicatorInventory = @(Get-InventoryComponent $chargeIndicatorName)
Require ($chargeIndicatorInventory.Count -eq 1 -and
    -not [string]::IsNullOrWhiteSpace(
        [string]$chargeIndicatorInventory[0].mesh)) `
    'Charge-indicator role must identify one render mesh.'
$chargeIndicatorComponent = @($shell | Where-Object {
    $_.Name -ceq ($chargeIndicatorName + '_GEN_VARIABLE')
})
Require ($chargeIndicatorComponent.Count -eq 1 -and
    $chargeIndicatorComponent[0].Type -ceq 'StaticMeshComponent') `
    'Cooked charge indicator is not one static-mesh component.'
Require (@($chargeIndicatorComponent[0].Properties.ComponentTags) -ccontains
    'Railgun.Model.ChargeIndicator') `
    'Cooked charge-indicator tag is missing.'
Require ($chargeIndicatorComponent[0].Properties.BodyInstance.CollisionEnabled -ceq
    'ECollisionEnabled::NoCollision') `
    'Charge indicator must not collide.'
$chargeIndicatorShadowProperties = @(PropertyNames $chargeIndicatorComponent[0] |
    Where-Object { $_ -cin @('CastShadow','bCastShadow') })
Require ($chargeIndicatorShadowProperties.Count -eq 1 -and
    -not [bool]$chargeIndicatorComponent[0].Properties.($chargeIndicatorShadowProperties[0])) `
    'Charge indicator must not cast shadows.'
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
$ammoCassetteMesh = @(Read-Candidate $ammoCassetteInventory.meshPackage)
$ammoCassetteStaticMesh = @($ammoCassetteMesh | Where-Object { $_.Type -ceq 'StaticMesh' })
$ammoCassetteBody = @($ammoCassetteMesh | Where-Object { $_.Type -ceq 'BodySetup' })
Require ($ammoCassetteStaticMesh.Count -eq 1 -and $ammoCassetteBody.Count -eq 1) `
    'Cooked ammo cassette mesh or BodySetup is missing.'
$ammoCassetteAggGeom = $ammoCassetteBody[0].Properties.AggGeom
$ammoCassetteSimpleCollisionCount =
    (ArrayPropertyCount $ammoCassetteAggGeom 'BoxElems') +
    (ArrayPropertyCount $ammoCassetteAggGeom 'SphereElems') +
    (ArrayPropertyCount $ammoCassetteAggGeom 'SphylElems') +
    (ArrayPropertyCount $ammoCassetteAggGeom 'ConvexElems')
Require ($ammoCassetteSimpleCollisionCount -gt 0) `
    'Cooked ammo cassette has no simple collision primitives.'
if (@($ammoCassetteBody[0].Properties.PSObject.Properties.Name) -ccontains
    'CollisionTraceFlag') {
    Require ($ammoCassetteBody[0].Properties.CollisionTraceFlag -cnotmatch `
        'UseComplexAsSimple') 'Ammo cassette collision became complex-only.'
}
foreach ($package in @(@($inventory.packages | Where-Object {
    $_ -like '*/Materials/*'
}) + @($ammoCassetteInventory.materialPackages) | Sort-Object -Unique)) {
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
foreach ($package in @($ammoCassetteInventory.texturePackages)) {
    $textureExports = @(Read-Candidate $package)
    Require (@($textureExports | Where-Object { $_.Type -ceq 'Texture2D' }).Count -eq 1) `
        ('Expected one cooked ammo cassette texture: ' + $package)
}
$gun = @(Read-Candidate $gunItemPackage)
$gunItem = @($gun | Where-Object { $_.Type -ceq 'VoyageItem' -and $_.Name -ceq 'DA_Item_Module_RailgunCannonMk01' })
Require ($gunItem.Count -eq 1) 'Railgun item primary export is missing or duplicated.'
$gunProperties = $gunItem[0].Properties
Require ($gunProperties.Category -ceq 'EVoyageItemCategory::Module') 'Railgun category mismatch.'
Require ($gunProperties.DroppedActor.AssetPathName -ceq '/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun_C') 'Railgun dropped actor mismatch.'
Require ($null -ne $gunProperties.Components) 'Railgun fabrication components are absent.'
$gunIcon = @(Read-Candidate '/Game/Mods/Railgun/Fabricator/T_RailgunIcon')
$gunTexture = @($gunIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunIcon' })
Require ($gunTexture.Count -eq 1 -and $gunTexture[0].SizeX -eq 256 -and $gunTexture[0].SizeY -eq 256) 'Railgun icon must be 256x256.'
$ammo = @(Read-Candidate $ammoPackage)
$ammoItem = @($ammo | Where-Object { $_.Type -ceq 'VoyageItemAmmo' -and $_.Name -ceq 'DA_Ammo_Railgun_FullRod' })
Require ($ammoItem.Count -eq 1) 'Railgun ammo primary export is missing or duplicated.'
$ammoProperties = $ammoItem[0].Properties
Require ($ammoProperties.Category -ceq 'EVoyageItemCategory::Ammo') 'Ammo category mismatch.'
Require ([double]$ammoProperties.Weight -gt 0.0) 'Ammo mass must be positive.'
Require ([Math]::Abs([double]$ammoInventoryProperties.MaxWeightLimit -
    ([double]$ammoProperties.Weight * 6.0)) -lt 0.00001) `
    'Railgun ammo inventory limit is not six times the authored ammo mass.'
Require ($null -ne $ammoProperties.Components) 'Ammo fabrication components are absent.'
Require (@($ammoProperties.DropVariations | Where-Object {
    $_.RenderAsset.AssetPathName -ceq
        '/Game/Mods/Railgun/Fabricator/AmmoCassette/SM_RailgunAmmoCassette.SM_RailgunAmmoCassette'
}).Count -ge 1) `
    'Ammo drop mesh mismatch.'
Require ($ammoProperties.DroppedActor.AssetPathName -ceq '/Game/Blueprints/BP_DynamicMeshActor.BP_DynamicMeshActor_C') 'Ammo dropped actor mismatch.'
$ammoIcon = @(Read-Candidate '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon')
$ammoTexture = @($ammoIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunAmmoIcon' })
Require ($ammoTexture.Count -eq 1 -and $ammoTexture[0].SizeX -eq 256 -and $ammoTexture[0].SizeY -eq 256) 'Ammo icon must be 256x256.'
$skillIcon = @(Read-Candidate '/Game/Mods/Railgun/Research/T_RailgunSkill')
$skillTexture = @($skillIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunSkill' })
Require ($skillTexture.Count -eq 1 -and $skillTexture[0].SizeX -eq 256 -and $skillTexture[0].SizeY -eq 256) 'Railgun skill icon must be 256x256.'
$reportPath = Join-Path $output 'validation.json'
[ordered]@{status='passed';runtime='pending';containerSha256=(Get-FileHash -LiteralPath $Container -Algorithm SHA256).Hash;assetEvidence=$evidence;materialEvidence=$materialEvidence;assertions='owned native module parent with exact inherited inventory function; magazine-anchored stock-profile interaction query; exact discovered ItemAsset; confirmed Item and Skill AssetManager scan roots; weight-limited six-round inventory derived from the authored ammo mass, with native BeginPlay limit setter, exact valid-item predicate, owned-ammo binding, stock container overlay and no temporary inventory probe; exact Voyage inventory-change delegate binding with initial and deferred post-load visual synchronization and no ammo-visual polling accumulator; six persistent UV-cropped white/faint ammo indicators activated right-to-left from the event-maintained count cache, with a zero-count red tint independent of the charge ring, one guarded initial sync, no HUD inventory polling and whole-block optics visibility; exact Voyage module-value delegate binding with idempotent remove/add, one initial charge-indicator snapshot, visual-only callback and end-play unbinding, with no charge-indicator write in the operator tick/fire graph; connected-and-powered insufficient-charge guard for subtle-red charge text and radial progress ring with opaque-white offline/ready recovery and unchanged radial background; no unreviewed native template values; auto-weld; inventory-matched component hierarchy and transforms; shell-owned BeginPlay and persistent post-load station initialization with a transient direct station reference, no global actor discovery and no player-controller startup gate; simple collision preserved; imported material packages remain readable and are recorded as evidence without constraining authored material type, parameters or parent; JSON-authored gun, ammo and skill primary assets preserve native identity, required runtime references and package integrity without pinning editable presentation or balance values; distinct 256x256 research icon'} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
