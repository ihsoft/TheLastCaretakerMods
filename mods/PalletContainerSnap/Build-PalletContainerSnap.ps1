[CmdletBinding()]
param(
    [string]$OutputRoot,
    [ValidateSet('OwnedCanary','RuntimeInjection','StockLargeRoute','StockLargePriority')][string]$Scope = 'OwnedCanary',
    [string]$ArtifactVersion,
    [ValidateSet('AutoLoader','Dml')][string]$ActivationMode = 'Dml',
    [switch]$IgnorePinnedGameVersion
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($Scope -ceq 'StockLargePriority') {
    if ([string]::IsNullOrWhiteSpace($ArtifactVersion)) {
        $ArtifactVersion = '0.8-canary-stock-large-priority'
    }
    & (Join-Path $PSScriptRoot 'Registry/Build-PalletContainerSnapStockLargePriority.ps1') `
        -OutputRoot $OutputRoot -ArtifactVersion $ArtifactVersion `
        -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
    return
}

if ($Scope -ceq 'StockLargeRoute') {
    throw 'Legacy unsuffixed StockLargeRoute builds are disabled; use StockLargePriority so stock overrides use the required _P stem.'
}
$runtimeInjection = $Scope -ceq 'RuntimeInjection'
$dmlActivation = $runtimeInjection -and $ActivationMode -ceq 'Dml'
$supportedArtifactVersionForScope = if ($runtimeInjection) {
    '0.14-canary-seed-item-name'
} else {
    '0.6-canary-four-slot-snap-mode'
}
if (-not [string]::IsNullOrWhiteSpace($ArtifactVersion) -and
    $ArtifactVersion -cne $supportedArtifactVersionForScope) {
    throw "$Scope scope only supports artifact version $supportedArtifactVersionForScope."
}
if ($runtimeInjection -and -not $dmlActivation) {
    throw 'The 0.14 RuntimeInjection candidate supports only the reviewed DML LogicMods activation layout.'
}

$modRoot = $PSScriptRoot
$repo = (Resolve-Path -LiteralPath (Join-Path $modRoot '../..')).Path
$ownerTmp = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/PalletContainerSnap'))
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $ownerTmp ('build-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
}
$output = [IO.Path]::GetFullPath($OutputRoot)
if (-not ($output + [IO.Path]::DirectorySeparatorChar).StartsWith(
        ($ownerTmp + [IO.Path]::DirectorySeparatorChar), [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must be a fresh directory below $ownerTmp"
}
if (Test-Path -LiteralPath $output) { throw 'Use a fresh OutputRoot identity.' }
[IO.Directory]::CreateDirectory($output) | Out-Null

$engine = 'K:\Epic Games\UE_5.8\Engine'
$project = Join-Path $modRoot 'Voyage.uproject'
$build = Join-Path $engine 'Build/BatchFiles/Build.bat'
$editor = Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
$unrealPak = Join-Path $engine 'Binaries/Win64/UnrealPak.exe'
$engineVersionPath = Join-Path $engine 'Build/Build.version'
$contractPath = Join-Path $modRoot 'Contracts/pallet-container-snap-contract.json'
$descriptorSource = Join-Path $modRoot $(if ($runtimeInjection) {
    'Registry/PalletContainerSnapCatalogue.runtime-injection.uplugin'
} else {
    'Registry/PalletContainerSnapCatalogue.uplugin'
})
$registryPolicyPath = Join-Path $modRoot 'Registry/pallet-container-snap-registry-policy.json'
$registryHelperPath = Join-Path $modRoot 'Registry/PalletContainerSnapRegistry.ps1'
$retoc = Join-Path $repo '.tools/bin/retoc.exe'
$uassetGui = Join-Path $repo '.tools/bin/UAssetGUI.exe'
$gameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
$globalUtoc = Join-Path $gameRoot 'Voyage/Content/Paks/global.utoc'
$supportedSlots = @(
    [double[]]@(60.6917798519,57.0609331131,50.9266495705),
    [double[]]@(60.6917798519,-59.5599830151,50.9266495705),
    [double[]]@(-59.2448532581,-59.5599830151,50.9266495705),
    [double[]]@(-59.2448532581,57.0609331131,50.9266495705)
)
$supportedRotation = [double[]]@(0.0,0.0,0.0)
$supportedComponent = '/Game/Data/Assets/Parts/DA_Part_AlloyFrame'
$supportedComponentCount = 1
$supportedCraftFilter = 9
$supportedCraftTime = 1.0
$supportedCraftElectricityCost = 0.0
$supportedSkillRequirement = 11
$supportedSkillCost = 0
$supportedItemObjectPath = '/Game/Data/Assets/Items/PalletContainerSnap/DA_Item_PalletSnapCanary.DA_Item_PalletSnapCanary'
$supportedArtifactVersion = $supportedArtifactVersionForScope
$supportedArchiveName = 'PalletContainerSnap_' + $supportedArtifactVersion + '.zip'
$supportedContainerStem = if ($runtimeInjection) { 'PalletContainerSnap_P' } else { 'PalletContainerSnap' }
$runtimeInjectionActorPackage = '/Game/Mods/PalletContainerSnap/ModActor'
$runtimeInjectionActorClass = '/Game/Mods/PalletContainerSnap/ModActor.ModActor_C'
$runtimeInjectionTarget = '/Game/Data/Assets/Items/DA_Item_PlayerBoxLarge.DA_Item_PlayerBoxLarge'
$seedGeneratorPackage = '/Game/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_HumanSeedPalletSnap'
$seedGeneratorClass = '/Game/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_HumanSeedPalletSnap.BP_CraftingLocationGenerator_HumanSeedPalletSnap_C'
$seedRuntimeInjectionTarget = '/Game/Data/Assets/Consumables/DA_HumanSeed.DA_HumanSeed'
$seedSpawnActorPackage = '/Game/Mods/PalletContainerSnap/BP_Item_HumanSeed_PlaceDiagnostic'
$seedSpawnActorClass = '/Game/Mods/PalletContainerSnap/BP_Item_HumanSeed_PlaceDiagnostic.BP_Item_HumanSeed_PlaceDiagnostic_C'
$seedSpawnActorParentClass = '/Game/Blueprints/Items/HumanGrow/BP_Item_HumanSeed.BP_Item_HumanSeed_C'
$seedSurfaceTraceHalfLength = 100.0
$seedSpawnTrigger = 'F6 Pressed'
$seedSpawnForwardOffset = 200.0
$seedSpawnUpOffset = 100.0
$seedSpawnCollisionHandling = 'AdjustIfPossibleButDontSpawnIfColliding'
$seedInspectTrigger = 'F7 Pressed'
$seedPlaceTrigger = 'F9 Pressed'
$seedPlaceOwner = '/Game/Blueprints/BP_FirstPersonCharacter_New.BP_FirstPersonCharacter_New_C'
$seedGetItemOwner = '/Script/Voyage.VoyageItemInterface'
$injectionActivatedMarker = 'PalletContainerSnap injection actor activated'
$injectionDispatchedMarker = 'PalletContainerSnap generator assignment dispatched'
$injectionCreateFailedMarker = 'PalletContainerSnap generator creation failed'
$generatorSelectedMarker = 'PalletContainerSnap owned generator selected'
$seedBoolDispatchedMarker = 'PalletContainerSnap Human Seed placement enable dispatched'
$seedInjectionDispatchedMarker = 'PalletContainerSnap Human Seed generator assignment dispatched'
$seedInjectionCreateFailedMarker = 'PalletContainerSnap Human Seed generator creation failed'
$seedGeneratorSelectedMarker = 'PalletContainerSnap Human Seed generator selected'
$seedSurfaceFoundMarker = 'PalletContainerSnap Human Seed pallet surface trace succeeded'
$seedSurfaceMissingMarker = 'PalletContainerSnap Human Seed pallet surface trace failed'
$spawnInputControllerMissingMarker = 'PalletContainerSnap Human Seed spawn input unavailable: no player controller'
$spawnPawnMissingMarker = 'PalletContainerSnap Human Seed spawn failed: no player pawn'
$spawnSucceededMarker = 'PalletContainerSnap Human Seed spawned'
$spawnFailedMarker = 'PalletContainerSnap Human Seed spawn failed: collision or initialization'
$supportedStockSkill = '/Game/Data/Assets/Skill/Utility/Containers/DA_Skill_Engineering_FortiHold_Storage_Container'
$supportedStockSkillJsonSha256 = '8486ED1F137F83528D8B3B680C80BCCFEBF83BF5DD75DDD52A6B200D5514ECBC'
$accepted011LargeGeneratorJsonSha256 = '1B6CD1D8F9EFA4D697E4A1FB38395CEF5F7CFC1995B484B355C46113047B57AD'
$accepted011SeedGeneratorJsonSha256 = '6350D08F0819CBCCA86215E2C2DAE112446A09BC8B230E7D400E6A3357F5B2E9'

function Write-Utf8Json([string]$Path, [object]$Value) {
    [IO.File]::WriteAllText(
        $Path, (($Value | ConvertTo-Json -Depth 12) + [Environment]::NewLine),
        (New-Object Text.UTF8Encoding($false)))
}

function Invoke-NativeStage([string]$Name, [string]$Executable, [string[]]$Arguments, [int]$TimeoutSeconds = 900) {
    if ([IO.Path]::GetExtension($Executable) -ieq '.bat') {
        $batchLog = Join-Path $output ($Name + '.batch.log')
        & $Executable @Arguments *> $batchLog
        if ($LASTEXITCODE -ne 0) { throw "$Name failed ($LASTEXITCODE); log: $batchLog" }
        return [pscustomobject]@{ status = 'passed'; reportPath = $batchLog }
    }
    $run = & (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') `
        -Executable $Executable -Arguments $Arguments -TimeoutSeconds $TimeoutSeconds `
        -MemoryLimitMB 10240 -OutputRoot (Join-Path $output ($Name + '-runner'))
    if ($run.status -cne 'passed') { throw "$Name failed; report: $($run.reportPath)" }
    return $run
}

function Get-SourceSnapshot {
    $roots = @(
        (Join-Path $modRoot '.gitignore'),
        (Join-Path $modRoot 'AGENTS.md'),
        (Join-Path $modRoot 'BACKLOG.md'),
        (Join-Path $modRoot 'README.md'),
        (Join-Path $modRoot 'Build-PalletContainerSnap.ps1'),
        (Join-Path $modRoot 'Voyage.uproject'),
        (Join-Path $modRoot 'Contracts'),
        (Join-Path $modRoot 'Registry'),
        (Join-Path $modRoot 'Source')
    )
    $files = @()
    foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root -PathType Leaf) { $files += Get-Item -LiteralPath $root }
        elseif (Test-Path -LiteralPath $root -PathType Container) { $files += Get-ChildItem -LiteralPath $root -File -Recurse }
    }
    return @($files | Sort-Object FullName | ForEach-Object {
        [pscustomobject]@{
            path = $_.FullName.Substring($repo.Length + 1).Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    })
}

function Find-TokenValue([object]$Value, [string]$Token) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [ValueType]) { return }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [Management.Automation.PSCustomObject])) {
        foreach ($entry in $Value) { Find-TokenValue $entry $Token }
        return
    }
    $tokenProperty = $Value.PSObject.Properties['Token']
    $valueProperty = $Value.PSObject.Properties['Value']
    if ($null -ne $tokenProperty -and $tokenProperty.Value -ceq $Token -and $null -ne $valueProperty) {
        Write-Output $valueProperty.Value
    }
    foreach ($property in $Value.PSObject.Properties) {
        if ($property.Name -cne 'Token' -and $property.Name -cne 'Value') {
            Find-TokenValue $property.Value $Token
        }
    }
}

function Find-NodesWithPropertyValue([object]$Value, [string]$PropertyName, [object]$Expected) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [ValueType]) { return }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [Management.Automation.PSCustomObject])) {
        foreach ($entry in $Value) { Find-NodesWithPropertyValue $entry $PropertyName $Expected }
        return
    }
    $candidate = $Value.PSObject.Properties[$PropertyName]
    if ($null -ne $candidate -and $candidate.Value -ceq $Expected) { Write-Output $Value }
    foreach ($property in $Value.PSObject.Properties) {
        Find-NodesWithPropertyValue $property.Value $PropertyName $Expected
    }
}

function Find-FunctionCallEnvelopes(
    [object]$Value,
    [string]$ObjectName,
    [string]$ObjectPath
) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [ValueType]) { return }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [Management.Automation.PSCustomObject])) {
        foreach ($entry in $Value) { Find-FunctionCallEnvelopes $entry $ObjectName $ObjectPath }
        return
    }
    $functionProperty = $Value.PSObject.Properties['Function']
    $parametersProperty = $Value.PSObject.Properties['Parameters']
    $functionObjectName = if ($null -ne $functionProperty -and $null -ne $functionProperty.Value) {
        $functionProperty.Value.PSObject.Properties['ObjectName']
    } else { $null }
    $functionObjectPath = if ($null -ne $functionProperty -and $null -ne $functionProperty.Value) {
        $functionProperty.Value.PSObject.Properties['ObjectPath']
    } else { $null }
    if ($null -ne $functionProperty -and $null -ne $functionProperty.Value -and
        $null -ne $parametersProperty -and $null -ne $functionObjectName -and
        $null -ne $functionObjectPath -and
        [string]$functionObjectName.Value -ceq $ObjectName -and
        [string]$functionObjectPath.Value -ceq $ObjectPath) {
        Write-Output $Value
    }
    foreach ($property in $Value.PSObject.Properties) {
        Find-FunctionCallEnvelopes $property.Value $ObjectName $ObjectPath
    }
}

function Find-FunctionCallAssignments(
    [object]$Value,
    [string]$ObjectName,
    [string]$ObjectPath
) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [ValueType]) { return }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [Management.Automation.PSCustomObject])) {
        foreach ($entry in $Value) { Find-FunctionCallAssignments $entry $ObjectName $ObjectPath }
        return
    }
    $expressionProperty = $Value.PSObject.Properties['Expression']
    if ($null -ne $expressionProperty -and $null -ne $expressionProperty.Value) {
        $expression = $expressionProperty.Value
        $functionProperty = $expression.PSObject.Properties['Function']
        if ($null -ne $functionProperty -and $null -ne $functionProperty.Value) {
            $nameProperty = $functionProperty.Value.PSObject.Properties['ObjectName']
            $pathProperty = $functionProperty.Value.PSObject.Properties['ObjectPath']
            if ($null -ne $nameProperty -and $null -ne $pathProperty -and
                [string]$nameProperty.Value -ceq $ObjectName -and
                [string]$pathProperty.Value -ceq $ObjectPath) {
                Write-Output $Value
            }
        }
    }
    foreach ($property in $Value.PSObject.Properties) {
        Find-FunctionCallAssignments $property.Value $ObjectName $ObjectPath
    }
}

function Find-ContextCallAssignments(
    [object]$Value,
    [string]$ContextToken,
    [string]$FunctionName
) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [ValueType]) { return }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [Management.Automation.PSCustomObject])) {
        foreach ($entry in $Value) { Find-ContextCallAssignments $entry $ContextToken $FunctionName }
        return
    }
    $expressionProperty = $Value.PSObject.Properties['Expression']
    $expressionToken = if ($null -ne $expressionProperty -and $null -ne $expressionProperty.Value) {
        $expressionProperty.Value.PSObject.Properties['Token']
    } else { $null }
    if ($null -ne $expressionToken -and [string]$expressionToken.Value -ceq 'EX_Context') {
        $contextProperty = $expressionProperty.Value.PSObject.Properties['ContextExpression']
        $contextTokenProperty = if ($null -ne $contextProperty -and $null -ne $contextProperty.Value) {
            $contextProperty.Value.PSObject.Properties['Token']
        } else { $null }
        $contextFunctionProperty = if ($null -ne $contextProperty -and $null -ne $contextProperty.Value) {
            $contextProperty.Value.PSObject.Properties['Function']
        } else { $null }
        if ($null -ne $contextTokenProperty -and $null -ne $contextFunctionProperty -and
            [string]$contextTokenProperty.Value -ceq $ContextToken -and
            [string]$contextFunctionProperty.Value -ceq $FunctionName) {
            Write-Output $Value
        }
    }
    foreach ($property in $Value.PSObject.Properties) {
        Find-ContextCallAssignments $property.Value $ContextToken $FunctionName
    }
}

function Find-PlacementComponentVirtualCalls([object]$Value, [string]$FunctionName) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [ValueType]) { return }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [Management.Automation.PSCustomObject])) {
        foreach ($entry in $Value) { Find-PlacementComponentVirtualCalls $entry $FunctionName }
        return
    }
    $token = $Value.PSObject.Properties['Token']
    $context = $Value.PSObject.Properties['ContextExpression']
    $object = $Value.PSObject.Properties['ObjectExpression']
    if ($null -ne $token -and [string]$token.Value -ceq 'EX_Context' -and
        $null -ne $context -and $null -ne $object) {
        $contextToken = $context.Value.PSObject.Properties['Token']
        $contextFunction = $context.Value.PSObject.Properties['Function']
        $objectToken = $object.Value.PSObject.Properties['Token']
        $objectVariable = $object.Value.PSObject.Properties['Variable']
        if ($null -ne $contextToken -and [string]$contextToken.Value -ceq 'EX_LocalVirtualFunction' -and
            $null -ne $contextFunction -and [string]$contextFunction.Value -ceq $FunctionName -and
            $null -ne $objectToken -and [string]$objectToken.Value -ceq 'EX_InstanceVariable' -and
            $null -ne $objectVariable) {
            $property = $objectVariable.Value.PSObject.Properties['Property']
            if ($null -ne $property -and [string]$property.Value.Name -ceq 'PlacementComponent' -and
                [string]$property.Value.PropertyClass.ObjectName -ceq "BlueprintGeneratedClass'BP_FabricationPlacementComponent_C'" -and
                [string]$property.Value.PropertyClass.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_FabricationPlacementComponent.0') {
                Write-Output $Value
            }
        }
    }
    foreach ($property in $Value.PSObject.Properties) {
        Find-PlacementComponentVirtualCalls $property.Value $FunctionName
    }
}

function Get-DirectLocalParameterNames([object]$Assignment) {
    return @($Assignment.Expression.Parameters | ForEach-Object {
        if ($_.Token -ceq 'EX_LocalVariable') { [string]$_.Variable.Property.Name }
        else { $null }
    })
}

function Get-ExpressionFunctionName([object]$Assignment) {
    $functionProperty = $Assignment.Expression.PSObject.Properties['Function']
    if ($null -eq $functionProperty -or $null -eq $functionProperty.Value) { return '' }
    $nameProperty = $functionProperty.Value.PSObject.Properties['ObjectName']
    if ($null -eq $nameProperty) { return '' }
    return [string]$nameProperty.Value
}

function Test-ExactSlotModeChain([object[]]$Assignments, [string]$Consumer) {
    $transforms = @($Assignments | Where-Object {
        (Get-ExpressionFunctionName $_) -ceq "Class'KismetMathLibrary:TransformLocation'"
    })
    $distances = @($Assignments | Where-Object {
        (Get-ExpressionFunctionName $_) -ceq "Class'KismetMathLibrary:Vector_DistanceSquared'"
    })
    $less = @($Assignments | Where-Object {
        (Get-ExpressionFunctionName $_) -ceq "Class'KismetMathLibrary:Less_DoubleDouble'"
    })
    $vectors = @($Assignments | Where-Object {
        (Get-ExpressionFunctionName $_) -ceq "Class'KismetMathLibrary:SelectVector'"
    })
    $floats = @($Assignments | Where-Object {
        (Get-ExpressionFunctionName $_) -ceq "Class'KismetMathLibrary:SelectFloat'"
    })
    $offsets = @($Assignments | Where-Object {
        $_.Expression.Token -ceq 'EX_LocalVirtualFunction' -and $_.Expression.Function -ceq 'CalculateOffset'
    })
    $modeComparisons = @($Assignments | Where-Object {
        (Get-ExpressionFunctionName $_) -ceq "Class'KismetMathLibrary:EqualEqual_DoubleDouble'" -and
        (($_.Expression | ConvertTo-Json -Depth 20 -Compress).Contains('AngularGranularity_Last')) -and
        @(Find-TokenValue $_.Expression 'EX_DoubleConst' | Where-Object { [double]$_ -eq 0.0 }).Count -eq 1
    })
    if ($transforms.Count -ne 4 -or $distances.Count -ne 4 -or $less.Count -ne 3 -or
        $vectors.Count -ne 4 -or $floats.Count -ne 2 -or $offsets.Count -ne 1 -or
        $modeComparisons.Count -ne 1) {
        throw "$Consumer slot-mode chain has the wrong operation multiplicity."
    }

    $slotNames = @()
    $distanceNames = @()
    for ($slotIndex = 0; $slotIndex -lt 4; $slotIndex++) {
        $slotMatch = @($transforms | Where-Object {
            $values = @(Find-TokenValue $_.Expression 'EX_VectorConst')
            @($values | Where-Object {
                [Math]::Abs([double]$_.X - $supportedSlots[$slotIndex][0]) -le 0.0001 -and
                [Math]::Abs([double]$_.Y - $supportedSlots[$slotIndex][1]) -le 0.0001 -and
                [Math]::Abs([double]$_.Z - $supportedSlots[$slotIndex][2]) -le 0.0001
            }).Count -eq 1
        })
        if ($slotMatch.Count -ne 1) { throw "$Consumer slot $slotIndex has no unique authored transform." }
        $slotName = [string]$slotMatch[0].Variable.Variable.Property.Name
        $distanceMatch = @($distances | Where-Object {
            $parameters = @(Get-DirectLocalParameterNames $_)
            $parameters.Count -eq 2 -and
            $parameters[0] -like 'CallFunc_BreakHitResult_ImpactPoint*' -and
            $parameters[1] -ceq $slotName
        })
        if ($distanceMatch.Count -ne 1) { throw "$Consumer slot $slotIndex distance lacks exact fresh-impact dataflow." }
        $slotNames += $slotName
        $distanceNames += [string]$distanceMatch[0].Variable.Variable.Property.Name
    }

    $modeName = [string]$modeComparisons[0].Variable.Variable.Property.Name
    $modeSelect = @($vectors | Where-Object {
        $parameters = @(Get-DirectLocalParameterNames $_)
        $parameters.Count -eq 3 -and $parameters[2] -ceq $modeName
    })
    if ($modeSelect.Count -ne 1) { throw "$Consumer has no unique AngularGranularity_Last mode select." }
    $offsetName = [string]$offsets[0].Variable.Variable.Property.Name
    $modeParameters = @(Get-DirectLocalParameterNames $modeSelect[0])
    if ($modeParameters[1] -cne $offsetName) { throw "$Consumer free path is not CalculateOffset." }
    $offsetText = $offsets[0].Expression | ConvertTo-Json -Depth 20 -Compress
    if (-not $offsetText.Contains('CallFunc_BreakHitResult_ImpactPoint') -or
        -not $offsetText.Contains('CallFunc_BreakHitResult_ImpactNormal') -or
        -not $offsetText.Contains('"Name":"Offset"')) {
        throw "$Consumer CalculateOffset inputs differ from fresh impact/native Offset."
    }

    $currentVector = $modeParameters[0]
    for ($slotIndex = 3; $slotIndex -ge 1; $slotIndex--) {
        $select = @($vectors | Where-Object { [string]$_.Variable.Variable.Property.Name -ceq $currentVector })
        if ($select.Count -ne 1) { throw "$Consumer strict selector $slotIndex is missing." }
        $selectParameters = @(Get-DirectLocalParameterNames $select[0])
        $condition = @($less | Where-Object {
            [string]$_.Variable.Variable.Property.Name -ceq $selectParameters[2]
        })
        if ($selectParameters.Count -ne 3 -or $selectParameters[0] -cne $slotNames[$slotIndex] -or
            $condition.Count -ne 1) {
            throw "$Consumer strict selector $slotIndex does not prefer only a closer authored slot."
        }
        $lessParameters = @(Get-DirectLocalParameterNames $condition[0])
        if ($lessParameters.Count -ne 2 -or $lessParameters[0] -cne $distanceNames[$slotIndex]) {
            throw "$Consumer strict distance comparison $slotIndex is not paired with its slot."
        }
        if ($slotIndex -eq 1) {
            if ($selectParameters[1] -cne $slotNames[0] -or $lessParameters[1] -cne $distanceNames[0]) {
                throw "$Consumer tie base does not preserve authored slot zero."
            }
        } else {
            $previousFloat = @($floats | Where-Object {
                [string]$_.Variable.Variable.Property.Name -ceq $lessParameters[1]
            })
            if ($previousFloat.Count -ne 1) { throw "$Consumer prior minimum $slotIndex is missing." }
            $floatParameters = @(Get-DirectLocalParameterNames $previousFloat[0])
            $previousSelect = @($vectors | Where-Object {
                [string]$_.Variable.Variable.Property.Name -ceq $selectParameters[1]
            })
            if ($floatParameters.Count -ne 3 -or $previousSelect.Count -ne 1 -or
                $floatParameters[0] -cne $distanceNames[$slotIndex - 1] -or
                $floatParameters[2] -cne (Get-DirectLocalParameterNames $previousSelect[0])[2]) {
                throw "$Consumer prior minimum and slot selector do not share the strict recurrence."
            }
            $currentVector = $selectParameters[1]
        }
    }
    return [pscustomobject]@{
        modeSelectName = [string]$modeSelect[0].Variable.Variable.Property.Name
        strictRootName = $modeParameters[0]
        calculateOffsetName = $offsetName
    }
}

$required = @($project,$build,$editor,$unrealPak,$engineVersionPath,$contractPath,$descriptorSource,$registryPolicyPath,$registryHelperPath,$retoc,$uassetGui,$globalUtoc)
foreach ($path in $required) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required input missing: $path" }
}

$contract = Get-Content -LiteralPath $contractPath -Raw | ConvertFrom-Json
$contractSlots = @($contract.authoredComposition.slots)
if ($contract.schemaVersion -ne 1 -or
    [string]$contract.game.steamBuildId -cne '25191271' -or
    [string]$contract.game.reviewedMappingEngineVersion -cne '5.8' -or
    [string]$contract.producer.engineVersion -cne '5.8.2' -or
    [string]$contract.producer.engineRoot -cne 'K:/Epic Games/UE_5.8' -or
    [string]$contract.parser.engineVersion -cne '5.8' -or
    [string]$contract.parser.uassetApiEngineVersion -cne 'VER_UE5_8' -or
    [string]$contract.parser.mappingSha256 -cne 'E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68' -or
    $contractSlots.Count -ne 4 -or
    [string]$contract.recipe.itemDisplayName -cne 'Pallet Snap Canary FortiHold' -or
    [string]$contract.recipe.skillDisplayName -cne 'Pallet Snap Canary Recipe' -or
    [string]$contract.recipe.component -cne $supportedComponent -or
    [int]$contract.recipe.componentCount -ne $supportedComponentCount -or
    [int]$contract.recipe.craftFilter -ne $supportedCraftFilter -or
    [double]$contract.recipe.craftTimeSeconds -ne $supportedCraftTime -or
    [double]$contract.recipe.craftElectricityCost -ne $supportedCraftElectricityCost -or
    [int]$contract.recipe.requirement -ne $supportedSkillRequirement -or
    [int]$contract.recipe.cost -ne $supportedSkillCost -or
    [string]$contract.recipe.stockReference.skill -cne $supportedStockSkill -or
    [string]$contract.recipe.stockReference.jsonSha256 -cne $supportedStockSkillJsonSha256 -or
    [string]$contract.recipe.stockReference.unlockMethod -cne 'Tier' -or
    [int]$contract.recipe.stockReference.requirement -ne $supportedSkillRequirement -or
    [string]$contract.recipe.stockReference.item -cne '/Game/Data/Assets/Items/DA_Item_PlayerBoxLarge' -or
    [string]$contract.release.artifactVersion -cne '0.6-canary-four-slot-snap-mode' -or
    [string]$contract.release.archiveName -cne 'PalletContainerSnap_0.6-canary-four-slot-snap-mode.zip' -or
    [string]$contract.validationExperiment.kind -cne 'diagnostic single-variable discriminator' -or
    [string]$contract.validationExperiment.guard -cne 'exact stock pallet attachment component owner' -or
    [string]$contract.validationExperiment.temporaryFlag -cne 'IgnoreNoFabrication' -or
    [bool]$contract.validationExperiment.temporaryValue -ne $true -or
    [string]$contract.validationExperiment.restore -cne 'snapshot before mutation and restore immediately after parent ValidateLocation' -or
    @($contract.validationExperiment.bypassedChecks).Count -ne 2 -or
    [string]$contract.validationExperiment.bypassedChecks[0] -cne 'target hierarchy NoFabrication without component AllowFabrication' -or
    [string]$contract.validationExperiment.bypassedChecks[1] -cne 'overlap hierarchy NoFabrication' -or
    @($contract.validationExperiment.preservedChecks).Count -ne 7 -or
    [string]$contract.validationExperiment.preservedChecks[0] -cne 'skeletal mesh overlap' -or
    [string]$contract.validationExperiment.preservedChecks[1] -cne 'cable overlap' -or
    [string]$contract.validationExperiment.preservedChecks[2] -cne 'collision object and response filtering' -or
    [string]$contract.validationExperiment.preservedChecks[3] -cne 'physics collision' -or
    [string]$contract.validationExperiment.preservedChecks[4] -cne 'bottom support' -or
    [string]$contract.validationExperiment.preservedChecks[5] -cne 'water' -or
    [string]$contract.validationExperiment.preservedChecks[6] -cne 'surface presence' -or
    [string]$contract.checkOriginExperiment.kind -cne 'diagnostic single-input correction' -or
    [string]$contract.checkOriginExperiment.guard -cne 'exact stock pallet hit actor' -or
    [string]$contract.checkOriginExperiment.formula -cne 'AuthoredSlotWorld + PalletActorWorldUpUnit * PlacementComponent.CheckOffset' -or
    [string]$contract.checkOriginExperiment.replaces -cne 'parent hover-derived CheckLocation minus PlacementLocation delta' -or
    [string]$contract.checkOriginExperiment.preserved -cne 'authored slot, rotation, attachment target, inherited support/overlap/water/surface checks and collision filters' -or
    [string]$contract.palletSupportExperiment.kind -cne 'exact-pallet placement support policy canary' -or
    [string]$contract.palletSupportExperiment.guard -cne 'blocking Out Hit on exact stock pallet with valid HitComponent reference-equal to AttachToComponent' -or
    [string]$contract.palletSupportExperiment.acceptedSupport -cne 'fresh blocking pallet target plus owned snapped-slot or hit-based free placement' -or
    [string]$contract.palletSupportExperiment.truePath -cne 'preserve the complete Setup-produced Out Hit without writes' -or
    [string]$contract.palletSupportExperiment.fallback -cne 'exact parent ValidateBottomAlignment' -or
    [string]$contract.palletSupportExperiment.preserved -cne 'overlap, water, surface policy, occupancy hypothesis, attachment target, collision filters and outside-pallet support validation' -or
    [string]$contract.placementModes.state -cne 'BP_FabricationPlacementComponent.AngularGranularity_Last Double equals 0.0' -or
    [string]$contract.placementModes.enabled -cne 'nearest squared world distance from fresh Out Hit.ImpactPoint to four pallet-transformed authored slots; strict ties keep array order; pallet-relative identity rotation' -or
    [string]$contract.placementModes.disabled -cne 'CalculateOffset(Out Hit.ImpactPoint, Out Hit.ImpactNormal, Offset) without TransformToGridFunc positional output; preserve parent PlacementRotation' -or
    [string]$contract.placementModes.checkLocation -cne 'selected world placement plus pallet unit world-up times live PlacementComponent.CheckOffset' -or
    [string]$contract.placementModes.occupancy -cne 'nearest slot is selected regardless occupied; inherited local physical overlap decides rejection' -or
    [string]$contract.humanSeedRuntimeExperiment.artifactVersion -cne '0.14-canary-seed-item-name' -or
    [string]$contract.humanSeedRuntimeExperiment.item -cne '/Game/Data/Assets/Consumables/DA_HumanSeed' -or
    [string]$contract.humanSeedRuntimeExperiment.generator -cne $seedGeneratorPackage -or
    [double]$contract.humanSeedRuntimeExperiment.surfaceTraceHalfLengthCentimeters -ne $seedSurfaceTraceHalfLength -or
    [string]$contract.humanSeedRuntimeExperiment.testSpawn.actor -cne $seedSpawnActorPackage -or
    [string]$contract.humanSeedRuntimeExperiment.testSpawn.parentActor -cne '/Game/Blueprints/Items/HumanGrow/BP_Item_HumanSeed' -or
    [string]$contract.humanSeedRuntimeExperiment.testSpawn.interface -cne $seedGetItemOwner -or
    [string]$contract.humanSeedRuntimeExperiment.testSpawn.getItem -cne '/Game/Data/Assets/Consumables/DA_HumanSeed' -or
    [string]$contract.humanSeedRuntimeExperiment.testSpawn.trigger -cne $seedSpawnTrigger -or
    [double]$contract.humanSeedRuntimeExperiment.testSpawn.forwardOffsetCentimeters -ne $seedSpawnForwardOffset -or
    [double]$contract.humanSeedRuntimeExperiment.testSpawn.upOffsetCentimeters -ne $seedSpawnUpOffset -or
    [string]$contract.humanSeedRuntimeExperiment.testSpawn.collisionHandling -cne $seedSpawnCollisionHandling -or
    [string]$contract.humanSeedRuntimeExperiment.placeDiagnostic.inspectTrigger -cne $seedInspectTrigger -or
    [string]$contract.humanSeedRuntimeExperiment.placeDiagnostic.placeTrigger -cne $seedPlaceTrigger -or
    -not [string]$contract.humanSeedRuntimeExperiment.placeDiagnostic.placeTarget.StartsWith($seedPlaceOwner, [StringComparison]::Ordinal) -or
    [string]$contract.ownedRegistryMetadata.kind -cne 'user-approved owned canary metadata design' -or
    [string]$contract.ownedRegistryMetadata.policyPath -cne 'Registry/pallet-container-snap-registry-policy.json' -or
    [string]$contract.ownedRegistryMetadata.itemObjectPath -cne $supportedItemObjectPath -or
    [bool]$contract.ownedRegistryMetadata.bIsDestructible -ne $false -or
    [bool]$contract.ownedRegistryMetadata.bIsRepairable -ne $false -or
    [string]$contract.ownedRegistryMetadata.quality -cne 'Poor' -or
    [string]$contract.ownedRegistryMetadata.tag -cne 'None' -or
    [string]$contract.runtimeStatus -cne 'not-run') {
    throw 'Unsupported PalletContainerSnap source contract.'
}
$descriptorContract = Get-Content -LiteralPath $descriptorSource -Raw | ConvertFrom-Json
for ($slotIndex = 0; $slotIndex -lt 4; $slotIndex++) {
    $contractSlot = [double[]]@($contractSlots[$slotIndex].palletLocalTranslationCentimeters)
    $contractRotation = [double[]]@($contractSlots[$slotIndex].palletLocalRotationDegrees)
    if ($contractSlot.Count -ne 3 -or $contractRotation.Count -ne 3) {
        throw "Authored slot $slotIndex shape is invalid."
    }
    for ($axis = 0; $axis -lt 3; $axis++) {
        if ([Math]::Abs($contractSlot[$axis] - $supportedSlots[$slotIndex][$axis]) -gt 0.0000001 -or
            [Math]::Abs($contractRotation[$axis] - $supportedRotation[$axis]) -gt 0.0000001) {
            throw "Authored slot $slotIndex does not match the supported transform."
        }
    }
}
if ([int]$descriptorContract.Version -ne $(if ($runtimeInjection) { 14 } else { 6 }) -or
    [string]$descriptorContract.VersionName -cne $supportedArtifactVersion) {
    throw 'Content plugin descriptor does not match the release identity contract.'
}
$fingerprint = & (Join-Path $repo 'tools/Get-VoyageBuildFingerprint.ps1') | ConvertFrom-Json
$gameBuild = & (Join-Path $repo 'tools/Assert-VoyageModGameBuild.ps1') `
    -ModRoot $modRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) `
    -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
$mapping = & (Join-Path $repo 'tools/Get-VoyageMappings.ps1')
if ([string]$mapping.steamBuildId -cne [string]$fingerprint.steam.buildId -or
    [string]$mapping.engineVersion -cne [string]$contract.parser.engineVersion -or
    [string]$mapping.uassetApiEngineVersion -cne [string]$contract.parser.uassetApiEngineVersion) {
    throw 'Reviewed mapping does not match the installed game or parser profile.'
}
$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
$actualProducerEngine = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
if ($actualProducerEngine -cne [string]$contract.producer.engineVersion) {
    throw "Producer requires UE $($contract.producer.engineVersion); found $actualProducerEngine"
}
$authoredGlb = [IO.Path]::GetFullPath([string]$contract.authoredComposition.path)
if (-not (Test-Path -LiteralPath $authoredGlb -PathType Leaf) -or
    (Get-FileHash -LiteralPath $authoredGlb -Algorithm SHA256).Hash -cne [string]$contract.authoredComposition.sha256) {
    throw 'Authored composition is missing or changed.'
}
$sourceBefore = @(Get-SourceSnapshot)

$projectRootBoundary = [IO.Path]::GetFullPath($modRoot) + [IO.Path]::DirectorySeparatorChar
foreach ($resetPath in @((Join-Path $modRoot 'Content'), (Join-Path $modRoot 'Saved/Cooked/Windows'))) {
    $resolvedReset = [IO.Path]::GetFullPath($resetPath)
    if (-not $resolvedReset.StartsWith($projectRootBoundary, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing reset outside mod root: $resolvedReset"
    }
    if (Test-Path -LiteralPath $resolvedReset) { Remove-Item -LiteralPath $resolvedReset -Recurse -Force }
}

$buildRun = Invoke-NativeStage 'build' $build @('VoyageEditor','Win64','Development',$project,'-WaitMutex','-NoHotReloadFromIDE')
$generationReport = Join-Path $output 'generation-report.json'
$generationArguments = @($project,'-run=GeneratePalletContainerSnap',('-Report=' + $generationReport))
if ($runtimeInjection) { $generationArguments += '-RuntimeInjection' }
$generationArguments += @('-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'generate-unreal.log')))
$generateRun = Invoke-NativeStage 'generate' $editor $generationArguments
$generated = Get-Content -LiteralPath $generationReport -Raw | ConvertFrom-Json
$generatedSlots = @($generated.slotsCentimeters)
if ([string]$generated.status -cne 'generated' -or $generatedSlots.Count -ne 4 -or
    [string]$generated.skillUnlockMethod -cne 'Tier' -or
    [int]$generated.skillRequirement -ne $supportedSkillRequirement -or
    [int]$generated.skillCost -ne $supportedSkillCost -or
    [string]$generated.calculateOffsetParentOwner -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.BP_CraftingLocationGenerator_Default_C' -or
    [string]$generated.calculateOffsetSignature -cne 'Vector Point, Vector Normal, Double Offset -> Vector' -or
    [string]$generated.validateOwner -cne '/Game/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_PalletContainerSnap.BP_CraftingLocationGenerator_PalletContainerSnap_C' -or
    [string]$generated.validateParentCallOwner -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.BP_CraftingLocationGenerator_Default_C' -or
    [int]$generated.validateParentCallCount -ne 2 -or
    [string]$generated.validateBottomOwner -cne '/Game/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_PalletContainerSnap.BP_CraftingLocationGenerator_PalletContainerSnap_C' -or
    [string]$generated.validateBottomParentCallOwner -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.BP_CraftingLocationGenerator_Default_C' -or
    [int]$generated.validateBottomParentCallCount -ne 1 -or
    [string]$generated.validationExperiment -cne 'pallet-only-temporary-IgnoreNoFabrication' -or
    [string]$generated.checkOriginExperiment -cne 'pallet-slot-plus-world-up-check-offset' -or
    [string]$generated.palletSupportExperiment -cne 'exact-pallet-authored-slot-support' -or
    [string]$generated.fourSlotMode -cne 'four-slot-nearest-impact-or-free-placement' -or
    [bool]$generated.runtimeInjection -ne $runtimeInjection -or
    ($runtimeInjection -and [string]$generated.runtimeInjectionActor -cne $runtimeInjectionActorClass) -or
    [string]$generated.runtimeInjectionTarget -cne $runtimeInjectionTarget -or
    ($runtimeInjection -and [string]$generated.seedGenerator -cne $seedGeneratorClass) -or
    ($runtimeInjection -and [string]$generated.seedSetupParentCallOwner -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.BP_CraftingLocationGenerator_Default_C') -or
    ($runtimeInjection -and [int]$generated.seedValidateParentCallCount -ne 2) -or
    ($runtimeInjection -and [int]$generated.seedValidateBottomParentCallCount -ne 1) -or
    ($runtimeInjection -and [string]$generated.seedRuntimeInjectionTarget -cne $seedRuntimeInjectionTarget) -or
    ($runtimeInjection -and [Math]::Abs([double]$generated.seedSurfaceTraceHalfLengthCentimeters - $seedSurfaceTraceHalfLength) -gt 0.0001) -or
    ($runtimeInjection -and [string]$generated.seedHeightOwner -cne '/Game/Blueprints/Fabrication/BP_FabricationPlacementComponent.BP_FabricationPlacementComponent_C') -or
    ($runtimeInjection -and [string]$generated.seedSpawnActor -cne $seedSpawnActorClass) -or
    ($runtimeInjection -and [string]$generated.seedSpawnTrigger -cne $seedSpawnTrigger) -or
    ($runtimeInjection -and [Math]::Abs([double]$generated.seedSpawnForwardOffsetCentimeters - $seedSpawnForwardOffset) -gt 0.0001) -or
    ($runtimeInjection -and [Math]::Abs([double]$generated.seedSpawnUpOffsetCentimeters - $seedSpawnUpOffset) -gt 0.0001) -or
    ($runtimeInjection -and [string]$generated.seedSpawnCollisionHandling -cne $seedSpawnCollisionHandling) -or
    [bool]$generated.stockPackagesCooked) {
    throw 'Generation report does not match the supported source contract.'
}
for ($slotIndex = 0; $slotIndex -lt 4; $slotIndex++) {
    $generatedSlot = [double[]]@($generatedSlots[$slotIndex])
    if ($generatedSlot.Count -ne 3) { throw "Generated slot $slotIndex shape is invalid." }
    for ($axis = 0; $axis -lt 3; $axis++) {
        if ([Math]::Abs($generatedSlot[$axis] - $supportedSlots[$slotIndex][$axis]) -gt 0.0001) {
            throw "Generated slot $slotIndex differs from the source contract."
        }
    }
}

$packages = @(
    '/Game/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_PalletContainerSnap',
    '/Game/Mods/PalletContainerSnap/BP_Inventory_PalletContainerSnap',
    '/Game/Data/Assets/Items/PalletContainerSnap/DA_Item_PalletSnapCanary',
    '/Game/Data/Assets/Skill/PalletContainerSnap/DA_Skill_PalletSnapCanary'
)
$relatives = @(
    'Voyage/Content/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_PalletContainerSnap',
    'Voyage/Content/Mods/PalletContainerSnap/BP_Inventory_PalletContainerSnap',
    'Voyage/Content/Data/Assets/Items/PalletContainerSnap/DA_Item_PalletSnapCanary',
    'Voyage/Content/Data/Assets/Skill/PalletContainerSnap/DA_Skill_PalletSnapCanary'
)
if ($runtimeInjection) {
    $packages += $seedGeneratorPackage
    $relatives += 'Voyage/Content/Mods/PalletContainerSnap/BP_CraftingLocationGenerator_HumanSeedPalletSnap'
    $packages += $runtimeInjectionActorPackage
    $relatives += 'Voyage/Content/Mods/PalletContainerSnap/ModActor'
    $packages += '/Game/Mods/PalletContainerSnap/WBP_PlaceDiagnostic'
    $relatives += 'Voyage/Content/Mods/PalletContainerSnap/WBP_PlaceDiagnostic'
    $packages += $seedSpawnActorPackage
    $relatives += 'Voyage/Content/Mods/PalletContainerSnap/BP_Item_HumanSeed_PlaceDiagnostic'
}
$projectCooked = Join-Path $modRoot 'Saved/Cooked/Windows'
$cooked = Join-Path $output 'cooked-accumulated'
for ($index = 0; $index -lt $packages.Count; $index++) {
    $null = Invoke-NativeStage ('cook-' + ($index + 1)) $editor @(
        $project,'-run=cook','-targetplatform=Windows','-SkipZenStore',
        '-CookSinglePackageNoRefs',('-Package=' + $packages[$index]),
        '-unattended','-nop4','-nosplash','-nullrhi',
        ('-abslog=' + (Join-Path $output ('cook-' + ($index + 1) + '-unreal.log'))))
    $destinationBase = Join-Path $cooked $relatives[$index]
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destinationBase)) | Out-Null
    foreach ($extension in @('.uasset','.uexp','.ubulk')) {
        $source = Join-Path $projectCooked ($relatives[$index] + $extension)
        if (Test-Path -LiteralPath $source -PathType Leaf) { Copy-Item -LiteralPath $source -Destination ($destinationBase + $extension) }
    }
    if (-not (Test-Path -LiteralPath ($destinationBase + '.uasset') -PathType Leaf)) {
        throw "Cooked package missing: $($relatives[$index])"
    }
}
$verifyTaggedRun = Invoke-NativeStage 'verify-tagged' $editor @(
    $project,'-run=GeneratePalletContainerSnap',('-VerifyCookedRoot=' + $cooked),
    '-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + (Join-Path $output 'verify-unreal.log')))

$loose = Join-Path $output 'loose'
foreach ($relative in $relatives) {
    $destinationBase = Join-Path $loose $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destinationBase)) | Out-Null
    foreach ($extension in @('.uasset','.uexp','.ubulk')) {
        $source = Join-Path $cooked ($relative + $extension)
        if (Test-Path -LiteralPath $source -PathType Leaf) { Copy-Item -LiteralPath $source -Destination ($destinationBase + $extension) }
    }
}
$scriptObjectsDirectory = Join-Path $output 'scriptobjects-only'
[IO.Directory]::CreateDirectory($scriptObjectsDirectory) | Out-Null
$scriptObjectsRun = Invoke-NativeStage 'extract-scriptobjects' $retoc @(
    'to-legacy','--version','UE5_8','--no-assets','--no-shaders',$globalUtoc,$scriptObjectsDirectory) 120
$scriptObjects = @(Get-ChildItem -LiteralPath $scriptObjectsDirectory -File -Recurse)
if ($scriptObjects.Count -ne 1 -or $scriptObjects[0].Name -cne 'scriptobjects.bin') {
    throw 'Metadata-only extraction did not produce exactly one scriptobjects.bin.'
}
Copy-Item -LiteralPath $scriptObjects[0].FullName -Destination (Join-Path $loose 'scriptobjects.bin')
if ($runtimeInjection) {
    $scriptObjectBytes = [IO.File]::ReadAllBytes($scriptObjects[0].FullName)
    $scriptObjectAscii = [Text.Encoding]::UTF8.GetString($scriptObjectBytes)
    $scriptObjectUnicode = [Text.Encoding]::Unicode.GetString($scriptObjectBytes)
    if ((-not $scriptObjectAscii.Contains('KismetSystemLibrary') -and
         -not $scriptObjectUnicode.Contains('KismetSystemLibrary')) -or
        (-not $scriptObjectAscii.Contains('SetObjectPropertyByName') -and
         -not $scriptObjectUnicode.Contains('SetObjectPropertyByName')) -or
        (-not $scriptObjectAscii.Contains('SetBoolPropertyByName') -and
         -not $scriptObjectUnicode.Contains('SetBoolPropertyByName')) -or
        (-not $scriptObjectAscii.Contains('K2_LineTraceComponent') -and
         -not $scriptObjectUnicode.Contains('K2_LineTraceComponent')) -or
        (-not $scriptObjectAscii.Contains('BeginDeferredActorSpawnFromClass') -and
         -not $scriptObjectUnicode.Contains('BeginDeferredActorSpawnFromClass')) -or
        (-not $scriptObjectAscii.Contains('FinishSpawningActor') -and
         -not $scriptObjectUnicode.Contains('FinishSpawningActor')) -or
        (-not $scriptObjectAscii.Contains('GetPlayerPawn') -and
         -not $scriptObjectUnicode.Contains('GetPlayerPawn')) -or
        (-not $scriptObjectAscii.Contains('GetPlayerController') -and
         -not $scriptObjectUnicode.Contains('GetPlayerController')) -or
        (-not $scriptObjectAscii.Contains('DoesImplementInterface') -and
         -not $scriptObjectUnicode.Contains('DoesImplementInterface')) -or
        (-not $scriptObjectAscii.Contains('GetPathName') -and
         -not $scriptObjectUnicode.Contains('GetPathName')) -or
        (-not $scriptObjectAscii.Contains('GetClassDisplayName') -and
         -not $scriptObjectUnicode.Contains('GetClassDisplayName')) -or
        (-not $scriptObjectAscii.Contains('VoyageItemInterface') -and
         -not $scriptObjectUnicode.Contains('VoyageItemInterface')) -or
        (-not $scriptObjectAscii.Contains('GetItem') -and
         -not $scriptObjectUnicode.Contains('GetItem')) -or
        (-not $scriptObjectAscii.Contains('WidgetBlueprintLibrary') -and
         -not $scriptObjectUnicode.Contains('WidgetBlueprintLibrary')) -or
        (-not $scriptObjectAscii.Contains('AddToViewport') -and
         -not $scriptObjectUnicode.Contains('AddToViewport')) -or
        (-not $scriptObjectAscii.Contains('SetText') -and
         -not $scriptObjectUnicode.Contains('SetText')) -or
        (-not $scriptObjectAscii.Contains('SetVisibility') -and
         -not $scriptObjectUnicode.Contains('SetVisibility'))) {
        throw 'Current shipping scriptobjects evidence lacks a required Engine setter, trace, input-owner, or spawn identity.'
    }
}

$releaseRoot = Join-Path $output 'release'
$payload = Join-Path $releaseRoot 'payload'
[IO.Directory]::CreateDirectory($payload) | Out-Null
$container = Join-Path $payload ($supportedContainerStem + '.utoc')
$packRun = Invoke-NativeStage 'pack-container' $retoc @('to-zen',$loose,$container,'--version','UE5_8') 120
$expected = Join-Path $output 'expected-packages.txt'
[IO.File]::WriteAllLines($expected, @($relatives | ForEach-Object { $_ + '.uasset' }))
$containerCheck = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') `
    -Container $container -ExpectedPackageList $expected -OutputRoot (Join-Path $output 'container-check')
if ($containerCheck.status -cne 'passed' -or -not [bool]$containerCheck.packageSetMatches -or
    $containerCheck.packageCount -ne $(if ($runtimeInjection) { 8 } else { 4 })) {
    throw 'Exact container package set verification failed.'
}

$readbacks = @()
for ($index = 0; $index -lt $relatives.Count; $index++) {
    $readbacks += ((& (Join-Path $repo 'tools/Get-VoyageAssetJson.ps1') `
        $relatives[$index] -Source Mod -ModContainer $container `
        -ModInspectionRoot (Join-Path $output ('readback-' + ($index + 1))) -AsJson) | ConvertFrom-Json)
}
$generatorJsonPath = [string]$readbacks[0].jsonPath
$containerJsonPath = [string]$readbacks[1].jsonPath
$itemJsonPath = [string]$readbacks[2].jsonPath
$skillJsonPath = [string]$readbacks[3].jsonPath
$seedGeneratorJsonPath = if ($runtimeInjection) { [string]$readbacks[4].jsonPath } else { $null }
$modActorJsonPath = if ($runtimeInjection) { [string]$readbacks[5].jsonPath } else { $null }
$diagnosticWidgetJsonPath = if ($runtimeInjection) { [string]$readbacks[6].jsonPath } else { $null }
$ownedSeedActorJsonPath = if ($runtimeInjection) { [string]$readbacks[7].jsonPath } else { $null }
$generatorJson = [object[]](Get-Content -LiteralPath $generatorJsonPath -Raw | ConvertFrom-Json)
$containerJson = [object[]](Get-Content -LiteralPath $containerJsonPath -Raw | ConvertFrom-Json)
$itemJson = [object[]](Get-Content -LiteralPath $itemJsonPath -Raw | ConvertFrom-Json)
$skillJson = [object[]](Get-Content -LiteralPath $skillJsonPath -Raw | ConvertFrom-Json)
$modActorJson = if ($runtimeInjection) {
    [object[]](Get-Content -LiteralPath $modActorJsonPath -Raw | ConvertFrom-Json)
} else { @() }
$seedGeneratorJson = if ($runtimeInjection) {
    [object[]](Get-Content -LiteralPath $seedGeneratorJsonPath -Raw | ConvertFrom-Json)
} else { @() }
$diagnosticWidgetJson = if ($runtimeInjection) {
    [object[]](Get-Content -LiteralPath $diagnosticWidgetJsonPath -Raw | ConvertFrom-Json)
} else { @() }
$ownedSeedActorJson = if ($runtimeInjection) {
    [object[]](Get-Content -LiteralPath $ownedSeedActorJsonPath -Raw | ConvertFrom-Json)
} else { @() }

$generatorClass = @($generatorJson | Where-Object { $_.Type -ceq 'BlueprintGeneratedClass' -and $_.Name -ceq 'BP_CraftingLocationGenerator_PalletContainerSnap_C' })
$setup = @($generatorJson | Where-Object { $_.Type -ceq 'Function' -and $_.Name -ceq 'SetupLocation' })
$validate = @($generatorJson | Where-Object { $_.Type -ceq 'Function' -and $_.Name -ceq 'ValidateLocation' })
$validateBottom = @($generatorJson | Where-Object { $_.Type -ceq 'Function' -and $_.Name -ceq 'ValidateBottomAlignment' })
$containerClass = @($containerJson | Where-Object { $_.Type -ceq 'BlueprintGeneratedClass' -and $_.Name -ceq 'BP_Inventory_PalletContainerSnap_C' })
$containerCdo = @($containerJson | Where-Object { $_.Name -ceq 'Default__BP_Inventory_PalletContainerSnap_C' })
$item = @($itemJson | Where-Object { $_.Type -ceq 'VoyageItem' -and $_.Name -ceq 'DA_Item_PalletSnapCanary' })
$generatorInstances = @($itemJson | Where-Object { $_.Type -ceq 'BP_CraftingLocationGenerator_PalletContainerSnap_C' })
$skill = @($skillJson | Where-Object { $_.Type -ceq 'VoyageSkill' -and $_.Name -ceq 'DA_Skill_PalletSnapCanary' })
$modActorClass = @($modActorJson | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass' -and $_.Name -ceq 'ModActor_C'
})
$modActorCdo = @($modActorJson | Where-Object { $_.Name -ceq 'Default__ModActor_C' })
$modActorUbergraph = @($modActorJson | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'ExecuteUbergraph_ModActor'
})
$modActorInputBinding = @($modActorJson | Where-Object {
    $_.Type -ceq 'InputKeyDelegateBinding' -and $_.Name -ceq 'InputKeyDelegateBinding_0'
})
$seedGeneratorClassRecord = @($seedGeneratorJson | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass' -and $_.Name -ceq 'BP_CraftingLocationGenerator_HumanSeedPalletSnap_C'
})
$seedSetup = @($seedGeneratorJson | Where-Object { $_.Type -ceq 'Function' -and $_.Name -ceq 'SetupLocation' })
$seedValidate = @($seedGeneratorJson | Where-Object { $_.Type -ceq 'Function' -and $_.Name -ceq 'ValidateLocation' })
$seedValidateBottom = @($seedGeneratorJson | Where-Object { $_.Type -ceq 'Function' -and $_.Name -ceq 'ValidateBottomAlignment' })
$diagnosticWidgetClass = @($diagnosticWidgetJson | Where-Object {
    $_.Type -ceq 'WidgetBlueprintGeneratedClass' -and $_.Name -ceq 'WBP_PlaceDiagnostic_C'
})
$ownedSeedActorClassRecord = @($ownedSeedActorJson | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass' -and $_.Name -ceq 'BP_Item_HumanSeed_PlaceDiagnostic_C'
})
$ownedSeedActorCdo = @($ownedSeedActorJson | Where-Object {
    $_.Name -ceq 'Default__BP_Item_HumanSeed_PlaceDiagnostic_C'
})
$ownedSeedGetItem = @($ownedSeedActorJson | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'GetItem'
})
$ownedSeedGetItemName = @($ownedSeedActorJson | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'GetItemName'
})
$ownedSeedActorText = $ownedSeedActorJson | ConvertTo-Json -Depth 100 -Compress
$ownedSeedInterfaces = @(if ($ownedSeedActorClassRecord.Count -eq 1) {
    @($ownedSeedActorClassRecord[0].Interfaces)
} else { @() })
$ownedSeedGetItemNameAssignments = @(if ($ownedSeedGetItemName.Count -eq 1) {
    @($ownedSeedGetItemName[0].ScriptBytecode | Where-Object {
        [string]$_.Token -ceq 'EX_Let' -and
        [string]$_.Variable.Variable.Property.Name -ceq 'ReturnValue' -and
        [string]$_.Variable.Variable.Property.Type -ceq 'TextProperty' -and
        [string]$_.Expression.Token -ceq 'EX_TextConst' -and
        [string]$_.Expression.Value.SourceString -ceq ''
    })
} else { @() })
$ownedSeedGetItemNameReturn = @(if ($ownedSeedGetItemName.Count -eq 1) {
    @($ownedSeedGetItemName[0].ChildProperties | Where-Object {
        [string]$_.Name -ceq 'ReturnValue' -and
        [string]$_.Type -ceq 'TextProperty' -and
        [string]$_.PropertyFlags -ceq 'Parm | OutParm | ReturnParm'
    })
} else { @() })
$ownedSeedGetItemAssignments = @(if ($ownedSeedGetItem.Count -eq 1) {
    @($ownedSeedGetItem[0].ScriptBytecode | Where-Object {
        [string]$_.Token -ceq 'EX_LetObj' -and
        [string]$_.Variable.Variable.Property.Name -ceq 'ReturnValue' -and
        [string]$_.Expression.Token -ceq 'EX_ObjectConst' -and
        [string]$_.Expression.Value.ObjectName -ceq "VoyageItemConsumable'DA_HumanSeed'" -and
        [string]$_.Expression.Value.ObjectPath -cne ''
    })
} else { @() })
$diagnosticText = @($diagnosticWidgetJson | Where-Object {
    $_.Type -ceq 'TextBlock' -and $_.Name -ceq 'DiagnosticText'
})
$setDiagnosticResult = @($diagnosticWidgetJson | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'SetDiagnosticResult'
})
$hideDiagnostic = @($diagnosticWidgetJson | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'HideDiagnostic'
})
if ($generatorClass.Count -ne 1 -or $setup.Count -ne 1 -or $validate.Count -ne 1 -or $validateBottom.Count -ne 1 -or $containerClass.Count -ne 1 -or
    $containerCdo.Count -ne 1 -or $item.Count -ne 1 -or $generatorInstances.Count -ne 1 -or $skill.Count -ne 1) {
    throw 'Cooked readback export multiplicity is invalid.'
}
if ($runtimeInjection -and
    ($modActorClass.Count -ne 1 -or $modActorCdo.Count -ne 1 -or $modActorUbergraph.Count -ne 1 -or
     $modActorInputBinding.Count -ne 1 -or
     $seedGeneratorClassRecord.Count -ne 1 -or $seedSetup.Count -ne 1 -or
     $seedValidate.Count -ne 1 -or $seedValidateBottom.Count -ne 1 -or
     $diagnosticWidgetClass.Count -ne 1 -or $diagnosticText.Count -ne 1 -or
     $setDiagnosticResult.Count -ne 1 -or $hideDiagnostic.Count -ne 1 -or
     $ownedSeedActorClassRecord.Count -ne 1 -or $ownedSeedActorCdo.Count -ne 1 -or
     $ownedSeedGetItem.Count -ne 1 -or $ownedSeedGetItemName.Count -ne 1)) {
    throw 'Runtime injection ModActor cooked readback multiplicity is invalid.'
}
$generatorRecord = $generatorClass[0]
$setupRecord = $setup[0]
$validateRecord = $validate[0]
$validateBottomRecord = $validateBottom[0]
$validateOutputProperties = @($validateRecord.ChildProperties | Where-Object {
    $_.Name -ceq 'Output_Get' -and $_.Type -ceq 'BoolProperty'
})
$containerRecord = $containerClass[0]
$containerCdoRecord = $containerCdo[0]
$itemRecord = $item[0]
$skillRecord = $skill[0]
$modActorText = if ($runtimeInjection) {
    Get-Content -LiteralPath $modActorJsonPath -Raw
} else { '' }
$generatorText = Get-Content -LiteralPath $generatorJsonPath -Raw
$seedGeneratorText = if ($runtimeInjection) {
    Get-Content -LiteralPath $seedGeneratorJsonPath -Raw
} else { '' }
if ($runtimeInjection -and
    ((Get-FileHash -LiteralPath $generatorJsonPath -Algorithm SHA256).Hash -cne $accepted011LargeGeneratorJsonSha256 -or
     (Get-FileHash -LiteralPath $seedGeneratorJsonPath -Algorithm SHA256).Hash -cne $accepted011SeedGeneratorJsonSha256)) {
    throw '0.14 diagnostic scope changed a canonical accepted 0.11 placement generator readback.'
}
if ($runtimeInjection) {
    if ([string]$ownedSeedActorClassRecord[0].Super.ObjectName -cne
            "BlueprintGeneratedClass'BP_Item_HumanSeed_C'" -or
        [string]$ownedSeedActorClassRecord[0].Super.ObjectPath -cne
            '/Game/Blueprints/Items/HumanGrow/BP_Item_HumanSeed.0' -or
        [string]$ownedSeedActorCdo[0].Template.ObjectName -cne
            "BP_Item_HumanSeed_C'Default__BP_Item_HumanSeed_C'" -or
        [string]$ownedSeedActorCdo[0].Template.ObjectPath -cne
            '/Game/Blueprints/Items/HumanGrow/BP_Item_HumanSeed.2' -or
        $ownedSeedActorCdo[0].PSObject.Properties['Properties'] -or
        $ownedSeedInterfaces.Count -ne 1 -or
        [string]$ownedSeedInterfaces[0].Class.ObjectName -cne "Class'VoyageItemInterface'" -or
        [string]$ownedSeedInterfaces[0].Class.ObjectPath -cne '/Script/Voyage' -or
        -not [bool]$ownedSeedInterfaces[0].bImplementedByK2 -or
        [string]$ownedSeedGetItem[0].Outer.ObjectName -cne
            "BlueprintGeneratedClass'BP_Item_HumanSeed_PlaceDiagnostic_C'" -or
        [string]$ownedSeedGetItem[0].Outer.ObjectPath -cne
            '/Game/Mods/PalletContainerSnap/BP_Item_HumanSeed_PlaceDiagnostic.0' -or
        $ownedSeedGetItemAssignments.Count -ne 1 -or
        [string]$ownedSeedGetItemAssignments[0].Expression.Value.ObjectPath -cne
            '/Game/Data/Assets/Consumables/DA_HumanSeed.0' -or
        [string]$ownedSeedGetItemName[0].Outer.ObjectName -cne
            "BlueprintGeneratedClass'BP_Item_HumanSeed_PlaceDiagnostic_C'" -or
        [string]$ownedSeedGetItemName[0].Outer.ObjectPath -cne
            '/Game/Mods/PalletContainerSnap/BP_Item_HumanSeed_PlaceDiagnostic.0' -or
        [string]$ownedSeedGetItemName[0].FunctionFlags -cne
            'FUNC_Event | FUNC_Public | FUNC_HasOutParms | FUNC_BlueprintCallable | FUNC_BlueprintEvent' -or
        $ownedSeedGetItemNameReturn.Count -ne 1 -or
        $ownedSeedGetItemNameAssignments.Count -ne 1 -or
        -not $ownedSeedActorText.Contains('DA_HumanSeed')) {
        throw 'Owned Human Seed child parent/interface/GetItem/GetItemName contract is invalid.'
    }
    $modActorScriptText = $modActorUbergraph[0].ScriptBytecode | ConvertTo-Json -Depth 30 -Compress
    $setterFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'KismetSystemLibrary:SetObjectPropertyByName'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $boolSetterFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'KismetSystemLibrary:SetBoolPropertyByName'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $spawnFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'GameplayStatics:SpawnObject'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $enableInputStatements = @($modActorUbergraph[0].ScriptBytecode | Where-Object {
        [string]$_.Token -ceq 'EX_VirtualFunction' -and [string]$_.Function -ceq 'EnableInput'
    })
    $getPlayerControllerFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'GameplayStatics:GetPlayerController'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $getPlayerPawnFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'GameplayStatics:GetPlayerPawn'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $beginActorSpawnFunctions = @(Find-FunctionCallEnvelopes $modActorUbergraph[0].ScriptBytecode `
        "Class'GameplayStatics:BeginDeferredActorSpawnFromClass'" '/Script/Engine')
    $finishActorSpawnFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'GameplayStatics:FinishSpawningActor'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $doesImplementInterfaceFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'KismetSystemLibrary:DoesImplementInterface'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $getPathNameFunctions = @(Find-FunctionCallAssignments $modActorUbergraph[0].ScriptBytecode `
        "Class'KismetSystemLibrary:GetPathName'" '/Script/Engine')
    $getClassDisplayNameFunctions = @(Find-FunctionCallAssignments $modActorUbergraph[0].ScriptBytecode `
        "Class'KismetSystemLibrary:GetClassDisplayName'" '/Script/Engine')
    $getItemMessages = @(Find-ContextCallAssignments $modActorUbergraph[0].ScriptBytecode `
        'EX_VirtualFunction' 'GetItem')
    $directPlaceCalls = @(Find-ContextCallAssignments $modActorUbergraph[0].ScriptBytecode `
        'EX_LocalVirtualFunction' 'Place')
    $getPlayerCharacterFunctions = @(Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode `
        'ObjectName' "Class'GameplayStatics:GetPlayerCharacter'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $beginActorSpawn = $beginActorSpawnFunctions[0]
    $spawnCollisionParameter = if ($null -ne $beginActorSpawn -and
        @($beginActorSpawn.Parameters).Count -eq 6) { $beginActorSpawn.Parameters[3] } else { $null }
    $spawnActorClassParameter = if ($null -ne $beginActorSpawn -and
        @($beginActorSpawn.Parameters).Count -eq 6) { $beginActorSpawn.Parameters[1] } else { $null }
    $getPathOutputNames = @($getPathNameFunctions | ForEach-Object {
        [string]$_.Variable.Variable.Property.Name
    } | Sort-Object -Unique)
    $getClassDisplayOutputNames = @($getClassDisplayNameFunctions | ForEach-Object {
        [string]$_.Variable.Variable.Property.Name
    } | Sort-Object -Unique)
    $getItemBranchContracts = @(
        [pscustomobject]@{
            result = 'CallFunc_GetItem_ReturnValue'
            interface = 'K2Node_DynamicCast_AsVoyage_Item_Interface'
            interfaceBool = 'K2Node_DynamicCast_bSuccess_1'
            implements = 'CallFunc_DoesImplementInterface_ReturnValue'
        },
        [pscustomobject]@{
            result = 'CallFunc_GetItem_ReturnValue_1'
            interface = 'K2Node_DynamicCast_AsVoyage_Item_Interface_1'
            interfaceBool = 'K2Node_DynamicCast_bSuccess_2'
            implements = 'CallFunc_DoesImplementInterface_ReturnValue_1'
        }
    )
    $getItemBranchEvidence = @($getItemBranchContracts | ForEach-Object {
        $branchContract = $_
        $message = @($getItemMessages | Where-Object {
            [string]$_.Variable.Variable.Property.Name -ceq $branchContract.result
        })
        if ($message.Count -ne 1) { return $false }
        $context = $message[0].Expression
        $interfaceProperty = $context.ObjectExpression.InterfaceValue.Variable.Property
        if ([string]$context.ObjectExpression.Token -cne 'EX_InterfaceContext' -or
            [string]$context.ObjectExpression.InterfaceValue.Token -cne 'EX_LocalVariable' -or
            [string]$interfaceProperty.Name -cne $branchContract.interface -or
            [string]$interfaceProperty.InterfaceClass.ObjectName -cne "Class'VoyageItemInterface'" -or
            [string]$interfaceProperty.InterfaceClass.ObjectPath -cne '/Script/Voyage' -or
            [string]$context.ContextExpression.Token -cne 'EX_VirtualFunction' -or
            [string]$context.ContextExpression.Function -cne 'GetItem' -or
            @($context.ContextExpression.Parameters).Count -ne 0 -or
            [string]$context.RValuePointer.Property.PropertyClass.ObjectName -cne "Class'VoyageItem'" -or
            [string]$context.RValuePointer.Property.PropertyClass.ObjectPath -cne '/Script/Voyage') {
            return $false
        }
        $callIndex = [int]$message[0].StatementIndex
        $implementsGuard = @($modActorUbergraph[0].ScriptBytecode | Where-Object {
            [string]$_.Token -ceq 'EX_JumpIfNot' -and
            [string]$_.BooleanExpression.Variable.Property.Name -ceq $branchContract.implements -and
            [int]$_.StatementIndex -lt $callIndex
        })
        $interfaceGuard = @($modActorUbergraph[0].ScriptBytecode | Where-Object {
            [string]$_.Token -ceq 'EX_JumpIfNot' -and
            [string]$_.BooleanExpression.Variable.Property.Name -ceq $branchContract.interfaceBool -and
            [int]$_.StatementIndex -lt $callIndex
        })
        $interfaceCast = @($modActorUbergraph[0].ScriptBytecode | Where-Object {
            [string]$_.Token -ceq 'EX_Let' -and
            [string]$_.Variable.Variable.Property.Name -ceq $branchContract.interface -and
            [string]$_.Expression.Token -ceq 'EX_ObjToInterfaceCast' -and
            [string]$_.Expression.InterfaceClass.ObjectName -ceq "Class'VoyageItemInterface'" -and
            [string]$_.Expression.InterfaceClass.ObjectPath -ceq '/Script/Voyage' -and
            [string]$_.Expression.Target.Variable.Property.Name -ceq 'LastSpawnedHumanSeed' -and
            [int]$_.StatementIndex -lt $callIndex
        })
        return ($implementsGuard.Count -eq 1 -and $interfaceGuard.Count -eq 1 -and $interfaceCast.Count -eq 1)
    })
    $placeCall = if ($directPlaceCalls.Count -eq 1) { $directPlaceCalls[0] } else { $null }
    $placeContext = if ($null -ne $placeCall) { $placeCall.Expression } else { $null }
    $placeIndex = if ($null -ne $placeCall) { [int]$placeCall.StatementIndex } else { -1 }
    $placeParameters = if ($null -ne $placeContext) { @($placeContext.ContextExpression.Parameters) } else { @() }
    $placeTargetProperty = if ($null -ne $placeContext) {
        $placeContext.ObjectExpression.Variable.Property
    } else { $null }
    $placeCast = @($modActorUbergraph[0].ScriptBytecode | Where-Object {
        [string]$_.Token -ceq 'EX_Let' -and
        [string]$_.Variable.Variable.Property.Name -ceq 'K2Node_DynamicCast_AsBP_First_Person_Character_New' -and
        [string]$_.Expression.Token -ceq 'EX_DynamicCast' -and
        [string]$_.Expression.InterfaceClass.ObjectName -ceq "BlueprintGeneratedClass'BP_FirstPersonCharacter_New_C'" -and
        [string]$_.Expression.InterfaceClass.ObjectPath -like '/Game/Blueprints/BP_FirstPersonCharacter_New.*' -and
        [int]$_.StatementIndex -lt $placeIndex
    })
    $placeCastGuard = @($modActorUbergraph[0].ScriptBytecode | Where-Object {
        [string]$_.Token -ceq 'EX_JumpIfNot' -and
        [string]$_.BooleanExpression.Variable.Property.Name -ceq 'K2Node_DynamicCast_bSuccess_3' -and
        [int]$_.StatementIndex -lt $placeIndex
    })
    $exactSeedEquality = @(Find-FunctionCallAssignments $modActorUbergraph[0].ScriptBytecode `
        "Class'KismetMathLibrary:EqualEqual_ObjectObject'" '/Script/Engine' | Where-Object {
            $serialized = $_.Expression | ConvertTo-Json -Depth 20 -Compress
            $serialized.Contains('Temp_object_Variable_1') -and $serialized.Contains('SeedTargetItem') -and
            [int]$_.StatementIndex -lt $placeIndex
        })
    $exactSeedEqualityGuard = if ($exactSeedEquality.Count -eq 1) {
        @($modActorUbergraph[0].ScriptBytecode | Where-Object {
            [string]$_.Token -ceq 'EX_JumpIfNot' -and
            [string]$_.BooleanExpression.Variable.Property.Name -ceq
                [string]$exactSeedEquality[0].Variable.Variable.Property.Name -and
            [int]$_.StatementIndex -lt $placeIndex
        })
    } else { @() }
    $inputBindings = @($modActorInputBinding[0].Properties.InputKeyDelegateBindings)
    $spawnInputBinding = @($inputBindings | Where-Object {
        [string]$_.FunctionNameToBind -ceq 'PalletContainerSnap_SpawnHumanSeed_CtrlShiftF6'
    })
    $inspectInputBinding = @($inputBindings | Where-Object {
        [string]$_.FunctionNameToBind -ceq 'PalletContainerSnap_InspectHumanSeed_CtrlShiftF7'
    })
    $placeInputBinding = @($inputBindings | Where-Object {
        [string]$_.FunctionNameToBind -ceq 'PalletContainerSnap_PlaceHumanSeed_CtrlShiftF9'
    })
    $enableInputStatement = if ($enableInputStatements.Count -eq 1) { $enableInputStatements[0] } else { $null }
    $enableInputGuard = if ($null -ne $enableInputStatement) {
        @($modActorUbergraph[0].ScriptBytecode | Where-Object {
            [string]$_.Token -ceq 'EX_JumpIfNot' -and
            [int]$_.StatementIndex -lt [int]$enableInputStatement.StatementIndex -and
            [int]$_.CodeOffset -gt [int]$enableInputStatement.StatementIndex
        } | Sort-Object StatementIndex | Select-Object -Last 1)
    } else { @() }
    $forbiddenGeneratorMemberAccess = @(
        Find-NodesWithPropertyValue $modActorUbergraph[0].ScriptBytecode 'Name' 'CraftingLocationGenerator' |
            Where-Object {
                $_.PSObject.Properties['Owner'] -and
                [string]$_.Owner.ObjectName -like "Class'VoyageItem*"
            }
    )
    if ([string]$modActorClass[0].SuperStruct.ObjectName -cne "Class'Actor'" -or
        [string]$modActorCdo[0].Properties.TargetItem.ObjectPath -cne '/Game/Data/Assets/Items/DA_Item_PlayerBoxLarge.0' -or
        [string]$modActorCdo[0].Properties.SeedTargetItem.ObjectPath -cne '/Game/Data/Assets/Consumables/DA_HumanSeed.0' -or
        $setterFunctions.Count -ne 2 -or
        $boolSetterFunctions.Count -ne 1 -or
        $spawnFunctions.Count -ne 2 -or
        $enableInputStatements.Count -ne 1 -or
        @($enableInputGuard).Count -ne 1 -or
        $getPlayerControllerFunctions.Count -lt 4 -or
        $getPlayerPawnFunctions.Count -ne 3 -or
        $getPlayerCharacterFunctions.Count -ne 1 -or
        $beginActorSpawnFunctions.Count -ne 1 -or
        $finishActorSpawnFunctions.Count -ne 1 -or
        $doesImplementInterfaceFunctions.Count -ne 2 -or
        $getPathNameFunctions.Count -lt 2 -or
        $getPathOutputNames -notcontains 'CallFunc_GetPathName_ReturnValue' -or
        $getPathOutputNames -notcontains 'CallFunc_GetPathName_ReturnValue_1' -or
        $getClassDisplayNameFunctions.Count -lt 2 -or
        $getClassDisplayOutputNames -notcontains 'CallFunc_GetClassDisplayName_ReturnValue' -or
        $getClassDisplayOutputNames -notcontains 'CallFunc_GetClassDisplayName_ReturnValue_1' -or
        $getItemMessages.Count -ne 2 -or
        @($getItemBranchEvidence | Where-Object { -not [bool]$_ }).Count -ne 0 -or
        $directPlaceCalls.Count -ne 1 -or
        [string]$placeContext.Token -cne 'EX_Context' -or
        [string]$placeContext.ObjectExpression.Token -cne 'EX_LocalVariable' -or
        [string]$placeTargetProperty.Name -cne 'K2Node_DynamicCast_AsBP_First_Person_Character_New' -or
        [string]$placeTargetProperty.PropertyClass.ObjectName -cne "BlueprintGeneratedClass'BP_FirstPersonCharacter_New_C'" -or
        [string]$placeTargetProperty.PropertyClass.ObjectPath -notlike '/Game/Blueprints/BP_FirstPersonCharacter_New.*' -or
        [string]$placeContext.ContextExpression.Token -cne 'EX_LocalVirtualFunction' -or
        [string]$placeContext.ContextExpression.Function -cne 'Place' -or
        $placeParameters.Count -ne 2 -or
        [string]$placeParameters[0].Token -cne 'EX_InstanceVariable' -or
        [string]$placeParameters[0].Variable.Property.Name -cne 'LastSpawnedHumanSeed' -or
        [string]$placeParameters[0].Variable.Owner.ObjectName -cne "BlueprintGeneratedClass'ModActor_C'" -or
        [string]$placeParameters[0].Variable.Owner.ObjectPath -cne '/Game/Mods/PalletContainerSnap/ModActor.0' -or
        [string]$placeParameters[1].Token -cne 'EX_LocalVariable' -or
        [string]$placeParameters[1].Variable.Property.Name -cne 'CallFunc_GetPlayerController_ReturnValue_2' -or
        [string]$placeParameters[1].Variable.Property.PropertyClass.ObjectName -cne "Class'PlayerController'" -or
        [string]$placeParameters[1].Variable.Property.PropertyClass.ObjectPath -cne '/Script/Engine' -or
        $placeCast.Count -ne 1 -or $placeCastGuard.Count -ne 1 -or
        $exactSeedEquality.Count -ne 1 -or @($exactSeedEqualityGuard).Count -ne 1 -or
        [string]$beginActorSpawn.Token -cne 'EX_CallMath' -or
        [string]$spawnCollisionParameter.Token -cne 'EX_ByteConst' -or
        [int]$spawnCollisionParameter.Value -ne 3 -or
        [string]$spawnActorClassParameter.Token -cne 'EX_ObjectConst' -or
        [string]$spawnActorClassParameter.Value.ObjectName -cne "BlueprintGeneratedClass'BP_Item_HumanSeed_PlaceDiagnostic_C'" -or
        [string]$spawnActorClassParameter.Value.ObjectPath -cne '/Game/Mods/PalletContainerSnap/BP_Item_HumanSeed_PlaceDiagnostic.0' -or
        $inputBindings.Count -ne 3 -or
        $spawnInputBinding.Count -ne 1 -or $inspectInputBinding.Count -ne 1 -or
        $placeInputBinding.Count -ne 1 -or
        @($spawnInputBinding + $inspectInputBinding + $placeInputBinding | Where-Object {
            [bool]$_.InputChord.bShift -or [bool]$_.InputChord.bCtrl -or
            [bool]$_.InputChord.bAlt -or [bool]$_.InputChord.bCmd -or
            [string]$_.InputKeyEvent -cne 'IE_Pressed' -or
            [bool]$_.bConsumeInput -or [bool]$_.bExecuteWhenPaused
        }).Count -ne 0 -or
        [string]$spawnInputBinding[0].InputChord.Key.KeyName -cne 'F6' -or
        [string]$inspectInputBinding[0].InputChord.Key.KeyName -cne 'F7' -or
        [string]$placeInputBinding[0].InputChord.Key.KeyName -cne 'F9' -or
        -not $modActorScriptText.Contains('CraftingLocationGenerator') -or
        -not $modActorScriptText.Contains('bAllowPlacement') -or
        -not $modActorScriptText.Contains('BP_CraftingLocationGenerator_PalletContainerSnap_C') -or
        -not $modActorScriptText.Contains('BP_CraftingLocationGenerator_HumanSeedPalletSnap_C') -or
        -not $modActorScriptText.Contains('TargetItem') -or
        -not $modActorScriptText.Contains('SeedTargetItem') -or
        -not $modActorText.Contains($injectionActivatedMarker) -or
        -not $modActorText.Contains($injectionDispatchedMarker) -or
        -not $modActorText.Contains($injectionCreateFailedMarker) -or
        -not $generatorText.Contains($generatorSelectedMarker) -or
        -not $modActorText.Contains($seedBoolDispatchedMarker) -or
        -not $modActorText.Contains($seedInjectionDispatchedMarker) -or
        -not $modActorText.Contains($seedInjectionCreateFailedMarker) -or
        -not $modActorText.Contains($spawnInputControllerMissingMarker) -or
        -not $modActorText.Contains($spawnPawnMissingMarker) -or
        -not $modActorText.Contains($spawnSucceededMarker) -or
        -not $modActorText.Contains($spawnFailedMarker) -or
        -not $modActorText.Contains('PalletContainerSnap_SpawnHumanSeed_CtrlShiftF6') -or
        -not $modActorText.Contains('PalletContainerSnap_InspectHumanSeed_CtrlShiftF7') -or
        -not $modActorText.Contains('PalletContainerSnap_PlaceHumanSeed_CtrlShiftF9') -or
        -not $modActorText.Contains('LastSpawnedHumanSeed') -or
        -not $modActorText.Contains('DiagnosticWidget') -or
        -not $modActorText.Contains('bAllowPlacement readback: unavailable') -or
        -not $modActorText.Contains('BP_Item_HumanSeed_PlaceDiagnostic_C') -or
        $modActorText.Contains('ReceiveTick') -or
        -not $seedGeneratorText.Contains($seedGeneratorSelectedMarker) -or
        -not $seedGeneratorText.Contains($seedSurfaceFoundMarker) -or
        -not $seedGeneratorText.Contains($seedSurfaceMissingMarker) -or
        $forbiddenGeneratorMemberAccess.Count -ne 0) {
        throw 'Runtime injection ModActor semantic contract validation failed.'
    }

    $diagnosticWidgetText = Get-Content -LiteralPath $diagnosticWidgetJsonPath -Raw
    if (-not $diagnosticWidgetText.Contains('DiagnosticText') -or
        -not $diagnosticWidgetText.Contains('SetDiagnosticResult') -or
        -not $diagnosticWidgetText.Contains('HideDiagnostic') -or
        -not $diagnosticWidgetText.Contains('SetText') -or
        -not $diagnosticWidgetText.Contains('SetVisibility') -or
        $diagnosticWidgetText.Contains('ReceiveTick') -or
        $diagnosticWidgetText.Contains('InputKeyDelegateBinding')) {
        throw 'Owned diagnostic widget semantic contract validation failed.'
    }

    $seedSetupRecord = $seedSetup[0]
    $seedSetupText = $seedSetupRecord.ScriptBytecode | ConvertTo-Json -Depth 40 -Compress
    $seedParentCalls = @(Find-NodesWithPropertyValue $seedSetupRecord.ScriptBytecode `
        'ObjectName' "Function'BP_CraftingLocationGenerator_Default_C:SetupLocation'" | Where-Object {
            [string]$_.ObjectPath -like '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.*'
        })
    $seedSurfaceTraces = @(Find-NodesWithPropertyValue $seedSetupRecord.ScriptBytecode `
        'ObjectName' "Class'PrimitiveComponent:K2_LineTraceComponent'" | Where-Object {
            [string]$_.ObjectPath -ceq '/Script/Engine'
        })
    $seedMinimumCalls = @(Find-PlacementComponentVirtualCalls `
        $seedSetupRecord.ScriptBytecode 'CalculateLocalMinimumZ')
    $seedHeightCalls = @(Find-PlacementComponentVirtualCalls `
        $seedSetupRecord.ScriptBytecode 'CalculateOffset')
    $seedFreeHeightCalls = @($seedHeightCalls | Where-Object {
        $parameters = @($_.ContextExpression.Parameters)
        $parameters.Count -eq 3 -and
        [string]$parameters[0].Token -ceq 'EX_LocalVariable' -and
        [string]$parameters[0].Variable.Property.Name -like 'CallFunc_BreakHitResult_ImpactPoint*' -and
        [string]$parameters[1].Token -ceq 'EX_LocalVariable' -and
        [string]$parameters[1].Variable.Property.Name -like 'CallFunc_BreakHitResult_ImpactNormal*' -and
        [string]$parameters[2].Token -ceq 'EX_DoubleConst' -and
        [double]$parameters[2].Value -eq 0.0
    })
    $seedSnappedHeightCalls = @($seedHeightCalls | Where-Object {
        $parameters = @($_.ContextExpression.Parameters)
        $parameters.Count -eq 3 -and
        [string]$parameters[0].Token -ceq 'EX_LocalVariable' -and
        [string]$parameters[0].Variable.Property.Name -like 'CallFunc_Add_VectorVector_ReturnValue*' -and
        [string]$parameters[1].Token -ceq 'EX_LocalVariable' -and
        [string]$parameters[1].Variable.Property.Name -like 'CallFunc_GetActorUpVector_ReturnValue*' -and
        [string]$parameters[2].Token -ceq 'EX_DoubleConst' -and
        [double]$parameters[2].Value -eq 0.0
    })
    $seedInstanceAssignments = @($seedSetupRecord.ScriptBytecode | Where-Object {
        $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_InstanceVariable'
    })
    $seedPlacementAssignments = @($seedInstanceAssignments | Where-Object {
        $_.Variable.Variable.Property.Name -ceq 'PlacementLocation'
    })
    $seedRotationAssignments = @($seedInstanceAssignments | Where-Object {
        $_.Variable.Variable.Property.Name -ceq 'PlacementRotation'
    })
    $seedAttachAssignments = @($seedInstanceAssignments | Where-Object {
        $_.Variable.Variable.Property.Name -ceq 'AttachToComponent'
    })
    $seedCheckAssignments = @($seedInstanceAssignments | Where-Object {
        $_.Variable.Variable.Property.Name -ceq 'CheckLocation'
    })
    if ([string]$seedGeneratorClassRecord[0].Super.ObjectName -cne "BlueprintGeneratedClass'BP_CraftingLocationGenerator_Default_C'" -or
        [string]$seedGeneratorClassRecord[0].Super.ObjectPath -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.0' -or
        $seedParentCalls.Count -ne 1 -or
        $seedSurfaceTraces.Count -ne 1 -or
        $seedMinimumCalls.Count -ne 1 -or
        $seedHeightCalls.Count -ne 4 -or
        $seedFreeHeightCalls.Count -ne 2 -or
        $seedSnappedHeightCalls.Count -ne 2 -or
        $seedPlacementAssignments.Count -ne 2 -or
        $seedRotationAssignments.Count -ne 2 -or
        $seedAttachAssignments.Count -ne 2 -or
        $seedCheckAssignments.Count -ne 2 -or
        -not $seedSetupText.Contains('Comps') -or
        -not $seedSetupText.Contains('AngularGranularity_Last') -or
        -not $seedSetupText.Contains('ImpactPoint') -or
        -not $seedSetupText.Contains('ImpactNormal') -or
        -not $seedSetupText.Contains('HitLocation') -or
        -not $seedSetupText.Contains('Vector_DistanceSquared') -or
        -not $seedSetupText.Contains('Less_DoubleDouble') -or
        -not $seedSetupText.Contains('Dot_VectorVector') -or
        -not $seedSetupText.Contains('BP_Donk_Pallet_New_C') -or
        -not $seedGeneratorText.Contains('SavedIgnoreNoFabrication')) {
        throw 'Human Seed generator cooked semantic contract validation failed.'
    }
}
$parentCalls = @(($setupRecord).ScriptBytecode | Where-Object { $_.Token -ceq 'EX_LocalFinalFunction' -and $_.Function.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.9' })
$instanceAssignments = @($setupRecord.ScriptBytecode | Where-Object {
    $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_InstanceVariable'
})
$placementAssignment = @($instanceAssignments | Where-Object { $_.Variable.Variable.Property.Name -ceq 'PlacementLocation' })
$rotationAssignment = @($instanceAssignments | Where-Object { $_.Variable.Variable.Property.Name -ceq 'PlacementRotation' })
$attachAssignment = @($instanceAssignments | Where-Object { $_.Variable.Variable.Property.Name -ceq 'AttachToComponent' })
$checkAssignment = @($instanceAssignments | Where-Object { $_.Variable.Variable.Property.Name -ceq 'CheckLocation' })
$targetBranch = @($setupRecord.ScriptBytecode | Where-Object {
    $_.Token -in @('EX_JumpIfNot','EX_PopExecutionFlowIfNot') -and
    $_.BooleanExpression.Token -ceq 'EX_LocalVariable' -and
    $_.BooleanExpression.Variable.Property.Name -like 'CallFunc_EqualEqual_ClassClass_ReturnValue*'
})
$localAssignments = @($setupRecord.ScriptBytecode | Where-Object {
    $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_LocalVariable'
})
$upAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_Context' -and
    $_.Expression.ContextExpression.Function.ObjectName -ceq "Class'Actor:GetActorUpVector'"
})
$scaleAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:Multiply_VectorFloat'"
})
$checkAddAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:Add_VectorVector'"
})
$slotTransformAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:TransformLocation'"
})
$distanceAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:Vector_DistanceSquared'"
})
$lessAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:Less_DoubleDouble'"
})
$vectorSelectAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:SelectVector'"
})
$doubleSelectAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:SelectFloat'"
})
$rotatorSelectAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:SelectRotator'"
})
$calculateOffsetAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_LocalVirtualFunction' -and
    $_.Expression.Function -ceq 'CalculateOffset'
})
$gridPositionCalls = @(Find-NodesWithPropertyValue $setupRecord.ScriptBytecode 'Function' 'TransformToGridFunc')
$angularContexts = @(Find-NodesWithPropertyValue $setupRecord.ScriptBytecode 'Token' 'EX_Context' | Where-Object {
    $_.ObjectExpression.Token -ceq 'EX_InstanceVariable' -and
    $_.ObjectExpression.Variable.Property.Name -ceq 'PlacementComponent' -and
    $_.ObjectExpression.Variable.Property.PropertyClass.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_FabricationPlacementComponent.0' -and
    $_.ContextExpression.Token -ceq 'EX_InstanceVariable' -and
    $_.ContextExpression.Variable.Owner.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_FabricationPlacementComponent.0' -and
    $_.ContextExpression.Variable.Property.Name -ceq 'AngularGranularity_Last' -and
    $_.ContextExpression.Variable.Property.Type -ceq 'DoubleProperty'
})
$modeCompareAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:EqualEqual_DoubleDouble'" -and
    (($_.Expression | ConvertTo-Json -Depth 20 -Compress).Contains('AngularGranularity_Last')) -and
    @(Find-TokenValue $_.Expression 'EX_DoubleConst' | Where-Object { [double]$_ -eq 0.0 }).Count -eq 1
})
$strictVectorSelectAssignments = @($vectorSelectAssignments | Where-Object {
    ($_.Expression | ConvertTo-Json -Depth 20 -Compress).Contains('CallFunc_Less_DoubleDouble_ReturnValue')
})
$modeVectorSelectAssignments = @($vectorSelectAssignments | Where-Object {
    ($_.Expression | ConvertTo-Json -Depth 20 -Compress).Contains('CallFunc_EqualEqual_DoubleDouble_ReturnValue')
})
$modeRotatorSelectAssignments = @($rotatorSelectAssignments | Where-Object {
    $serialized = $_.Expression | ConvertTo-Json -Depth 20 -Compress
    $serialized.Contains('CallFunc_EqualEqual_DoubleDouble_ReturnValue') -and
    $serialized.Contains('PlacementRotation') -and
    $serialized.Contains('CallFunc_K2_GetActorRotation_ReturnValue')
})
$placementChainAssignments = @($localAssignments | Where-Object {
    $_.StatementIndex -gt $targetBranch[0].StatementIndex -and
    $_.StatementIndex -lt $placementAssignment[0].StatementIndex
})
$checkChainAssignments = @($localAssignments | Where-Object {
    $_.StatementIndex -gt $attachAssignment[0].StatementIndex -and
    $_.StatementIndex -lt $checkAssignment[0].StatementIndex
})
$placementChainEvidence = Test-ExactSlotModeChain $placementChainAssignments 'PlacementLocation'
$checkChainEvidence = Test-ExactSlotModeChain $checkChainAssignments 'CheckLocation'
$slotMatches = @()
foreach ($supportedSlot in $supportedSlots) {
    $slotMatches += @($slotTransformAssignments | Where-Object {
        $vectors = @(Find-TokenValue $_.Expression 'EX_VectorConst')
        @($vectors | Where-Object {
            $null -ne $_.PSObject.Properties['X'] -and
            [Math]::Abs([double]$_.X - $supportedSlot[0]) -le 0.0001 -and
            [Math]::Abs([double]$_.Y - $supportedSlot[1]) -le 0.0001 -and
            [Math]::Abs([double]$_.Z - $supportedSlot[2]) -le 0.0001
        }).Count -eq 1
    })
}
$checkOffsetContexts = @(Find-NodesWithPropertyValue $setupRecord.ScriptBytecode 'Token' 'EX_Context' | Where-Object {
    $_.ObjectExpression.Token -ceq 'EX_InstanceVariable' -and
    $_.ObjectExpression.Variable.Owner.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.0' -and
    $_.ObjectExpression.Variable.Property.Name -ceq 'PlacementComponent' -and
    $_.ObjectExpression.Variable.Property.PropertyClass.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_FabricationPlacementComponent.0' -and
    $_.ContextExpression.Token -ceq 'EX_InstanceVariable' -and
    $_.ContextExpression.Variable.Owner.ObjectPath -ceq '/Game/Blueprints/Fabrication/BP_FabricationPlacementComponent.0' -and
    $_.ContextExpression.Variable.Property.Name -ceq 'CheckOffset' -and
    $_.ContextExpression.Variable.Property.Type -ceq 'DoubleProperty'
})
$scaleLocalInputs = @()
if ($scaleAssignments.Count -eq 1) {
    $scaleLocalInputs = @(Find-NodesWithPropertyValue $scaleAssignments[0].Expression 'Token' 'EX_LocalVariable' |
        ForEach-Object { $_.Variable.Property.Name })
}
$checkAddLocalInputs = @()
if ($checkAddAssignments.Count -eq 1) {
    $checkAddLocalInputs = @(Find-NodesWithPropertyValue $checkAddAssignments[0].Expression 'Token' 'EX_LocalVariable' |
        ForEach-Object { $_.Variable.Property.Name })
}
$checkOriginText = (@($upAssignments) + @($scaleAssignments) + @($checkAddAssignments) + @($checkAssignment) |
    ConvertTo-Json -Depth 30 -Compress)
$modePlacementName = [string]$placementChainEvidence.modeSelectName
$modeCheckName = [string]$checkChainEvidence.modeSelectName
$modeRotationName = if ($modeRotatorSelectAssignments.Count -eq 1) {
    $modeRotatorSelectAssignments[0].Variable.Variable.Property.Name
} else { '' }
$rotationSelectParameters = if ($modeRotatorSelectAssignments.Count -eq 1) {
    @($modeRotatorSelectAssignments[0].Expression.Parameters | ForEach-Object {
        [string]$_.Variable.Property.Name
    })
} else { @() }
$actorRotationAssignments = @($localAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_Context' -and
    $_.Expression.ContextExpression.Function.ObjectName -ceq "Class'Actor:K2_GetActorRotation'"
})
$distanceDataflowCount = @($distanceAssignments | Where-Object {
    $serialized = $_.Expression | ConvertTo-Json -Depth 20 -Compress
    $serialized.Contains('CallFunc_BreakHitResult_ImpactPoint') -and
    @($slotTransformAssignments | Where-Object {
        $serialized.Contains([string]$_.Variable.Variable.Property.Name)
    }).Count -eq 1
}).Count
$validateParentCalls = @($validateRecord.ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_LocalFinalFunction' -and
    $_.Function.ObjectName -ceq "Function'BP_CraftingLocationGenerator_Default_C:ValidateLocation'" -and
    $_.Function.ObjectPath -like '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.*'
})
$validateBranches = @($validateRecord.ScriptBytecode | Where-Object { $_.Token -ceq 'EX_JumpIfNot' })
$validateAssignments = @($validateRecord.ScriptBytecode | Where-Object {
    $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_InstanceVariable'
})
$snapshotAssignments = @($validateAssignments | Where-Object {
    $_.Variable.Variable.Property.Name -ceq 'SavedIgnoreNoFabrication' -and
    $_.Expression.Token -ceq 'EX_InstanceVariable' -and
    $_.Expression.Variable.Property.Name -ceq 'IgnoreNoFabrication'
})
$enableAssignments = @($validateAssignments | Where-Object {
    $_.Variable.Variable.Property.Name -ceq 'IgnoreNoFabrication' -and
    $_.Expression.Token -ceq 'EX_True'
})
$restoreAssignments = @($validateAssignments | Where-Object {
    $_.Variable.Variable.Property.Name -ceq 'IgnoreNoFabrication' -and
    $_.Expression.Token -ceq 'EX_InstanceVariable' -and
    $_.Expression.Variable.Property.Name -ceq 'SavedIgnoreNoFabrication'
})
$validateOutputAssignments = @($validateRecord.ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_LetBool' -and
    $_.Variable.Token -ceq 'EX_LocalOutVariable' -and
    $_.Variable.Variable.Property.Name -ceq 'Output_Get' -and
    $_.Expression.Token -ceq 'EX_LocalVariable' -and
    $_.Expression.Variable.Property.Name -like 'CallFunc_ValidateLocation_Output_Get*'
})
$palletClassConstants = @(Find-TokenValue $validateRecord.ScriptBytecode 'EX_ObjectConst' | Where-Object {
    $_.ObjectName -ceq "BlueprintGeneratedClass'BP_Donk_Pallet_New_C'" -and
    $_.ObjectPath -ceq '/Game/Blueprints/Modules/Misc/BP_Donk_Pallet_New.0'
})
$guardedParentCalls = @($validateParentCalls | Where-Object {
    $snapshotAssignments.Count -eq 1 -and $enableAssignments.Count -eq 1 -and $restoreAssignments.Count -eq 1 -and
    $_.StatementIndex -gt $enableAssignments[0].StatementIndex -and
    $_.StatementIndex -lt $restoreAssignments[0].StatementIndex
})
$outsideParentCalls = @($validateParentCalls | Where-Object {
    $validateBranches.Count -eq 1 -and $_.StatementIndex -eq $validateBranches[0].CodeOffset
})
$guardedOutputAssignments = @($validateOutputAssignments | Where-Object {
    $guardedParentCalls.Count -eq 1 -and
    $_.Expression.Variable.Property.Name -ceq $guardedParentCalls[0].Parameters[0].Variable.Property.Name
})
$outsideOutputAssignments = @($validateOutputAssignments | Where-Object {
    $outsideParentCalls.Count -eq 1 -and
    $_.Expression.Variable.Property.Name -ceq $outsideParentCalls[0].Parameters[0].Variable.Property.Name
})
$validateBottomParentCalls = @($validateBottomRecord.ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_LocalFinalFunction' -and
    $_.Function.ObjectName -ceq "Function'BP_CraftingLocationGenerator_Default_C:ValidateBottomAlignment'" -and
    $_.Function.ObjectPath -like '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.*'
})
$validateBottomBranches = @($validateBottomRecord.ScriptBytecode | Where-Object { $_.Token -ceq 'EX_JumpIfNot' })
$validateBottomJumps = @($validateBottomRecord.ScriptBytecode | Where-Object { $_.Token -ceq 'EX_Jump' })
$validateBottomReturns = @($validateBottomRecord.ScriptBytecode | Where-Object { $_.Token -ceq 'EX_Return' })
$validateBottomInstanceWrites = @($validateBottomRecord.ScriptBytecode | Where-Object {
    $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_InstanceVariable'
})
$validateBottomStructWrites = @($validateBottomRecord.ScriptBytecode | Where-Object {
    $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_StructMemberContext'
})
$validateBottomLocalAssignments = @($validateBottomRecord.ScriptBytecode | Where-Object {
    $_.Token -like 'EX_Let*' -and $_.Variable.Token -ceq 'EX_LocalVariable'
})
$validateBottomIsValidAssignments = @($validateBottomLocalAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetSystemLibrary:IsValid'"
})
$validateBottomObjectEqualityAssignments = @($validateBottomLocalAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:EqualEqual_ObjectObject'"
})
$validateBottomClassEqualityAssignments = @($validateBottomLocalAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:EqualEqual_ClassClass'"
})
$validateBottomAndAssignments = @($validateBottomLocalAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'KismetMathLibrary:BooleanAND'"
})
$validateBottomPalletConstants = @(Find-TokenValue $validateBottomRecord.ScriptBytecode 'EX_ObjectConst' | Where-Object {
    $_.ObjectName -ceq "BlueprintGeneratedClass'BP_Donk_Pallet_New_C'" -and
    $_.ObjectPath -ceq '/Game/Blueprints/Modules/Misc/BP_Donk_Pallet_New.0'
})
$validateBottomText = $validateBottomRecord.ScriptBytecode | ConvertTo-Json -Depth 40 -Compress
$validateBottomIsValidInputs = @()
if ($validateBottomIsValidAssignments.Count -eq 1) {
    $validateBottomIsValidInputs = @(Find-NodesWithPropertyValue $validateBottomIsValidAssignments[0].Expression 'Token' 'EX_LocalVariable' |
        ForEach-Object { $_.Variable.Property.Name })
}
$validateBottomObjectEqualityInputs = @()
if ($validateBottomObjectEqualityAssignments.Count -eq 1) {
    $validateBottomObjectEqualityInputs = @(Find-NodesWithPropertyValue $validateBottomObjectEqualityAssignments[0].Expression 'Token' 'EX_LocalVariable' |
        ForEach-Object { $_.Variable.Property.Name })
}
$validateBottomGetClassAssignments = @($validateBottomLocalAssignments | Where-Object {
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq "Class'GameplayStatics:GetObjectClass'"
})
$validateBottomGetClassInputs = @()
if ($validateBottomGetClassAssignments.Count -eq 1) {
    $validateBottomGetClassInputs = @(Find-NodesWithPropertyValue $validateBottomGetClassAssignments[0].Expression 'Token' 'EX_LocalVariable' |
        ForEach-Object { $_.Variable.Property.Name })
}
$validateBottomClassEqualityText = if ($validateBottomClassEqualityAssignments.Count -eq 1) {
    $validateBottomClassEqualityAssignments[0].Expression | ConvertTo-Json -Depth 20 -Compress
} else { '' }
$validateBottomClassEqualityPalletConstants = @()
if ($validateBottomClassEqualityAssignments.Count -eq 1) {
    $validateBottomClassEqualityPalletConstants = @(
        Find-TokenValue $validateBottomClassEqualityAssignments[0].Expression 'EX_ObjectConst' |
        Where-Object {
            $_.ObjectName -ceq "BlueprintGeneratedClass'BP_Donk_Pallet_New_C'" -and
            $_.ObjectPath -ceq '/Game/Blueprints/Modules/Misc/BP_Donk_Pallet_New.0'
        })
}
$validateBottomValidComponentAnd = @($validateBottomAndAssignments | Where-Object {
    $serialized = $_.Expression | ConvertTo-Json -Depth 20 -Compress
    $serialized.Contains('CallFunc_IsValid_ReturnValue') -and
    $serialized.Contains('CallFunc_EqualEqual_ObjectObject_ReturnValue')
})
$validateBottomBlockingPalletAnd = @($validateBottomAndAssignments | Where-Object {
    $serialized = $_.Expression | ConvertTo-Json -Depth 20 -Compress
    $serialized.Contains('CallFunc_BreakHitResult_bBlockingHit') -and
    $serialized.Contains('CallFunc_EqualEqual_ClassClass_ReturnValue')
})
$validateBottomFinalAnd = @($validateBottomAndAssignments | Where-Object {
    $serialized = $_.Expression | ConvertTo-Json -Depth 20 -Compress
    $serialized.Contains('CallFunc_BooleanAND_ReturnValue_1') -and
    $serialized.Contains('CallFunc_BooleanAND_ReturnValue')
})
$componentEntries = @($itemRecord.Properties.Components)
$skillItems = @($skillRecord.Properties.Items)
if ($null -eq $itemRecord.Properties.PSObject.Properties['CraftElectricityCost'] -or
    $null -eq $skillRecord.Properties.Unlock.PSObject.Properties['Requirement'] -or
    $null -eq $skillRecord.Properties.Unlock.PSObject.Properties['Cost']) {
    throw 'Cooked readback omitted an explicitly authored zero-value contract.'
}
if ($generatorRecord.Super.ObjectPath -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.0' -or
    $setupRecord.Super.ObjectPath -cne '/Game/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.9' -or
    $validateRecord.Super.ObjectName -cne "Function'BP_CraftingLocationGenerator_Default_C:ValidateLocation'" -or
    $validateBottomRecord.Super.ObjectName -cne "Function'BP_CraftingLocationGenerator_Default_C:ValidateBottomAlignment'" -or
    $validateOutputProperties.Count -ne 1 -or
    $validateOutputProperties[0].PropertyFlags -notlike '*Parm*OutParm*' -or
    $parentCalls.Count -ne 1 -or
    $validateParentCalls.Count -ne 2 -or $validateBranches.Count -ne 1 -or
    $snapshotAssignments.Count -ne 1 -or $enableAssignments.Count -ne 1 -or
    $restoreAssignments.Count -ne 1 -or $validateOutputAssignments.Count -ne 2 -or
    $palletClassConstants.Count -ne 1 -or $guardedParentCalls.Count -ne 1 -or
    $outsideParentCalls.Count -ne 1 -or $guardedOutputAssignments.Count -ne 1 -or
    $outsideOutputAssignments.Count -ne 1 -or
    $validateBottomParentCalls.Count -ne 1 -or $validateBottomBranches.Count -ne 1 -or
    $validateBottomJumps.Count -ne 1 -or $validateBottomReturns.Count -ne 1 -or
    $validateBottomInstanceWrites.Count -ne 0 -or $validateBottomStructWrites.Count -ne 0 -or
    $validateBottomIsValidAssignments.Count -ne 1 -or
    $validateBottomObjectEqualityAssignments.Count -ne 1 -or
    $validateBottomClassEqualityAssignments.Count -ne 1 -or
    $validateBottomAndAssignments.Count -ne 3 -or
    $validateBottomPalletConstants.Count -ne 1 -or
    $validateBottomIsValidInputs -notcontains 'CallFunc_BreakHitResult_HitComponent' -or
    $validateBottomObjectEqualityInputs -notcontains 'CallFunc_BreakHitResult_HitComponent' -or
    $validateBottomGetClassAssignments.Count -ne 1 -or
    $validateBottomGetClassInputs -notcontains 'CallFunc_BreakHitResult_HitActor' -or
    -not $validateBottomClassEqualityText.Contains('CallFunc_GetObjectClass_ReturnValue') -or
    $validateBottomClassEqualityPalletConstants.Count -ne 1 -or
    $validateBottomValidComponentAnd.Count -ne 1 -or
    $validateBottomBlockingPalletAnd.Count -ne 1 -or
    $validateBottomFinalAnd.Count -ne 1 -or
    $validateBottomBranches[0].BooleanExpression.Variable.Property.Name -cne $validateBottomFinalAnd[0].Variable.Variable.Property.Name -or
    -not $validateBottomText.Contains('AttachToComponent') -or
    -not $validateBottomText.Contains('CallFunc_BreakHitResult_bBlockingHit') -or
    -not $validateBottomText.Contains('CallFunc_BreakHitResult_HitActor') -or
    $validateBottomBranches[0].CodeOffset -ne $validateBottomParentCalls[0].StatementIndex -or
    $validateBottomJumps[0].StatementIndex -ge $validateBottomParentCalls[0].StatementIndex -or
    $validateBottomJumps[0].CodeOffset -ne $validateBottomReturns[0].StatementIndex -or
    $validateBottomParentCalls[0].StatementIndex -ge $validateBottomReturns[0].StatementIndex -or
    $validateBranches[0].StatementIndex -ge $snapshotAssignments[0].StatementIndex -or
    $snapshotAssignments[0].StatementIndex -ge $enableAssignments[0].StatementIndex -or
    $enableAssignments[0].StatementIndex -ge $guardedParentCalls[0].StatementIndex -or
    $guardedParentCalls[0].StatementIndex -ge $restoreAssignments[0].StatementIndex -or
    $restoreAssignments[0].StatementIndex -ge $guardedOutputAssignments[0].StatementIndex -or
    $guardedOutputAssignments[0].StatementIndex -ge $outsideParentCalls[0].StatementIndex -or
    $outsideParentCalls[0].StatementIndex -ge $outsideOutputAssignments[0].StatementIndex -or
    -not $generatorText.Contains("BlueprintGeneratedClass'BP_Donk_Pallet_New_C'") -or
    $targetBranch.Count -ne 1 -or
    $placementAssignment.Count -ne 1 -or $rotationAssignment.Count -ne 1 -or
    $attachAssignment.Count -ne 1 -or $checkAssignment.Count -ne 1 -or
    $upAssignments.Count -ne 1 -or $scaleAssignments.Count -ne 1 -or
    $checkAddAssignments.Count -ne 1 -or $slotTransformAssignments.Count -ne 8 -or
    $slotMatches.Count -ne 8 -or
    $distanceAssignments.Count -ne 8 -or
    $lessAssignments.Count -ne 6 -or
    $strictVectorSelectAssignments.Count -ne 6 -or
    $doubleSelectAssignments.Count -ne 4 -or
    $modeVectorSelectAssignments.Count -ne 2 -or
    $modeRotatorSelectAssignments.Count -ne 1 -or
    $actorRotationAssignments.Count -ne 1 -or
    $rotationSelectParameters.Count -ne 3 -or
    $rotationSelectParameters[0] -cne $actorRotationAssignments[0].Variable.Variable.Property.Name -or
    $rotationSelectParameters[1] -cne 'PlacementRotation' -or
    $rotationSelectParameters[2] -notlike 'CallFunc_EqualEqual_DoubleDouble_ReturnValue*' -or
    $calculateOffsetAssignments.Count -ne 2 -or
    $gridPositionCalls.Count -ne 0 -or
    $angularContexts.Count -ne 3 -or $modeCompareAssignments.Count -ne 3 -or
    $checkOffsetContexts.Count -ne 1 -or
    $scaleLocalInputs -notcontains $upAssignments[0].Variable.Variable.Property.Name -or
    $checkAddLocalInputs -notcontains $scaleAssignments[0].Variable.Variable.Property.Name -or
    $checkAddLocalInputs -notcontains $modeCheckName -or
    $placementAssignment[0].Expression.Token -cne 'EX_LocalVariable' -or
    $placementAssignment[0].Expression.Variable.Property.Name -cne $modePlacementName -or
    $rotationAssignment[0].Expression.Token -cne 'EX_LocalVariable' -or
    $rotationAssignment[0].Expression.Variable.Property.Name -cne $modeRotationName -or
    $checkAssignment[0].Expression.Token -cne 'EX_LocalVariable' -or
    $checkAssignment[0].Expression.Variable.Property.Name -cne $checkAddAssignments[0].Variable.Variable.Property.Name -or
    $generatorText.Contains('ParentCheckOffset') -or
    $parentCalls[0].StatementIndex -ge $targetBranch[0].StatementIndex -or
    $targetBranch[0].StatementIndex -ge $placementAssignment[0].StatementIndex -or
    $placementAssignment[0].StatementIndex -ge $rotationAssignment[0].StatementIndex -or
    $rotationAssignment[0].StatementIndex -ge $attachAssignment[0].StatementIndex -or
    $attachAssignment[0].StatementIndex -ge $checkAssignment[0].StatementIndex -or
    $containerRecord.Super.ObjectPath -cne '/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large.0' -or
    $containerCdoRecord.Properties.ContainerItem.ObjectName -cne "VoyageItem'DA_Item_PalletSnapCanary'" -or
    $itemRecord.Properties.Type.Name -cne 'Item' -or
    $itemRecord.Properties.Category -cne 'EVoyageItemCategory::InventoryContainer' -or
    -not [bool]$itemRecord.Properties.bAllowPlacement -or
    [int]$itemRecord.Properties.CraftFilter -ne $supportedCraftFilter -or
    [Math]::Abs([double]$itemRecord.Properties.CraftTime - $supportedCraftTime) -gt 0.00001 -or
    [Math]::Abs([double]$itemRecord.Properties.CraftElectricityCost - $supportedCraftElectricityCost) -gt 0.00001 -or
    $componentEntries.Count -ne 1 -or
    [string]$componentEntries[0].Key -cne "VoyageItemPart'$supportedComponent.DA_Part_AlloyFrame'" -or
    [int]$componentEntries[0].Value -ne $supportedComponentCount -or
    $itemRecord.Properties.DroppedActor.AssetPathName -cne '/Game/Mods/PalletContainerSnap/BP_Inventory_PalletContainerSnap.BP_Inventory_PalletContainerSnap_C' -or
    $itemRecord.Properties.CraftingLocationGenerator.ObjectName -notlike "BP_CraftingLocationGenerator_PalletContainerSnap_C'DA_Item_PalletSnapCanary:*'" -or
    $itemRecord.Properties.Name.SourceString -cne 'Pallet Snap Canary FortiHold' -or
    $skillRecord.Properties.Type.Name -cne 'Skill' -or
    $skillRecord.Properties.Unlock.UnlockMethod -cne 'EVoyageSkillUnlockMethod::Tier' -or
    [int]$skillRecord.Properties.Unlock.Requirement -ne $supportedSkillRequirement -or
    [int]$skillRecord.Properties.Unlock.Cost -ne $supportedSkillCost -or
    $skillItems.Count -ne 1 -or
    $skillItems[0].ObjectPath -notlike '/Game/Data/Assets/Items/PalletContainerSnap/DA_Item_PalletSnapCanary.*' -or
    $skillRecord.Properties.Name.SourceString -cne 'Pallet Snap Canary Recipe') {
    throw 'Cooked semantic contract validation failed.'
}

$registryItemJsonPath = Join-Path $output 'registry-item-readback.json'
$registrySkillJsonPath = Join-Path $output 'registry-skill-readback.json'
$registryItemReadback = Invoke-NativeStage 'registry-item-readback' $uassetGui @(
    '--portable','tojson',(Join-Path $cooked ($relatives[2] + '.uasset')),
    $registryItemJsonPath,[string]$contract.parser.engineVersion,$mapping.mappingsPath) 45
$registrySkillReadback = Invoke-NativeStage 'registry-skill-readback' $uassetGui @(
    '--portable','tojson',(Join-Path $cooked ($relatives[3] + '.uasset')),
    $registrySkillJsonPath,[string]$contract.parser.engineVersion,$mapping.mappingsPath) 45
foreach ($registryInput in @(
        [pscustomobject]@{path=$registryItemJsonPath;package=[string]$contract.assets.item;object='DA_Item_PalletSnapCanary'},
        [pscustomobject]@{path=$registrySkillJsonPath;package=[string]$contract.assets.skill;object='DA_Skill_PalletSnapCanary'})) {
    $raw = Get-Content -LiteralPath $registryInput.path -Raw | ConvertFrom-Json
    if ([string]$raw.FolderName -cne $registryInput.package -or
        @($raw.Exports | Where-Object { $_.ObjectName -ceq $registryInput.object }).Count -ne 1 -or
        @($raw.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count -ne 0) {
        throw "Registry JSON readback has the wrong package identity or unsafe exports: $($registryInput.path)"
    }
}
$fullSkillPackage = Get-Content -LiteralPath $registrySkillJsonPath -Raw | ConvertFrom-Json
$fullSkill = @($fullSkillPackage.Exports | Where-Object {
    $_.OuterIndex -eq 0 -and $_.ObjectName -ceq 'DA_Skill_PalletSnapCanary'
})
if ($fullSkill.Count -ne 1) { throw 'Canonical full Skill readback has no unique owned primary export.' }
$fullUnlock = @($fullSkill[0].Data | Where-Object { $_.Name -ceq 'Unlock' })
if ($fullUnlock.Count -ne 1 -or
    [string]$fullUnlock[0].'$type' -cne 'UAssetAPI.PropertyTypes.Structs.StructPropertyData, UAssetAPI' -or
    [string]$fullUnlock[0].StructType -cne 'VoyageSkillUnlock') {
    throw 'Canonical full Skill readback has the wrong Unlock struct contract.'
}
$fullUnlockMethod = @($fullUnlock[0].Value | Where-Object { $_.Name -ceq 'UnlockMethod' })
$fullRequirement = @($fullUnlock[0].Value | Where-Object { $_.Name -ceq 'Requirement' })
$fullCost = @($fullUnlock[0].Value | Where-Object { $_.Name -ceq 'Cost' })
if ($fullUnlockMethod.Count -ne 1 -or $fullRequirement.Count -ne 1 -or $fullCost.Count -ne 1 -or
    [string]$fullUnlockMethod[0].'$type' -cne 'UAssetAPI.PropertyTypes.Objects.EnumPropertyData, UAssetAPI' -or
    [string]$fullUnlockMethod[0].EnumType -cne 'EVoyageSkillUnlockMethod' -or
    [string]$fullUnlockMethod[0].Value -cne 'EVoyageSkillUnlockMethod::Tier' -or
    [string]$fullRequirement[0].'$type' -cne 'UAssetAPI.PropertyTypes.Objects.IntPropertyData, UAssetAPI' -or
    [int]$fullRequirement[0].Value -ne $supportedSkillRequirement -or
    [string]$fullCost[0].'$type' -cne 'UAssetAPI.PropertyTypes.Objects.IntPropertyData, UAssetAPI' -or
    [int]$fullCost[0].Value -ne $supportedSkillCost) {
    throw 'Canonical full Skill readback does not preserve Tier 11 / Cost 0.'
}

$registry = Join-Path $output 'PalletContainerSnapCatalogue-AssetRegistry.bin'
. $registryHelperPath
$registryResult = New-PalletContainerSnapRegistry -RepositoryRoot $repo `
    -AssetJsonPath @($registryItemJsonPath,$registrySkillJsonPath) -OutputPath $registry `
    -EngineRoot $engine -EvidenceRoot (Join-Path $output 'asset-registry-writer') `
    -PolicyPath $registryPolicyPath
if ($registryResult.status -cne 'passed' -or $registryResult.assetCount -ne 2 -or
    $registryResult.packageCount -ne 2 -or -not [bool]$registryResult.reopenVerified) {
    throw 'Plugin-local registry generation/reopen failed.'
}
$registryResponse = Join-Path $output 'registry-response.txt'
[IO.File]::WriteAllText($registryResponse,
    ('"' + [IO.Path]::GetFullPath($registry) + '" "../../../Voyage/Mods/PalletContainerSnapCatalogue/AssetRegistry.bin"' + [Environment]::NewLine),
    [Text.Encoding]::ASCII)
$registryPak = Join-Path $output 'PalletContainerSnap.registry.pak'
$registryPackRun = Invoke-NativeStage 'pack-registry' $unrealPak @($registryPak,('-Create=' + $registryResponse),'-Dest=../../../') 120
$registryReadback = Join-Path $output 'registry-readback'
[IO.Directory]::CreateDirectory($registryReadback) | Out-Null
$registryReadbackRun = Invoke-NativeStage 'readback-registry' $unrealPak @($registryPak,'-Extract',$registryReadback,'-Filter=AssetRegistry.bin') 120
$registryReadbackFiles = @(Get-ChildItem -LiteralPath $registryReadback -File -Recurse)
if ($registryReadbackFiles.Count -ne 1 -or $registryReadbackFiles[0].Name -cne 'AssetRegistry.bin' -or
    (Get-FileHash -LiteralPath $registryReadbackFiles[0].FullName -Algorithm SHA256).Hash -cne
        (Get-FileHash -LiteralPath $registry -Algorithm SHA256).Hash) {
    throw 'Packaged registry readback hash mismatch.'
}
$containerPak = [IO.Path]::ChangeExtension($container, '.pak')
$emptyPak = Join-Path $output 'PalletContainerSnap.empty.pak'
Move-Item -LiteralPath $containerPak -Destination $emptyPak
Move-Item -LiteralPath $registryPak -Destination $containerPak
$containerCheckFinal = & (Join-Path $repo 'tools/Test-VoyageContainer.ps1') `
    -Container $container -ExpectedPackageList $expected -OutputRoot (Join-Path $output 'container-check-final')
if ($containerCheckFinal.status -cne 'passed' -or -not [bool]$containerCheckFinal.packageSetMatches) {
    throw 'Final container with catalogue PAK failed verification.'
}

$descriptorDirectory = Join-Path $releaseRoot 'Voyage/Mods/PalletContainerSnapCatalogue'
[IO.Directory]::CreateDirectory($descriptorDirectory) | Out-Null
$descriptor = Join-Path $descriptorDirectory 'PalletContainerSnapCatalogue.uplugin'
Copy-Item -LiteralPath $descriptorSource -Destination $descriptor
$autoload = $null
if ($runtimeInjection -and -not $dmlActivation) {
    $autoload = Join-Path $payload ($supportedContainerStem + '.autoload')
    [IO.File]::WriteAllText(
        $autoload,
        ('entryClass: ' + $runtimeInjectionActorClass + [Environment]::NewLine +
         'activateIn: gameplay' + [Environment]::NewLine),
        (New-Object Text.UTF8Encoding($false)))
}
$archive = Join-Path $releaseRoot $supportedArchiveName
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archiveStream = [IO.File]::Open($archive,[IO.FileMode]::CreateNew,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {
    $zip = [IO.Compression.ZipArchive]::new($archiveStream,[IO.Compression.ZipArchiveMode]::Create,$false)
    try {
        $containerArchiveRoot = if ($dmlActivation) { 'Content/Paks/LogicMods/' } else { 'Content/Paks/' }
        $archiveEntries = @(
            [pscustomobject]@{source=$containerPak;name=($containerArchiveRoot + $supportedContainerStem + '.pak')},
            [pscustomobject]@{source=[IO.Path]::ChangeExtension($container,'.ucas');name=($containerArchiveRoot + $supportedContainerStem + '.ucas')},
            [pscustomobject]@{source=$container;name=($containerArchiveRoot + $supportedContainerStem + '.utoc')},
            [pscustomobject]@{source=$descriptor;name='Mods/PalletContainerSnapCatalogue/PalletContainerSnapCatalogue.uplugin'})
        if ($runtimeInjection -and -not $dmlActivation) {
            $archiveEntries += [pscustomobject]@{source=$autoload;name=('Content/Paks/' + $supportedContainerStem + '.autoload')}
        }
        foreach ($entry in $archiveEntries) {
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,$entry.source,$entry.name,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    } finally { $zip.Dispose() }
} finally { $archiveStream.Dispose() }

$releaseManifest = & (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') `
    -ReleaseRoot $releaseRoot -Mod 'PalletContainerSnap' -Version $supportedArtifactVersion `
    -Container $container -Archive $archive -SourcePath $modRoot `
    -ContentPluginDescriptor $descriptor -AllowDirtySource -AsJson | ConvertFrom-Json
if ($releaseManifest.status -cne 'created-and-validated') { throw 'Schema-3 release manifest validation failed.' }

$sourceAfter = @(Get-SourceSnapshot)
if (($sourceBefore | ConvertTo-Json -Depth 5 -Compress) -cne ($sourceAfter | ConvertTo-Json -Depth 5 -Compress)) {
    throw 'PalletContainerSnap source changed during the build.'
}
$provenance = [ordered]@{
    schemaVersion = 1
    status = 'prepared'
    preparationMode = 'fresh-full-producer'
    runtime = 'not-run'
    artifactVersion = $supportedArtifactVersion
    steamBuildId = [string]$fingerprint.steam.buildId
    ignoredPinnedGameVersion = [bool]$gameBuild.ignoredPinnedGameVersion
    reviewedMappingEngineVersion = [string]$contract.game.reviewedMappingEngineVersion
    producerEngineVersion = $actualProducerEngine
    parserEngineVersion = [string]$contract.parser.engineVersion
    uassetApiEngineVersion = [string]$contract.parser.uassetApiEngineVersion
    mappingSha256 = [string]$mapping.sha256
    authoredGlb = $authoredGlb
    authoredGlbSha256 = (Get-FileHash -LiteralPath $authoredGlb -Algorithm SHA256).Hash
    ownedMetadataDesign = $contract.ownedRegistryMetadata
    validationExperiment = $contract.validationExperiment
    checkOriginExperiment = $contract.checkOriginExperiment
    palletSupportExperiment = $contract.palletSupportExperiment
    placementModes = $contract.placementModes
    registryPolicyPath = [IO.Path]::GetFullPath($registryPolicyPath)
    registryPolicySha256 = (Get-FileHash -LiteralPath $registryPolicyPath -Algorithm SHA256).Hash
    scope = $Scope
    containerStem = $supportedContainerStem
    runtimeInjection = $runtimeInjection
    activationMode = if ($runtimeInjection) { $ActivationMode } else { $null }
    runtimeInjectionActor = if ($runtimeInjection) { $runtimeInjectionActorClass } else { $null }
    runtimeInjectionTarget = if ($runtimeInjection) { $runtimeInjectionTarget } else { $null }
    seedRuntimeInjectionTarget = if ($runtimeInjection) { $seedRuntimeInjectionTarget } else { $null }
    seedGenerator = if ($runtimeInjection) { $seedGeneratorClass } else { $null }
    seedSurfaceTraceHalfLengthCentimeters = if ($runtimeInjection) { $seedSurfaceTraceHalfLength } else { $null }
    autoload = $autoload
    scriptObjects = $scriptObjects[0].FullName
    scriptObjectsSha256 = (Get-FileHash -LiteralPath $scriptObjects[0].FullName -Algorithm SHA256).Hash
    sourceFiles = $sourceAfter
    generationReport = $generationReport
    taggedVerifyReport = $verifyTaggedRun.reportPath
    containerReport = $containerCheckFinal.reportPath
    registryManifest = $registryResult.manifestPath
    releaseManifest = $releaseManifest.manifestPath
}
$provenancePath = Join-Path $output 'build-provenance.json'
Write-Utf8Json $provenancePath $provenance
$result = [ordered]@{
    status = 'prepared'
    preparationMode = 'fresh-full-producer'
    runtime = 'not-run'
    artifactVersion = $supportedArtifactVersion
    scope = $Scope
    outputRoot = $output
    container = $container
    archive = $archive
    releaseManifest = $releaseManifest.manifestPath
    provenance = $provenancePath
    generationReport = $generationReport
    containerReport = $containerCheckFinal.reportPath
    registryManifest = $registryResult.manifestPath
    packageCount = [int]$containerCheckFinal.packageCount
    packageSetMatches = [bool]$containerCheckFinal.packageSetMatches
    registryAssetCount = [int]$registryResult.assetCount
    registryPackageCount = [int]$registryResult.packageCount
    registryReopenVerified = [bool]$registryResult.reopenVerified
    validationExperiment = $contract.validationExperiment
    checkOriginExperiment = $contract.checkOriginExperiment
    palletSupportExperiment = $contract.palletSupportExperiment
    placementModes = $contract.placementModes
    generatorJson = $generatorJsonPath
    seedGeneratorJson = $seedGeneratorJsonPath
    containerJson = $containerJsonPath
    itemJson = $itemJsonPath
    skillJson = $skillJsonPath
    modActorJson = $modActorJsonPath
    diagnosticWidgetJson = $diagnosticWidgetJsonPath
    activationMode = if ($runtimeInjection) { $ActivationMode } else { $null }
    accepted011LargeGeneratorJsonSha256 = if ($runtimeInjection) { $accepted011LargeGeneratorJsonSha256 } else { $null }
    accepted011SeedGeneratorJsonSha256 = if ($runtimeInjection) { $accepted011SeedGeneratorJsonSha256 } else { $null }
    autoload = $autoload
}
$resultPath = Join-Path $output 'build-result.json'
Write-Utf8Json $resultPath $result
[pscustomobject]$result | ConvertTo-Json -Depth 8 -Compress
