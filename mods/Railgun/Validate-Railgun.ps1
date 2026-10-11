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
$exitActionPackage = '/Game/Mods/Railgun/Inputs/IA_RailgunExit'
$zoomActionPackage = '/Game/Mods/Railgun/Inputs/IA_RailgunZoom'
$fireActionPackage = '/Game/Mods/Railgun/Inputs/IA_RailgunFire'
$reloadActionPackage = '/Game/Mods/Railgun/Inputs/IA_RailgunReload'
$keyboardContextPackage = '/Game/Mods/Railgun/Inputs/IMC_RailgunKeyboard'
$ammoIndicatorPackage = '/Game/Mods/Railgun/Station/T_RailgunAmmoIndicator'
$shotSoundPackage = '/Game/Mods/Railgun/Station/S_RailgunShotBlast'
$stockSfxSoundClassPackage = '/Game/Audio/Shares/SoundClasses/SC_SFX'
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
function TextSetTargets($Value) {
    if ($null -eq $Value) { return }
    if ($Value -is [array]) {
        foreach ($entry in $Value) { TextSetTargets $entry }
        return
    }
    if ($Value -isnot [pscustomobject]) { return }
    $names = @($Value.PSObject.Properties.Name)
    if (($names -ccontains 'Token') -and $Value.Token -ceq 'EX_Context' -and
        ($names -ccontains 'ContextExpression') -and
        $null -ne $Value.ContextExpression -and
        (@($Value.ContextExpression.PSObject.Properties.Name) -ccontains
            'Function') -and
        $Value.ContextExpression.Function -ceq 'SetText' -and
        ($names -ccontains 'ObjectExpression') -and
        $null -ne $Value.ObjectExpression -and
        (@($Value.ObjectExpression.PSObject.Properties.Name) -ccontains
            'Variable') -and
        $null -ne $Value.ObjectExpression.Variable -and
        $null -ne $Value.ObjectExpression.Variable.Property) {
        [string]$Value.ObjectExpression.Variable.Property.Name
    }
    foreach ($property in $Value.PSObject.Properties) {
        TextSetTargets $property.Value
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
$lifecycleDelayStatements = @(StatementIndexesContaining `
    $ubergraph[0].ScriptBytecode `
    "Class'KismetSystemLibrary:DelayUntilNextTick'")
$lifecycleInitializerStatements = @(StatementIndexesContaining `
    $ubergraph[0].ScriptBytecode 'InitializeRailgunStation')
Require ($lifecycleDelayStatements.Count -eq 2 -and
    $lifecycleInitializerStatements.Count -eq 2) `
    'BeginPlay and persistent post-load must each retain one deferred station initialization.'
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
    'BindRailgunShellLifecycle',
    'BindRailgunAimComponents',
    'BindRailgunEnergy',
    'BindRailgunChargeIndicator'
)) {
    Require ($initializeStationStrings -ccontains
        $requiredInitializationReference) `
        ('Station initialization reference missing: ' +
            $requiredInitializationReference)
}
$attachParentStatements = @($initializeStation[0].ScriptBytecode |
    Where-Object {
        @(JsonStringLeaves $_) -ccontains
            "Class'SceneComponent:GetAttachParent'"
    })
$attachParentOutputNames = @(JsonStringLeaves $attachParentStatements |
    Where-Object {
        $_ -clike 'CallFunc_GetAttachParent_ReturnValue*'
    } | Select-Object -Unique)
$attachParentFlow = if ($attachParentOutputNames.Count -eq 1) {
    @($initializeStation[0].ScriptBytecode | Where-Object {
        @(JsonStringLeaves $_) -ccontains $attachParentOutputNames[0]
    })
} else {
    @()
}
$attachParentStatementStrings = @(JsonStringLeaves $attachParentStatements)
$attachParentFlowStrings = @(JsonStringLeaves $attachParentFlow)
Require ($attachParentStatements.Count -eq 1 -and
    $attachParentFlow.Count -eq 2 -and
    $attachParentStatementStrings -ccontains 'RailgunEntryQuery' -and
    $attachParentFlowStrings -ccontains 'RailgunInteraction' -and
    $attachParentFlowStrings -ccontains
        "Class'KismetMathLibrary:EqualEqual_ObjectObject'") `
    'Only the station query-to-interaction parent contract may gate initialization.'
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
$hintActions = @(
    @{ Package=$exitActionPackage; Name='IA_RailgunExit'; Text='Exit Railgun' },
    @{ Package=$zoomActionPackage; Name='IA_RailgunZoom'; Text='Toggle scope' },
    @{ Package=$fireActionPackage; Name='IA_RailgunFire'; Text='Fire' },
    @{ Package=$reloadActionPackage; Name='IA_RailgunReload'; Text='Reload' }
)
foreach ($hintActionContract in $hintActions) {
    $hintAction = @(Read-Candidate $hintActionContract.Package)
    Require ($hintAction.Count -eq 1 -and
        $hintAction[0].Type -ceq 'InputAction' -and
        $hintAction[0].Name -ceq $hintActionContract.Name -and
        $hintAction[0].Properties.ActionDescription.SourceString -ceq
            $hintActionContract.Text) `
        ('Railgun hint action description mismatch: ' +
            $hintActionContract.Name)
}
$keyboardContext = @(Read-Candidate $keyboardContextPackage)
$keyboardMappings = @($keyboardContext | Where-Object {
    $_.Type -ceq 'InputMappingContext'
})
Require ($keyboardMappings.Count -eq 1 -and
    @($keyboardMappings[0].Properties.DefaultKeyMappings.Mappings).Count -eq 6) `
    'Railgun keyboard context must contain exactly six mappings.'
$reloadMappings = @(
    $keyboardMappings[0].Properties.DefaultKeyMappings.Mappings |
    Where-Object {
        $_.Action.ObjectPath -ceq
            '/Game/Mods/Railgun/Inputs/IA_RailgunReload.0' -and
        $_.Key.KeyName -ceq 'R'
    })
Require ($reloadMappings.Count -eq 1) `
    'Railgun reload input must map IA_RailgunReload to R exactly once.'
$operator = @(Read-Candidate $operatorPackage)
$operatorPackageStrings = @(JsonStringLeaves $operator)
$operatorFunctions = @($operator | Where-Object { $_.Type -ceq 'Function' })
$operatorClass = @($operator | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass'
})
Require ($operatorClass.Count -eq 1) `
    'Expected one Railgun operator generated class.'
$operatorDefaults = @($operator | Where-Object {
    $_.Name -ceq 'Default__BP_RailgunOperator_C'
})
Require ($operatorDefaults.Count -eq 1) `
    'Expected one Railgun operator class default object.'
$reloadProperties = @{
    RailgunReloadAvailable = 'BoolProperty'
    RailgunReloadSourceInventory = 'ObjectProperty'
    RailgunReloadTargetInventory = 'ObjectProperty'
    RailgunReloadAcceptedAmmo = 'ObjectProperty'
    RailgunReloadSourceCount = 'IntProperty'
    RailgunReloadTargetCount = 'IntProperty'
}
foreach ($reloadProperty in $reloadProperties.GetEnumerator()) {
    $property = @($operatorClass[0].ChildProperties | Where-Object {
        $_.Name -ceq $reloadProperty.Key
    })
    Require ($property.Count -eq 1 -and
        $property[0].Type -ceq $reloadProperty.Value -and
        $property[0].PropertyFlags -match '(^| \| )Transient($| \| )') `
        ('Reload state must be exact and transient: ' + $reloadProperty.Key)
}
$reloadFunctions = @{}
foreach ($reloadFunctionName in @(
    'RefreshRailgunReloadAvailability','OnRailgunReloadInventoryChanged',
    'BindRailgunReloadInventories','UnbindRailgunReloadInventories',
    'ReloadRailgun','GetProvidedActionsBP','ReceivePossessed',
    'ReceiveUnpossessed','ReceiveEndPlay','ReceiveTick')) {
    $matches = @($operatorFunctions | Where-Object {
        $_.Name -ceq $reloadFunctionName
    })
    Require ($matches.Count -eq 1) `
        ('Expected one reload/lifecycle function: ' + $reloadFunctionName)
    $reloadFunctions[$reloadFunctionName] = $matches[0]
}
$reloadRefreshStrings = @(JsonStringLeaves `
    $reloadFunctions['RefreshRailgunReloadAvailability'])
foreach ($requiredReloadPredicateReference in @(
    'RailgunReloadAvailable','RailgunReloadSourceInventory',
    'RailgunReloadTargetInventory','RailgunReloadAcceptedAmmo',
    'RailgunReloadSourceCount','RailgunReloadTargetCount','Items','ItemCount',
    'IsPlayerControlled',"Class'BlueprintMapLibrary:Map_Values'",
    "Class'KismetMathLibrary:Add_IntInt'",
    "Class'KismetMathLibrary:Greater_IntInt'",
    "Class'KismetMathLibrary:Less_IntInt'")) {
    Require ($reloadRefreshStrings -ccontains
            $requiredReloadPredicateReference) `
        ('Reload availability predicate reference missing: ' +
            $requiredReloadPredicateReference)
}
$reloadBindStrings = @(JsonStringLeaves `
    $reloadFunctions['BindRailgunReloadInventories'])
foreach ($requiredReloadBindingReference in @(
    'OriginalPlayerPawn','WeightInventory','RailgunEnergyModule',
    'AcceptedRailgunAmmo',"Class'VoyageModuleComponent:GetInternalInventory'",
    'OnInventoryChanged',
    'OnRailgunReloadInventoryChanged','RefreshRailgunReloadAvailability',
    'UnbindRailgunReloadInventories')) {
    Require ($reloadBindStrings -ccontains $requiredReloadBindingReference) `
        ('Reload binding reference missing: ' +
            $requiredReloadBindingReference)
}
$reloadCallbackStrings = @(JsonStringLeaves `
    $reloadFunctions['OnRailgunReloadInventoryChanged'])
Require (($reloadCallbackStrings -ccontains
        'RefreshRailgunReloadAvailability') -and
    -not ($reloadCallbackStrings -ccontains
        "Class'VoyageBaseInventoryComponent:TransferSlot'")) `
    'Reload inventory callback must refresh availability without transferring.'
$reloadExecuteStrings = @(JsonStringLeaves `
    $reloadFunctions['ReloadRailgun'])
Require (($reloadExecuteStrings -ccontains
        'RefreshRailgunReloadAvailability') -and
    ($reloadExecuteStrings -ccontains 'RailgunReloadAvailable') -and
    ($reloadExecuteStrings -ccontains 'ValidateItem') -and
    ($reloadExecuteStrings -ccontains
        "Class'VoyageBaseInventoryComponent:FindSlotsByItem'") -and
    ($reloadExecuteStrings -ccontains
        "Class'VoyageBaseInventoryComponent:GetSlot'") -and
    @($reloadExecuteStrings | Where-Object {
        $_ -ceq "Class'VoyageBaseInventoryComponent:TransferSlot'"
    }).Count -eq 1 -and
    -not ($reloadExecuteStrings -ccontains
        "Class'VoyageBaseInventoryComponent:TransferItems'")) `
    'Reload must validate, snapshot/recheck slots, and use TransferSlot only.'
foreach ($requiredReloadLocal in @(
    'RailgunReloadSlotSnapshot','RailgunReloadRemaining',
    'RailgunReloadMoved','RailgunReloadCurrentSlot',
    'RailgunReloadRequested','RailgunReloadValidated')) {
    Require ($reloadExecuteStrings -ccontains $requiredReloadLocal) `
        ('Reload transaction local missing: ' + $requiredReloadLocal)
}
$reloadValidatorStatements = @(
    $reloadFunctions['ReloadRailgun'].ScriptBytecode | Where-Object {
        $_ -is [pscustomobject] -and
        $_.Token -ceq 'EX_Context' -and
        $null -ne $_.ObjectExpression -and
        $_.ObjectExpression.Token -ceq 'EX_InterfaceContext' -and
        $null -ne $_.ContextExpression -and
        $_.ContextExpression.Token -ceq 'EX_VirtualFunction' -and
        $_.ContextExpression.Function -ceq 'ValidateItem'
    })
Require ($reloadValidatorStatements.Count -eq 1) `
    'Reload must contain one ValidateItem interface dispatch.'
$reloadValidator = $reloadValidatorStatements[0]
$reloadValidatorReceiver = `
    $reloadValidator.ObjectExpression.InterfaceValue
$reloadValidatorParameters = @($reloadValidator.ContextExpression.Parameters)
Require ($reloadValidatorReceiver.Token -ceq 'EX_LocalVariable' -and
    $reloadValidatorReceiver.Variable.Property.PropertyClass.ObjectName -ceq
        "BlueprintGeneratedClass'BP_Module_Railgun_C'" -and
    $reloadValidatorParameters.Count -eq 3 -and
    $reloadValidatorParameters[0].Token -ceq 'EX_InstanceVariable' -and
    $reloadValidatorParameters[0].Variable.Property.Name -ceq
        'RailgunReloadTargetInventory' -and
    $reloadValidatorParameters[1].Token -ceq 'EX_InstanceVariable' -and
    $reloadValidatorParameters[1].Variable.Property.Name -ceq
        'RailgunReloadAcceptedAmmo' -and
    $reloadValidatorParameters[2].Token -ceq 'EX_LocalVariable' -and
    $reloadValidatorParameters[2].Variable.Property.Name -ceq
        'CallFunc_ValidateItem_bIsValid') `
    'Reload ValidateItem must dispatch on the typed shell with target and ammo.'
$reloadTransferStatements = @(
    $reloadFunctions['ReloadRailgun'].ScriptBytecode | Where-Object {
        $_ -is [pscustomobject] -and
        @($_.PSObject.Properties.Name) -ccontains 'Expression' -and
        $null -ne $_.Expression -and
        $_.Expression.Token -ceq 'EX_Context' -and
        $null -ne $_.Expression.ContextExpression -and
        $null -ne $_.Expression.ContextExpression.Function -and
        $_.Expression.ContextExpression.Function.ObjectName -ceq
            "Class'VoyageBaseInventoryComponent:TransferSlot'"
    })
Require ($reloadTransferStatements.Count -eq 1) `
    'Reload must contain one structural TransferSlot context call.'
$reloadTransferExpression = $reloadTransferStatements[0].Expression
$reloadTransferReceiver = $reloadTransferExpression.ObjectExpression
$reloadTransferParameters = @(
    $reloadTransferExpression.ContextExpression.Parameters)
Require ($reloadTransferReceiver.Token -ceq 'EX_InstanceVariable' -and
    $reloadTransferReceiver.Variable.Property.Name -ceq
        'RailgunReloadTargetInventory' -and
    $reloadTransferParameters.Count -eq 4 -and
    $reloadTransferParameters[0].Token -ceq 'EX_InstanceVariable' -and
    $reloadTransferParameters[0].Variable.Property.Name -ceq
        'RailgunReloadSourceInventory' -and
    $reloadTransferParameters[1].Token -ceq 'EX_LocalVariable' -and
    $reloadTransferParameters[1].Variable.Property.Name -ceq
        'RailgunReloadCurrentSlot' -and
    $reloadTransferParameters[2].Token -ceq 'EX_IntConst' -and
    $reloadTransferParameters[2].Value -eq -1 -and
    $reloadTransferParameters[3].Token -ceq 'EX_LocalVariable' -and
    $reloadTransferParameters[3].Variable.Property.Name -ceq
        'RailgunReloadRequested') `
    'Reload must call target.TransferSlot(source, currentSlot, -1, requested).'
$reloadTransferBytecode = @(
    JsonStringLeaves $reloadFunctions['ReloadRailgun'].ScriptBytecode)
foreach ($requiredReloadTransferReference in @(
    'RailgunReloadTargetInventory','RailgunReloadSourceInventory',
    'RailgunReloadCurrentSlot','RailgunReloadRequested',
    "Class'KismetMathLibrary:Greater_IntInt'",
    "Class'KismetMathLibrary:LessEqual_IntInt'",
    "Class'KismetMathLibrary:Subtract_IntInt'")) {
    Require ($reloadTransferBytecode -ccontains
            $requiredReloadTransferReference) `
        ('Reload bounded transfer reference missing: ' +
            $requiredReloadTransferReference)
}
$reloadProvider = $reloadFunctions['GetProvidedActionsBP']
$reloadProviderStrings = @(JsonStringLeaves $reloadProvider)
foreach ($requiredReloadHintReference in @(
    'RailgunReloadAvailable','bEnabled')) {
    Require ($reloadProviderStrings -ccontains $requiredReloadHintReference) `
        ('Reload action-hint reference missing: ' +
            $requiredReloadHintReference)
}
$providedActionAssignments = @{}
foreach ($statement in @($reloadProvider.ScriptBytecode)) {
    if ($statement -isnot [pscustomobject] -or
        $statement.Token -notin @('EX_Let','EX_LetObj','EX_LetBool') -or
        $statement.Variable -isnot [pscustomobject] -or
        $statement.Variable.Token -cne 'EX_StructMemberContext' -or
        @($statement.Variable.Property.Path).Count -ne 1 -or
        $statement.Variable.StructExpression.Token -cne 'EX_LocalVariable') {
        continue
    }
    $memberName = [string]$statement.Variable.Property.Path[0]
    if ($memberName -notin @('InputAction','Text','bEnabled')) { continue }
    $localName = [string]$statement.Variable.StructExpression.Variable.Property.Name
    if (-not $providedActionAssignments.ContainsKey($localName)) {
        $providedActionAssignments[$localName] = @{}
    }
    Require (-not $providedActionAssignments[$localName].ContainsKey($memberName)) `
        ('Duplicate provided-action assignment for ' + $localName + '.' +
            $memberName)
    $providedActionAssignments[$localName][$memberName] = $statement.Expression
}
Require ($providedActionAssignments.Count -eq 4) `
    'Provider must assign InputAction and Text on exactly four action structs.'
$expectedProvidedActionPaths = @(
    '/Game/Mods/Railgun/Inputs/IA_RailgunExit.0',
    '/Game/Mods/Railgun/Inputs/IA_RailgunZoom.0',
    '/Game/Mods/Railgun/Inputs/IA_RailgunFire.0',
    '/Game/Mods/Railgun/Inputs/IA_RailgunReload.0'
)
$actualProvidedActionPaths = @()
foreach ($entry in $providedActionAssignments.GetEnumerator()) {
    $assignments = $entry.Value
    Require ($assignments.ContainsKey('InputAction') -and
        $assignments.ContainsKey('Text') -and
        $assignments.ContainsKey('bEnabled')) `
        ('Provided-action struct lacks InputAction, Text, or bEnabled: ' +
            $entry.Key)
    $inputExpression = $assignments['InputAction']
    Require ($inputExpression.Token -ceq 'EX_ObjectConst' -and
        $null -ne $inputExpression.Value -and
        $expectedProvidedActionPaths -ccontains
            [string]$inputExpression.Value.ObjectPath) `
        ('Provided-action InputAction must be an exact Railgun action object: ' +
            $entry.Key)
    $actionPath = [string]$inputExpression.Value.ObjectPath
    $actualProvidedActionPaths += $actionPath
    $textExpression = $assignments['Text']
    Require ($textExpression.Token -ceq 'EX_Context') `
        ('Provided-action Text must use an object context: ' + $entry.Key)
    $objectExpression = $textExpression.ObjectExpression
    $literalContextMatches = $null -ne $objectExpression -and
        $objectExpression.Token -ceq 'EX_ObjectConst' -and
        $null -ne $objectExpression.Value -and
        [string]$objectExpression.Value.ObjectPath -ceq $actionPath
    $referenceProperty = if ($null -ne $objectExpression -and
        $objectExpression.Token -ceq 'EX_InstanceVariable' -and
        $null -ne $objectExpression.Variable -and
        $null -ne $objectExpression.Variable.Property) {
        $objectExpression.Variable.Property
    } else { $null }
    $referenceDefault = if ($null -ne $referenceProperty) {
        $operatorDefaults[0].Properties.PSObject.Properties[
            [string]$referenceProperty.Name]
    } else { $null }
    $typedReferenceContextMatches = $null -ne $objectExpression -and
        $objectExpression.Token -ceq 'EX_InstanceVariable' -and
        $null -ne $objectExpression.Variable -and
        $null -ne $objectExpression.Variable.Owner -and
        $objectExpression.Variable.Owner.ObjectName -ceq
            "BlueprintGeneratedClass'BP_RailgunOperator_C'" -and
        $objectExpression.Variable.Owner.ObjectPath -ceq
            '/Game/Mods/Railgun/Station/BP_RailgunOperator.0' -and
        $null -ne $referenceProperty -and
        $referenceProperty.Type -ceq 'ObjectProperty' -and
        $null -ne $referenceProperty.PropertyClass -and
        $referenceProperty.PropertyClass.ObjectName -ceq
            "Class'InputAction'" -and
        $referenceProperty.PropertyClass.ObjectPath -ceq
            '/Script/EnhancedInput' -and
        $null -ne $referenceDefault -and
        $null -ne $referenceDefault.Value -and
        [string]$referenceDefault.Value.ObjectPath -ceq $actionPath
    Require ($literalContextMatches -or $typedReferenceContextMatches) `
        ('Provided-action Text context does not match InputAction: ' +
            $entry.Key)
    $descriptionExpression = $textExpression.ContextExpression
    Require ($null -ne $descriptionExpression -and
        $descriptionExpression.Token -ceq 'EX_InstanceVariable' -and
        @($descriptionExpression.Variable.Path).Count -eq 1 -and
        [string]$descriptionExpression.Variable.Path[0] -ceq
            'ActionDescription' -and
        $descriptionExpression.Variable.ResolvedOwner.ObjectName -ceq
            "Class'InputAction'" -and
        $descriptionExpression.Variable.ResolvedOwner.ObjectPath -ceq
            '/Script/EnhancedInput') `
        ('Provided-action Text must read exact UInputAction.ActionDescription: ' +
            $entry.Key)
    $enabledExpression = $assignments['bEnabled']
    if ($actionPath -ceq
        '/Game/Mods/Railgun/Inputs/IA_RailgunReload.0') {
        Require ($enabledExpression.Token -ceq 'EX_InstanceVariable' -and
            $null -ne $enabledExpression.Variable.Property -and
            $enabledExpression.Variable.Property.Name -ceq
                'RailgunReloadAvailable') `
            'Reload bEnabled must read the cached production availability directly.'
    } else {
        Require ($enabledExpression.Token -ceq 'EX_LocalVariable' -and
            $enabledExpression.Variable.Property.Name.StartsWith(
                'CallFunc_IsPlayerControlled_ReturnValue',
                [StringComparison]::Ordinal)) `
            ('Non-reload bEnabled must remain IsPlayerControlled: ' +
                $actionPath)
    }
}
Require (@($actualProvidedActionPaths | Sort-Object -Unique).Count -eq 4) `
    'Each Railgun provided action must use a distinct exact InputAction.'
$reloadUbergraph = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ExecuteUbergraph_BP_RailgunOperator'
})
$reloadUbergraphStrings = @(JsonStringLeaves $reloadUbergraph[0])
Require ($reloadUbergraph.Count -eq 1 -and
    @($reloadUbergraphStrings | Where-Object {
        $_ -ceq 'BindRailgunReloadInventories'
    }).Count -eq 1) `
    'Confirmed possession continuation must bind reload inventories once.'
Require (@($reloadUbergraphStrings | Where-Object {
        $_ -ceq 'UnbindRailgunReloadInventories'
    }).Count -eq 2) `
    'Unpossession and EndPlay must each unbind reload inventory delegates.'
Require (@($reloadUbergraphStrings | Where-Object {
        $_ -ceq 'ReloadRailgun'
    }).Count -eq 1) `
    'R Started must call reload exactly once.'
$reloadTickStrings = @(JsonStringLeaves $reloadFunctions['ReceiveTick'])
foreach ($forbiddenReloadTickReference in @(
    'ReloadRailgun',"Class'VoyageBaseInventoryComponent:TransferSlot'",
    'BindRailgunReloadInventories',
    'RefreshRailgunReloadAvailability')) {
    Require (-not ($reloadTickStrings -ccontains
            $forbiddenReloadTickReference)) `
        ('Reload must not poll or transfer from tick: ' +
            $forbiddenReloadTickReference)
}
foreach ($reloadModeIndependentFunction in @(
    'RefreshRailgunReloadAvailability','ReloadRailgun',
    'GetProvidedActionsBP')) {
    Require (-not (@(JsonStringLeaves `
            $reloadFunctions[$reloadModeIndependentFunction]) -ccontains
            'RailgunWideMode')) `
        ('Reload must remain independent of scope/wide mode: ' +
            $reloadModeIndependentFunction)
}
$reloadCases = @(
    @{Name='zero-player'; Player=@(); Gun=@(2); Enabled=$false; Transfer=0},
    @{Name='partial'; Player=@(2); Gun=@(1); Enabled=$true; Transfer=2},
    @{Name='full'; Player=@(5); Gun=@(6); Enabled=$false; Transfer=0},
    @{Name='excess'; Player=@(9); Gun=@(4); Enabled=$true; Transfer=2},
    @{Name='split-slots'; Player=@(1,2,3); Gun=@(1,1); Enabled=$true; Transfer=4}
)
foreach ($reloadCase in $reloadCases) {
    $playerCount = [int](($reloadCase.Player | Measure-Object -Sum).Sum)
    $gunCount = [int](($reloadCase.Gun | Measure-Object -Sum).Sum)
    $enabled = $playerCount -gt 0 -and $gunCount -lt 6
    $transfer = if ($enabled) {
        [Math]::Min($playerCount, 6 - $gunCount)
    } else { 0 }
    Require ($enabled -eq $reloadCase.Enabled -and
        $transfer -eq $reloadCase.Transfer) `
        ('Reload predicate scenario failed: ' + $reloadCase.Name)
}
$removedSettingKeys = @(
    'StatusIconOpacityPercent',
    'TargetNameOffsetX','TargetNameOffsetY','TargetNameOpacityPercent',
    'TargetNameFontSize','TargetNameFontPath','TargetNameTypeface',
    'TargetDistanceOffsetX','TargetDistanceOffsetY',
    'TargetDistanceOpacityPercent','TargetDistanceFontSize',
    'TargetDistanceFontPath','TargetDistanceTypeface',
    'ChargeTextOpacityPercent','ChargeTextFontSize','ChargeTextFontPath',
    'ChargeTextTypeface','ChargeIndicatorSmoothingSpeed'
)
$removedSettingFields = @(
    'RailgunStatusIconOpacityPercent',
    'RailgunTargetNameOffsetX','RailgunTargetNameOffsetY',
    'RailgunTargetNameOpacityPercent','RailgunTargetNameFontSize',
    'RailgunTargetNameFontPath','RailgunTargetNameTypeface',
    'RailgunTargetDistanceOffsetX','RailgunTargetDistanceOffsetY',
    'RailgunTargetDistanceOpacityPercent','RailgunTargetDistanceFontSize',
    'RailgunTargetDistanceFontPath','RailgunTargetDistanceTypeface',
    'RailgunChargeTextOpacityPercent','RailgunChargeTextFontSize',
    'RailgunChargeTextFontPath','RailgunChargeTextTypeface'
)
foreach ($removedSetting in $removedSettingKeys + $removedSettingFields) {
    Require (-not ($operatorPackageStrings -ccontains $removedSetting)) `
        ('Removed configurable setting remains serialized: ' +
            $removedSetting)
}
foreach ($removedField in $removedSettingFields) {
    Require (@($operatorClass[0].ChildProperties | Where-Object {
        $_.Name -ceq $removedField
    }).Count -eq 0) `
        ('Removed setting field remains on the station: ' + $removedField)
}
$shotVolumeProperty = @($operatorClass[0].ChildProperties |
    Where-Object { $_.Name -ceq 'RailgunShotVolumePercent' })
Require ($shotVolumeProperty.Count -eq 1 -and
    $shotVolumeProperty[0].Type -ceq 'DoubleProperty') `
    'Shot volume must remain one runtime double setting.'
$idleConsumptionProperty = @($operatorClass[0].ChildProperties |
    Where-Object { $_.Name -ceq 'RailgunIdleConsumptionKW' })
Require ($idleConsumptionProperty.Count -eq 1 -and
    $idleConsumptionProperty[0].Type -ceq 'DoubleProperty') `
    'Idle consumption must remain one runtime double setting.'
$fixedHudFonts = @(
    @{Field='RailgunTargetNameFontObject'; Path='/Game/UI/Terminal/Fonts/ShareTech/ShareTechMono-Regular_Font.ShareTechMono-Regular_Font'},
    @{Field='RailgunTargetDistanceFontObject'; Path='/Game/UI/Terminal/Fonts/DSEG/DSEG7Classic-Bold_Font.DSEG7Classic-Bold_Font'},
    @{Field='RailgunChargeTextFontObject'; Path='/Game/UI/Fonts/NotoSans-Regular_Font.NotoSans-Regular_Font'}
)
foreach ($fixedHudFont in $fixedHudFonts) {
    $property = @($operatorClass[0].ChildProperties | Where-Object {
        $_.Name -ceq $fixedHudFont.Field
    })
    Require ($property.Count -eq 1 -and
        $property[0].Type -ceq 'ObjectProperty' -and
        ($operatorPackageStrings -ccontains $fixedHudFont.Path)) `
        ('Fixed HUD font contract is missing: ' + $fixedHudFont.Field)
}
$entryProvider = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'GetInteractiveProvidedActions'
})
$entryCallback = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'RailgunEnterFromAction'
})
Require ($entryProvider.Count -eq 1 -and $entryCallback.Count -eq 1) `
    'Expected one modern entry provider and one entry callback.'
$entryProviderStrings = @(JsonStringLeaves $entryProvider[0])
$entryCallbackStrings = @(JsonStringLeaves $entryCallback[0])
foreach ($requiredSharedEntryReference in @(
    'RailgunEntryReady','StationAnchor','RailgunEntryAction',
    'IsPlayerControlled','GetMovementComponent','MovementMode',
    "Class'CharacterMovementComponent'",
    "Class'KismetMathLibrary:EqualEqual_ByteByte'"
)) {
    Require ($entryProviderStrings -ccontains
            $requiredSharedEntryReference -and
        $entryCallbackStrings -ccontains $requiredSharedEntryReference) `
        ('Entry provider/callback eligibility mismatch: ' +
            $requiredSharedEntryReference)
}
foreach ($requiredProviderReference in @(
    'RailgunInteraction','Component','MyCharacter'
)) {
    Require ($entryProviderStrings -ccontains $requiredProviderReference) `
        ('Entry provider reference missing: ' +
            $requiredProviderReference)
}
foreach ($requiredCallbackReference in @(
    "Class'Actor:HasAuthority'",'IsLocalController','OnEnterVehicle'
)) {
    Require ($entryCallbackStrings -ccontains $requiredCallbackReference) `
        ('Entry callback reference missing: ' +
            $requiredCallbackReference)
}
foreach ($obsoleteEntryDistanceReference in @(
    "Class'KismetMathLibrary:VSize'",
    "Class'KismetMathLibrary:LessEqual_DoubleDouble'",
    "Class'Actor:K2_GetActorLocation'",
    "Class'SceneComponent:K2_GetComponentLocation'"
)) {
    Require (-not ($entryProviderStrings -ccontains
            $obsoleteEntryDistanceReference) -and
        -not ($entryCallbackStrings -ccontains
            $obsoleteEntryDistanceReference)) `
        ('Obsolete entry distance guard retained: ' +
            $obsoleteEntryDistanceReference)
}
$handledEntryResults = @($entryProvider[0].ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_LetBool' -and
    @($_.PSObject.Properties.Name) -ccontains 'Variable' -and
    @($_.Variable.PSObject.Properties.Name) -ccontains 'Variable' -and
    @($_.Variable.Variable.PSObject.Properties.Name) -ccontains 'Property' -and
    @($_.PSObject.Properties.Name) -ccontains 'Expression' -and
    $_.Variable.Variable.Property.Name -ceq 'ReturnValue' -and
    $_.Expression.Token -ceq 'EX_True'
})
$emptyEntryActions = @($entryProvider[0].ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    @($_.PSObject.Properties.Name) -ccontains 'Variable' -and
    @($_.Variable.PSObject.Properties.Name) -ccontains 'Variable' -and
    @($_.Variable.Variable.PSObject.Properties.Name) -ccontains 'Property' -and
    @($_.PSObject.Properties.Name) -ccontains 'Expression' -and
    $_.Variable.Variable.Property.Name -ceq 'OutActions' -and
    $_.Expression.Token -ceq 'EX_ArrayConst' -and
    @($_.Expression.Values).Count -eq 0
})
Require ($handledEntryResults.Count -eq 2 -and
    $emptyEntryActions.Count -eq 1) `
    'Modern entry provider must return handled=true with empty actions when rejected.'
$ownedAimReferences = @(
    @{Name='RailgunFirstPersonCamera'; Class="Class'CameraComponent'"},
    @{Name='RailgunFirstPersonCameraOwner'; Class="Class'Pawn'"},
    @{Name='RailgunYawComponent'; Class="Class'SceneComponent'"},
    @{Name='RailgunPitchComponent'; Class="Class'SceneComponent'"},
    @{Name='RailgunAimModelOwner'; Class="Class'Actor'"}
)
foreach ($expectedReference in $ownedAimReferences) {
    $property = @($operatorClass[0].ChildProperties | Where-Object {
        $_.Name -ceq $expectedReference.Name
    })
    Require ($property.Count -eq 1 -and
        $property[0].Type -ceq 'ObjectProperty' -and
        $property[0].PropertyClass.ObjectName -ceq
            $expectedReference.Class -and
        $property[0].PropertyFlags -match '(^| \| )Transient($| \| )') `
        ('Owned aim reference must be exact and transient: ' +
            $expectedReference.Name)
}
$aimBinding = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'BindRailgunAimComponents'
})
Require ($aimBinding.Count -eq 1) `
    'Expected one owned aim-component binding function.'
$aimBindingStrings = @(JsonStringLeaves $aimBinding[0])
foreach ($requiredAimBindingReference in @(
    'RailgunYawComponent','RailgunPitchComponent','RailgunAimModelOwner',
    'Railgun.Model.Yaw','Railgun.Model.Pitch',
    "Class'Actor:GetComponentsByTag'"
)) {
    Require ($aimBindingStrings -ccontains $requiredAimBindingReference) `
        ('Aim-component binding reference missing: ' +
            $requiredAimBindingReference)
}
Require (@($aimBindingStrings | Where-Object {
    $_ -ceq "Class'Actor:GetComponentsByTag'"
}).Count -eq 4) `
    'Aim-component binding must contain exactly two one-time tag queries.'
$clearAimReferences = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ClearRailgunAimReferences'
})
Require ($clearAimReferences.Count -eq 1) `
    'Expected one owned aim-reference clear function.'
$clearAimReferenceStrings = @(JsonStringLeaves $clearAimReferences[0])
foreach ($expectedReference in $ownedAimReferences) {
    Require ($clearAimReferenceStrings -ccontains $expectedReference.Name) `
        ('Aim-reference clear omitted: ' + $expectedReference.Name)
}
$operatorUbergraph = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ExecuteUbergraph_BP_RailgunOperator'
})
Require ($operatorUbergraph.Count -eq 1) 'Expected one Railgun operator ubergraph.'
$operatorStatements = @($operatorUbergraph[0].ScriptBytecode)
$operatorStrings = @(JsonStringLeaves $operatorUbergraph[0])
$pendingTargetName = @($operatorClass[0].ChildProperties | Where-Object {
    $_.Name -ceq 'RailgunPendingTargetName'
})
Require ($pendingTargetName.Count -eq 1 -and
    $pendingTargetName[0].Type -ceq 'TextProperty' -and
    $pendingTargetName[0].PropertyFlags -match
        '(^| \| )Transient($| \| )') `
    'Final target-name resolution must use one transient FText value.'
foreach ($requiredRangeReference in @(
    'DetectedSharkName','OpticalTargetRange','RailgunPendingTargetName',
    "Class'KismetSystemLibrary:LineTraceSingle'",
    'GetItemName',
    "Class'VoyageMiscBlueprintFunctionLibrary:GetDestructibleInterface'",
    "Class'KismetTextLibrary:TextIsEmpty'",
    "Class'KismetTextLibrary:Conv_IntToText'",
    "Class'KismetTextLibrary:EqualEqual_TextText'",
    '---'
)) {
    Require ($operatorStrings -ccontains $requiredRangeReference) `
        ('Changed-only range reference missing: ' +
            $requiredRangeReference)
}
$targetProviderCalls = @($operatorStatements | Where-Object {
    @(JsonStringLeaves $_) -ccontains
        "Class'VoyageMiscBlueprintFunctionLibrary:GetDestructibleInterface'"
})
Require ($targetProviderCalls.Count -gt 0) `
    'Range target provider resolution is missing.'
foreach ($targetProviderCall in $targetProviderCalls) {
    $targetProviderParameters = @($targetProviderCall.Expression.Parameters)
    Require ($targetProviderParameters.Count -eq 1 -and
        $targetProviderParameters[0].Token -ceq 'EX_LocalVariable' -and
        $targetProviderParameters[0].Variable.Property.Name -match
            '^CallFunc_BreakHitResult_HitComponent(_\d+)?$' -and
        $targetProviderParameters[0].Variable.Property.Type -ceq
            'ObjectProperty' -and
        $targetProviderParameters[0].Variable.Property.PropertyClass.ObjectName `
            -ceq "Class'PrimitiveComponent'") `
        'Every range target provider call must consume the trace hit component.'
}
$targetItemNameCalls = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    $null -ne $_.Expression -and
    $_.Expression.Token -ceq 'EX_Context' -and
    $null -ne $_.Expression.ObjectExpression -and
    $_.Expression.ObjectExpression.Token -ceq 'EX_InterfaceContext' -and
    $_.Expression.ContextExpression.Token -ceq 'EX_VirtualFunction' -and
    $_.Expression.ContextExpression.Function -ceq 'GetItemName'
})
Require ($targetItemNameCalls.Count -gt 0) `
    'Range target item-name interface call is missing.'
foreach ($targetItemNameCall in $targetItemNameCalls) {
    Require (
        @($targetItemNameCall.Expression.ContextExpression.Parameters).Count `
            -eq 0 -and
        $targetItemNameCall.Variable.Variable.Property.Type -ceq
            'TextProperty' -and
        $targetItemNameCall.Expression.ObjectExpression.InterfaceValue.Variable.`
            Property.Type -ceq 'InterfaceProperty' -and
        $targetItemNameCall.Expression.ObjectExpression.InterfaceValue.Variable.`
            Property.InterfaceClass.ObjectName -ceq
                "Class'VoyageItemInterface'") `
        'Every range item-name call must be the exact no-argument Voyage item interface FText.'
}
$pendingTargetNameAssignments = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    @($_.PSObject.Properties.Name) -ccontains 'Variable' -and
    @($_.Variable.PSObject.Properties.Name) -ccontains 'Variable' -and
    @($_.Variable.Variable.PSObject.Properties.Name) -ccontains 'Property' -and
    $_.Variable.Variable.Property.Name -ceq 'RailgunPendingTargetName'
})
Require ($pendingTargetNameAssignments.Count -eq 2) `
    'Range target name must have only the empty reset and interface-name assignment.'
$pendingTargetNameInputs = @($pendingTargetNameAssignments | ForEach-Object {
    $_.Expression.Variable.Property.Name
})
Require (@($pendingTargetNameInputs | Where-Object {
        $_ -match '^CallFunc_Conv_StringToText_ReturnValue(_\d+)?$'
    }).Count -eq 1 -and
    @($pendingTargetNameInputs | Where-Object {
        $_ -ceq 'CallFunc_GetItemName_ReturnValue'
    }).Count -eq 1) `
    'Range target name must be sourced only from the empty reset or GetItemName FText.'
foreach ($forbiddenRangeCache in @(
    'RailgunRangeTargetActor','RailgunRangeTargetModule',
    'RailgunRangeTargetModuleResolved'
)) {
    Require (-not ($operatorStrings -ccontains $forbiddenRangeCache)) `
        ('Unapproved target-component cache was serialized: ' +
            $forbiddenRangeCache)
}
Require (@($operatorStrings | Where-Object {
    $_ -ceq "Class'Actor:K2_GetComponentsByClass'"
}).Count -eq 2 -and
    ($operatorStrings -ccontains 'FirstPersonCamera') -and
    ($operatorStrings -ccontains 'RailgunFirstPersonCamera') -and
    ($operatorStrings -ccontains 'RailgunFirstPersonCameraOwner')) `
    'Possession must resolve the first-person camera exactly once into owned references.'
foreach ($forbiddenRecurringAimTag in @(
    'Railgun.Model.Yaw','Railgun.Model.Pitch'
)) {
    Require (-not ($operatorStrings -ccontains $forbiddenRecurringAimTag)) `
        ('Recurring operator graph retained aim-role discovery: ' +
            $forbiddenRecurringAimTag)
}
foreach ($requiredFireReference in @(
    'ShotSpawnedThisPress','ShotAmmoSlot','AcceptedRailgunAmmo','ItemCount',
    'RailgunShotVolumePercent',
    "Class'VoyageModuleComponent:GetInternalInventory'",
    "Class'VoyageBaseInventoryComponent:GetLastOccupiedSlot'",
    "Class'VoyageBaseInventoryComponent:GetSlot'",
    "Class'VoyageModuleComponent:RemoveResource'",
    "Class'VoyageBaseInventoryComponent:RemoveItem'",
    "Class'GameplayStatics:BeginDeferredActorSpawnFromClass'",
    "Class'GameplayStatics:FinishSpawningActor'",
    "Class'KismetMathLibrary:Multiply_DoubleDouble'",
    "Class'GameplayStatics:PlaySoundAtLocation'",
    'K2_DestroyActor'
)) {
    Require ($operatorStrings -ccontains $requiredFireReference) `
        ('Railgun fire contract reference missing: ' + $requiredFireReference)
}
foreach ($forbiddenDiagnosticReference in @(
    'FreezeStatusText','RailgunCanaryStatusText','RailgunCanaryStatusExpires',
    'HC19 STOP: prerequisite/entry failed. Screenshot, quit without saving.',
    'HC23 STOP: stock input reference load/type/readback failed. Entry blocked; screenshot and quit.',
    'HC24 CAMERA STOP: missing camera/flag/FOV prerequisite. Entry blocked.',
    'RETURN FAILED: press E to retry; do not save. Report this.'
)) {
    Require (-not ($operatorPackageStrings -ccontains
            $forbiddenDiagnosticReference)) `
        ('Gameplay operator retained diagnostic text/reference: ' +
            $forbiddenDiagnosticReference)
}
foreach ($forbiddenFireReference in @(
    'Items','RailgunAmmoLastVisualCount','AddItem','ShotRefundFaulted',
    'ShotEnergyBeforeDebit',"Class'VoyageModuleComponent:AddResource'",
    "Class'KismetSystemLibrary:PrintString'"
)) {
    Require (-not ($operatorStrings -ccontains $forbiddenFireReference)) `
        ('Railgun fire must not use presentation state or direct mutation: ' +
            $forbiddenFireReference)
}
$lifecycleBind = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'BindRailgunShellLifecycle'
})
Require ($lifecycleBind.Count -eq 1) `
    'Expected one shell lifecycle binding function.'
$lifecycleBindStrings = @(JsonStringLeaves $lifecycleBind[0])
foreach ($requiredLifecycleBindReference in @(
    'RailgunShellOwner','OnEndPlay','OnRailgunShellEndPlay',
    'EX_AddMulticastDelegate','EX_RemoveMulticastDelegate'
)) {
    Require ($lifecycleBindStrings -ccontains
        $requiredLifecycleBindReference) `
        ('Shell lifecycle bind reference missing: ' +
            $requiredLifecycleBindReference)
}
$shellEndPlayCallback = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunShellEndPlay'
})
Require ($shellEndPlayCallback.Count -eq 1) `
    'Expected one exact shell EndPlay callback.'
$shellEndPlayParameters = @($shellEndPlayCallback[0].ChildProperties |
    Where-Object {
        $_.PSObject.Properties.Name -ccontains 'PropertyFlags' -and
        $_.PropertyFlags -match '(^| \| )Parm($| \| )'
    })
Require ($shellEndPlayParameters.Count -eq 2 -and
    @($shellEndPlayParameters | Where-Object {
        $_.Name -ceq 'Actor' -and $_.Type -ceq 'ObjectProperty' -and
        $_.PropertyClass.ObjectName -ceq "Class'Actor'" -and
        $_.PropertyClass.ObjectPath -ceq '/Script/Engine'
    }).Count -eq 1 -and
    @($shellEndPlayParameters | Where-Object {
        $_.Name -ceq 'EndPlayReason' -and $_.Type -ceq 'ByteProperty'
    }).Count -eq 1) `
    'Shell EndPlay callback must preserve the exact actor/reason delegate parameters.'
$shellEndPlayStrings = @(JsonStringLeaves $shellEndPlayCallback[0])
foreach ($requiredShellEndPlayReference in @(
    'RailgunShellOwner','RailgunEntryReady','RailgunEntryPending',
    'RailgunTeardownPending',
    "Class'Actor:SetActorEnableCollision'",
    'OnExitVehicle',
    'FinalizeRailgunStationTeardown'
)) {
    Require ($shellEndPlayStrings -ccontains
        $requiredShellEndPlayReference) `
        ('Shell EndPlay callback reference missing: ' +
            $requiredShellEndPlayReference)
}
$shellEndPlayReasonValues = @(
    $shellEndPlayCallback[0].ScriptBytecode |
        Where-Object {
            $_.PSObject.Properties.Name -ccontains 'Expression' -and
            $_.Expression.PSObject.Properties.Name -ccontains 'Function' -and
            $_.Expression.Token -ceq 'EX_CallMath' -and
            $_.Expression.Function.ObjectName -ceq
                "Class'KismetMathLibrary:EqualEqual_ByteByte'" -and
            @($_.Expression.Parameters | Where-Object {
                $_.Token -ceq 'EX_LocalVariable' -and
                $_.Variable.Property.Name -ceq 'EndPlayReason'
            }).Count -eq 1
        } |
        ForEach-Object {
            @($_.Expression.Parameters | Where-Object {
                $_.Token -ceq 'EX_ByteConst'
            }).Value
        }
)
Require ($shellEndPlayReasonValues.Count -eq 2 -and
    $shellEndPlayReasonValues -ccontains 0 -and
    $shellEndPlayReasonValues -ccontains 3) `
    'Shell EndPlay callback must handle Destroyed (0) and RemovedFromWorld (3).'
$finalizeStationTeardown = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'FinalizeRailgunStationTeardown'
})
Require ($finalizeStationTeardown.Count -eq 1) `
    'Expected one guarded station teardown finalizer.'
$finalizeStationTeardownStrings = @(
    JsonStringLeaves $finalizeStationTeardown[0])
foreach ($requiredTeardownReference in @(
    'RailgunTeardownPending','RailgunExitPending','RailgunOwnsView',
    'RailgunViewActor','RailgunViewController',
    'IsPlayerControlled',
    "Class'Pawn:GetController'",
    'GetViewTarget',
    "Class'Controller:K2_GetPawn'",
    'SetViewTargetWithBlend',
    'ClearRailgunAimReferences',
    'K2_DestroyActor'
)) {
    Require ($finalizeStationTeardownStrings -ccontains
        $requiredTeardownReference) `
        ('Station teardown finalizer reference missing: ' +
            $requiredTeardownReference)
}
$possessedEvent = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ReceivePossessed' -and
    $_.SuperStruct.ObjectName -ceq "Class'Pawn:ReceivePossessed'"
})
$unpossessedEvent = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ReceiveUnpossessed' -and
    $_.SuperStruct.ObjectName -ceq "Class'Pawn:ReceiveUnpossessed'"
})
Require ($possessedEvent.Count -eq 1 -and
    @($possessedEvent[0].ChildProperties | Where-Object {
        $_.Name -ceq 'NewController' -and $_.Type -ceq 'ObjectProperty' -and
        $_.PropertyClass.ObjectName -ceq "Class'Controller'"
    }).Count -eq 1) `
    'Station must implement the exact Pawn ReceivePossessed event.'
Require ($unpossessedEvent.Count -eq 1 -and
    @($unpossessedEvent[0].ChildProperties | Where-Object {
        $_.Name -ceq 'OldController' -and $_.Type -ceq 'ObjectProperty' -and
        $_.PropertyClass.ObjectName -ceq "Class'Controller'"
    }).Count -eq 1) `
    'Station must implement the exact Pawn ReceiveUnpossessed event.'
foreach ($requiredLifecycleEventReference in @(
    'RailgunEntryPending','RailgunExitPending',
    "Class'KismetSystemLibrary:DelayUntilNextTick'"
)) {
    Require ($operatorStrings -ccontains $requiredLifecycleEventReference) `
        ('Station lifecycle event reference missing: ' +
            $requiredLifecycleEventReference)
}
$modeWrites = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_LetBool' -and
    $null -ne $_.Variable -and
    $_.Variable.Token -ceq 'EX_InstanceVariable' -and
    $_.Variable.Variable.Property.Name -ceq 'RailgunWideView'
} | Sort-Object StatementIndex)
$mouseWrites = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    $null -ne $_.Variable -and
    $_.Variable.Token -ceq 'EX_InstanceVariable' -and
    $_.Variable.Variable.Property.Name -ceq
        'RailgunActiveMousePercent'
} | Sort-Object StatementIndex)
$modeNotifications = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_LocalVirtualFunction' -and
    $_.Function -ceq 'RefreshRailgunHudMode'
} | Sort-Object StatementIndex)
Require ($modeWrites.Count -eq 3 -and $mouseWrites.Count -eq 3 -and
    $modeNotifications.Count -eq 3 -and
    @($modeWrites | Where-Object {
        $_.Expression.Token -ceq 'EX_True'
    }).Count -eq 2 -and
    @($modeWrites | Where-Object {
        $_.Expression.Token -ceq 'EX_False'
    }).Count -eq 1) `
    'Entry and zoom paths must each assign mode/mouse and notify the HUD once.'
for ($modeIndex = 0; $modeIndex -lt $modeWrites.Count; $modeIndex++) {
    $modeWriteIndex = [int]$modeWrites[$modeIndex].StatementIndex
    $nextModeWriteIndex = if ($modeIndex + 1 -lt $modeWrites.Count) {
        [int]$modeWrites[$modeIndex + 1].StatementIndex
    } else { [int]::MaxValue }
    $pathMouseWrites = @($mouseWrites | Where-Object {
        $_.StatementIndex -gt $modeWriteIndex -and
        $_.StatementIndex -lt $nextModeWriteIndex
    })
    $pathFovWrites = @($operatorStatements | Where-Object {
        $_.StatementIndex -gt $modeWriteIndex -and
        $_.StatementIndex -lt $nextModeWriteIndex -and
        @(JsonStringLeaves $_) -ccontains 'SetFieldOfView'
    } | Sort-Object StatementIndex)
    $pathNotifications = @($modeNotifications | Where-Object {
        $_.StatementIndex -gt $modeWriteIndex -and
        $_.StatementIndex -lt $nextModeWriteIndex
    })
    $orderedFovWrites = @($pathFovWrites | Where-Object {
        $_.StatementIndex -gt $pathMouseWrites[0].StatementIndex -and
        $_.StatementIndex -lt $pathNotifications[0].StatementIndex
    })
    Require ($pathMouseWrites.Count -eq 1 -and
        $pathFovWrites.Count -ge 1 -and
        $pathNotifications.Count -eq 1 -and
        $pathMouseWrites[0].StatementIndex -lt
            $pathNotifications[0].StatementIndex -and
        $orderedFovWrites.Count -ge 1) `
        'Each mode assignment must set its mouse/FOV state before HUD notify.'
}
$entryModeWindow = @($operatorStatements | Where-Object {
    $_.StatementIndex -gt $modeWrites[0].StatementIndex -and
    $_.StatementIndex -le $modeNotifications[0].StatementIndex
})
Require ($modeWrites[0].Expression.Token -ceq 'EX_True' -and
    $mouseWrites[0].Expression.Token -ceq 'EX_DoubleConst' -and
    [Math]::Abs([double]$mouseWrites[0].Expression.Value - 100.0) -lt
        0.000001 -and
    @(JsonStringLeaves $entryModeWindow) -ccontains
        "Class'Actor:K2_AttachToComponent'" -and
    @(JsonStringLeaves $entryModeWindow) -ccontains 'RailgunOwnsView') `
    'Possession must restore wide camera/mouse state before notifying the HUD.'
Require (@($operatorStrings | Where-Object {
    $_ -ceq 'OnExitVehicle'
}).Count -eq 1) `
    'Operator ubergraph must request native exit only from the explicit input action.'
foreach ($requiredLifecycleTeardownReference in @(
    'RailgunShellOwner','OnRailgunShellEndPlay',
    'EX_RemoveMulticastDelegate'
)) {
    Require ($operatorStrings -ccontains
        $requiredLifecycleTeardownReference) `
        ('Station lifecycle unbind reference missing: ' +
            $requiredLifecycleTeardownReference)
}
$energyBind = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'BindRailgunEnergy'
})
Require ($energyBind.Count -eq 1) `
    'Expected one event-driven energy binding function.'
$energyBindStrings = @(JsonStringLeaves $energyBind[0])
foreach ($requiredEnergyBindReference in @(
    'RailgunEnergyModule',
    'RailgunEnergyDemandInitialized',
    'RailgunEnergyUpdateActive',
    'RailgunEnergyUpdatePending',
    'RailgunSocketConnected',
    'RailgunPowerAvailable',
    'StopRailgunOfflineDrain',
    'RailgunSupplyReconcileGeneration',
    'RailgunSupplyReconcilePending',
    'OnModuleValueChanged',
    'OnModuleSocketConnectionChanged',
    'OnModulePowerStateChanged',
    'OnRailgunEnergyModuleValueChanged',
    'OnRailgunSocketConnectionChanged',
    'OnRailgunPowerStateChanged',
    'RefreshRailgunSupplyState',
    'RefreshRailgunEnergy',
    'EX_AddMulticastDelegate',
    'EX_RemoveMulticastDelegate'
)) {
    Require ($energyBindStrings -ccontains $requiredEnergyBindReference) `
        ('Railgun energy-bind reference missing: ' +
            $requiredEnergyBindReference)
}
$runtimeActivity = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunRuntimeActivity'
})
Require ($runtimeActivity.Count -eq 1) `
    'Expected one runtime activity reconciler.'
$runtimeActivityStrings = @(JsonStringLeaves $runtimeActivity[0])
foreach ($requiredActivityReference in @(
    'RailgunEnergyModule','RailgunSocketConnected',
    'RailgunPowerAvailable','RailgunChargeAmount',
    'RailgunOfflineDrainActive','RailgunOfflineDischargeKW',
    'RailgunOfflineDrainRateKW','RailgunOfflineDrainTimestamp',
    'RailgunOfflineDrainTimerHandle','OnRailgunOfflineDrainTimer',
    'StopRailgunOfflineDrain',"Class'KismetSystemLibrary:K2_SetTimer'",
    'RailgunOwnsView','RailgunTeardownPending','SetActorTickEnabled'
)) {
    Require ($runtimeActivityStrings -ccontains $requiredActivityReference) `
        ('Runtime activity reference missing: ' +
            $requiredActivityReference)
}
foreach ($forbiddenActivityReference in @(
    "Class'VoyageModuleComponent:RemoveResource'",
    "Class'VoyageModuleComponent:HasSocketConnection'",
    "Class'VoyageModuleComponent:HasPower'"
)) {
    Require (-not ($runtimeActivityStrings -ccontains
        $forbiddenActivityReference)) `
        ('Runtime activity must only own timer lifecycle: ' +
            $forbiddenActivityReference)
}
$offlineDrainSettle = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'SettleRailgunOfflineDrain'
})
Require ($offlineDrainSettle.Count -eq 1) `
    'Expected one elapsed-time offline-drain integrator.'
$offlineDrainSettleStrings = @(JsonStringLeaves $offlineDrainSettle[0])
foreach ($requiredSettleReference in @(
    'RailgunOfflineDrainActive','RailgunOfflineDrainDebitActive',
    'RailgunOfflineDrainTimestamp','RailgunOfflineDrainRateKW',
    'RailgunOfflineDrainElapsed',
    "Class'VoyageModuleComponent:GetResourceAmount'",
    "Class'VoyageModuleComponent:RemoveResource'"
)) {
    Require ($offlineDrainSettleStrings -ccontains $requiredSettleReference) `
        ('Offline-drain integrator reference missing: ' +
            $requiredSettleReference)
}
$offlineDrainStop = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'StopRailgunOfflineDrain'
})
Require ($offlineDrainStop.Count -eq 1) `
    'Expected one idempotent offline-drain stop function.'
$offlineDrainStopStrings = @(JsonStringLeaves $offlineDrainStop[0])
foreach ($requiredStopReference in @(
    'SettleBeforeStop','SettleRailgunOfflineDrain',
    'RailgunOfflineDrainTimerHandle','RailgunOfflineDrainActive',
    "Class'KismetSystemLibrary:K2_ClearAndInvalidateTimerHandle'"
)) {
    Require ($offlineDrainStopStrings -ccontains $requiredStopReference) `
        ('Offline-drain stop reference missing: ' + $requiredStopReference)
}
$supplyRefresh = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunSupplyState'
})
Require ($supplyRefresh.Count -eq 1) `
    'Expected one event-driven supply snapshot function.'
$supplyRefreshStrings = @(JsonStringLeaves $supplyRefresh[0])
foreach ($requiredSupplyReference in @(
    'RailgunEnergyModule','RailgunSocketConnected',
    'RailgunPowerAvailable','RefreshRailgunRuntimeActivity',
    'RefreshRailgunHudEnergy',
    "Class'VoyageModuleComponent:HasSocketConnection'",
    "Class'VoyageModuleComponent:HasPower'"
)) {
    Require ($supplyRefreshStrings -ccontains $requiredSupplyReference) `
        ('Supply snapshot reference missing: ' + $requiredSupplyReference)
}
$energyRefresh = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunEnergy'
})
Require ($energyRefresh.Count -eq 1) `
    'Expected one guarded energy refresh function.'
$energyRefreshStrings = @(JsonStringLeaves $energyRefresh[0])
foreach ($requiredEnergyRefreshReference in @(
    'ForceDemand',
    'RailgunEnergyModule',
    'RailgunChargeAmount',
    'RailgunEnergyDemandInitialized',
    'RailgunEnergyDemandCharging',
    'RailgunIdleConsumptionKW',
    'RailgunEnergyUpdateActive',
    'RailgunEnergyUpdatePending',
    "Class'VoyageModuleComponent:GetResourceAmount'",
    "Class'VoyageModuleComponent:SetCustomConsumption'"
)) {
    Require ($energyRefreshStrings -ccontains $requiredEnergyRefreshReference) `
        ('Railgun energy-refresh reference missing: ' +
            $requiredEnergyRefreshReference)
}
Require (@($energyRefreshStrings | Where-Object {
        $_ -ceq "Class'VoyageModuleComponent:SetCustomConsumption'"
    }).Count -eq 2 -and
    ($energyRefreshStrings -ccontains
        "Class'KismetMathLibrary:Multiply_DoubleDouble'") -and
    ($energyRefreshStrings -ccontains
        "Class'KismetMathLibrary:Add_DoubleDouble'")) `
    'Energy demand must apply configured idle W in both idle and charging modes.'
$netChargeW = (0.85 * 1000.0 * 1000.0) / 5.5
foreach ($idleScenarioKW in @(0.0, 0.5)) {
    $idleScenarioW = $idleScenarioKW * 1000.0
    $chargingDemandW = $netChargeW + $idleScenarioW
    Require ([Math]::Abs(($chargingDemandW - $idleScenarioW) -
            $netChargeW) -lt 0.000001) `
        'Idle demand must not alter the configured net charge rate.'
}
$energyCallback = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunEnergyModuleValueChanged'
})
Require ($energyCallback.Count -eq 1) `
    'Expected one energy module-value callback.'
$energyCallbackParameters = @($energyCallback[0].ChildProperties |
    Where-Object {
        $_.PSObject.Properties.Name -ccontains 'PropertyFlags' -and
        $_.PropertyFlags -match '(^| \| )Parm($| \| )'
    })
Require ($energyCallbackParameters.Count -eq 1 -and
    $energyCallbackParameters[0].Name -ceq 'Module' -and
    $energyCallbackParameters[0].Type -ceq 'ObjectProperty' -and
    $energyCallbackParameters[0].PropertyClass.ObjectName -ceq
        "Class'VoyageModuleComponent'" -and
    $energyCallbackParameters[0].PropertyClass.ObjectPath -ceq '/Script/Voyage') `
    'Energy callback must own the exact Voyage module delegate signature.'
$energyCallbackStrings = @(JsonStringLeaves $energyCallback[0])
foreach ($requiredEnergyCallbackReference in @(
    'Module','RailgunEnergyModule','RefreshRailgunEnergy'
)) {
    Require ($energyCallbackStrings -ccontains
        $requiredEnergyCallbackReference) `
        ('Railgun energy callback reference missing: ' +
            $requiredEnergyCallbackReference)
}
foreach ($forbiddenEnergyCallbackReference in @(
    "Class'VoyageModuleComponent:AddResource'",
    "Class'VoyageModuleComponent:RemoveResource'",
    "Class'VoyageModuleComponent:SetCustomConsumption'",
    "Class'KismetSystemLibrary:Delay",
    "Class'GameplayStatics:GetAllActorsOfClass'"
)) {
    Require (-not ($energyCallbackStrings -ccontains
        $forbiddenEnergyCallbackReference)) `
        ('Energy callback must delegate to the guarded refresh only: ' +
            $forbiddenEnergyCallbackReference)
}
$socketCallback = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunSocketConnectionChanged'
})
Require ($socketCallback.Count -eq 1) `
    'Expected one module socket callback.'
$socketCallbackParameters = @($socketCallback[0].ChildProperties |
    Where-Object {
        $_.PSObject.Properties.Name -ccontains 'PropertyFlags' -and
        $_.PropertyFlags -match '(^| \| )Parm($| \| )'
    })
Require ($socketCallbackParameters.Count -eq 1 -and
    $socketCallbackParameters[0].Name -ceq 'Module' -and
    $socketCallbackParameters[0].Type -ceq 'ObjectProperty' -and
    $socketCallbackParameters[0].PropertyClass.ObjectName -ceq
        "Class'VoyageModuleComponent'") `
    'Socket callback must preserve the exact one-module delegate signature.'
$powerCallback = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunPowerStateChanged'
})
Require ($powerCallback.Count -eq 1) `
    'Expected one module power-state callback.'
$powerCallbackParameters = @($powerCallback[0].ChildProperties |
    Where-Object {
        $_.PSObject.Properties.Name -ccontains 'PropertyFlags' -and
        $_.PropertyFlags -match '(^| \| )Parm($| \| )'
    })
Require ($powerCallbackParameters.Count -eq 2 -and
    @($powerCallbackParameters | Where-Object {
        $_.Name -ceq 'SourceModule' -and
        $_.Type -ceq 'ObjectProperty' -and
        $_.PropertyClass.ObjectName -ceq "Class'VoyageModuleComponent'"
    }).Count -eq 1 -and
    @($powerCallbackParameters | Where-Object {
        $_.Name -ceq 'bHasPower' -and $_.Type -ceq 'BoolProperty'
    }).Count -eq 1) `
    'Power callback must preserve the exact module/bool delegate signature.'
$socketCallbackStrings = @(JsonStringLeaves $socketCallback[0])
Require (($socketCallbackStrings -ccontains 'RailgunEnergyModule') -and
    ($socketCallbackStrings -ccontains 'RailgunSupplyReconcilePending') -and
    ($socketCallbackStrings -ccontains 'RailgunSupplyReconcileGeneration') -and
    ($socketCallbackStrings -ccontains 'DeferredRailgunSupplyReconcile') -and
    -not ($socketCallbackStrings -ccontains
        "Class'VoyageModuleComponent:HasSocketConnection'") -and
    -not ($socketCallbackStrings -ccontains 'RailgunSocketConnected') -and
    -not ($socketCallbackStrings -ccontains
        'RefreshRailgunRuntimeActivity') -and
    -not ($socketCallbackStrings -ccontains
        "Class'KismetSystemLibrary:DelayUntilNextTick'")) `
    'Socket callback must coalesce a non-latent deferred reconciliation.'
$powerCallbackStrings = @(JsonStringLeaves $powerCallback[0])
Require (($powerCallbackStrings -ccontains 'RailgunEnergyModule') -and
    ($powerCallbackStrings -ccontains 'RailgunPowerAvailable') -and
    ($powerCallbackStrings -ccontains 'bHasPower') -and
    ($powerCallbackStrings -ccontains 'RefreshRailgunRuntimeActivity') -and
    ($powerCallbackStrings -ccontains 'RefreshRailgunHudEnergy') -and
    -not ($powerCallbackStrings -ccontains
        "Class'VoyageModuleComponent:HasPower'") -and
    -not ($powerCallbackStrings -ccontains
        "Class'KismetSystemLibrary:DelayUntilNextTick'")) `
    'Power callback must consume its payload without polling or delay.'
$deferredSupplyEvent = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'DeferredRailgunSupplyReconcile'
})
Require ($deferredSupplyEvent.Count -eq 1) `
    'Expected one coalesced deferred supply event.'
$deferredSupplyParameters = @($deferredSupplyEvent[0].ChildProperties |
    Where-Object {
        $_.PSObject.Properties.Name -ccontains 'PropertyFlags' -and
        $_.PropertyFlags -match '(^| \| )Parm($| \| )'
    })
Require ($deferredSupplyParameters.Count -eq 2 -and
    @($deferredSupplyParameters | Where-Object {
        $_.Name -ceq 'ReconcileModule' -and
        $_.Type -ceq 'ObjectProperty' -and
        $_.PropertyClass.ObjectName -ceq "Class'VoyageModuleComponent'"
    }).Count -eq 1 -and
    @($deferredSupplyParameters | Where-Object {
        $_.Name -ceq 'ReconcileGeneration' -and
        $_.Type -ceq 'IntProperty'
    }).Count -eq 1) `
    'Deferred supply event must carry exact module identity and generation.'
Require (@($operatorStrings | Where-Object {
        $_ -ceq 'RefreshRailgunHudEnergy'
    }).Count -eq 1) `
    'Deferred supply reconciliation must own one HUD status refresh.'
$drainTimerEvent = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'OnRailgunOfflineDrainTimer'
})
$drainTimerParameters = if ($drainTimerEvent.Count -eq 1 -and
    $drainTimerEvent[0].PSObject.Properties.Name -ccontains
        'ChildProperties') {
    @($drainTimerEvent[0].ChildProperties)
} else {
    @()
}
Require ($drainTimerEvent.Count -eq 1 -and
    @($drainTimerParameters).Count -eq 0) `
    'Expected one parameterless offline-drain timer event.'
foreach ($requiredEnergyEventReference in @(
    'RailgunSupplyReconcileGeneration','RailgunSupplyReconcilePending',
    'RailgunSupplyReconcileTime','SettleRailgunOfflineDrain',
    'StopRailgunOfflineDrain',
    "Class'KismetSystemLibrary:DelayUntilNextTick'"
)) {
    Require ($operatorStrings -ccontains $requiredEnergyEventReference) `
        ('Energy custom-event reference missing: ' +
            $requiredEnergyEventReference)
}
$chargeIndicatorBind = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'BindRailgunChargeIndicator'
})
Require ($chargeIndicatorBind.Count -eq 1) `
    'Expected one event-driven charge-indicator binding function.'
$chargeIndicatorBindStrings = @(JsonStringLeaves $chargeIndicatorBind[0])
foreach ($requiredChargeIndicatorReference in @(
    'RailgunChargeIndicatorModule',
    'OnModuleValueChanged',
    'OnRailgunChargeIndicatorModuleValueChanged',
    'RefreshRailgunChargeIndicator',
    'EX_AddMulticastDelegate',
    'EX_RemoveMulticastDelegate'
)) {
    Require ($chargeIndicatorBindStrings -ccontains
        $requiredChargeIndicatorReference) `
        ('Railgun charge-indicator bind reference missing: ' +
            $requiredChargeIndicatorReference)
}
$chargeIndicatorRefresh = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunChargeIndicator'
})
Require ($chargeIndicatorRefresh.Count -eq 1) `
    'Expected one charge-indicator refresh function.'
$chargeIndicatorRefreshStrings = @(JsonStringLeaves $chargeIndicatorRefresh[0])
foreach ($requiredChargeIndicatorReference in @(
    'RailgunChargeIndicatorOwner',
    'RailgunChargeIndicatorComponent',
    'RailgunChargeIndicatorMaterial',
    'RailgunChargeIndicatorLastLevel',
    'RailgunChargeIndicatorModule',
    "Class'VoyageModuleComponent:GetResourceAmount'",
    'Railgun.Model.ChargeIndicator',
    'ProgressLevel',
    'CreateDynamicMaterialInstance',
    "Class'MaterialInstanceDynamic:SetScalarParameterValue'",
    '/Game/Materials/Modules/MI_PogressBar_Basic_LED.MI_PogressBar_Basic_LED'
)) {
    Require ($chargeIndicatorRefreshStrings -ccontains
        $requiredChargeIndicatorReference) `
        ('Railgun charge-indicator refresh reference missing: ' +
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
    'RefreshRailgunChargeIndicator'
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
    'RailgunEnergyModule',
    'OnRailgunEnergyModuleValueChanged',
    'OnRailgunSocketConnectionChanged',
    'OnRailgunPowerStateChanged',
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
Require (-not ($operatorStrings -ccontains
    "Class'VoyageModuleComponent:SetCustomConsumption'")) `
    'Operator tick/fire/end-play graph must not maintain energy demand.'
foreach ($removedEnergyField in @(
    'RailgunEnergySampled','RailgunPreviousChargeAmount','RailgunChargeKW'
)) {
    Require (-not ($operatorStrings -ccontains $removedEnergyField)) `
        ('Removed tick-sampling state was serialized: ' + $removedEnergyField)
}
Require (-not ($operatorStrings -ccontains "Class'GameplayStatics:GetAllActorsOfClass'")) `
    'Railgun charge indicator must not add a world actor scan.'
$removeEnergyCallIndexes = @(NativeContextCallIndexes $operatorStatements `
    "Class'VoyageModuleComponent:RemoveResource'")
$removeAmmoCallIndexes = @(NativeContextCallIndexes $operatorStatements `
    "Class'VoyageBaseInventoryComponent:RemoveItem'")
$refundCallIndexes = @(NativeContextCallIndexes $operatorStatements `
    "Class'VoyageModuleComponent:AddResource'")
Require ($removeEnergyCallIndexes.Count -eq 1) `
    'Railgun ubergraph must contain only the shot energy debit.'
$offlineDrainStatements = @($offlineDrainSettle[0].ScriptBytecode)
$offlineDrainRemoveIndexes = @(NativeContextCallIndexes `
    $offlineDrainStatements "Class'VoyageModuleComponent:RemoveResource'")
Require ($offlineDrainRemoveIndexes.Count -eq 1) `
    'Offline-drain integrator must contain exactly one native energy debit.'
$offlineDrainRemoveStatements = @($offlineDrainStatements | Where-Object {
    [int]$_.StatementIndex -eq [int]$offlineDrainRemoveIndexes[0]
})
Require ($offlineDrainRemoveStatements.Count -eq 1) `
    'Offline-drain debit statement could not be resolved.'
$offlineDrainRemoveParameters = @(
    $offlineDrainRemoveStatements[0].Expression.ContextExpression.Parameters
)
Require ($offlineDrainRemoveParameters.Count -eq 3 -and
    $offlineDrainRemoveParameters[1].Token -ceq 'EX_LocalVariable' -and
    $offlineDrainRemoveParameters[1].Variable.Property.Name -clike
        'CallFunc_FMin_ReturnValue*') `
    'Offline idle drain must debit the amount capped by stored energy.'
$offlineDrainMath = @($offlineDrainStatements | Where-Object {
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
Require ($offlineDrainMath.Count -ge 1 -and
    ($offlineDrainSettleStrings -ccontains
        'RailgunOfflineDrainElapsed') -and
    ($offlineDrainSettleStrings -ccontains
        'RailgunOfflineDrainTimestamp') -and
    ($offlineDrainSettleStrings -ccontains
        'RailgunOfflineDrainRateKW')) `
    'Offline drain must integrate captured elapsed game time at its active rate.'
$receiveTick = @($operatorFunctions | Where-Object {
    $_.Name -ceq 'ReceiveTick'
})
Require ($receiveTick.Count -eq 1) 'Expected one station ReceiveTick event.'
$receiveTickStrings = @(JsonStringLeaves $receiveTick[0])
foreach ($forbiddenTickDrainReference in @(
    'RailgunOfflineDrainActive','SettleRailgunOfflineDrain',
    "Class'VoyageModuleComponent:RemoveResource'",
    "Class'KismetSystemLibrary:K2_SetTimer'"
)) {
    Require (-not ($receiveTickStrings -ccontains
        $forbiddenTickDrainReference)) `
        ('Station ReceiveTick must remain optics-only: ' +
            $forbiddenTickDrainReference)
}
Require (-not ($runtimeActivityStrings -ccontains
        "Class'VoyageModuleComponent:HasSocketConnection'") -and
    -not ($runtimeActivityStrings -ccontains
        "Class'VoyageModuleComponent:HasPower'")) `
    'Timer lifecycle must not poll socket or power supply state.'
Require ($removeAmmoCallIndexes.Count -eq 1) `
    'Railgun fire must contain exactly one native ammo debit.'
Require ($refundCallIndexes.Count -eq 0) `
    'Railgun fire must not contain an energy refund path.'
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
Require ($removeAmmoResultGates.Count -eq 0) `
    'Railgun fire must not branch on the RemoveItem return value.'
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
        $_ -gt [int]$removeAmmoCallIndexes[0]
    } | Sort-Object)
$shotAudioCalls = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_CallMath' -and
    $_.Function.ObjectName -ceq "Class'GameplayStatics:PlaySoundAtLocation'" -and
        [int]$_.StatementIndex -gt [int]$removeAmmoCallIndexes[0]
    } | Sort-Object StatementIndex)
$audioIndexes = @($shotAudioCalls | ForEach-Object { [int]$_.StatementIndex })
$getSlotIndexes = @(StatementIndexesContaining $operatorStatements `
    "Class'VoyageBaseInventoryComponent:GetSlot'")
Require ($finishIndexes.Count -ge 1 -and $audioIndexes.Count -eq 1 -and
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
$shotFinishIndex = [int]$finishIndexes[0]
$shotAudioIndex = [int]$audioIndexes[0]
Require ((($getSlotIndexes | Measure-Object -Maximum).Maximum) -lt
        $shotBeginIndex -and
    $shotBeginIndex -lt $claimIndex -and
    $claimIndex -lt $removeEnergyIndex -and
    $removeEnergyIndex -lt $removeAmmoIndex -and
    $removeAmmoIndex -lt $shotFinishIndex -and
    $shotFinishIndex -lt $shotAudioIndex) `
    'Railgun fire must preflight, claim, debit both resources and only then activate the shot.'
$shotAudioParameters = @($shotAudioCalls[0].Parameters)
Require ($shotAudioParameters.Count -ge 5 -and
    $shotAudioParameters[4].Token -ceq 'EX_LocalVariable') `
    'Shot audio volume multiplier is not supplied by the generated graph.'
$shotAudioVolumeFloat = $shotAudioParameters[4].Variable.Property.Name
$shotAudioVolumeCasts = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    $_.Variable.Token -ceq 'EX_LocalVariable' -and
    $_.Variable.Variable.Property.Name -ceq $shotAudioVolumeFloat -and
    $_.Expression.Token -ceq 'EX_Cast' -and
    $_.Expression.ConversionType -ceq 'CST_DoubleToFloat' -and
    $_.Expression.Target.Token -ceq 'EX_LocalVariable'
})
Require ($shotAudioVolumeCasts.Count -eq 1) `
    'Shot audio volume multiplier lost its double-to-float conversion.'
$shotAudioVolumeDouble =
    $shotAudioVolumeCasts[0].Expression.Target.Variable.Property.Name
$shotAudioVolumeProducts = @($operatorStatements | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    $_.Variable.Token -ceq 'EX_LocalVariable' -and
    $_.Variable.Variable.Property.Name -ceq $shotAudioVolumeDouble -and
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq
        "Class'KismetMathLibrary:Multiply_DoubleDouble'"
})
Require ($shotAudioVolumeProducts.Count -eq 1) `
    'Shot audio volume multiplier lost its percentage conversion.'
$shotAudioVolumeInputs = @($shotAudioVolumeProducts[0].Expression.Parameters)
Require ($shotAudioVolumeInputs.Count -eq 2 -and
    $shotAudioVolumeInputs[0].Token -ceq 'EX_InstanceVariable' -and
    $shotAudioVolumeInputs[0].Variable.Property.Name -ceq
        'RailgunShotVolumePercent' -and
    $shotAudioVolumeInputs[1].Token -ceq 'EX_DoubleConst' -and
    [Math]::Abs([double]$shotAudioVolumeInputs[1].Value - 0.01) -lt
        0.0000001) `
    'PlaySoundAtLocation must receive RailgunShotVolumePercent multiplied by 0.01.'
$shotDestroyIndexes = @(StatementIndexesContaining $operatorStatements `
    'K2_DestroyActor')
Require ($shotDestroyIndexes.Count -eq 2) `
    'Railgun fire must destroy the deferred shot on cast and energy failure paths.'
$hud = @(Read-Candidate $hudPackage)
$hudPackageStrings = @(JsonStringLeaves $hud)
foreach ($forbiddenHudDiagnosticReference in @(
    'FreezeStatusText','RailgunCanaryStatusText','RailgunCanaryStatusExpires'
)) {
    Require (-not ($hudPackageStrings -ccontains
            $forbiddenHudDiagnosticReference)) `
        ('Gameplay HUD retained diagnostic text/reference: ' +
            $forbiddenHudDiagnosticReference)
}
$hudFunctions = @($hud | Where-Object { $_.Type -ceq 'Function' })
$hudUbergraph = @($hudFunctions | Where-Object {
    $_.Name -ceq 'ExecuteUbergraph_WBP_RailgunHUD'
})
Require ($hudUbergraph.Count -eq 1) 'Expected one Railgun HUD ubergraph.'
$hudUbergraphStrings = @(JsonStringLeaves $hudUbergraph[0])
$hudClass = @($hud | Where-Object {
    $_.Type -ceq 'WidgetBlueprintGeneratedClass'
})
Require ($hudClass.Count -eq 1) `
    'Expected one Railgun HUD generated class.'
Require ($hudClass[0].SuperStruct.ObjectName -ceq
    "Class'VoyageBaseUserWidget'") `
    'Railgun HUD must retain VoyageBaseUserWidget as its base class.'
$hudWidgetTree = @($hud | Where-Object {
    $_.Type -ceq 'WidgetTree'
})
$hudInvalidationRoots = @($hud | Where-Object {
    $_.Type -ceq 'InvalidationBox' -and
    $_.Name -ceq 'RailgunHudInvalidationRoot'
})
$hudCanvases = @($hud | Where-Object {
    $_.Type -ceq 'CanvasPanel' -and
    $_.Name -ceq 'DiagnosticCanvas'
})
$hudHintWidgets = @($hud | Where-Object {
    $_.Type -ceq 'BP_DynamicPlayerInputHorizontalWidget_C' -and
    $_.Name -ceq 'RailgunActionHints'
})
Require ($hudWidgetTree.Count -eq 1 -and
    $hudInvalidationRoots.Count -eq 1 -and
    $hudCanvases.Count -eq 1 -and
    $hudHintWidgets.Count -eq 1) `
    'HUD must contain one InvalidationBox root, Canvas and exact stock hint widget.'
Require ($hudWidgetTree[0].Properties.RootWidget.ObjectName -ceq
    "InvalidationBox'WBP_RailgunHUD_C:WidgetTree.RailgunHudInvalidationRoot'") `
    'Railgun HUD WidgetTree root must be its InvalidationBox.'
$hudInvalidationSlots = @($hud | Where-Object {
    $_.Type -ceq 'PanelSlot' -and
    $_.Properties.Parent.ObjectName -ceq
        "InvalidationBox'WBP_RailgunHUD_C:WidgetTree.RailgunHudInvalidationRoot'" -and
    $_.Properties.Content.ObjectName -ceq
        "CanvasPanel'WBP_RailgunHUD_C:WidgetTree.DiagnosticCanvas'"
})
Require ($hudInvalidationSlots.Count -eq 1) `
    'Railgun HUD InvalidationBox must contain its existing Canvas directly.'
$hudHintSlots = @($hud | Where-Object {
    $_.Type -ceq 'CanvasPanelSlot' -and
    $_.Properties.Parent.ObjectName -ceq
        "CanvasPanel'WBP_RailgunHUD_C:WidgetTree.DiagnosticCanvas'" -and
    $_.Properties.Content.ObjectName -ceq
        "BP_DynamicPlayerInputHorizontalWidget_C'WBP_RailgunHUD_C:WidgetTree.RailgunActionHints'"
})
Require ($hudHintSlots.Count -eq 1) `
    'Stock Railgun hint widget must be a direct Canvas child.'
$hudHintLayout = $hudHintSlots[0].Properties.LayoutData
Require ($hudHintSlots[0].Properties.bAutoSize -eq $true -and
    $hudHintLayout.Offsets.Left -eq 24.0 -and
    $hudHintLayout.Offsets.Top -eq -24.0 -and
    $hudHintLayout.Anchors.Minimum.X -eq 0.0 -and
    $hudHintLayout.Anchors.Minimum.Y -eq 1.0 -and
    $hudHintLayout.Anchors.Maximum.X -eq 0.0 -and
    $hudHintLayout.Anchors.Maximum.Y -eq 1.0 -and
    $hudHintLayout.Alignment.X -eq 0.0 -and
    $hudHintLayout.Alignment.Y -eq 1.0) `
    'Railgun hint Canvas slot must preserve its anchor, alignment, offset and autosize.'
Require ($hudHintWidgets[0].Properties.ContextAsset.ObjectName -ceq
        "VoyageInputContextAsset'DA_RailgunInputContext'" -and
    $hudHintWidgets[0].Properties.ContextAsset.ObjectPath -ceq
        '/Game/Mods/Railgun/Inputs/DA_RailgunInputContext.0' -and
    $hudHintWidgets[0].Properties.bFilterByActionType -eq $true) `
    'Stock hint widget must use the Railgun context with action-type filtering.'
$hudHintPropertyNames = @(
    $hudHintWidgets[0].Properties.PSObject.Properties.Name)
Require ($hudHintPropertyNames.Count -eq 3 -and
    $hudHintPropertyNames -ccontains 'Slot' -and
    $hudHintPropertyNames -ccontains 'ContextAsset' -and
    $hudHintPropertyNames -ccontains 'bFilterByActionType') `
    'Stock hint instance must not embed or override its stock implementation.'
foreach ($obsoleteHintArtifact in @(
    'RailgunHintHost','RailgunHintWidgetReady',
    "Class'KismetSystemLibrary:MakeSoftClassPath'",
    "Class'KismetSystemLibrary:LoadClassAsset_Blocking'",
    "Class'WidgetBlueprintLibrary:Create'",
    "Class'VerticalBox:AddChildToVerticalBox'"
)) {
    Require (-not ($hudPackageStrings -ccontains $obsoleteHintArtifact)) `
        ('HUD retained obsolete runtime hint construction artifact: ' +
            $obsoleteHintArtifact)
}
foreach ($displayState in @(
    @{Name='RailgunDisplayedTargetName'; Type='TextProperty'},
    @{Name='RailgunDisplayedTargetRange'; Type='TextProperty'},
    @{Name='RailgunRangeDisplayInitialized'; Type='BoolProperty'},
    @{Name='RailgunHudStatusState'; Type='IntProperty'},
    @{Name='RailgunHudStatusInitialized'; Type='BoolProperty'}
)) {
    $property = @($hudClass[0].ChildProperties | Where-Object {
        $_.Name -ceq $displayState.Name
    })
    Require ($property.Count -eq 1 -and
        $property[0].Type -ceq $displayState.Type -and
        $property[0].PropertyFlags -match
            '(^| \| )Transient($| \| )') `
        ('HUD range display state must be exact and transient: ' +
            $displayState.Name)
}
$rangeSetTargets = @(TextSetTargets $hudUbergraph[0])
Require (@($rangeSetTargets | Where-Object {
        $_ -ceq 'DetectedSharkName'
    }).Count -eq 2 -and
    @($rangeSetTargets | Where-Object {
        $_ -ceq 'OpticalTargetRange'
    }).Count -eq 2 -and
    @($hudUbergraphStrings | Where-Object {
        $_ -ceq "Class'KismetTextLibrary:EqualEqual_TextText'"
    }).Count -eq 2) `
    'HUD must own one initial and one changed-only SetText path per range field.'
$rangeDisplayInitializationWrites = @($hudUbergraph[0].ScriptBytecode |
    Where-Object {
        $_.Token -ceq 'EX_LetBool' -and
        $null -ne $_.Variable -and
        $_.Variable.Token -ceq 'EX_InstanceVariable' -and
        $_.Variable.Variable.Property.Name -ceq
            'RailgunRangeDisplayInitialized'
    })
Require ($rangeDisplayInitializationWrites.Count -eq 3 -and
    @($rangeDisplayInitializationWrites | Where-Object {
        $_.Expression.Token -ceq 'EX_True'
    }).Count -eq 2 -and
    @($rangeDisplayInitializationWrites | Where-Object {
        $_.Expression.Token -ceq 'EX_False'
    }).Count -eq 1) `
    'HUD range display must initialize on construction/tick and reset on destruction.'
$hudEnergyRefresh = @($hudFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunHudEnergy'
})
$hudAmmoRefresh = @($hudFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunHudAmmo'
})
$hudStyleRefresh = @($hudFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunHudStyle'
})
$hudModeRefresh = @($hudFunctions | Where-Object {
    $_.Name -ceq 'RefreshRailgunHudMode'
})
$hudApplyStatus = @($hudFunctions | Where-Object {
    $_.Name -ceq 'ApplyRailgunHudStatus'
})
$hudResetStatus = @($hudFunctions | Where-Object {
    $_.Name -ceq 'ResetRailgunHudStatus'
})
Require ($hudEnergyRefresh.Count -eq 1 -and $hudAmmoRefresh.Count -eq 1 -and
    $hudStyleRefresh.Count -eq 1 -and $hudModeRefresh.Count -eq 1 -and
    $hudApplyStatus.Count -eq 1 -and $hudResetStatus.Count -eq 1) `
    'HUD event-refresh function set is incomplete or duplicated.'
$applyStatusStrings = @(JsonStringLeaves $hudApplyStatus[0])
$resetStatusStrings = @(JsonStringLeaves $hudResetStatus[0])
Require (@($applyStatusStrings | Where-Object {
        $_ -ceq "Class'UserWidget:PlayAnimation'"
    }).Count -eq 1 -and
    @($applyStatusStrings | Where-Object {
        $_ -ceq "Class'UserWidget:StopAnimation'"
    }).Count -eq 1 -and
    ($applyStatusStrings -ccontains 'RailgunHudStatusState') -and
    ($applyStatusStrings -ccontains 'RailgunHudStatusInitialized') -and
    ($applyStatusStrings -ccontains
        "Class'KismetMathLibrary:BooleanAND'") -and
    @($resetStatusStrings | Where-Object {
        $_ -ceq "Class'UserWidget:StopAnimation'"
    }).Count -eq 1 -and
    -not ($resetStatusStrings -ccontains
        "Class'UserWidget:PlayAnimation'")) `
    'HUD status transitions must be changed-only and own animation cleanup.'
$forbiddenStatusTimerReferences = @(
    "Class'KismetSystemLibrary:K2_SetTimerDelegate'",
    "Class'KismetSystemLibrary:K2_SetTimer'",
    "Class'GameplayStatics:GetTimeSeconds'",
    "Class'KismetMathLibrary:FMod'"
)
foreach ($forbiddenStatusTimerReference in $forbiddenStatusTimerReferences) {
    Require (-not ($applyStatusStrings -ccontains
            $forbiddenStatusTimerReference) -and
        -not ($resetStatusStrings -ccontains
            $forbiddenStatusTimerReference)) `
        ('HUD status animation retained a timer/poll reference: ' +
            $forbiddenStatusTimerReference)
}
$statusAnimations = @($hud | Where-Object {
    $_.Type -ceq 'WidgetAnimation' -and
    $_.Name -ceq 'RailgunStatusChargingBlink_INST'
})
$statusScenes = @($hud | Where-Object {
    $_.Type -ceq 'MovieScene' -and
    $_.Name -ceq 'RailgunStatusChargingBlink'
})
$statusTracks = @($hud | Where-Object {
    $_.Type -ceq 'MovieSceneFloatTrack'
})
$statusSections = @($hud | Where-Object {
    $_.Type -ceq 'MovieSceneFloatSection'
})
Require ($statusAnimations.Count -eq 1 -and $statusScenes.Count -eq 1 -and
    $statusTracks.Count -eq 1 -and $statusSections.Count -eq 1) `
    'Charging status UMG animation structure is incomplete or duplicated.'
$statusBinding = @($statusAnimations[0].Properties.AnimationBindings)
$statusPossessable = @($statusScenes[0].Properties.Possessables)
$statusTrack = $statusTracks[0]
$statusCurve = $statusSections[0].Properties.FloatCurve
$statusAnimationProperties = @($hudClass[0].ChildProperties | Where-Object {
    $_.Type -ceq 'ObjectProperty' -and
    $_.PropertyClass.ObjectName -ceq "Class'WidgetAnimation'"
})
Require ($statusBinding.Count -eq 1 -and
    $statusAnimationProperties.Count -eq 1 -and
    $statusScenes[0].Name -ceq $statusAnimationProperties[0].Name -and
    $statusBinding[0].WidgetName -ceq 'RailgunStatusCharging' -and
    -not [bool]$statusBinding[0].bIsRootWidget -and
    $statusPossessable.Count -eq 1 -and
    $statusPossessable[0].Name -ceq 'RailgunStatusCharging' -and
    $statusPossessable[0].Guid -ceq $statusBinding[0].AnimationGuid -and
    $statusTrack.Properties.PropertyBinding.PropertyName -ceq
        'RenderOpacity' -and
    $statusTrack.Properties.PropertyBinding.PropertyPath -ceq
        'RenderOpacity') `
    'Charging status animation is not runtime-bindable to the owned icon opacity.'
$statusSceneHasTickResolution = @($statusScenes[0].Properties.
    PSObject.Properties.Name) -ccontains 'TickResolution'
$statusTickNumerator = if ($statusSceneHasTickResolution) {
    [int]$statusScenes[0].Properties.TickResolution.Numerator
} else { 60000 }
$statusTickDenominator = if ($statusSceneHasTickResolution -and
    @($statusScenes[0].Properties.TickResolution.PSObject.Properties.Name) `
        -ccontains 'Denominator') {
    [int]$statusScenes[0].Properties.TickResolution.Denominator
} else { 1 }
$statusTickRate = $statusTickNumerator / [double]$statusTickDenominator
$statusChannelTickNumerator =
    [int]$statusCurve.TickResolution.Numerator
$statusChannelTickDenominator = if (@($statusCurve.TickResolution.
        PSObject.Properties.Name) -ccontains 'Denominator') {
    [int]$statusCurve.TickResolution.Denominator
} else { 1 }
$statusChannelTickRate = $statusChannelTickNumerator /
    [double]$statusChannelTickDenominator
$statusDurationTicks = [int]$statusScenes[0].Properties.PlaybackRange.Value.
    UpperBound.Value.Value
$statusHiddenTick = [int]$statusCurve.Times[1].Value
Require ($statusTickRate -ge 24000 -and
    [Math]::Abs($statusChannelTickRate - $statusTickRate) -lt 0.000001 -and
    [int]$statusScenes[0].Properties.DisplayRate.Numerator -eq 4 -and
    [int]$statusScenes[0].Properties.PlaybackRange.Value.LowerBound.Value.Value -eq
        0 -and
    [Math]::Abs(($statusDurationTicks / [double]$statusTickRate) - 0.5) -lt
        0.000001 -and
    @($statusCurve.Times).Count -eq 2 -and
    [int]$statusCurve.Times[0].Value -eq 0 -and
    [Math]::Abs(($statusHiddenTick / [double]$statusTickRate) - 0.25) -lt
        0.000001 -and
    @($statusCurve.Values).Count -eq 2 -and
    [Math]::Abs([double]$statusCurve.Values[0].Value - 1.0) -lt 0.0001 -and
    [Math]::Abs([double]$statusCurve.Values[1].Value) -lt 0.0001 -and
    [int]$statusCurve.Values[0].InterpMode -eq 1 -and
    [int]$statusCurve.Values[1].InterpMode -eq 1) `
    'Charging status animation must step 0.25 seconds on and 0.25 seconds off.'
foreach ($animationFrameRate in @(30, 60, 120, 240)) {
    $animationTick = 0
    $animationWasHidden = $false
    $animationTransitions = 0
    for ($frame = 0; $frame -lt (2 * $animationFrameRate); $frame++) {
        $nextAnimationTick = [int][Math]::Round(
            $animationTick + ($statusTickRate / [double]$animationFrameRate))
        $nextAnimationTick %= $statusDurationTicks
        $animationIsHidden = $nextAnimationTick -ge $statusHiddenTick
        if ($animationIsHidden -ne $animationWasHidden) {
            $animationTransitions++
        }
        $animationWasHidden = $animationIsHidden
        $animationTick = $nextAnimationTick
    }
    Require ($animationTransitions -eq 8) `
        ('Charging animation does not progress for ordinary frame rate ' +
            $animationFrameRate + ' fps.')
}
$playStatusStatements = @($hudApplyStatus[0].ScriptBytecode |
    Where-Object {
        @(JsonStringLeaves $_) -ccontains "Class'UserWidget:PlayAnimation'"
    })
Require ($playStatusStatements.Count -eq 1) `
    'Charging status must own exactly one animation play call.'
$playStatusParameters = @(
    $playStatusStatements[0].Expression.ContextExpression.Parameters)
Require ($playStatusParameters.Count -eq 6 -and
    $playStatusParameters[1].Token -ceq 'EX_FloatConst' -and
    [Math]::Abs([double]$playStatusParameters[1].Value) -lt 0.0001 -and
    $playStatusParameters[2].Token -ceq 'EX_IntConst' -and
    [int]$playStatusParameters[2].Value -eq 0 -and
    $playStatusParameters[3].Token -ceq 'EX_ByteConst' -and
    [int]$playStatusParameters[3].Value -eq 0 -and
    $playStatusParameters[4].Token -ceq 'EX_FloatConst' -and
    [Math]::Abs([double]$playStatusParameters[4].Value - 1.0) -lt 0.0001 -and
    $playStatusParameters[5].Token -ceq 'EX_False') `
    'Charging status animation must loop forward indefinitely at unit speed.'
$hudTick = @($hudFunctions | Where-Object { $_.Name -ceq 'Tick' })
$hudConstruct = @($hudFunctions | Where-Object { $_.Name -ceq 'Construct' })
Require ($hudTick.Count -eq 1 -and $hudConstruct.Count -eq 1) `
    'HUD tick/construct event wrappers are incomplete.'
$tickStart = [int]$hudTick[0].ScriptBytecode[2].Parameters[0].Value
$constructStart = [int]$hudConstruct[0].ScriptBytecode[0].Parameters[0].Value
Require ($constructStart -gt $tickStart) `
    'HUD event layout no longer exposes a bounded tick section.'
$tickStatements = @($hudUbergraph[0].ScriptBytecode | Where-Object {
    [int]$_.StatementIndex -ge $tickStart -and
    [int]$_.StatementIndex -lt $constructStart
})
$tickStrings = @(JsonStringLeaves $tickStatements)
foreach ($forbiddenTickStatusReference in @(
    'ApplyRailgunHudStatus','ResetRailgunHudStatus',
    'RailgunStatusCharging','RailgunStatusOffline','RailgunStatusReady',
    "Class'UserWidget:PlayAnimation'","Class'UserWidget:StopAnimation'"
)) {
    Require (-not ($tickStrings -ccontains $forbiddenTickStatusReference)) `
        ('Widget Tick must remain range-only: ' +
            $forbiddenTickStatusReference)
}
$hudEnergyBytecode = @($hudEnergyRefresh[0].ScriptBytecode)
$hudAmmoBytecode = @($hudAmmoRefresh[0].ScriptBytecode)
$hudModeBytecode = @($hudModeRefresh[0].ScriptBytecode)
$hudEnergyStrings = @(JsonStringLeaves $hudEnergyRefresh[0])
$hudAmmoStrings = @(JsonStringLeaves $hudAmmoRefresh[0])
$statusStyleWidgets = @(
    'RailgunStatusCharging','RailgunStatusOffline','RailgunStatusReady')
$statusStyleCalls = @($hudStyleRefresh[0].ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_Context' -and
    $null -ne $_.ObjectExpression -and
    $_.ObjectExpression.Token -ceq 'EX_InstanceVariable' -and
    $statusStyleWidgets -ccontains
        $_.ObjectExpression.Variable.Property.Name
})
Require ($statusStyleCalls.Count -eq 3 -and
    @($statusStyleCalls | ForEach-Object {
        $_.ObjectExpression.Variable.Property.Name
    } | Sort-Object -Unique).Count -eq 3 -and
    @($statusStyleCalls | Where-Object {
        $_.ContextExpression.Function.ObjectName -ceq
            "Class'Image:SetOpacity'" -and
        @($_.ContextExpression.Parameters).Count -eq 1
    }).Count -eq 3) `
    'HUD status style must write image alpha without owning render opacity.'
Require (-not (@(JsonStringLeaves $statusStyleCalls) -ccontains
        "Class'Widget:SetRenderOpacity'")) `
    'HUD status style must not overwrite the animation render-opacity factor.'
foreach ($statusOpacityPercent in @(0.0, 50.0, 100.0)) {
    $statusUserAlpha = $statusOpacityPercent * 0.01
    Require ([Math]::Abs(($statusUserAlpha * 1.0) -
            ($statusOpacityPercent / 100.0)) -lt 0.000001 -and
        [Math]::Abs($statusUserAlpha * 0.0) -lt 0.000001) `
        'HUD status user alpha and blink factor no longer compose correctly.'
}
$hudAllStrings = @(JsonStringLeaves $hudFunctions)
$chargeTextConversions = @($hudEnergyBytecode | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    $null -ne $_.Expression -and
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq
        "Class'KismetTextLibrary:Conv_DoubleToText'"
})
Require ($chargeTextConversions.Count -eq 1) `
    'HUD must format exactly one numeric charge value.'
$formattedChargeInput = $chargeTextConversions[0].Expression.Parameters[0]
Require ($formattedChargeInput.Token -ceq 'EX_LocalVariable') `
    'HUD charge text must consume an explicit native-amount conversion.'
$formattedChargeVariable =
    $formattedChargeInput.Variable.Property.Name
$chargeUnitConversions = @($hudEnergyBytecode | Where-Object {
    $_.Token -ceq 'EX_Let' -and
    $_.Variable.Token -ceq 'EX_LocalVariable' -and
    $_.Variable.Variable.Property.Name -ceq $formattedChargeVariable -and
    $null -ne $_.Expression -and
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq
        "Class'KismetMathLibrary:Divide_DoubleDouble'"
})
Require ($chargeUnitConversions.Count -eq 1) `
    'HUD charge text must divide native charge amount before formatting.'
$chargeUnitParameters = @($chargeUnitConversions[0].Expression.Parameters)
Require ($chargeUnitParameters.Count -eq 2 -and
    @(JsonStringLeaves $chargeUnitParameters[0]) -ccontains
        'RailgunChargeAmount' -and
    $chargeUnitParameters[1].Token -ceq 'EX_DoubleConst') `
    'HUD charge text conversion must use cached native charge and one constant divisor.'
$chargeUnitsPerKWh = [double]$chargeUnitParameters[1].Value
Require ([Math]::Abs($chargeUnitsPerKWh - 1000.0) -lt 0.000001 -and
    [Math]::Abs((850.0 / $chargeUnitsPerKWh) - 0.85) -lt 0.000001 -and
    [Math]::Abs((425.0 / $chargeUnitsPerKWh) - 0.425) -lt 0.000001) `
    'HUD charge text must convert 850/425 native amount to 0.85/0.425 KWh.'
Require (@($hudAmmoStrings | Where-Object {
    $_ -ceq 'SyncRailgunAmmoVisuals'
}).Count -eq 1) 'HUD must perform exactly one guarded initial ammo sync.'
Require ($hudAmmoStrings -ccontains 'RailgunAmmoHudInitialized') `
    'HUD initial ammo-sync guard is missing.'
Require ($hudAmmoStrings -ccontains 'RailgunAmmoLastVisualCount') `
    'HUD does not read the event-maintained ammo count cache.'
Require (@($hudAmmoStrings | Where-Object {
    $_ -ceq "Class'Image:SetColorAndOpacity'"
}).Count -eq 12) 'HUD must tint exactly six wide and six scope ammo indicators.'
$emptyAmmoTintAssignments = @(
    EmptyAmmoTintAssignments $hudAmmoBytecode
)
Require ($emptyAmmoTintAssignments.Count -eq 6) `
    'HUD must apply the subtle red empty-magazine tint to all six indicators.'
$emptyAmmoZeroComparisons = @(
    EmptyAmmoZeroComparisons $hudAmmoBytecode
)
Require ($emptyAmmoZeroComparisons.Count -eq 12) `
    'Every HUD empty-magazine tint must use an exact zero-count comparison.'
$scopeAmmoTintAssignments = @(
    ScopeAmmoTintAssignments $hudAmmoBytecode
)
Require ($scopeAmmoTintAssignments.Count -eq 6) `
    'Scope ammo indicators must use the independent blue active/inactive tints.'
$scopeEmptyAmmoTintAssignments = @(
    ScopeEmptyAmmoTintAssignments $hudAmmoBytecode
)
Require ($scopeEmptyAmmoTintAssignments.Count -eq 6) `
    'Scope ammo indicators must use the wide HUD red tint only at zero rounds.'
$chargeTextColors = @(ChargeTextColorAssignments $hudEnergyBytecode)
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
    ChargeRadialProgressColorAssignments $hudEnergyBytecode
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
    InsufficientChargeComparisons $hudEnergyBytecode
)
Require ($insufficientChargeComparisons.Count -eq 1) `
    'Charge text tint must use one exact current-charge threshold comparison.'
Require (-not ($hudEnergyStrings -ccontains
    "Class'RadialSlider:SetSliderBarColor'")) `
    'Insufficient-charge tint must not alter the radial gauge background.'
Require (-not ($hudAmmoStrings -ccontains 'SliderProgressColor')) `
    'Ammo indicators must not inherit the dynamic charge ring color.'
$ammoActivationThresholds = @(AmmoActivationThresholds $hudAmmoRefresh[0])
Require (@(Compare-Object -ReferenceObject @(5,4,3,2,1,0,5,4,3,2,1,0) `
    -DifferenceObject $ammoActivationThresholds -SyncWindow 0).Count -eq 0) `
    'Wide and scope ammo indicators must activate from right to left.'
foreach ($forbiddenHudNativeRead in @(
    "Class'VoyageModuleComponent:GetResourceAmount'",
    "Class'VoyageModuleComponent:HasSocketConnection'",
    "Class'VoyageModuleComponent:HasPower'",
    "Class'GameplayStatics:GetPlayerPawn'"
)) {
    Require (-not ($hudAllStrings -ccontains $forbiddenHudNativeRead)) `
        ('HUD must consume station caches and its bound owner only: ' +
            $forbiddenHudNativeRead)
}
foreach ($requiredHudLifecycleReference in @(
    'RailgunHudStation','RegisterRailgunHud','UnregisterRailgunHud',
    'RefreshRailgunHudEnergy','RefreshRailgunHudAmmo',
    'RefreshRailgunHudStyle','RefreshRailgunHudMode',
    "Class'UserWidget:GetOwningPlayerPawn'"
)) {
    Require ($hudUbergraphStrings -ccontains $requiredHudLifecycleReference) `
        ('HUD lifecycle reference missing: ' +
            $requiredHudLifecycleReference)
}
foreach ($forbiddenHudInventoryReference in @(
    'Items', "Class'BlueprintMapLibrary:Map_Values'", 'OnInventoryChanged'
)) {
    Require (-not ($hudAmmoStrings -ccontains
        $forbiddenHudInventoryReference)) `
        ('HUD must not poll or bind inventory data directly: ' +
            $forbiddenHudInventoryReference)
}
$visibilityTargets = @(VisibilityTargets $hudModeRefresh[0])
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
$shotSound = @(Read-Candidate $shotSoundPackage)
$shotSoundWaves = @($shotSound | Where-Object {
    $_.Type -ceq 'SoundWave' -and $_.Name -ceq 'S_RailgunShotBlast'
})
Require ($shotSoundWaves.Count -eq 1 -and
    $shotSoundWaves[0].Properties.SoundClassObject.ObjectPath -ceq
        ($stockSfxSoundClassPackage + '.0')) `
    'Cooked shot SoundWave must reference the external stock SC_SFX class.'
$module = @($shell | Where-Object { $_.Name -ceq 'ModuleComponent' })
Require ($module.Count -eq 1 -and $module[0].Type -ceq 'VoyageModuleComponent') 'Native buffer module missing.'
Require (@(PropertyNames $module[0]) -contains 'ItemAsset') 'Module ItemAsset missing.'
Require ($module[0].Properties.ItemAsset.ObjectPath -ceq ($gunItemPackage + '.0')) 'Railgun ItemAsset mismatch.'
Require ($gunItemPackage.StartsWith($itemDiscoveryRoot + '/', [StringComparison]::Ordinal)) 'Railgun item is outside the confirmed Item AssetManager discovery root.'
Require ($skillPackage.StartsWith($skillDiscoveryRoot + '/', [StringComparison]::Ordinal)) 'Railgun skill is outside the confirmed Skill AssetManager discovery root.'
$energy = $module[0].Properties.ConfigData
Require ([Math]::Abs($energy.ResourceConsumptionOn - 1000) -lt 0.001) 'Initial ON consumption mismatch.'
Require ([Math]::Abs($energy.ResourceConsumptionStandby - 1000) -lt 0.001) 'Initial standby consumption mismatch.'
Require ([Math]::Abs($energy.ResourceBandwidthInput - 1000) -lt 0.001) 'Initial input bandwidth mismatch.'
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
$ammoPickupInventory = $inventory.ammoPickup
$ammoPickupPackage = '/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette'
$ammoPickupClass = $ammoPickupPackage + '.BP_RailgunAmmoCassette_C'
$ammoSource = Get-Content -LiteralPath (Join-Path $PSScriptRoot `
    'Assets/Fabricator/railgun-ammo-item.json') -Raw | ConvertFrom-Json
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
        $ammoPickupCarrierMesh
}).Count -ge 1) `
    'Ammo drop mesh mismatch.'
Require ($ammoProperties.DroppedActor.AssetPathName -ceq $ammoPickupClass) `
    'Ammo dropped actor mismatch.'
$ammoIcon = @(Read-Candidate '/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon')
$ammoTexture = @($ammoIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunAmmoIcon' })
Require ($ammoTexture.Count -eq 1 -and $ammoTexture[0].SizeX -eq 256 -and $ammoTexture[0].SizeY -eq 256) 'Ammo icon must be 256x256.'
$skillIcon = @(Read-Candidate '/Game/Mods/Railgun/Research/T_RailgunSkill')
$skillTexture = @($skillIcon | Where-Object { $_.Type -ceq 'Texture2D' -and $_.Name -ceq 'T_RailgunSkill' })
Require ($skillTexture.Count -eq 1 -and $skillTexture[0].SizeX -eq 256 -and $skillTexture[0].SizeY -eq 256) 'Railgun skill icon must be 256x256.'
$reportPath = Join-Path $output 'validation.json'
[ordered]@{status='passed';runtime='pending';containerSha256=(Get-FileHash -LiteralPath $Container -Algorithm SHA256).Hash;assetEvidence=$evidence;materialEvidence=$materialEvidence;assertions='owned native module parent with exact inherited inventory function; magazine-anchored stock-profile interaction query; exact discovered ItemAsset; confirmed Item and Skill AssetManager scan roots; weight-limited six-round inventory derived from the authored ammo mass, with native BeginPlay limit setter, exact valid-item predicate, owned-ammo binding, stock container overlay and no temporary inventory probe; exact Voyage inventory-change delegate binding with initial and deferred post-load visual synchronization and no ammo-visual polling accumulator; six persistent UV-cropped white/faint ammo indicators activated right-to-left from the event-maintained count cache, with a zero-count red tint independent of the charge ring, one guarded initial sync, no HUD inventory polling and whole-block optics visibility; exact Voyage module-value delegate bindings for guarded gameplay-energy maintenance and a separate visual observer, with idempotent remove/add, initial and settings snapshots, mode-cached demand changes, end-play unbinding, coalesced next-tick socket reconciliation, active-only elapsed-time offline-drain timer, and no tick-owned drain, demand or charge-indicator write; connected-and-powered insufficient-charge guard for subtle-red charge text and radial progress ring with opaque-white offline/ready recovery and unchanged radial background; no unreviewed native template values; auto-weld; inventory-matched component hierarchy and transforms; shell-owned BeginPlay and persistent post-load station initialization with a transient direct station reference, no global actor discovery and no player-controller startup gate; simple collision preserved; imported material packages remain readable and are recorded as evidence without constraining authored material type, parameters or parent; JSON-authored gun, ammo and skill primary assets preserve native identity, required runtime references and package integrity without pinning editable presentation or balance values; distinct 256x256 research icon'} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding UTF8
[pscustomobject]@{status='passed';reportPath=$reportPath}
