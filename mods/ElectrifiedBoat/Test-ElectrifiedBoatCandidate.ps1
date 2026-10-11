[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackageRoot,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedSteamBuildId = '25191271'
$expectedExecutableSha256 = '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B'
$expectedEditorVersion = '5.8.2'
$containerName = 'ElectrifiedBoat'
$entryClass = '/Game/Mods/ElectrifiedBoat/ModActor.ModActor_C'
$expectedPackage = '../../../Voyage/Content/Mods/ElectrifiedBoat/ModActor.uasset'
$legacyAuthoredIdentity = 'BoatElectric' + 'WallSocket'
$legacyVirtualRoot = '/Game/Mods/' + $legacyAuthoredIdentity + '/'
$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$package = (Resolve-Path -LiteralPath $PackageRoot).Path
$utoc = Join-Path $package ($containerName + '.utoc')
$manifestPath = Join-Path $package 'build-manifest.json'
$autoloadPath = Join-Path $package ($containerName + '.autoload')
$expectedListPath = Join-Path $package 'expected-packages.txt'
$levelInstanceMirrorPath = Join-Path $modRoot 'Source\Voyage\VoyageLevelInstanceComponent.h'
$generatorSourcePath = Join-Path $modRoot 'Source\ElectrifiedBoatGenerator\GenerateElectrifiedBoatCommandlet.cpp'
$voyageModuleSourcePath = Join-Path $modRoot 'Source\Voyage\Voyage.cpp'
$generatorBuildRulesPath = Join-Path $modRoot 'Source\ElectrifiedBoatGenerator\ElectrifiedBoatGenerator.Build.cs'
$levelInstanceMirror = [IO.File]::ReadAllText($levelInstanceMirrorPath)
$generatorSource = [IO.File]::ReadAllText($generatorSourcePath)
$voyageModuleSource = [IO.File]::ReadAllText($voyageModuleSourcePath)
$generatorBuildRules = [IO.File]::ReadAllText($generatorBuildRulesPath)
if (-not $levelInstanceMirror.Contains('TArray<TWeakObjectPtr<AActor>> OwnedActors') -or
    $levelInstanceMirror.Contains('TArray<TObjectPtr<AActor>> OwnedActors') -or
    -not $generatorSource.Contains('ExposeVoyageLevelInstanceOwnedActorsToBlueprint()') -or
    -not $voyageModuleSource.Contains('CastField<FWeakObjectProperty>(OwnedActors->Inner)') -or
    -not $voyageModuleSource.Contains('CPF_BlueprintVisible | CPF_BlueprintReadOnly') -or
    -not $voyageModuleSource.Contains('virtual void StartupModule() override')) {
    throw 'OwnedActors must preserve its native weak-object array and use only the bounded editor-process Blueprint visibility override.'
}
foreach ($forbiddenSourceToken in @(
    'WBP_FlowDiagnostic',
    'ReceiveTick',
    'SetTimer',
    'ClearTimer',
    'Diagnostic',
    'UMG',
    'SlateCore'
)) {
    if ($generatorSource.Contains($forbiddenSourceToken)) {
        throw "Generator source still contains removed diagnostic contract token: $forbiddenSourceToken"
    }
}
if ($generatorBuildRules.Contains('UMG') -or
    $generatorBuildRules.Contains('Slate')) {
    throw 'Generator build rules still depend on removed diagnostic UI modules.'
}
if ($generatorSource.Contains('ModuleSocketsProperty') -or
    $generatorSource.Contains('RegistrationValid')) {
    throw 'Generator source still contains the removed inert post-registration socket-list checks.'
}

foreach ($required in @(
    (Join-Path $package ($containerName + '.pak')),
    (Join-Path $package ($containerName + '.ucas')),
    $utoc,
    $manifestPath,
    $autoloadPath,
    $expectedListPath
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Candidate file is missing: $required"
    }
}

$validationRoot = Join-Path (Split-Path -Parent $package) `
    ('validation-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
[IO.Directory]::CreateDirectory($validationRoot) | Out-Null
$fingerprintPath = Join-Path $validationRoot 'game-fingerprint.json'
& (Join-Path $repoRoot 'tools/Get-VoyageBuildFingerprint.ps1') `
    -GameRoot $GameRoot -OutputPath $fingerprintPath | Out-Null
$fingerprint = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
if ([string]$fingerprint.steam.buildId -cne $expectedSteamBuildId -or
    [string]$fingerprint.executable.sha256 -cne $expectedExecutableSha256) {
    throw 'Installed game fingerprint does not match the candidate contract.'
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([string]$manifest.steamBuildId -cne $expectedSteamBuildId -or
    [string]$manifest.executableSha256 -cne $expectedExecutableSha256 -or
    [string]$manifest.entryClass -cne $entryClass -or
    [string]$manifest.containerName -cne $containerName -or
    [string]$manifest.architecture -cne 'additive-owned-package-only' -or
    [string]$manifest.editorVersion -cne $expectedEditorVersion) {
    throw 'Build manifest identity or architecture contract is invalid.'
}
if (-not [string]$manifest.ownedActorRead -or
    -not ([string]$manifest.ownedActorRead).Contains('weak/null/weak ProcessEvent canary')) {
    throw 'Build manifest does not prove the exact weak OwnedActors read canary.'
}
if (@($manifest.ownedPackages).Count -ne 1 -or
    [string]$manifest.ownedPackages[0] -cne '/Game/Mods/ElectrifiedBoat/ModActor' -or
    [string]$manifest.diagnostic -cne 'none' -or
    -not ([string]$manifest.steadyState).Contains('no actor Tick')) {
    throw 'Build manifest does not describe the exact one-package, no-diagnostic, no-Tick runtime.'
}

$expectedAutoload = 'entryClass: ' + $entryClass + [Environment]::NewLine
if ([IO.File]::ReadAllText($autoloadPath) -cne $expectedAutoload) {
    throw 'The .autoload sidecar does not name the exact ModActor entry class.'
}
$expectedPackages = @(Get-Content -LiteralPath $expectedListPath)
if ($expectedPackages.Count -ne 1 -or
    [string]$expectedPackages[0] -cne $expectedPackage) {
    throw 'Expected package list is not the exact one-package additive set.'
}

$containerResult = & (Join-Path $repoRoot 'tools/Test-VoyageContainer.ps1') `
    -Container $utoc `
    -ExpectedPackageList $expectedListPath `
    -OutputRoot (Join-Path $validationRoot 'container')
if ($containerResult.status -cne 'passed' -or
    $containerResult.packageSetMatches -ne $true) {
    throw "Container validation failed: $($containerResult.reportPath)"
}

$assetResultJson = & (Join-Path $repoRoot 'tools/Get-VoyageAssetJson.ps1') `
    -Query '/Game/Mods/ElectrifiedBoat/ModActor' `
    -Source Mod `
    -ModContainer $utoc `
    -ModInspectionRoot (Join-Path $validationRoot 'asset') `
    -AsJson
$assetResult = $assetResultJson | ConvertFrom-Json
$assetJson = [IO.File]::ReadAllText([string]$assetResult.jsonPath)
$stockResultJson = & (Join-Path $repoRoot 'tools/Get-VoyageAssetJson.ps1') `
    -Query '/Game/Blueprints/Modules/Utility/Wireless/BP_WallSocket_Electric' `
    -Source Game `
    -AsJson
$stockResult = $stockResultJson | ConvertFrom-Json
$stockJson = [IO.File]::ReadAllText([string]$stockResult.jsonPath)
$stockDocument = ConvertFrom-Json -InputObject $stockJson
$stockExports = @()
foreach ($stockExport in $stockDocument) {
    $stockExports += $stockExport
}
$stockClass = @($stockExports | Where-Object {
    $_.Type -ceq 'BlueprintGeneratedClass' -and
    $_.Name -ceq 'BP_WallSocket_Electric_C'
})
if ($stockClass.Count -ne 1) {
    throw 'Stock Electric Wall Socket generated class was not resolved exactly once.'
}
$stockViewProperty = @(($stockClass[0]).ChildProperties | Where-Object {
    $_.Name -ceq 'VoyageModuleSocketView' -and
    $_.Type -ceq 'ObjectProperty' -and
    $_.PropertyClass.ObjectName -ceq "Class'VoyageModuleSocketViewComponent'" -and
    $_.PropertyClass.ObjectPath -ceq '/Script/Voyage'
})
if ($stockViewProperty.Count -ne 1) {
    throw 'Stock generated class lacks the exact VoyageModuleSocketView instance property contract.'
}
if (@(($stockClass[0]).ChildProperties | Where-Object {
    $_.Name -ceq 'VoyageModuleSocketView_GEN_VARIABLE'
}).Count -ne 0) {
    throw 'Template object name was incorrectly accepted as a stock instance property.'
}
if ($assetJson.Contains('VoyageModuleSocketView_GEN_VARIABLE')) {
    throw 'Cooked ModActor references the SCS template name instead of the stock instance property VoyageModuleSocketView.'
}
foreach ($requiredToken in @(
    'OnActorAttached',
    'VoyagePersistentSubsystem',
    'HandleActorAttached',
    'GetRootPrimitiveComponent',
    'GetSecondaryGroupId',
    'RemoveSecondaryGroup',
    'AddExternalSocket',
    'RemoveExternalSocket',
    'SetSocketID',
    'Port',
    'bAutoInitialize',
    'bAddModuleRequirement',
    'bIsVirtual',
    'bIsVirtualGrouped',
    'bIsVirtualShareToGroup',
    'VoyageLevelInstanceComponent',
    'OwnedActors',
    'Boat_Electric',
    'BP_AttachmentVirtualSocketActor_Electricity_C',
    'GetParentModule',
    'GetModuleFromActor',
    'OnEndPlay',
    'HandleRegisteredActorEndPlay',
    'RegisteredExternalActors',
    'RegisteredExternalSockets',
    'RegisteredExpectedOwners',
    'SetActorTickEnabled',
    'Array_Contains',
    'PairedModule',
    'EX_PushExecutionFlow',
    'WallSocket',
    'Boat',
    'BP_WallSocket_Electric_C',
    'Child',
    'ParentComponent',
    'VoyageModuleSocketView'
)) {
    if (-not $assetJson.Contains($requiredToken)) {
        throw "Cooked ModActor is missing runtime contract token: $requiredToken"
    }
}

$assetDocument = ConvertFrom-Json -InputObject $assetJson
$exports = @()
foreach ($assetExport in $assetDocument) {
    $exports += $assetExport
}
$handleAttached = @($exports | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'HandleActorAttached'
})
if ($handleAttached.Count -ne 1) {
    throw 'Cooked ModActor does not contain exactly one production attach callback.'
}
$handleAttachedJson = $handleAttached[0] | ConvertTo-Json -Depth 100 -Compress
$processCalls = @($handleAttached[0].ScriptBytecode | Where-Object {
    $_.PSObject.Properties.Name -contains 'Function' -and
    $_.Function -ceq 'ProcessModule'
})
$processCallParameters = if ($processCalls.Count -eq 1) {
    @($processCalls[0].Parameters)
} else {
    @()
}
if ($processCalls.Count -ne 1 -or
    $processCallParameters.Count -ne 3 -or
    $processCallParameters[0].RValuePointer.ResolvedOwner.ObjectName -cne "Class'VoyageModuleActor'" -or
    $processCallParameters[0].RValuePointer.Path[0] -cne 'ModuleComponent' -or
    $processCallParameters[1].Variable.Property.PropertyClass.ObjectName -cne "BlueprintGeneratedClass'BP_WallSocket_Electric_C'" -or
    $processCallParameters[2].Variable.Property.PropertyClass.ObjectName -cne "Class'VoyageBoatPawn'" -or
    -not $handleAttachedJson.Contains('GetRootPrimitiveComponent') -or
    -not $handleAttachedJson.Contains('VoyageBoatPawn')) {
    throw 'HandleActorAttached must resolve and Boat-guard Module/WallActor/Boat before ProcessModule.'
}

$processModule = @($exports | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'ProcessModule'
})
if ($processModule.Count -ne 1) {
    throw 'Cooked ModActor does not contain exactly one ProcessModule function.'
}
$processModuleJson = $processModule[0] | ConvertTo-Json -Depth 100 -Compress
$processParameters = @($processModule[0].ChildProperties | Where-Object {
    $_.PSObject.Properties.Name -contains 'PropertyFlags' -and
    $_.PropertyFlags -match '(^| \| )Parm($| \| )'
})
$expectedProcessParameters = @(
    @{ Name = 'Module'; Type = 'ObjectProperty'; ClassName = "Class'VoyageModuleComponent'" },
    @{ Name = 'WallActor'; Type = 'ObjectProperty'; ClassName = "BlueprintGeneratedClass'BP_WallSocket_Electric_C'" },
    @{ Name = 'Boat'; Type = 'ObjectProperty'; ClassName = "Class'VoyageBoatPawn'" }
)
if ($processParameters.Count -ne $expectedProcessParameters.Count) {
    throw 'ProcessModule must expose exactly the reviewed Module/WallActor/Boat parameters.'
}
for ($processParameterIndex = 0; $processParameterIndex -lt $expectedProcessParameters.Count; $processParameterIndex++) {
    $actualProcessParameter = $processParameters[$processParameterIndex]
    $expectedProcessParameter = $expectedProcessParameters[$processParameterIndex]
    if ($actualProcessParameter.Name -cne $expectedProcessParameter.Name -or
        $actualProcessParameter.Type -cne $expectedProcessParameter.Type -or
        $actualProcessParameter.PropertyClass.ObjectName -cne $expectedProcessParameter.ClassName) {
        throw "ProcessModule parameter $processParameterIndex does not match the reviewed typed contract."
    }
}
$setIdIndex = $processModuleJson.IndexOf('ModuleSocketComponent:SetSocketID')
$addExternalIndex = $processModuleJson.IndexOf('VoyageModuleComponent:AddExternalSocket')
$managedGuardIndex = $processModuleJson.IndexOf('RegisteredExternalActors')
$processContractFailures = @()
if ($setIdIndex -lt 0) { $processContractFailures += 'SetSocketID missing' }
if ($addExternalIndex -lt 0) { $processContractFailures += 'AddExternalSocket missing' }
if ($setIdIndex -ge 0 -and $addExternalIndex -ge 0 -and
    $setIdIndex -ge $addExternalIndex) {
    $processContractFailures += 'SetSocketID is not before AddExternalSocket'
}
if ($managedGuardIndex -lt 0 -or
    ($setIdIndex -ge 0 -and $managedGuardIndex -ge $setIdIndex)) {
    $processContractFailures += 'managed registration guard is missing or late'
}
$parentResolverCalls = @($processModule[0].ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_Context' -and
    $_.ObjectExpression.Token -ceq 'EX_LocalVariable' -and
    $_.ObjectExpression.Variable.Property.PropertyClass.ObjectName -ceq
        "BlueprintGeneratedClass'BP_AttachmentVirtualSocketActor_Electricity_C'" -and
    $_.ContextExpression.Token -ceq 'EX_LocalVirtualFunction' -and
    $_.ContextExpression.Function -ceq 'GetParentModule'
})
if ($parentResolverCalls.Count -ne 1 -or
    @($parentResolverCalls[0].ContextExpression.Parameters).Count -ne 1 -or
    $parentResolverCalls[0].ContextExpression.Parameters[0].Variable.Property.Type -cne 'ObjectProperty' -or
    $parentResolverCalls[0].ContextExpression.Parameters[0].Variable.Property.PropertyClass.ObjectName -cne "Class'Actor'") {
    $processContractFailures += 'exact stock GetParentModule out-Actor dispatch missing'
}
$moduleResolverCalls = @($processModule[0].ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_LetObj' -and
    $_.Expression.Token -ceq 'EX_CallMath' -and
    $_.Expression.Function.ObjectName -ceq
        "Class'VoyageMiscBlueprintFunctionLibrary:GetModuleFromActor'"
})
if ($moduleResolverCalls.Count -ne 1 -or
    $moduleResolverCalls[0].Variable.Variable.Property.PropertyClass.ObjectName -cne "Class'VoyageModuleComponent'" -or
    @($moduleResolverCalls[0].Expression.Parameters).Count -ne 1 -or
    $moduleResolverCalls[0].Expression.Parameters[0].Variable.Property.PropertyClass.ObjectName -cne "Class'Actor'" -or
    $parentResolverCalls.Count -ne 1 -or
    $moduleResolverCalls[0].Expression.Parameters[0].Variable.Property.Name -cne
        $parentResolverCalls[0].ContextExpression.Parameters[0].Variable.Property.Name) {
    $processContractFailures += 'native GetModuleFromActor does not consume the exact stock parent Actor'
}
if ($processModuleJson.Contains('"Name":"SocketID"') -or
    $processModuleJson.Contains('"Path":["SocketID"]')) {
    $processContractFailures += 'direct SocketID property access'
}
foreach ($forbiddenProcessToken in @(
    'UpdateSecondaryGroup',
    'GetMasterModuleFromActor',
    'GetRootPrimitiveComponent',
    'ExternalCandidateOwnerCount',
    'ConnectionCable',
    'ConnectedToSocket',
    'EqualEqual_IntInt',
    '"Name":"ModuleOwner"',
    '"Path":["ModuleOwner"]',
    'ExternalSocketIdCollision',
    'Array_AddUnique',
    '"Name":"Sockets"',
    '"Path":["Sockets"]',
    'Array_RemoveItem',
    'MakeModuleSocketIOData',
    'EX_StructConst'
)) {
    if ($processModuleJson.Contains($forbiddenProcessToken)) {
        $processContractFailures += "forbidden token $forbiddenProcessToken"
    }
}
if ($processContractFailures.Count -ne 0) {
    throw ('ProcessModule contract failed: ' +
        ($processContractFailures -join '; '))
}
$processClassReadCount = @($processModule[0].ScriptBytecode | Where-Object {
    $_.Token -ceq 'EX_LetObj' -and
    $_.PSObject.Properties.Name -contains 'Expression' -and
    $_.Expression.PSObject.Properties.Name -contains 'Function' -and
    $_.Expression.Function.PSObject.Properties.Name -contains 'ObjectName' -and
    $_.Expression.Function.ObjectName -ceq "Class'GameplayStatics:GetObjectClass'"
}).Count
if ($processClassReadCount -ne 1) {
    throw "ProcessModule must class-check only external-port candidates, not resolve the wall actor again; observed $processClassReadCount GetObjectClass refs."
}
$readOwnedActor = @($exports | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'ReadOwnedActorAt'
})
if ($readOwnedActor.Count -ne 1) {
    throw 'Cooked ModActor does not contain exactly one weak OwnedActors read helper.'
}
$readOwnedActorJson = $readOwnedActor[0] | ConvertTo-Json -Depth 100 -Compress
if (-not $readOwnedActorJson.Contains('OwnedActors') -or
    -not $readOwnedActorJson.Contains('EX_ArrayGetByRef') -or
    $readOwnedActorJson.Contains('KismetArrayLibrary:Array_Get')) {
    throw 'ReadOwnedActorAt must compile to native weak-array EX_ArrayGetByRef without Array_Get.'
}
if (-not $processModuleJson.Contains('ReadOwnedActorAt') -or
    -not $processModuleJson.Contains('ExternalOwnedActorSnapshot')) {
    throw 'ProcessModule must consume weak OwnedActors only through the strong transient snapshot helper.'
}
foreach ($requiredProcessToken in @(
    'OwnedActors',
    'Boat_Electric',
    'GetParentModule',
    'GetModuleFromActor',
    'Port',
    'AddExternalSocket',
    'RegisteredExternalActors',
    'RegisteredExternalSockets',
    'RegisteredExpectedOwners'
)) {
    if (-not $processModuleJson.Contains($requiredProcessToken)) {
        throw "ProcessModule lacks external-port contract token: $requiredProcessToken"
    }
}
$arrayAddCount = ([regex]::Matches(
    $processModuleJson,
    'KismetArrayLibrary:Array_Add(?!Unique)')).Count
if ($arrayAddCount -ne 4 -or
    $processModuleJson.Contains('KismetArrayLibrary:Array_AddUnique')) {
    throw "ProcessModule must use one ordinary snapshot Array_Add plus three registry Array_Add calls and no Array_AddUnique; observed $arrayAddCount."
}

$endPlayCleanup = @($exports | Where-Object {
    $_.Type -ceq 'Function' -and $_.Name -ceq 'HandleRegisteredActorEndPlay'
})
if ($endPlayCleanup.Count -ne 1) {
    throw 'Cooked ModActor does not contain exactly one registered-socket EndPlay cleanup function.'
}
$endPlayCleanupJson = $endPlayCleanup[0] | ConvertTo-Json -Depth 100 -Compress
foreach ($requiredCleanupToken in @(
    'RegisteredExternalActors',
    'RegisteredExternalSockets',
    'RegisteredExpectedOwners',
    'ModuleOwner',
    'EqualEqual_ObjectObject',
    'RemoveExternalSocket',
    'Array_Remove'
)) {
    if (-not $endPlayCleanupJson.Contains($requiredCleanupToken)) {
        throw "EndPlay cleanup lacks owner-guard/parallel-registry token: $requiredCleanupToken"
    }
}
if ($endPlayCleanupJson.Contains('"MemberName":"Sockets"') -or
    $endPlayCleanupJson.Contains('"Name":"Sockets"')) {
    throw 'EndPlay cleanup must call RemoveExternalSocket rather than mutate a module Sockets array.'
}
foreach ($forbiddenToken in @(
    $legacyAuthoredIdentity,
    $legacyVirtualRoot,
    'SphereOverlapActors',
    'GetAllActorsOfClass',
    'OnModuleRegistered',
    'OnModuleUnregistered',
    'TrackedModules',
    'AppliedBoatModules',
    'UpdateSecondaryGroup',
    'GetAllActorsWithTag',
    '/Game/Mods/ElectrifiedBoat/BP_WallSocket_Electric',
    'WBP_FlowDiagnostic',
    'Diagnostic',
    'ReceiveTick',
    'K2_SetTimerDelegate',
    'K2_ClearTimerDelegate',
    '/Script/UMG',
    'WidgetBlueprintLibrary',
    'CreateWidget',
    'AddToViewport',
    'FlowDirection',
    'FlowStrength',
    'ResourcesTransferredLastFrame',
    'ResourceTransferPerSecond'
)) {
    if ($assetJson.Contains($forbiddenToken)) {
        throw "Cooked ModActor contains forbidden contract token: $forbiddenToken"
    }
}

$allFunctionJson = @($exports | Where-Object { $_.Type -ceq 'Function' }) |
    ConvertTo-Json -Depth 100 -Compress
$processCallCount = ([regex]::Matches(
    $allFunctionJson,
    '"Function":"ProcessModule"')).Count
if ($processCallCount -ne 1) {
    throw "Cooked ModActor must call ProcessModule exactly once from HandleActorAttached; observed $processCallCount."
}
$defaultObject = @($exports | Where-Object {
    $_.Name -ceq 'Default__ModActor_C' -and
    ([string]$_.Flags).Contains('RF_ClassDefaultObject')
})
$cookedTick = if ($defaultObject.Count -eq 1) {
    $defaultObject[0].Properties.PrimaryActorTick
} else {
    $null
}
$canEverTickOverride = if ($null -ne $cookedTick) {
    $cookedTick.PSObject.Properties['bCanEverTick']
} else {
    $null
}
$startWithTickOverride = if ($null -ne $cookedTick) {
    $cookedTick.PSObject.Properties['bStartWithTickEnabled']
} else {
    $null
}
# UE 5.8.2 AActor::InitializeDefaults sets bCanEverTick=false. The cooked
# serializer omits that inherited value; any explicit override must still be
# false. bStartWithTickEnabled defaults true, so require the cooked false
# override generated by this ModActor.
if ($defaultObject.Count -ne 1 -or
    $null -eq $cookedTick -or
    ($null -ne $canEverTickOverride -and
        $canEverTickOverride.Value -ne $false) -or
    $null -eq $startWithTickOverride -or
    $startWithTickOverride.Value -ne $false) {
    throw 'Cooked ModActor CDO must explicitly disable actor Tick and start-with-Tick.'
}

$result = [ordered]@{
    status = 'passed'
    packageRoot = $package
    containerReportPath = $containerResult.reportPath
    assetJsonPath = [string]$assetResult.jsonPath
    steamBuildId = $expectedSteamBuildId
    executableSha256 = $expectedExecutableSha256
    packageCount = 1
    gameplayValidated = $false
    saveReloadValidated = $false
    removalValidated = $false
}
$resultPath = Join-Path $validationRoot 'validation-result.json'
[IO.File]::WriteAllText(
    $resultPath,
    (($result | ConvertTo-Json -Depth 5) + [Environment]::NewLine),
    (New-Object Text.UTF8Encoding($false)))
[pscustomobject]$result
