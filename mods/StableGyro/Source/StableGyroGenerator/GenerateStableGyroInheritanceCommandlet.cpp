// HAND-WRITTEN BUILD TOOL SOURCE: generates the Gyro replacement child.
// Contracts revalidated for Steam build 25929985.
// Native altitude-assistance fields and SetBoolPropertyByName were separately
// revalidated for Steam build 25956314.
// The generator is editor-only and never ships in the mod container.

#include "GenerateStableGyroInheritanceCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "ActorLifecycleGraphNames.h"
#include "Components/ActorComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InterchangeManager.h"
#include "GameFramework/GameUserSettings.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_EnhancedInputAction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_Literal.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "StableGyroAssetNames.h"
#include "StableGyroSettings.h"
#include "VoyageModSettingsGraph.h"
#include "VoyageGameUserSettings.h"
#include "VoyageInputAction.h"
#include "VoyageMiscBlueprintFunctionLibrary.h"
#include "VoyageModuleSocketViewComponent.h"
#include "VoyageVehicleGyroCopter.h"
#include "VoyageVehiclePawn.h"

namespace
{
constexpr TCHAR BasePackage[] = TEXT("/Game/Blueprints/Vehicles/BP_GyroCopter_Possessable");
constexpr TCHAR ChildPackage[] = TEXT("/Game/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot");
constexpr TCHAR BaseAssetName[] = TEXT("BP_GyroCopter_Possessable");
constexpr TCHAR ChildAssetName[] = TEXT("BP_GyroCopter_StablePilot");
constexpr TCHAR StabilizationHelperPath[] = TEXT("/Game/Mods/StableGyro/ModActor.ModActor");
constexpr TCHAR PitchHelperPath[] = TEXT("/Game/Mods/StableGyro/PitchInputActor.PitchInputActor");
constexpr TCHAR HookCablePackage[] = TEXT("/Game/Data/Assets/Cable/DA_Cable_Hook");
constexpr TCHAR HookCableObjectPath[] =
    TEXT("/Game/Data/Assets/Cable/DA_Cable_Hook.DA_Cable_Hook");
constexpr TCHAR MooringSocketDataPath[] =
    TEXT("/Game/Data/Assets/ModuleSockets/DA_Socket_MooringSmall.DA_Socket_MooringSmall");
constexpr TCHAR CableConnectSoundPath[] =
    TEXT("/Game/Audio/SFX/World/Systems/Sys_Cables/CUE_Sys_Cable_Electric_Connect.CUE_Sys_Cable_Electric_Connect");
constexpr TCHAR CableDisconnectSoundPath[] =
    TEXT("/Game/Audio/SFX/World/Systems/Sys_Cables/CUE_Sys_Cable_Electric_Disconnect.CUE_Sys_Cable_Electric_Disconnect");
constexpr TCHAR SwitchDirectionSoundPath[] =
    TEXT("/Game/Audio/SFX/World/Systems/Sys_Device_Gen/CUE_Sys_Gen_Lever_Up.CUE_Sys_Gen_Lever_Up");
constexpr TCHAR MooringMaterialPackage[] =
    TEXT("/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1");
constexpr TCHAR MooringMaterialObjectPath[] =
    TEXT("/Game/AssetSets/Vehicles/Gyrocopter/Materials/Gyrocopter_Seat_p1.Gyrocopter_Seat_p1");
constexpr TCHAR PositiveMooringMeshPackage[] =
    TEXT("/Game/Mods/StableGyro/StableGyroMooringPlatePositiveY/StaticMeshes/StableGyroMooringPlatePositiveY");
constexpr TCHAR NegativeMooringMeshPackage[] =
    TEXT("/Game/Mods/StableGyro/StableGyroMooringPlateNegativeY/StaticMeshes/StableGyroMooringPlateNegativeY");
const FName BaseBlueprintName(TEXT("GenerateStableGyroInheritanceBase"));
const FName ChildBlueprintName(TEXT("GenerateStableGyroInheritanceChild"));
const FName StabilizationHelperComponentName(TEXT("StableGyroHelper"));
const FName PitchHelperComponentName(TEXT("StableGyroPitchInputHelper"));
const FName RearLeftMooringComponentName(TEXT("StableGyroMooringSocketRearLeft"));
const FName RearRightMooringComponentName(TEXT("StableGyroMooringSocketRearRight"));
const FName FrontPositiveMooringPlateName(TEXT("StableGyroMooringPlateFrontPositiveY"));
const FName FrontNegativeMooringPlateName(TEXT("StableGyroMooringPlateFrontNegativeY"));
const FName RearPositiveMooringPlateName(TEXT("StableGyroMooringPlateRearPositiveY"));
const FName RearNegativeMooringPlateName(TEXT("StableGyroMooringPlateRearNegativeY"));
const FName VehicleMeshComponentName(TEXT("VehicleMesh"));
const FName ProvidedActionsFunctionName(TEXT("GetProvidedActionsBP"));
const FName CargoAvailabilityLocal(TEXT("StableGyroCargoAvailable"));
const FName MooringVehicleComponentsLocal(TEXT("StableGyroMooringVehicleComponents"));
const FName MooringVehicleMeshLocal(TEXT("StableGyroMooringVehicleMesh"));
const FName DisabledMooringLeftChildrenLocal(TEXT("StableGyroDisabledMooringLeftChildren"));
const FName DisabledMooringRightChildrenLocal(TEXT("StableGyroDisabledMooringRightChildren"));
const FName CargoPressArmed(TEXT("StableGyroCargoPressArmed"));
const FName CargoPressFired(TEXT("StableGyroCargoPressFired"));
const FName CargoControllerChangedCallbackName(TEXT("HandleCargoControllerChanged"));
const FName ReloadSettingsFunctionName(TEXT("ReloadSettings"));
const FName ControllerChangedCallbackName(TEXT("HandleControllerChanged"));
const FName CollisionHitCallbackName(TEXT("HandleCollisionHit"));
const FName AltitudeLockActive(TEXT("AltitudeLockActive"));
const FName AltitudeExactHoldActive(TEXT("AltitudeExactHoldActive"));
const FName SpaceWasHeld(TEXT("SpaceWasHeld"));
const FName DescentWasHeld(TEXT("DescentWasHeld"));
const FName AltitudeBrakingActive(TEXT("AltitudeBrakingActive"));
const FName AltitudeDescentBrakingActive(TEXT("AltitudeDescentBrakingActive"));
const FName LockedAltitude(TEXT("LockedAltitude"));
const FName RootComponent(TEXT("RootComponent"));
const FName TiltForwardInput(TEXT("TiltForwardInput"));
const FName TiltInput(TEXT("TiltInput"));
const FName PropellerVelocity(TEXT("PropellerVelocity"));
const FName Collective(TEXT("Collective"));
const FName HoverPitchScale(TEXT("HoverPitchScale"));
const FName CurrentThrottle(TEXT("CurrentThrottle"));
const FName EngineRunning(TEXT("bEngineRunning"));
const FName CollisionReleaseUntilTime(TEXT("CollisionReleaseUntilTime"));
const FName OwnedPrimitiveComponents(TEXT("OwnedPrimitiveComponents"));
const FName PreviousCorrectedHorizontalVelocity(TEXT("PreviousCorrectedHorizontalVelocity"));
const FName HorizontalVelocitySampleValid(TEXT("HorizontalVelocitySampleValid"));
const FName PreviousCorrectedVerticalVelocity(TEXT("PreviousCorrectedVerticalVelocity"));
const FName VerticalVelocitySampleValid(TEXT("VerticalVelocitySampleValid"));
const FName IntegratedPitch(TEXT("IntegratedPitch"));
const FName CachedGyro(TEXT("CachedGyro"));
const FName CachedStabilizationHelper(TEXT("CachedStabilizationHelper"));
const FName OwnedChildActorComponents(TEXT("OwnedChildActorComponents"));
const FName NativeAltitudeOwnershipActive(TEXT("NativeAltitudeOwnershipActive"));
const FName OriginalAutoHoverCollective(TEXT("OriginalAutoHoverCollective"));
const FName OriginalAltitudeAssist(TEXT("OriginalAltitudeAssist"));
const FName DropCargoTriggerTime(TEXT("DropCargoTriggerTime"));
const FName FourPortSettingsLoaded(TEXT("StableGyroFourPortSettingsLoaded"));
const FName EnsureFourPortSettingsCached(TEXT("EnsureFourPortSettingsCached"));
const FName GetFourPortSetting(TEXT("GetFourPortSetting"));
const FName CachedRearLeftMooringSocket(TEXT("StableGyroRearLeftMooringSocket"));
const FName CachedRearRightMooringSocket(TEXT("StableGyroRearRightMooringSocket"));
const FName DisabledMooringCleanupQueued(TEXT("StableGyroDisabledMooringCleanupQueued"));
const FName DisableOwnedMooringObjectFunction(TEXT("DisableOwnedMooringObject"));
const FName RunDisabledMooringCleanupFunction(TEXT("RunDisabledMooringCleanup"));
const FName QueueDisabledMooringCleanupFunction(TEXT("QueueDisabledMooringCleanup"));
const FName DisabledMooringConnectionCallback(TEXT("HandleDisabledMooringConnectionChanged"));
const FName AutoHoverCollective(TEXT("bAutoHoverCollective"));
const FName AltitudeAssist(TEXT("bAltitudeAssist"));
const FName RestoreNativeAltitudeOwnershipFunction(TEXT("RestoreNativeAltitudeOwnership"));
const FName UpdateNativeAltitudeOwnershipFunction(TEXT("UpdateNativeAltitudeOwnership"));
namespace Settings = StableGyroSettings;

namespace MooringLayout
{
constexpr TCHAR HookCableAssetName[] = TEXT("DA_Cable_Hook");
constexpr TCHAR MaterialAssetName[] = TEXT("Gyrocopter_Seat_p1");
constexpr TCHAR ImportDestination[] = TEXT("/Game/Mods/StableGyro");
constexpr TCHAR PositiveGlbParameter[] = TEXT("MooringPositiveYGlb=");
constexpr TCHAR NegativeGlbParameter[] = TEXT("MooringNegativeYGlb=");
constexpr TCHAR MooringResourceEnum[] = TEXT("EModuleResourceType::Mooring");
constexpr TCHAR StockPlateObjectName[] = TEXT("MooringPlate");
constexpr TCHAR StockPositiveSocketObjectName[] = TEXT("MooringSocket");
constexpr TCHAR StockNegativeSocketObjectName[] = TEXT("MooringSocket1");
constexpr TCHAR StockPositiveSocketLocationDefault[] =
    TEXT("(X=78.782426,Y=50.862349,Z=-32.989642)");
constexpr TCHAR StockNegativeSocketLocationDefault[] =
    TEXT("(X=78.783752,Y=-50.714545,Z=-32.805209)");
constexpr TCHAR ManualAttachmentPin[] = TEXT("bManualAttachment");
constexpr TCHAR RelativeTransformPin[] = TEXT("RelativeTransform");
constexpr TCHAR DeferredFinishPin[] = TEXT("bDeferredFinish");
constexpr TCHAR TemplateNamePin[] = TEXT("TemplateName");
constexpr TCHAR ComponentPin[] = TEXT("Component");
constexpr TCHAR ParentPin[] = TEXT("Parent");
constexpr TCHAR SocketNamePin[] = TEXT("SocketName");
constexpr TCHAR LocationRulePin[] = TEXT("LocationRule");
constexpr TCHAR RotationRulePin[] = TEXT("RotationRule");
constexpr TCHAR ScaleRulePin[] = TEXT("ScaleRule");
constexpr TCHAR WeldBodiesPin[] = TEXT("bWeldSimulatedBodies");
constexpr TCHAR KeepRelativeRule[] = TEXT("KeepRelative");
constexpr TCHAR VectorTripleFormat[] = TEXT("%.9f,%.9f,%.9f");
constexpr TCHAR NoSocket[] = TEXT("None");
const FName LocationPin(TEXT("Location"));
const FName RotationPin(TEXT("Rotation"));
const FName ScalePin(TEXT("Scale"));
constexpr uint32 RearLeftSocketId = 2056977161u;
constexpr uint32 RearRightSocketId = 2056977162u;
constexpr int32 PositiveSourceTriangleCount = 6895;
constexpr int32 NegativeSourceTriangleCount = 6895;
constexpr int32 RuntimeTriangleCountPerSide = 6852;
constexpr float IntegratedCableLength = 1000.0f;
const FName PortProperty(TEXT("Port"));
const FName UserConstructionScriptFunction(TEXT("UserConstructionScript"));
const FName SetVisibilityFunction(TEXT("SetVisibility"));
const FName NewVisibility(TEXT("bNewVisibility"));
const FName PropagateVisibilityToChildren(TEXT("bPropagateToChildren"));
const FName SetHiddenInGameFunction(TEXT("SetHiddenInGame"));
const FName NewHidden(TEXT("NewHidden"));
const FName SetCollisionEnabledFunction(TEXT("SetCollisionEnabled"));
const FName CollisionEnabledType(TEXT("NewType"));
const FName SetGenerateOverlapEventsFunction(TEXT("SetGenerateOverlapEvents"));
const FName GenerateOverlapEvents(TEXT("bInGenerateOverlapEvents"));
const FName GetChildrenComponentsFunction(TEXT("GetChildrenComponents"));
const FName IncludeAllDescendants(TEXT("bIncludeAllDescendants"));
const FName Children(TEXT("Children"));
const FName TargetObject(TEXT("TargetObject"));
const FName SocketConnectionState(TEXT("bSocketConnection"));
const FName TimerObject(TEXT("Object"));
const FName TimerFunctionName(TEXT("FunctionName"));
const FName MaterialSlotName(MaterialAssetName);
const FVector RearLeftSocketLocation(49.075660, 83.677528, -41.093475);
const FVector RearRightSocketLocation(49.076986, -83.529724, -40.909042);
const FRotator RearLeftSocketRotation(9.615132, -50.542805, -119.624176);
const FRotator RearRightSocketRotation(-8.652774, -130.16699, -118.37153);
const FVector FrontPositivePlateLocation(3.300752, -3.646131, 0.900426);
const FVector FrontNegativePlateLocation(3.300752, 3.646131, 0.900426);
const FVector RearPositivePlateLocation(-26.406014, 29.169048, -7.203407);
const FVector RearNegativePlateLocation(-26.406014, -29.169048, -7.203407);
}

namespace MooringSerializationFields
{
const FName ItemTransferTimer(TEXT("ItemTransferTimer"));
const FName SocketId(TEXT("SocketID"));
const FName LeakMultiplier(TEXT("LeakMultiplier"));
const FName AddModuleRequirement(TEXT("bAddModuleRequirement"));
const FName CustomRequirement(TEXT("CustomRequirement"));
const FName AutoInitialize(TEXT("bAutoInitialize"));
const FName AcceptsSignals(TEXT("bAcceptsSignals"));
const FName OwningModule(TEXT("OwningModule"));
const FName Standalone(TEXT("bStandalone"));
const FName IsVirtual(TEXT("bIsVirtual"));
const FName IsVirtualGrouped(TEXT("bIsVirtualGrouped"));
const FName IsVirtualShareToGroup(TEXT("bIsVirtualShareToGroup"));
const FName ConnectSound(TEXT("ConnectSound"));
const FName DisconnectSound(TEXT("DisconnectSound"));
const FName SwitchDirectionSound(TEXT("SwitchDirectionSound"));
const FName SocketConnectionChanged(TEXT("OnSocketConnectionChanged"));
const FName ResourcesTransferredLastFrame(TEXT("ResourcesTransferredLastFrame"));
const FName ResourcesTransferredLastFrameAbsolute(TEXT("ResourcesTransferredLastFrameAbsolute"));
const FName ResourcesTransferredLastFrameType(TEXT("ResourcesTransferredLastFrameType"));
const FName ResourceTransferPerSecond(TEXT("ResourceTransferPerSecond"));
const FName IntegratedCable(TEXT("IntegratedCable"));
const FName IntegratedCableLength(TEXT("IntegratedCableLength"));
const FName IntegratedCablePlaceholderMesh(TEXT("IntegratedCablePlaceholderMesh"));
const FName ConnectedToSocket(TEXT("ConnectedToSocket"));
const FName ModuleOwner(TEXT("ModuleOwner"));
const FName ModuleOwnerOriginal(TEXT("ModuleOwnerOriginal"));
const FName ConnectionCable(TEXT("ConnectionCable"));
const FName SocketCollisionBox(TEXT("SocketCollisionBox"));
const FName NiagaraFx(TEXT("NiagaraFX"));
const FName NiagaraFxLeak(TEXT("NiagaraFXLeak"));
const FName NiagaraFxLeakSystem(TEXT("NiagaraFXLeakSystem"));
const FName NiagaraFxSystem(TEXT("NiagaraFXSystem"));
const FName ConnectedSocketsCache(TEXT("ConnectedSocketsCache"));
const FName Socket(TEXT("Socket"));
const FName Mesh(TEXT("mesh"));
const FName DataAsset(TEXT("DataAsset"));
const FName MultiplyScale(TEXT("bMultiplyScale"));
const FName Type(TEXT("Type"));
const FName Bandwidth(TEXT("Bandwidth"));
const FName IgnoreCableBandwidth(TEXT("bIgnoreCableBandwidth"));
const FName DefaultDirection(TEXT("DefaultDirection"));
const FName BalanceResourceLevels(TEXT("bBalanceResourceLevels"));
const FName MatchByResourceCategory(TEXT("bMatchByResourceCategory"));
const FName ResourceCategory(TEXT("ResourceCategory"));
const FName AdditionalAcceptedResourceTypes(TEXT("AdditionalAcceptedResourceTypes"));
}

namespace ArcadeAltitude
{
constexpr TCHAR Disabled[] = TEXT("false");
constexpr TCHAR Enabled[] = TEXT("true");
constexpr TCHAR Zero[] = TEXT("0.0");
constexpr TCHAR PercentScale[] = TEXT("100.0");
constexpr TCHAR SpaceKey[] = TEXT("SpaceBar");
constexpr TCHAR DescentKey[] = TEXT("LeftControl");
constexpr TCHAR NoBone[] = TEXT("None");
const FName NewLocation(TEXT("NewLocation"));
const FName Sweep(TEXT("bSweep"));
const FName Teleport(TEXT("bTeleport"));
const FName InVector(TEXT("InVec"));
const FName VectorX(TEXT("X"));
const FName VectorY(TEXT("Y"));
const FName VectorZ(TEXT("Z"));
const FName BoneName(TEXT("BoneName"));
const FName AddToCurrent(TEXT("bAddToCurrent"));
const FName NewVelocity(TEXT("NewVel"));
const FName SweepHitResult(TEXT("SweepHitResult"));
const FName BlockingHit(TEXT("bBlockingHit"));
const FName InitialOverlap(TEXT("bInitialOverlap"));
}

namespace CollisionRelease
{
constexpr TCHAR GraceSeconds[] = TEXT("0.25");
constexpr TCHAR StandardMacros[] =
    TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
const FName ForEachLoop(TEXT("ForEachLoop"));
const FName Array(TEXT("Array"));
const FName ArrayElement(TEXT("Array Element"));
const FName ComponentClass(TEXT("ComponentClass"));
const FName NotifyCollision(TEXT("bNewNotifyRigidBodyCollision"));
const FName SelfActor(TEXT("SelfActor"));
const FName OtherActor(TEXT("OtherActor"));
const FName NormalImpulse(TEXT("NormalImpulse"));
const FName Hit(TEXT("Hit"));
const FName WorldContextObject(TEXT("WorldContextObject"));
}

namespace NativeAltitudeOwnership
{
const FName SetBoolPropertyByName(TEXT("SetBoolPropertyByName"));
const FName PropertyName(TEXT("PropertyName"));
const FName ComponentClass(TEXT("ComponentClass"));
const FName Array(TEXT("Array"));
const FName ArrayElement(TEXT("Array Element"));
}

namespace HoverPitchReset
{
constexpr TCHAR DefaultScale[] = TEXT("1.0");
const FName SetDoublePropertyByName(TEXT("SetDoublePropertyByName"));
const FName PropertyName(TEXT("PropertyName"));
}

namespace ArcadeHorizontal
{
constexpr TCHAR NeutralTolerance[] = TEXT("0.0001");
constexpr TCHAR GrowthTolerance[] = TEXT("0.01");
constexpr TCHAR ZeroVectorDefault[] = TEXT("(X=0.000000,Y=0.000000,Z=0.000000)");
const FName Tolerance(TEXT("Tolerance"));
const FName Current(TEXT("Current"));
const FName Target(TEXT("Target"));
const FName DeltaTime(TEXT("DeltaTime"));
const FName InterpSpeed(TEXT("InterpSpeed"));
}

namespace PinNames
{
using BlueprintGraphNames::Pins::FirstArrayElement;
using BlueprintGraphNames::Pins::ReturnValue;
const FName InputAction(TEXT("InputAction"));
const FName Name(TEXT("Name"));
const FName Category(TEXT("Category"));
const FName Text(TEXT("Text"));
const FName Enabled(TEXT("bEnabled"));
const FName Priority(TEXT("Priority"));
const FName TriggerTime(TEXT("TriggerTime"));
const FName Type(TEXT("Type"));
const FName LanguageType(TEXT("LanguageType"));
const FName TargetArray(TEXT("TargetArray"));
const FName SourceArray(TEXT("SourceArray"));
}
namespace P = BlueprintGraphNames::Pins;
namespace SelectPins = BlueprintGraphNames::Pins::Select;

namespace ProvidedActionDefaults
{
constexpr TCHAR Enabled[] = TEXT("true");
constexpr TCHAR Priority[] = TEXT("10");
constexpr TCHAR Type[] = TEXT("EPlayerInputInterfaceActionType::Central");
constexpr TCHAR RussianLanguage[] = TEXT("EVoyageLanguageType::Russian");
}

namespace CargoAction
{
constexpr TCHAR DropCargoActionProperty[] = TEXT("DropCargoAction");
constexpr TCHAR Disabled[] = TEXT("false");
constexpr TCHAR Enabled[] = TEXT("true");
constexpr TCHAR DestroyCable[] = TEXT("false");
const FName SetObjectPropertyByName(TEXT("SetObjectPropertyByName"));
const FName ComponentClass(TEXT("ComponentClass"));
const FName Array(TEXT("Array"));
const FName ArrayElement(TEXT("Array Element"));
const FName ResourceType(TEXT("ResourceType"));
const FName Socket(TEXT("Socket"));
const FName Destroy(TEXT("bDestroy"));
const FName Started(TEXT("Started"));
const FName Triggered(TEXT("Triggered"));
const FName Completed(TEXT("Completed"));
const FName Canceled(TEXT("Canceled"));
const FName ElapsedSeconds(TEXT("ElapsedSeconds"));
const FName ActionDescription(TEXT("ActionDescription"));
}

template <typename NodeType>
NodeType* FinishNode(NodeType* Node, UEdGraph* Graph, int32 X, int32 Y)
{
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    Node->SetFlags(RF_Transactional);
    Node->AllocateDefaultPins();
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    return Node;
}

UK2Node_CallFunction* AddCall(
    UEdGraph* Graph, UClass* Owner, const FName Function, int32 X, int32 Y)
{
    UFunction* Reflected = Owner->FindFunctionByName(Function);
    checkf(Reflected, TEXT("Missing reflected function %s.%s"),
        *Owner->GetPathName(), *Function.ToString());
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Reflected);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* AddRead(
    UEdGraph* Graph, const FName Name, UClass* Owner, int32 X, int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    if (Owner) Node->VariableReference.SetExternalMember(Name, Owner);
    else Node->VariableReference.SetSelfMember(Name);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* AddWrite(
    UEdGraph* Graph, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    Node->VariableReference.SetSelfMember(Name);
    return FinishNode(Node, Graph, X, Y);
}

bool Connect(UEdGraphPin* From, UEdGraphPin* To)
{
    if (!GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not link %s.%s to %s.%s"),
            *From->GetOwningNode()->GetName(), *From->PinName.ToString(),
            *To->GetOwningNode()->GetName(), *To->PinName.ToString());
        return false;
    }
    return true;
}

void SetDefault(UEdGraphNode* Node, const FName Name, const TCHAR* Value)
{
    UEdGraphPin* Pin = Node->FindPin(Name);
    checkf(Pin, TEXT("Missing pin '%s' on node '%s'"),
        *Name.ToString(), *Node->GetName());
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Pin, Value);
}

UEdGraphPin* RequirePin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Pin = Node->FindPin(Name);
    checkf(Pin, TEXT("Missing pin '%s' on node '%s'"), *Name.ToString(), *Node->GetName());
    return Pin;
}

UEdGraphPin* FindOutputPin(UEdGraphNode* Node)
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output) return Pin;
    }
    return nullptr;
}

UEdGraphPin* FindInputPin(UEdGraphNode* Node)
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input) return Pin;
    }
    return nullptr;
}

UEdGraphPin* RequireExecInputPin(UEdGraphNode* Node)
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input &&
            Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) return Pin;
    }
    checkf(false, TEXT("Missing execution input on node '%s'"), *Node->GetName());
    return nullptr;
}

FMulticastDelegateProperty* ControllerChangedDelegateProperty()
{
    FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(
        APawn::StaticClass(), BlueprintGraphNames::ActorDelegates::ReceiveControllerChanged);
    check(Property && Property->SignatureFunction &&
        Property->SignatureFunction->GetPathName() ==
            BlueprintGraphNames::ActorDelegates::PawnControllerChangedSignaturePath &&
        Property->SignatureFunction->NumParms == 3);
    FObjectProperty* Pawn = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, BlueprintGraphNames::Pins::Pawn);
    FObjectProperty* OldController = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, ActorLifecycleGraphNames::OldController);
    FObjectProperty* NewController = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, ActorLifecycleGraphNames::NewController);
    check(Pawn && Pawn->HasAnyPropertyFlags(CPF_Parm) &&
        Pawn->PropertyClass == APawn::StaticClass() &&
        OldController && OldController->HasAnyPropertyFlags(CPF_Parm) &&
        OldController->PropertyClass == AController::StaticClass() &&
        NewController && NewController->HasAnyPropertyFlags(CPF_Parm) &&
        NewController->PropertyClass == AController::StaticClass());
    return Property;
}

FMulticastDelegateProperty* CollisionHitDelegateProperty()
{
    FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(
        AActor::StaticClass(), GET_MEMBER_NAME_CHECKED(AActor, OnActorHit));
    check(Property && Property->SignatureFunction &&
        Property->SignatureFunction->NumParms == 4);
    FObjectProperty* SelfActor = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, CollisionRelease::SelfActor);
    FObjectProperty* OtherActor = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, CollisionRelease::OtherActor);
    FStructProperty* NormalImpulse = FindFProperty<FStructProperty>(
        Property->SignatureFunction, CollisionRelease::NormalImpulse);
    FStructProperty* Hit = FindFProperty<FStructProperty>(
        Property->SignatureFunction, CollisionRelease::Hit);
    check(SelfActor && SelfActor->HasAnyPropertyFlags(CPF_Parm) &&
        SelfActor->PropertyClass == AActor::StaticClass() &&
        OtherActor && OtherActor->HasAnyPropertyFlags(CPF_Parm) &&
        OtherActor->PropertyClass == AActor::StaticClass() &&
        NormalImpulse && NormalImpulse->HasAnyPropertyFlags(CPF_Parm) &&
        NormalImpulse->Struct == TBaseStructure<FVector>::Get() &&
        Hit && Hit->HasAnyPropertyFlags(CPF_Parm) &&
        Hit->Struct == FHitResult::StaticStruct());
    return Property;
}

FMulticastDelegateProperty* SocketConnectionDelegateProperty()
{
    FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(
        UModuleSocketComponent::StaticClass(),
        MooringSerializationFields::SocketConnectionChanged);
    if (!Property || !Property->SignatureFunction ||
        Property->SignatureFunction->GetName() !=
            TEXT("VoyageSocketConnectionDelegate__DelegateSignature") ||
        Property->SignatureFunction->NumParms != 1)
        return nullptr;
    FBoolProperty* State = FindFProperty<FBoolProperty>(
        Property->SignatureFunction, MooringLayout::SocketConnectionState);
    return State && State->HasAnyPropertyFlags(CPF_Parm) ? Property : nullptr;
}

bool ValidateNativeAltitudeOwnershipContracts()
{
    for (const FName PropertyName : {AutoHoverCollective, AltitudeAssist})
    {
        const FBoolProperty* Property = FindFProperty<FBoolProperty>(
            AVoyageVehicleGyroCopter::StaticClass(), PropertyName);
        if (!Property || !Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            !Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly) ||
            Property->HasAnyPropertyFlags(CPF_SaveGame | CPF_Transient)) return false;
    }
    UFunction* Setter = UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        NativeAltitudeOwnership::SetBoolPropertyByName);
    if (!Setter || Setter->GetOuterUClass() != UKismetSystemLibrary::StaticClass() ||
        !Setter->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Static)) return false;
    const FObjectProperty* Object = FindFProperty<FObjectProperty>(Setter, P::Object);
    const FNameProperty* Name = FindFProperty<FNameProperty>(
        Setter, NativeAltitudeOwnership::PropertyName);
    const FBoolProperty* Value = FindFProperty<FBoolProperty>(Setter, P::Value);
    return Object && Object->PropertyClass == UObject::StaticClass() && Name && Value &&
        Object->HasAnyPropertyFlags(CPF_Parm) && Name->HasAnyPropertyFlags(CPF_Parm) &&
        Value->HasAnyPropertyFlags(CPF_Parm);
}

bool ValidateHoverPitchResetContracts()
{
    const FFloatProperty* Scale = FindFProperty<FFloatProperty>(
        AVoyageVehicleGyroCopter::StaticClass(), HoverPitchScale);
    if (!Scale || !Scale->HasAllPropertyFlags(
            CPF_BlueprintVisible | CPF_BlueprintReadOnly | CPF_SaveGame |
            CPF_NativeAccessSpecifierProtected) ||
        Scale->HasAnyPropertyFlags(CPF_Transient)) return false;

    UFunction* Setter = UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        HoverPitchReset::SetDoublePropertyByName);
    if (!Setter || Setter->GetOuterUClass() != UKismetSystemLibrary::StaticClass() ||
        !Setter->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Static)) return false;
    const FObjectProperty* Object = FindFProperty<FObjectProperty>(Setter, P::Object);
    const FNameProperty* Name = FindFProperty<FNameProperty>(
        Setter, HoverPitchReset::PropertyName);
    const FDoubleProperty* Value = FindFProperty<FDoubleProperty>(Setter, P::Value);
    return Object && Object->PropertyClass == UObject::StaticClass() && Name && Value &&
        Object->HasAnyPropertyFlags(CPF_Parm) && Name->HasAnyPropertyFlags(CPF_Parm) &&
        Value->HasAnyPropertyFlags(CPF_Parm);
}

UEdGraph* StandardForEachLoop()
{
    UBlueprint* Macros = LoadObject<UBlueprint>(nullptr, CollisionRelease::StandardMacros);
    check(Macros);
    for (UEdGraph* Candidate : Macros->MacroGraphs)
    {
        if (Candidate && Candidate->GetFName() == CollisionRelease::ForEachLoop)
            return Candidate;
    }
    checkNoEntry();
    return nullptr;
}

bool AddCollisionHitCallback(UBlueprint* Blueprint)
{
    FMulticastDelegateProperty* DelegateProperty = CollisionHitDelegateProperty();
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, CollisionHitCallbackName, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, Graph, true, DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            Entry = Candidate;
    if (!Entry) return false;
    RequirePin(Entry, P::Then)->BreakAllPinLinks();

    UK2Node_CallFunction* IsExternalHit = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_ObjectObject), 220, -620);
    UK2Node_IfThenElse* ExternalHitBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 450, -700);
    UK2Node_Self* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 450, -430);
    UK2Node_CallFunction* GameTime = AddCall(
        Graph, UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetTimeSeconds), 680, -430);
    UK2Node_CallFunction* Deadline = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), 910, -430);
    SetDefault(Deadline, BlueprintGraphNames::Pins::Binary::RightOperand,
        CollisionRelease::GraceSeconds);
    UK2Node_VariableSet* StoreDeadline = AddWrite(
        Graph, CollisionReleaseUntilTime, 680, -700);

    bool Ok = true;
    UEdGraphPin* Tail = RequirePin(StoreDeadline, P::Then);
    auto Reset = [&](const FName Field, const TCHAR* Value, const int32 X)
    {
        UK2Node_VariableSet* Set = AddWrite(Graph, Field, X, -700);
        SetDefault(Set, Field, Value);
        Ok &= Connect(Tail, RequirePin(Set, P::Execute));
        Tail = RequirePin(Set, P::Then);
    };
    Reset(AltitudeLockActive, ArcadeAltitude::Disabled, 910);
    Reset(AltitudeExactHoldActive, ArcadeAltitude::Disabled, 1140);
    Reset(SpaceWasHeld, ArcadeAltitude::Disabled, 1370);
    Reset(DescentWasHeld, ArcadeAltitude::Disabled, 1600);
    Reset(AltitudeBrakingActive, ArcadeAltitude::Disabled, 1830);
    Reset(AltitudeDescentBrakingActive, ArcadeAltitude::Disabled, 2060);
    Reset(HorizontalVelocitySampleValid, ArcadeAltitude::Disabled, 2290);
    Reset(VerticalVelocitySampleValid, ArcadeAltitude::Disabled, 2520);

    Ok &= Connect(RequirePin(Entry, P::Then), RequirePin(ExternalHitBranch, P::Execute));
    Ok &= Connect(RequirePin(Entry, CollisionRelease::OtherActor),
        RequirePin(IsExternalHit, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(Entry, CollisionRelease::SelfActor),
        RequirePin(IsExternalHit, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(IsExternalHit, P::ReturnValue),
        RequirePin(ExternalHitBranch, P::Condition));
    Ok &= Connect(RequirePin(ExternalHitBranch, P::Then),
        RequirePin(StoreDeadline, P::Execute));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(GameTime, CollisionRelease::WorldContextObject));
    Ok &= Connect(RequirePin(GameTime, P::ReturnValue),
        RequirePin(Deadline, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(Deadline, P::ReturnValue),
        RequirePin(StoreDeadline, CollisionReleaseUntilTime));
    return Ok;
}

bool AddControllerChangedCallback(UBlueprint* Blueprint, const bool PitchHelper)
{
    FMulticastDelegateProperty* DelegateProperty = ControllerChangedDelegateProperty();
    UFunction* Reload = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(ReloadSettingsFunctionName) : nullptr;
    if (!Reload || Reload->GetOuterUClass() != Blueprint->GeneratedClass) return false;
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, ControllerChangedCallbackName, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, Graph, true, DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            Entry = Candidate;
    if (!Entry) return false;
    UEdGraphPin* Tail = RequirePin(Entry, P::Then);
    Tail->BreakAllPinLinks();
    bool Ok = true;
    if (PitchHelper)
    {
        UFunction* Restore = Blueprint->GeneratedClass
            ? Blueprint->GeneratedClass->FindFunctionByName(
                RestoreNativeAltitudeOwnershipFunction)
            : nullptr;
        if (!Restore || Restore->GetOuterUClass() != Blueprint->GeneratedClass) return false;
        UK2Node_CallFunction* RestoreCall = AddCall(
            Graph, Blueprint->GeneratedClass, RestoreNativeAltitudeOwnershipFunction,
            220, -900);
        Ok &= Connect(Tail, RequirePin(RestoreCall, P::Execute));
        Tail = RequirePin(RestoreCall, P::Then);
    }
    auto Reset = [&](const FName Field, const TCHAR* Value, const int32 Y)
    {
        UK2Node_VariableSet* Set = AddWrite(Graph, Field, 220, Y);
        SetDefault(Set, Field, Value);
        Ok &= Connect(Tail, RequirePin(Set, P::Execute));
        Tail = RequirePin(Set, P::Then);
    };
    int32 ResetY = -700;
    if (PitchHelper)
    {
        Reset(IntegratedPitch, ArcadeAltitude::Zero, ResetY);
    }
    else
    {
        Reset(AltitudeLockActive, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(AltitudeExactHoldActive, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(SpaceWasHeld, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(DescentWasHeld, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(AltitudeBrakingActive, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(AltitudeDescentBrakingActive, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(LockedAltitude, ArcadeAltitude::Zero, ResetY);
        ResetY += 130;
        Reset(PreviousCorrectedHorizontalVelocity, ArcadeHorizontal::ZeroVectorDefault, ResetY);
        ResetY += 130;
        Reset(HorizontalVelocitySampleValid, ArcadeAltitude::Disabled, ResetY);
        ResetY += 130;
        Reset(PreviousCorrectedVerticalVelocity, ArcadeAltitude::Zero, ResetY);
        ResetY += 130;
        Reset(VerticalVelocitySampleValid, ArcadeAltitude::Disabled, ResetY);
    }
    UK2Node_DynamicCast* Player = NewObject<UK2Node_DynamicCast>(Graph);
    Player->TargetType = APlayerController::StaticClass();
    Player->SetPurity(false);
    FinishNode(Player, Graph, 450, -400);
    UK2Node_CallFunction* ReloadCall = AddCall(
        Graph, Blueprint->GeneratedClass, ReloadSettingsFunctionName, 680, -400);
    Ok &= Connect(Tail, RequirePin(Player, P::Execute));
    Ok &= Connect(RequirePin(Entry, ActorLifecycleGraphNames::NewController),
        Player->GetCastSourcePin());
    Ok &= Connect(Player->GetValidCastPin(), RequirePin(ReloadCall, P::Execute));
    if (PitchHelper)
    {
        UK2Node_Self* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 910, -100);
        UK2Node_CallFunction* HelperAfterController = AddCall(
            Graph, AActor::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(AActor, AddTickPrerequisiteActor), 910, -400);
        UK2Node_CallFunction* PawnAfterHelper = AddCall(
            Graph, AActor::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(AActor, AddTickPrerequisiteActor), 1140, -400);
        Ok &= Connect(RequirePin(ReloadCall, P::Then),
            RequirePin(HelperAfterController, P::Execute));
        Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
            RequirePin(HelperAfterController, P::FunctionTarget));
        Ok &= Connect(Player->GetCastResultPin(),
            RequirePin(HelperAfterController, P::PrerequisiteActor));
        Ok &= Connect(RequirePin(HelperAfterController, P::Then),
            RequirePin(PawnAfterHelper, P::Execute));
        Ok &= Connect(RequirePin(Entry, BlueprintGraphNames::Pins::Pawn),
            RequirePin(PawnAfterHelper, P::FunctionTarget));
        Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
            RequirePin(PawnAfterHelper, P::PrerequisiteActor));
    }
    else
    {
        UK2Node_CallFunction* ResetScale = AddCall(
            Graph, UKismetSystemLibrary::StaticClass(),
            HoverPitchReset::SetDoublePropertyByName, 910, -400);
        SetDefault(ResetScale, HoverPitchReset::PropertyName, *HoverPitchScale.ToString());
        SetDefault(ResetScale, P::Value, HoverPitchReset::DefaultScale);
        Ok &= Connect(RequirePin(ReloadCall, P::Then), RequirePin(ResetScale, P::Execute));
        Ok &= Connect(RequirePin(Entry, BlueprintGraphNames::Pins::Pawn),
            RequirePin(ResetScale, P::Object));
    }
    return Ok;
}

bool AddControllerChangedBinding(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !Blueprint->GeneratedClass ||
        !Blueprint->GeneratedClass->FindFunctionByName(ControllerChangedCallbackName)) return false;
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
        if (Event && Event->EventReference.GetMemberName() ==
            BlueprintGraphNames::Events::ActorReceiveBeginPlay) BeginPlay = Event;
    }
    if (!BeginPlay) return false;
    UEdGraphPin* BeginTail = RequirePin(BeginPlay, P::Then);
    if (BeginTail->LinkedTo.Num() != 1) return false;
    UEdGraphPin* ExistingWork = BeginTail->LinkedTo[0];
    BeginTail->BreakAllPinLinks();
    UK2Node_ExecutionSequence* Sequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 100, -900);
    bool Ok = true;
    Ok &= Connect(BeginTail, RequirePin(Sequence, P::Execute));
    Ok &= Connect(Sequence->GetThenPinGivenIndex(0), ExistingWork);

    UK2Node_CallFunction* Parent = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor), 220, -1020);
    UK2Node_DynamicCast* Pawn = NewObject<UK2Node_DynamicCast>(Graph);
    Pawn->TargetType = APawn::StaticClass();
    Pawn->SetPurity(false);
    FinishNode(Pawn, Graph, 450, -900);
    FMulticastDelegateProperty* DelegateProperty = ControllerChangedDelegateProperty();
    UK2Node_CreateDelegate* Callback = FinishNode(
        NewObject<UK2Node_CreateDelegate>(Graph), Graph, 680, -1080);
    Callback->SetFunction(ControllerChangedCallbackName);
    UK2Node_RemoveDelegate* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false, APawn::StaticClass());
    FinishNode(Remove, Graph, 910, -900);
    UK2Node_AddDelegate* AddBinding = NewObject<UK2Node_AddDelegate>(Graph);
    AddBinding->SetFromProperty(DelegateProperty, false, APawn::StaticClass());
    FinishNode(AddBinding, Graph, 1140, -900);
    Ok &= Connect(Sequence->GetThenPinGivenIndex(1), RequirePin(Pawn, P::Execute));
    Ok &= Connect(RequirePin(Parent, P::ReturnValue), Pawn->GetCastSourcePin());
    Ok &= Connect(Pawn->GetValidCastPin(), RequirePin(Remove, P::Execute));
    Ok &= Connect(Pawn->GetCastResultPin(), RequirePin(Remove, P::FunctionTarget));
    Ok &= Connect(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    Ok &= Connect(RequirePin(Remove, P::Then), RequirePin(AddBinding, P::Execute));
    Ok &= Connect(Pawn->GetCastResultPin(), RequirePin(AddBinding, P::FunctionTarget));
    Ok &= Connect(Callback->GetDelegateOutPin(), AddBinding->GetDelegatePin());
    return Ok;
}

bool AddCollisionHitBinding(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !Blueprint->GeneratedClass ||
        !Blueprint->GeneratedClass->FindFunctionByName(CollisionHitCallbackName)) return false;
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
        if (Event && Event->EventReference.GetMemberName() ==
            BlueprintGraphNames::Events::ActorReceiveBeginPlay) BeginPlay = Event;
    }
    if (!BeginPlay) return false;
    UEdGraphPin* BeginTail = RequirePin(BeginPlay, P::Then);
    if (BeginTail->LinkedTo.Num() != 1) return false;
    UEdGraphPin* ExistingWork = BeginTail->LinkedTo[0];
    BeginTail->BreakAllPinLinks();

    UK2Node_ExecutionSequence* Sequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 100, -1300);
    UK2Node_CallFunction* Parent = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor), 220, -1120);
    FMulticastDelegateProperty* DelegateProperty = CollisionHitDelegateProperty();
    UK2Node_CreateDelegate* Callback = FinishNode(
        NewObject<UK2Node_CreateDelegate>(Graph), Graph, 450, -1300);
    Callback->SetFunction(CollisionHitCallbackName);
    UK2Node_RemoveDelegate* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false, AActor::StaticClass());
    FinishNode(Remove, Graph, 680, -1120);
    UK2Node_AddDelegate* AddBinding = NewObject<UK2Node_AddDelegate>(Graph);
    AddBinding->SetFromProperty(DelegateProperty, false, AActor::StaticClass());
    FinishNode(AddBinding, Graph, 910, -1120);

    UK2Node_CallFunction* Components = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        1140, -1120);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(Components, CollisionRelease::ComponentClass),
        UPrimitiveComponent::StaticClass());
    UK2Node_VariableSet* CacheComponents = AddWrite(
        Graph, OwnedPrimitiveComponents, 1370, -1300);
    UK2Node_VariableGet* CachedComponents = AddRead(
        Graph, OwnedPrimitiveComponents, nullptr, 1370, -960);
    UK2Node_MacroInstance* Loop = NewObject<UK2Node_MacroInstance>(Graph);
    Loop->SetMacroGraph(StandardForEachLoop());
    FinishNode(Loop, Graph, 1600, -1120);
    UK2Node_DynamicCast* Primitive = NewObject<UK2Node_DynamicCast>(Graph);
    Primitive->TargetType = UPrimitiveComponent::StaticClass();
    Primitive->SetPurity(false);
    FinishNode(Primitive, Graph, 1830, -1120);
    UK2Node_CallFunction* EnableNotifications = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetNotifyRigidBodyCollision),
        2060, -1120);
    SetDefault(EnableNotifications, CollisionRelease::NotifyCollision, ArcadeAltitude::Enabled);

    bool Ok = true;
    Ok &= Connect(BeginTail, RequirePin(Sequence, P::Execute));
    Ok &= Connect(Sequence->GetThenPinGivenIndex(0), ExistingWork);
    Ok &= Connect(Sequence->GetThenPinGivenIndex(1), RequirePin(Remove, P::Execute));
    Ok &= Connect(RequirePin(Parent, P::ReturnValue), RequirePin(Remove, P::FunctionTarget));
    Ok &= Connect(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    Ok &= Connect(RequirePin(Remove, P::Then), RequirePin(AddBinding, P::Execute));
    Ok &= Connect(RequirePin(Parent, P::ReturnValue), RequirePin(AddBinding, P::FunctionTarget));
    Ok &= Connect(Callback->GetDelegateOutPin(), AddBinding->GetDelegatePin());
    Ok &= Connect(RequirePin(Parent, P::ReturnValue), RequirePin(Components, P::FunctionTarget));
    Ok &= Connect(RequirePin(Components, P::ReturnValue),
        RequirePin(CacheComponents, OwnedPrimitiveComponents));
    Ok &= Connect(RequirePin(AddBinding, P::Then), RequirePin(CacheComponents, P::Execute));
    Ok &= Connect(RequirePin(CacheComponents, P::Then), RequireExecInputPin(Loop));
    Ok &= Connect(RequirePin(CachedComponents, OwnedPrimitiveComponents),
        RequirePin(Loop, CollisionRelease::Array));
    Ok &= Connect(RequirePin(Loop, P::LoopBody), RequirePin(Primitive, P::Execute));
    Ok &= Connect(RequirePin(Loop, CollisionRelease::ArrayElement),
        Primitive->GetCastSourcePin());
    Ok &= Connect(Primitive->GetValidCastPin(), RequirePin(EnableNotifications, P::Execute));
    Ok &= Connect(Primitive->GetCastResultPin(),
        RequirePin(EnableNotifications, P::FunctionTarget));
    return Ok;
}

UK2Node_FunctionEntry* AddVoidFunction(UBlueprint* Blueprint, const FName Name, UEdGraph*& Graph)
{
    Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, Graph, false, static_cast<UClass*>(nullptr));
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
        {
            RequirePin(Entry, P::Then)->BreakAllPinLinks();
            return Entry;
        }
    }
    return nullptr;
}

UK2Node_CallFunction* AddNativeBoolSetter(
    UEdGraph* Graph, const FName Property, const TCHAR* Value, const int32 X, const int32 Y)
{
    UK2Node_CallFunction* Setter = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        NativeAltitudeOwnership::SetBoolPropertyByName, X, Y);
    SetDefault(Setter, NativeAltitudeOwnership::PropertyName, *Property.ToString());
    SetDefault(Setter, P::Value, Value);
    return Setter;
}

bool AddRestoreNativeAltitudeOwnership(UBlueprint* Blueprint)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddVoidFunction(
        Blueprint, RestoreNativeAltitudeOwnershipFunction, Graph);
    if (!Entry) return false;

    UK2Node_VariableGet* Active = AddRead(
        Graph, NativeAltitudeOwnershipActive, nullptr, 220, -520);
    UK2Node_IfThenElse* ActiveBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 450, -700);
    UK2Node_VariableGet* Gyro = AddRead(Graph, CachedGyro, nullptr, 450, -480);
    UK2Node_VariableGet* OriginalAuto = AddRead(
        Graph, OriginalAutoHoverCollective, nullptr, 680, -360);
    UK2Node_VariableGet* OriginalAssist = AddRead(
        Graph, OriginalAltitudeAssist, nullptr, 910, -360);
    UK2Node_CallFunction* RestoreAuto = AddNativeBoolSetter(
        Graph, AutoHoverCollective, ArcadeAltitude::Disabled, 680, -700);
    UK2Node_CallFunction* RestoreAssist = AddNativeBoolSetter(
        Graph, AltitudeAssist, ArcadeAltitude::Disabled, 910, -700);
    UK2Node_VariableSet* ClearActive = AddWrite(
        Graph, NativeAltitudeOwnershipActive, 1140, -700);
    SetDefault(ClearActive, NativeAltitudeOwnershipActive, ArcadeAltitude::Disabled);

    bool Ok = true;
    Ok &= Connect(RequirePin(Entry, P::Then), RequirePin(ActiveBranch, P::Execute));
    Ok &= Connect(RequirePin(Active, NativeAltitudeOwnershipActive),
        RequirePin(ActiveBranch, P::Condition));
    Ok &= Connect(RequirePin(ActiveBranch, P::Then), RequirePin(RestoreAuto, P::Execute));
    Ok &= Connect(RequirePin(Gyro, CachedGyro), RequirePin(RestoreAuto, P::Object));
    Ok &= Connect(RequirePin(OriginalAuto, OriginalAutoHoverCollective),
        RequirePin(RestoreAuto, P::Value));
    Ok &= Connect(RequirePin(RestoreAuto, P::Then), RequirePin(RestoreAssist, P::Execute));
    Ok &= Connect(RequirePin(Gyro, CachedGyro), RequirePin(RestoreAssist, P::Object));
    Ok &= Connect(RequirePin(OriginalAssist, OriginalAltitudeAssist),
        RequirePin(RestoreAssist, P::Value));
    Ok &= Connect(RequirePin(RestoreAssist, P::Then), RequirePin(ClearActive, P::Execute));
    return Ok;
}

bool AddUpdateNativeAltitudeOwnership(UBlueprint* Blueprint, UClass* StabilizationClass)
{
    UFunction* Restore = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(RestoreNativeAltitudeOwnershipFunction)
        : nullptr;
    if (!Restore || Restore->GetOuterUClass() != Blueprint->GeneratedClass ||
        !StabilizationClass) return false;

    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddVoidFunction(
        Blueprint, UpdateNativeAltitudeOwnershipFunction, Graph);
    if (!Entry) return false;

    UK2Node_VariableGet* Gyro = AddRead(Graph, CachedGyro, nullptr, 220, -260);
    UK2Node_VariableGet* Stabilization = AddRead(
        Graph, CachedStabilizationHelper, nullptr, 220, -80);
    UK2Node_CallFunction* GyroValid = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 450, -400);
    UK2Node_CallFunction* StabilizationValid = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 450, -220);
    UK2Node_CallFunction* ReferencesValid = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 680, -310);

    UK2Node_CallFunction* Controlled = AddCall(
        Graph, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled),
        450, 20);
    UK2Node_VariableGet* Engine = AddRead(
        Graph, EngineRunning, AVoyageVehicleGyroCopter::StaticClass(), 450, 170);
    UK2Node_VariableGet* Throttle = AddRead(
        Graph, CurrentThrottle, AVoyageVehicleGyroCopter::StaticClass(), 450, 320);
    UK2Node_CallFunction* PositiveThrottle = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 680, 320);
    SetDefault(PositiveThrottle, BlueprintGraphNames::Pins::Binary::RightOperand,
        ArcadeAltitude::Zero);
    UK2Node_VariableGet* Rotor = AddRead(
        Graph, PropellerVelocity, AVoyageVehicleGyroCopter::StaticClass(), 450, 470);
    UK2Node_VariableGet* MinimumRotor = AddRead(
        Graph, Settings::StabilizationMinimumPropellerVelocity, StabilizationClass, 450, 620);
    UK2Node_CallFunction* RotorReady = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 680, 520);
    UK2Node_VariableGet* CollectiveValue = AddRead(
        Graph, Collective, AVoyageVehicleGyroCopter::StaticClass(), 1140, -260);
    UK2Node_VariableGet* MinimumThrottlePercent = AddRead(
        Graph, Settings::AltitudeStabilizationMinimumThrottlePercent,
        StabilizationClass, 1140, -100);
    UK2Node_CallFunction* MinimumCollective = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble), 1370, -100);
    SetDefault(MinimumCollective, BlueprintGraphNames::Pins::Binary::RightOperand,
        ArcadeAltitude::PercentScale);
    UK2Node_CallFunction* CollectiveReady = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 1600, -180);

    UK2Node_CallFunction* Controller = AddCall(
        Graph, APawn::StaticClass(), BlueprintGraphNames::ActorFunctions::GetController, 450, 770);
    UK2Node_DynamicCast* Player = NewObject<UK2Node_DynamicCast>(Graph);
    Player->TargetType = APlayerController::StaticClass();
    Player->SetPurity(false);
    FinishNode(Player, Graph, 680, 770);
    UK2Node_CallFunction* SpaceHeld = AddCall(
        Graph, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, IsInputKeyDown), 910, 700);
    SetDefault(SpaceHeld, P::Key, ArcadeAltitude::SpaceKey);
    UK2Node_CallFunction* DescentHeld = AddCall(
        Graph, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, IsInputKeyDown), 910, 850);
    SetDefault(DescentHeld, P::Key, ArcadeAltitude::DescentKey);
    UK2Node_CallFunction* SpaceNeutral = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool), 1140, 700);
    UK2Node_CallFunction* DescentNeutral = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool), 1140, 850);

    UK2Node_Self* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 450, 1040);
    UK2Node_CallFunction* Time = AddCall(
        Graph, UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetTimeSeconds), 680, 1040);
    UK2Node_VariableGet* Deadline = AddRead(
        Graph, CollisionReleaseUntilTime, StabilizationClass, 680, 1190);
    UK2Node_CallFunction* CooldownExpired = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble), 910, 1100);

    TArray<UEdGraphPin*> Eligibility;
    Eligibility.Add(RequirePin(Engine, EngineRunning));
    Eligibility.Add(RequirePin(PositiveThrottle, P::ReturnValue));
    Eligibility.Add(RequirePin(RotorReady, P::ReturnValue));
    Eligibility.Add(RequirePin(CollectiveReady, P::ReturnValue));
    Eligibility.Add(RequirePin(SpaceNeutral, P::ReturnValue));
    Eligibility.Add(RequirePin(DescentNeutral, P::ReturnValue));
    Eligibility.Add(RequirePin(CooldownExpired, P::ReturnValue));
    UEdGraphPin* CombinedEligibility = Eligibility[0];
    int32 EligibilityX = 1370;
    for (int32 Index = 1; Index < Eligibility.Num(); ++Index)
    {
        UK2Node_CallFunction* And = AddCall(
            Graph, UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
            EligibilityX, 80 + Index * 120);
        bool Ok = Connect(CombinedEligibility,
            RequirePin(And, BlueprintGraphNames::Pins::Binary::LeftOperand));
        Ok &= Connect(Eligibility[Index],
            RequirePin(And, BlueprintGraphNames::Pins::Binary::RightOperand));
        if (!Ok) return false;
        CombinedEligibility = RequirePin(And, P::ReturnValue);
        EligibilityX += 180;
    }

    UK2Node_IfThenElse* ReferencesValidBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2620, -700);
    UK2Node_IfThenElse* ControlledBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2800, -820);
    UK2Node_IfThenElse* EligibleBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 3030, -700);
    UK2Node_VariableGet* Active = AddRead(
        Graph, NativeAltitudeOwnershipActive, nullptr, 2800, -500);
    UK2Node_IfThenElse* AlreadyActiveBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 3030, -700);
    UK2Node_VariableGet* CurrentAuto = AddRead(
        Graph, AutoHoverCollective, AVoyageVehicleGyroCopter::StaticClass(), 3030, -420);
    UK2Node_VariableGet* CurrentAssist = AddRead(
        Graph, AltitudeAssist, AVoyageVehicleGyroCopter::StaticClass(), 3030, -260);
    UK2Node_VariableSet* StoreAuto = AddWrite(
        Graph, OriginalAutoHoverCollective, 3260, -700);
    UK2Node_VariableSet* StoreAssist = AddWrite(
        Graph, OriginalAltitudeAssist, 3490, -700);
    UK2Node_CallFunction* DisableAuto = AddNativeBoolSetter(
        Graph, AutoHoverCollective, ArcadeAltitude::Disabled, 3720, -700);
    UK2Node_CallFunction* DisableAssist = AddNativeBoolSetter(
        Graph, AltitudeAssist, ArcadeAltitude::Disabled, 3950, -700);
    UK2Node_VariableSet* MarkActive = AddWrite(
        Graph, NativeAltitudeOwnershipActive, 4180, -700);
    SetDefault(MarkActive, NativeAltitudeOwnershipActive, ArcadeAltitude::Enabled);
    UK2Node_CallFunction* RestoreCall = AddCall(
        Graph, Blueprint->GeneratedClass, RestoreNativeAltitudeOwnershipFunction, 3030, -900);
    UK2Node_CallFunction* RestoreInvalidCall = AddCall(
        Graph, Blueprint->GeneratedClass, RestoreNativeAltitudeOwnershipFunction, 2800, -1050);
    UK2Node_CallFunction* RestoreUncontrolledCall = AddCall(
        Graph, Blueprint->GeneratedClass, RestoreNativeAltitudeOwnershipFunction, 3030, -1050);
    UK2Node_CallFunction* RestoreNonPlayerCall = AddCall(
        Graph, Blueprint->GeneratedClass, RestoreNativeAltitudeOwnershipFunction, 3260, -1050);

    bool Ok = true;
    Ok &= Connect(RequirePin(Gyro, CachedGyro), RequirePin(GyroValid, P::Object));
    Ok &= Connect(RequirePin(Stabilization, CachedStabilizationHelper),
        RequirePin(StabilizationValid, P::Object));
    Ok &= Connect(RequirePin(GyroValid, P::ReturnValue),
        RequirePin(ReferencesValid, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(StabilizationValid, P::ReturnValue),
        RequirePin(ReferencesValid, BlueprintGraphNames::Pins::Binary::RightOperand));
    const TArray<UEdGraphNode*> GyroTargets = {
        Controlled, Engine, Throttle, Rotor, CollectiveValue, Controller,
        CurrentAuto, CurrentAssist};
    for (UEdGraphNode* Node : GyroTargets)
        Ok &= Connect(RequirePin(Gyro, CachedGyro), RequirePin(Node, P::FunctionTarget));
    Ok &= Connect(RequirePin(Throttle, CurrentThrottle),
        RequirePin(PositiveThrottle, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(Rotor, PropellerVelocity),
        RequirePin(RotorReady, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(Stabilization, CachedStabilizationHelper),
        RequirePin(MinimumRotor, P::FunctionTarget));
    Ok &= Connect(RequirePin(MinimumRotor, Settings::StabilizationMinimumPropellerVelocity),
        RequirePin(RotorReady, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(Stabilization, CachedStabilizationHelper),
        RequirePin(MinimumThrottlePercent, P::FunctionTarget));
    Ok &= Connect(RequirePin(MinimumThrottlePercent,
        Settings::AltitudeStabilizationMinimumThrottlePercent),
        RequirePin(MinimumCollective, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(CollectiveValue, Collective),
        RequirePin(CollectiveReady, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(MinimumCollective, P::ReturnValue),
        RequirePin(CollectiveReady, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(Controller, P::ReturnValue), Player->GetCastSourcePin());
    Ok &= Connect(Player->GetCastResultPin(), RequirePin(SpaceHeld, P::FunctionTarget));
    Ok &= Connect(Player->GetCastResultPin(), RequirePin(DescentHeld, P::FunctionTarget));
    Ok &= Connect(RequirePin(SpaceHeld, P::ReturnValue),
        RequirePin(SpaceNeutral, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(DescentHeld, P::ReturnValue),
        RequirePin(DescentNeutral, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(Time, CollisionRelease::WorldContextObject));
    Ok &= Connect(RequirePin(Stabilization, CachedStabilizationHelper),
        RequirePin(Deadline, P::FunctionTarget));
    Ok &= Connect(RequirePin(Deadline, CollisionReleaseUntilTime),
        RequirePin(CooldownExpired, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(Time, P::ReturnValue),
        RequirePin(CooldownExpired, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(Entry, P::Then), RequirePin(ReferencesValidBranch, P::Execute));
    Ok &= Connect(RequirePin(ReferencesValid, P::ReturnValue),
        RequirePin(ReferencesValidBranch, P::Condition));
    Ok &= Connect(RequirePin(ReferencesValidBranch, P::Then),
        RequirePin(ControlledBranch, P::Execute));
    Ok &= Connect(RequirePin(Controlled, P::ReturnValue),
        RequirePin(ControlledBranch, P::Condition));
    Ok &= Connect(RequirePin(ControlledBranch, P::Then), RequirePin(Player, P::Execute));
    Ok &= Connect(Player->GetValidCastPin(), RequirePin(EligibleBranch, P::Execute));
    Ok &= Connect(CombinedEligibility, RequirePin(EligibleBranch, P::Condition));
    Ok &= Connect(RequirePin(EligibleBranch, P::Then),
        RequirePin(AlreadyActiveBranch, P::Execute));
    Ok &= Connect(RequirePin(Active, NativeAltitudeOwnershipActive),
        RequirePin(AlreadyActiveBranch, P::Condition));
    Ok &= Connect(RequirePin(AlreadyActiveBranch, P::Else), RequirePin(StoreAuto, P::Execute));
    Ok &= Connect(RequirePin(CurrentAuto, AutoHoverCollective),
        RequirePin(StoreAuto, OriginalAutoHoverCollective));
    Ok &= Connect(RequirePin(StoreAuto, P::Then), RequirePin(StoreAssist, P::Execute));
    Ok &= Connect(RequirePin(CurrentAssist, AltitudeAssist),
        RequirePin(StoreAssist, OriginalAltitudeAssist));
    Ok &= Connect(RequirePin(StoreAssist, P::Then), RequirePin(DisableAuto, P::Execute));
    Ok &= Connect(RequirePin(Gyro, CachedGyro), RequirePin(DisableAuto, P::Object));
    Ok &= Connect(RequirePin(DisableAuto, P::Then), RequirePin(DisableAssist, P::Execute));
    Ok &= Connect(RequirePin(Gyro, CachedGyro), RequirePin(DisableAssist, P::Object));
    Ok &= Connect(RequirePin(DisableAssist, P::Then), RequirePin(MarkActive, P::Execute));
    Ok &= Connect(RequirePin(ReferencesValidBranch, P::Else),
        RequirePin(RestoreInvalidCall, P::Execute));
    Ok &= Connect(RequirePin(ControlledBranch, P::Else),
        RequirePin(RestoreUncontrolledCall, P::Execute));
    Ok &= Connect(Player->GetInvalidCastPin(), RequirePin(RestoreNonPlayerCall, P::Execute));
    Ok &= Connect(RequirePin(EligibleBranch, P::Else), RequirePin(RestoreCall, P::Execute));
    return Ok;
}

bool AddNativeAltitudeOwnershipCacheBinding(
    UBlueprint* Blueprint, UClass* StabilizationClass)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !StabilizationClass) return false;
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
        if (Event && Event->EventReference.GetMemberName() ==
            BlueprintGraphNames::Events::ActorReceiveBeginPlay) BeginPlay = Event;
    }
    if (!BeginPlay) return false;
    UEdGraphPin* BeginTail = RequirePin(BeginPlay, P::Then);
    if (BeginTail->LinkedTo.Num() != 1) return false;
    UEdGraphPin* ExistingWork = BeginTail->LinkedTo[0];
    BeginTail->BreakAllPinLinks();

    UK2Node_ExecutionSequence* Sequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 100, -1700);
    UK2Node_CallFunction* Parent = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor),
        220, -1510);
    UK2Node_DynamicCast* Gyro = NewObject<UK2Node_DynamicCast>(Graph);
    Gyro->TargetType = AVoyageVehicleGyroCopter::StaticClass();
    Gyro->SetPurity(false);
    FinishNode(Gyro, Graph, 450, -1700);
    UK2Node_VariableSet* StoreGyro = AddWrite(Graph, CachedGyro, 680, -1700);
    UK2Node_CallFunction* Components = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        680, -1470);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(Components, NativeAltitudeOwnership::ComponentClass),
        UChildActorComponent::StaticClass());
    UK2Node_VariableSet* StoreComponents = AddWrite(
        Graph, OwnedChildActorComponents, 910, -1700);
    UK2Node_VariableGet* CachedComponents = AddRead(
        Graph, OwnedChildActorComponents, nullptr, 910, -1430);
    UK2Node_MacroInstance* Loop = NewObject<UK2Node_MacroInstance>(Graph);
    Loop->SetMacroGraph(StandardForEachLoop());
    FinishNode(Loop, Graph, 1140, -1700);
    UK2Node_DynamicCast* ChildComponent = NewObject<UK2Node_DynamicCast>(Graph);
    ChildComponent->TargetType = UChildActorComponent::StaticClass();
    ChildComponent->SetPurity(false);
    FinishNode(ChildComponent, Graph, 1370, -1700);
    UK2Node_VariableGet* ChildActor = AddRead(
        Graph, BlueprintGraphNames::ActorProperties::ChildActor,
        UChildActorComponent::StaticClass(), 1600, -1470);
    UK2Node_DynamicCast* Stabilization = NewObject<UK2Node_DynamicCast>(Graph);
    Stabilization->TargetType = StabilizationClass;
    Stabilization->SetPurity(false);
    FinishNode(Stabilization, Graph, 1830, -1700);
    UK2Node_VariableSet* StoreStabilization = AddWrite(
        Graph, CachedStabilizationHelper, 2060, -1700);

    bool Ok = true;
    Ok &= Connect(BeginTail, RequirePin(Sequence, P::Execute));
    Ok &= Connect(Sequence->GetThenPinGivenIndex(0), ExistingWork);
    Ok &= Connect(Sequence->GetThenPinGivenIndex(1), RequirePin(Gyro, P::Execute));
    Ok &= Connect(RequirePin(Parent, P::ReturnValue), Gyro->GetCastSourcePin());
    Ok &= Connect(Gyro->GetValidCastPin(), RequirePin(StoreGyro, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(StoreGyro, CachedGyro));
    Ok &= Connect(RequirePin(Parent, P::ReturnValue), RequirePin(Components, P::FunctionTarget));
    Ok &= Connect(RequirePin(StoreGyro, P::Then), RequirePin(StoreComponents, P::Execute));
    Ok &= Connect(RequirePin(Components, P::ReturnValue),
        RequirePin(StoreComponents, OwnedChildActorComponents));
    Ok &= Connect(RequirePin(StoreComponents, P::Then), RequireExecInputPin(Loop));
    Ok &= Connect(RequirePin(CachedComponents, OwnedChildActorComponents),
        RequirePin(Loop, NativeAltitudeOwnership::Array));
    Ok &= Connect(RequirePin(Loop, P::LoopBody), RequirePin(ChildComponent, P::Execute));
    Ok &= Connect(RequirePin(Loop, NativeAltitudeOwnership::ArrayElement),
        ChildComponent->GetCastSourcePin());
    Ok &= Connect(ChildComponent->GetValidCastPin(), RequirePin(Stabilization, P::Execute));
    Ok &= Connect(ChildComponent->GetCastResultPin(), RequirePin(ChildActor, P::FunctionTarget));
    Ok &= Connect(RequirePin(ChildActor, BlueprintGraphNames::ActorProperties::ChildActor),
        Stabilization->GetCastSourcePin());
    Ok &= Connect(Stabilization->GetValidCastPin(),
        RequirePin(StoreStabilization, P::Execute));
    Ok &= Connect(Stabilization->GetCastResultPin(),
        RequirePin(StoreStabilization, CachedStabilizationHelper));
    return Ok;
}

bool AddNativeAltitudeOwnershipTickCall(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    UFunction* Update = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(UpdateNativeAltitudeOwnershipFunction)
        : nullptr;
    if (!Graph || !Update || Update->GetOuterUClass() != Blueprint->GeneratedClass) return false;

    UK2Node_IfThenElse* PitchEnabledBranch = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node);
        if (!Branch) continue;
        UEdGraphPin* Condition = RequirePin(Branch, P::Condition);
        if (Condition->LinkedTo.Num() != 1) continue;
        UK2Node_VariableGet* Source = Cast<UK2Node_VariableGet>(
            Condition->LinkedTo[0]->GetOwningNode());
        if (Source && Source->VariableReference.GetMemberName() ==
            Settings::EnablePitchCorrection)
        {
            if (PitchEnabledBranch) return false;
            PitchEnabledBranch = Branch;
        }
    }
    if (!PitchEnabledBranch) return false;
    UEdGraphPin* Execute = RequirePin(PitchEnabledBranch, P::Execute);
    if (Execute->LinkedTo.Num() != 1) return false;
    UEdGraphPin* ExistingWork = Execute->LinkedTo[0];
    Execute->BreakAllPinLinks();
    UK2Node_CallFunction* UpdateCall = AddCall(
        Graph, Blueprint->GeneratedClass, UpdateNativeAltitudeOwnershipFunction, 1370, -180);
    bool Ok = true;
    Ok &= Connect(ExistingWork, RequirePin(UpdateCall, P::Execute));
    Ok &= Connect(RequirePin(UpdateCall, P::Then), Execute);
    return Ok;
}

bool AddNativeAltitudeOwnershipEndPlay(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    UFunction* Restore = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(RestoreNativeAltitudeOwnershipFunction)
        : nullptr;
    if (!Graph || !Restore || Restore->GetOuterUClass() != Blueprint->GeneratedClass) return false;
    UK2Node_Event* EndPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
        if (!Event || Event->EventReference.GetMemberName() !=
            ActorLifecycleGraphNames::EndPlayEvent) continue;
        if (EndPlay) return false;
        EndPlay = Event;
    }
    if (!EndPlay)
    {
        EndPlay = NewObject<UK2Node_Event>(Graph);
        EndPlay->EventReference.SetExternalMember(
            ActorLifecycleGraphNames::EndPlayEvent, AActor::StaticClass());
        EndPlay->bOverrideFunction = true;
        FinishNode(EndPlay, Graph, 0, -2100);
    }
    UEdGraphPin* Tail = RequirePin(EndPlay, P::Then);
    if (Tail->LinkedTo.Num() > 1) return false;
    UEdGraphPin* ExistingWork = Tail->LinkedTo.Num() == 1 ? Tail->LinkedTo[0] : nullptr;
    Tail->BreakAllPinLinks();
    UK2Node_CallFunction* RestoreCall = AddCall(
        Graph, Blueprint->GeneratedClass, RestoreNativeAltitudeOwnershipFunction, 220, -2100);
    bool Ok = Connect(Tail, RequirePin(RestoreCall, P::Execute));
    if (ExistingWork)
        Ok &= Connect(RequirePin(RestoreCall, P::Then), ExistingWork);
    return Ok;
}

UK2Node_VariableGet* AddLocalRead(
    UBlueprint* Blueprint, UEdGraph* Graph, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    Node->VariableReference.SetLocalMember(
        Name, Graph->GetName(),
        FBlueprintEditorUtils::FindLocalVariableGuidByName(Blueprint, Graph, Name));
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* AddLocalWrite(
    UBlueprint* Blueprint, UEdGraph* Graph, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    Node->VariableReference.SetLocalMember(
        Name, Graph->GetName(),
        FBlueprintEditorUtils::FindLocalVariableGuidByName(Blueprint, Graph, Name));
    return FinishNode(Node, Graph, X, Y);
}

bool AddCargoControllerChangedCallback(UBlueprint* Blueprint)
{
    FMulticastDelegateProperty* DelegateProperty = ControllerChangedDelegateProperty();
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, CargoControllerChangedCallbackName, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, Graph, true, DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            Entry = Candidate;
    if (!Entry) return false;
    UEdGraphPin* Tail = RequirePin(Entry, P::Then);
    Tail->BreakAllPinLinks();
    UK2Node_VariableSet* ClearArmed = AddWrite(Graph, CargoPressArmed, 220, -120);
    SetDefault(ClearArmed, CargoPressArmed, CargoAction::Disabled);
    UK2Node_VariableSet* ClearFired = AddWrite(Graph, CargoPressFired, 450, -120);
    SetDefault(ClearFired, CargoPressFired, CargoAction::Disabled);
    return Connect(Tail, RequirePin(ClearArmed, P::Execute)) &&
        Connect(RequirePin(ClearArmed, P::Then), RequirePin(ClearFired, P::Execute));
}

bool AddCargoControllerChangedBinding(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !Blueprint->GeneratedClass ||
        !Blueprint->GeneratedClass->FindFunctionByName(CargoControllerChangedCallbackName))
        return false;
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
        if (Event && Event->EventReference.GetMemberName() ==
            BlueprintGraphNames::Events::ActorReceiveBeginPlay) BeginPlay = Event;
    }
    if (!BeginPlay) return false;
    UEdGraphPin* BeginTail = RequirePin(BeginPlay, P::Then);
    if (BeginTail->LinkedTo.Num() != 1) return false;
    UEdGraphPin* ExistingWork = BeginTail->LinkedTo[0];
    BeginTail->BreakAllPinLinks();

    UK2Node_ExecutionSequence* Sequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 100, -720);
    UK2Node_Self* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 220, -540);
    FMulticastDelegateProperty* DelegateProperty = ControllerChangedDelegateProperty();
    UK2Node_CreateDelegate* Callback = FinishNode(
        NewObject<UK2Node_CreateDelegate>(Graph), Graph, 220, -900);
    Callback->SetFunction(CargoControllerChangedCallbackName);
    UK2Node_RemoveDelegate* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false, APawn::StaticClass());
    FinishNode(Remove, Graph, 450, -720);
    UK2Node_AddDelegate* AddBinding = NewObject<UK2Node_AddDelegate>(Graph);
    AddBinding->SetFromProperty(DelegateProperty, false, APawn::StaticClass());
    FinishNode(AddBinding, Graph, 680, -720);

    bool Ok = true;
    Ok &= Connect(BeginTail, RequirePin(Sequence, P::Execute));
    Ok &= Connect(Sequence->GetThenPinGivenIndex(0), ExistingWork);
    Ok &= Connect(Sequence->GetThenPinGivenIndex(1), RequirePin(Remove, P::Execute));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(Remove, P::FunctionTarget));
    Ok &= Connect(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    Ok &= Connect(RequirePin(Remove, P::Then), RequirePin(AddBinding, P::Execute));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(AddBinding, P::FunctionTarget));
    Ok &= Connect(Callback->GetDelegateOutPin(), AddBinding->GetDelegatePin());
    return Ok;
}

bool AddFourPortSettingsCache(UBlueprint* Blueprint)
{
    if (!VoyageModSettings::Graph::AddSettingMembers(
            Blueprint, Settings::Settings, UE_ARRAY_COUNT(Settings::Settings),
            [](const VoyageModSettings::FSetting& Setting)
            {
                return Setting.Field == Settings::EnableFourMooringPorts;
            }) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Blueprint, FourPortSettingsLoaded,
            []
            {
                FEdGraphPinType Type;
                Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
                return Type;
            }(), CargoAction::Disabled))
    {
        return false;
    }
    for (const FName Field : {Settings::EnableFourMooringPorts, FourPortSettingsLoaded})
    {
        FBPVariableDescription* Variable = Blueprint->NewVariables.FindByPredicate(
            [&Field](const FBPVariableDescription& Candidate)
            {
                return Candidate.VarName == Field;
            });
        if (!Variable) return false;
        Variable->PropertyFlags |= CPF_Transient;
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error ||
        !VoyageModSettings::Graph::AddSettingsLoader(
            Blueprint, ReloadSettingsFunctionName, Settings::RelativePath,
            Settings::Settings, UE_ARRAY_COUNT(Settings::Settings),
            [](const VoyageModSettings::FSetting& Setting)
            {
                return Setting.Field == Settings::EnableFourMooringPorts;
            }))
    {
        return false;
    }

    UEdGraph* CacheGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, EnsureFourPortSettingsCached, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, CacheGraph, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* CacheEntry = nullptr;
    UK2Node_FunctionResult* CacheResult = nullptr;
    for (UEdGraphNode* Node : CacheGraph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) CacheEntry = Candidate;
        if (auto* Candidate = Cast<UK2Node_FunctionResult>(Node)) CacheResult = Candidate;
    }
    if (!CacheEntry) return false;
    CacheEntry->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable);
    if (!CacheResult)
        CacheResult = FinishNode(NewObject<UK2Node_FunctionResult>(CacheGraph), CacheGraph, 1140, 0);
    UEdGraphPin* CacheStart = RequirePin(CacheEntry, P::Then);
    CacheStart->BreakAllPinLinks();
    UK2Node_VariableGet* Loaded = AddRead(
        CacheGraph, FourPortSettingsLoaded, nullptr, 220, -180);
    UK2Node_IfThenElse* NeedsLoad = FinishNode(
        NewObject<UK2Node_IfThenElse>(CacheGraph), CacheGraph, 450, 0);
    UFunction* Reload = Blueprint->GeneratedClass->FindFunctionByName(
        ReloadSettingsFunctionName, EIncludeSuperFlag::ExcludeSuper);
    if (!Reload || Reload->GetOwnerClass() != Blueprint->GeneratedClass) return false;
    UK2Node_CallFunction* ReloadCall = AddCall(CacheGraph, Reload->GetOwnerClass(),
        ReloadSettingsFunctionName, 680, 0);
    UK2Node_VariableSet* MarkLoaded = AddWrite(
        CacheGraph, FourPortSettingsLoaded, 910, 0);
    SetDefault(MarkLoaded, FourPortSettingsLoaded, ProvidedActionDefaults::Enabled);

    bool Ok = true;
    Ok &= Connect(CacheStart, RequirePin(NeedsLoad, P::Execute));
    Ok &= Connect(Loaded->GetValuePin(), RequirePin(NeedsLoad, P::Condition));
    Ok &= Connect(RequirePin(NeedsLoad, P::Then), RequirePin(CacheResult, P::Execute));
    Ok &= Connect(RequirePin(NeedsLoad, P::Else), RequirePin(ReloadCall, P::Execute));
    Ok &= Connect(RequirePin(ReloadCall, P::Then), RequirePin(MarkLoaded, P::Execute));
    Ok &= Connect(RequirePin(MarkLoaded, P::Then), RequirePin(CacheResult, P::Execute));
    if (!Ok) return false;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) return false;

    UEdGraph* GetterGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, GetFourPortSetting, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, GetterGraph, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* GetterEntry = nullptr;
    UK2Node_FunctionResult* GetterResult = nullptr;
    for (UEdGraphNode* Node : GetterGraph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) GetterEntry = Candidate;
        if (auto* Candidate = Cast<UK2Node_FunctionResult>(Node)) GetterResult = Candidate;
    }
    if (!GetterEntry) return false;
    GetterEntry->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable);
    if (!GetterResult)
        GetterResult = FinishNode(NewObject<UK2Node_FunctionResult>(GetterGraph), GetterGraph, 1140, 0);
    FEdGraphPinType BoolType;
    BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    if (!GetterResult->CreateUserDefinedPin(P::ReturnValue, BoolType, EGPD_Input)) return false;
    UEdGraphPin* GetterStart = RequirePin(GetterEntry, P::Then);
    GetterStart->BreakAllPinLinks();
    UK2Node_CallFunction* Defaults = AddCall(
        GetterGraph, UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary, GetClassDefaultObject),
        220, 0);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(Defaults, FName(TEXT("Class"))), Blueprint->GeneratedClass);
    UK2Node_DynamicCast* CastDefaults = NewObject<UK2Node_DynamicCast>(GetterGraph);
    CastDefaults->TargetType = Blueprint->GeneratedClass;
    FinishNode(CastDefaults, GetterGraph, 450, 0);
    UFunction* Ensure = Blueprint->GeneratedClass->FindFunctionByName(
        EnsureFourPortSettingsCached, EIncludeSuperFlag::ExcludeSuper);
    if (!Ensure || Ensure->GetOwnerClass() != Blueprint->GeneratedClass) return false;
    UK2Node_CallFunction* EnsureCall = AddCall(
        GetterGraph, Ensure->GetOwnerClass(), EnsureFourPortSettingsCached, 680, 0);
    UK2Node_VariableGet* Enabled = AddRead(
        GetterGraph, Settings::EnableFourMooringPorts, Blueprint->GeneratedClass, 910, 180);
    Ok = true;
    Ok &= Connect(GetterStart, RequirePin(Defaults, P::Execute));
    Ok &= Connect(RequirePin(Defaults, P::Then), RequirePin(CastDefaults, P::Execute));
    Ok &= Connect(RequirePin(Defaults, P::ReturnValue), CastDefaults->GetCastSourcePin());
    Ok &= Connect(CastDefaults->GetValidCastPin(), RequirePin(EnsureCall, P::Execute));
    Ok &= Connect(CastDefaults->GetCastResultPin(), RequirePin(EnsureCall, P::FunctionTarget));
    Ok &= Connect(CastDefaults->GetCastResultPin(), RequirePin(Enabled, P::FunctionTarget));
    Ok &= Connect(RequirePin(EnsureCall, P::Then), RequirePin(GetterResult, P::Execute));
    Ok &= Connect(Enabled->GetValuePin(), RequirePin(GetterResult, P::ReturnValue));
    if (!Ok) return false;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Blueprint->Status != BS_Error;
}

bool AddCargoOwnershipGraph(UBlueprint* Blueprint, UInputAction* DropCargoAction)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !DropCargoAction) return false;
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
        if (Event && Event->EventReference.GetMemberName() ==
            BlueprintGraphNames::Events::ActorReceiveBeginPlay) BeginPlay = Event;
    }
    if (!BeginPlay)
    {
        BeginPlay = NewObject<UK2Node_Event>(Graph);
        BeginPlay->EventReference.SetExternalMember(
            BlueprintGraphNames::Events::ActorReceiveBeginPlay, AActor::StaticClass());
        BeginPlay->bOverrideFunction = true;
        FinishNode(BeginPlay, Graph, 0, -400);
    }
    UEdGraphPin* BeginTail = RequirePin(BeginPlay, P::Then);
    if (BeginTail->LinkedTo.Num() > 1) return false;
    UK2Node_CallParentFunction* ParentBeginPlay = nullptr;
    if (BeginTail->LinkedTo.Num() == 1)
    {
        ParentBeginPlay = Cast<UK2Node_CallParentFunction>(
            BeginTail->LinkedTo[0]->GetOwningNode());
        if (!ParentBeginPlay ||
            ParentBeginPlay->FunctionReference.GetMemberName() !=
                BlueprintGraphNames::Events::ActorReceiveBeginPlay)
        {
            return false;
        }
    }
    BeginTail->BreakAllPinLinks();

    if (!ParentBeginPlay)
    {
        UFunction* BeginPlayFunction = AActor::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::Events::ActorReceiveBeginPlay);
        if (!BeginPlayFunction) return false;
        ParentBeginPlay = NewObject<UK2Node_CallParentFunction>(Graph);
        ParentBeginPlay->SetFromFunction(BeginPlayFunction);
        FinishNode(ParentBeginPlay, Graph, 220, -400);
    }
    UK2Node_CallFunction* ClearNativeAction = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        CargoAction::SetObjectPropertyByName, 450, -400);
    UK2Node_CallFunction* BeginFourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, 450, -580);
    UK2Node_IfThenElse* BeginFourPortsBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 680, -400);
    UK2Node_Self* BeginSelf = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 0, -220);
    SetDefault(ClearNativeAction, HoverPitchReset::PropertyName,
        CargoAction::DropCargoActionProperty);

    UK2Node_EnhancedInputAction* Input = NewObject<UK2Node_EnhancedInputAction>(Graph);
    Input->InputAction = DropCargoAction;
    FinishNode(Input, Graph, 0, 0);

    UK2Node_VariableSet* StartedClearFired = AddWrite(
        Graph, CargoPressFired, 220, -40);
    SetDefault(StartedClearFired, CargoPressFired, CargoAction::Disabled);
    UK2Node_VariableSet* StartedArm = AddWrite(Graph, CargoPressArmed, 450, -40);
    SetDefault(StartedArm, CargoPressArmed, ProvidedActionDefaults::Enabled);
    UK2Node_CallFunction* StartedFourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, 220, -180);
    UK2Node_IfThenElse* StartedFourPortsBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 450, -180);

    UK2Node_CallFunction* PlayerControlled = AddCall(
        Graph, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled),
        220, 240);
    UK2Node_VariableGet* Armed = AddRead(Graph, CargoPressArmed, nullptr, 220, 390);
    UK2Node_VariableGet* Fired = AddRead(Graph, CargoPressFired, nullptr, 220, 540);
    UK2Node_CallFunction* NotFired = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool), 450, 540);
    UK2Node_VariableGet* HoldDuration = AddRead(
        Graph, DropCargoTriggerTime, AVoyageVehicleGyroCopter::StaticClass(), 220, 690);
    UK2Node_CallFunction* HeldLongEnough = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 450, 690);
    UK2Node_CallFunction* ControlledAndArmed = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 680, 330);
    UK2Node_CallFunction* FreshAndElapsed = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 680, 600);
    UK2Node_CallFunction* MayReleaseCargo = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 910, 450);
    UK2Node_IfThenElse* PlayerBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1140, 240);
    UK2Node_CallFunction* TriggeredFourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, 910, 80);
    UK2Node_IfThenElse* TriggeredFourPortsBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1140, 80);
    UK2Node_VariableSet* LatchFired = AddWrite(Graph, CargoPressFired, 1370, 240);
    SetDefault(LatchFired, CargoPressFired, ProvidedActionDefaults::Enabled);
    UK2Node_CallFunction* Components = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        1370, 450);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(Components, CargoAction::ComponentClass),
        UModuleSocketComponent::StaticClass());
    UK2Node_MacroInstance* Loop = NewObject<UK2Node_MacroInstance>(Graph);
    Loop->SetMacroGraph(StandardForEachLoop());
    FinishNode(Loop, Graph, 1600, 240);
    UK2Node_DynamicCast* Socket = NewObject<UK2Node_DynamicCast>(Graph);
    Socket->TargetType = UModuleSocketComponent::StaticClass();
    Socket->SetPurity(false);
    FinishNode(Socket, Graph, 1830, 240);
    UK2Node_CallFunction* Valid = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 2060, 450);
    UK2Node_IfThenElse* ValidBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2060, 240);
    UK2Node_VariableGet* Port = AddRead(
        Graph, MooringLayout::PortProperty,
        UModuleSocketComponent::StaticClass(), 2290, 450);
    UK2Node_BreakStruct* BreakPort = NewObject<UK2Node_BreakStruct>(Graph);
    BreakPort->StructType = FModuleSocketIOData::StaticStruct();
    BreakPort->bMadeAfterOverridePinRemoval = true;
    FinishNode(BreakPort, Graph, 2520, 450);
    UK2Node_EnumEquality* IsMooring = FinishNode(
        NewObject<UK2Node_EnumEquality>(Graph), Graph, 2750, 450);
    UK2Node_IfThenElse* MooringBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2980, 240);
    UK2Node_CallFunction* Connected = AddCall(
        Graph, UModuleSocketComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UModuleSocketComponent, IsSocketConnectedOrHasCable),
        3210, 450);
    UK2Node_IfThenElse* ConnectedBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 3210, 240);
    UK2Node_CallFunction* Disconnect = AddCall(
        Graph, UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageMiscBlueprintFunctionLibrary, DisconnectCableFromSocket), 3440, 240);
    SetDefault(Disconnect, CargoAction::Destroy, CargoAction::DestroyCable);

    UK2Node_VariableSet* CompletedClearArmed = AddWrite(
        Graph, CargoPressArmed, 220, 940);
    SetDefault(CompletedClearArmed, CargoPressArmed, CargoAction::Disabled);
    UK2Node_VariableSet* CompletedClearFired = AddWrite(
        Graph, CargoPressFired, 450, 940);
    SetDefault(CompletedClearFired, CargoPressFired, CargoAction::Disabled);
    UK2Node_VariableSet* CanceledClearArmed = AddWrite(
        Graph, CargoPressArmed, 220, 1110);
    SetDefault(CanceledClearArmed, CargoPressArmed, CargoAction::Disabled);
    UK2Node_VariableSet* CanceledClearFired = AddWrite(
        Graph, CargoPressFired, 450, 1110);
    SetDefault(CanceledClearFired, CargoPressFired, CargoAction::Disabled);

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    bool Ok = true;
    Ok &= Connect(BeginTail, RequirePin(ParentBeginPlay, P::Execute));
    Ok &= Connect(RequirePin(ParentBeginPlay, P::Then),
        RequirePin(BeginFourPorts, P::Execute));
    Ok &= Connect(RequirePin(BeginFourPorts, P::Then),
        RequirePin(BeginFourPortsBranch, P::Execute));
    Ok &= Connect(RequirePin(BeginFourPorts, P::ReturnValue),
        RequirePin(BeginFourPortsBranch, P::Condition));
    Ok &= Connect(RequirePin(BeginFourPortsBranch, P::Then),
        RequirePin(ClearNativeAction, P::Execute));
    Ok &= Connect(BeginSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(ClearNativeAction, P::Object));
    Ok &= Connect(RequirePin(Input, CargoAction::Started),
        RequirePin(StartedFourPorts, P::Execute));
    Ok &= Connect(RequirePin(StartedFourPorts, P::Then),
        RequirePin(StartedFourPortsBranch, P::Execute));
    Ok &= Connect(RequirePin(StartedFourPorts, P::ReturnValue),
        RequirePin(StartedFourPortsBranch, P::Condition));
    Ok &= Connect(RequirePin(StartedFourPortsBranch, P::Then),
        RequirePin(StartedClearFired, P::Execute));
    Ok &= Connect(RequirePin(StartedClearFired, P::Then), RequirePin(StartedArm, P::Execute));
    Ok &= Connect(RequirePin(Input, CargoAction::Triggered),
        RequirePin(TriggeredFourPorts, P::Execute));
    Ok &= Connect(RequirePin(TriggeredFourPorts, P::Then),
        RequirePin(TriggeredFourPortsBranch, P::Execute));
    Ok &= Connect(RequirePin(TriggeredFourPorts, P::ReturnValue),
        RequirePin(TriggeredFourPortsBranch, P::Condition));
    Ok &= Connect(RequirePin(TriggeredFourPortsBranch, P::Then),
        RequirePin(PlayerBranch, P::Execute));
    UK2Node_Self* InputSelf = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 220, 220);
    Ok &= Connect(InputSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(PlayerControlled, P::FunctionTarget));
    Ok &= Connect(InputSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(HoldDuration, P::FunctionTarget));
    Ok &= Connect(RequirePin(PlayerControlled, P::ReturnValue),
        RequirePin(ControlledAndArmed, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(Armed->GetValuePin(),
        RequirePin(ControlledAndArmed, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(Fired->GetValuePin(),
        RequirePin(NotFired, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(Input, CargoAction::ElapsedSeconds),
        RequirePin(HeldLongEnough, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(HoldDuration->GetValuePin(),
        RequirePin(HeldLongEnough, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(NotFired, P::ReturnValue),
        RequirePin(FreshAndElapsed, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(HeldLongEnough, P::ReturnValue),
        RequirePin(FreshAndElapsed, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(ControlledAndArmed, P::ReturnValue),
        RequirePin(MayReleaseCargo, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(FreshAndElapsed, P::ReturnValue),
        RequirePin(MayReleaseCargo, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(MayReleaseCargo, P::ReturnValue),
        RequirePin(PlayerBranch, P::Condition));
    Ok &= Connect(InputSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(Components, P::FunctionTarget));
    Ok &= Connect(RequirePin(PlayerBranch, P::Then), RequirePin(LatchFired, P::Execute));
    Ok &= Connect(RequirePin(LatchFired, P::Then), RequireExecInputPin(Loop));
    Ok &= Connect(RequirePin(Components, P::ReturnValue), RequirePin(Loop, CargoAction::Array));
    Ok &= Connect(RequirePin(Loop, P::LoopBody), RequirePin(Socket, P::Execute));
    Ok &= Connect(RequirePin(Loop, CargoAction::ArrayElement), Socket->GetCastSourcePin());
    Ok &= Connect(Socket->GetValidCastPin(), RequirePin(ValidBranch, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(Valid, P::Object));
    Ok &= Connect(RequirePin(Valid, P::ReturnValue), RequirePin(ValidBranch, P::Condition));
    Ok &= Connect(RequirePin(ValidBranch, P::Then), RequirePin(MooringBranch, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(Port, P::FunctionTarget));
    Ok &= Connect(Port->GetValuePin(), FindInputPin(BreakPort));
    Ok &= Connect(RequirePin(BreakPort, CargoAction::ResourceType), IsMooring->GetInput1Pin());
    Schema->TrySetDefaultValue(*IsMooring->GetInput2Pin(),
        MooringLayout::MooringResourceEnum);
    Ok &= Connect(IsMooring->GetReturnValuePin(), RequirePin(MooringBranch, P::Condition));
    Ok &= Connect(RequirePin(MooringBranch, P::Then), RequirePin(ConnectedBranch, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(Connected, P::FunctionTarget));
    Ok &= Connect(RequirePin(Connected, P::ReturnValue), RequirePin(ConnectedBranch, P::Condition));
    Ok &= Connect(RequirePin(ConnectedBranch, P::Then), RequirePin(Disconnect, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(Disconnect, CargoAction::Socket));
    Ok &= Connect(RequirePin(Input, CargoAction::Completed),
        RequirePin(CompletedClearArmed, P::Execute));
    Ok &= Connect(RequirePin(CompletedClearArmed, P::Then),
        RequirePin(CompletedClearFired, P::Execute));
    Ok &= Connect(RequirePin(Input, CargoAction::Canceled),
        RequirePin(CanceledClearArmed, P::Execute));
    Ok &= Connect(RequirePin(CanceledClearArmed, P::Then),
        RequirePin(CanceledClearFired, P::Execute));
    return Ok;
}

bool AddProvidedActionsBpOverride(
    UBlueprint* Blueprint,
    UInputAction* ResetAction,
    UInputAction* DropCargoAction,
    UClass* PitchHelperClass)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, ProvidedActionsFunctionName, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, Graph, false, AVoyageVehiclePawn::StaticClass());

    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
        if (UK2Node_FunctionResult* Candidate = Cast<UK2Node_FunctionResult>(Node)) Result = Candidate;
    }
    if (!Entry || !Result || !DropCargoAction) return false;
    FEdGraphPinType BoolType;
    BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    if (!FBlueprintEditorUtils::AddLocalVariable(
            Blueprint, Graph, CargoAvailabilityLocal, BoolType, CargoAction::Disabled))
        return false;
    UEdGraphPin* EntryThen = RequirePin(Entry, UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* ResultExec = RequirePin(Result, UEdGraphSchema_K2::PN_Execute);
    UEdGraphPin* ResultValue = RequirePin(Result, PinNames::ReturnValue);
    EntryThen->BreakAllPinLinks();
    ResultExec->BreakAllPinLinks();
    ResultValue->BreakAllPinLinks();
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

    UFunction* ProvidedFunction = Blueprint->ParentClass->FindFunctionByName(
        ProvidedActionsFunctionName);
    if (!ProvidedFunction ||
        ProvidedFunction->GetOuterUClass() != AVoyageVehiclePawn::StaticClass())
    {
        UE_LOG(LogTemp, Error,
            TEXT("Inherited GetProvidedActionsBP is not declared by VoyageVehiclePawn"));
        return false;
    }
    UK2Node_CallParentFunction* Parent = NewObject<UK2Node_CallParentFunction>(Graph);
    Parent->SetFromFunction(ProvidedFunction);
    FinishNode(Parent, Graph, -900, -120);
    UK2Node_CallFunction* FourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, -660, -260);
    UK2Node_IfThenElse* FourPortsBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, -420, -120);

    UK2Node_VariableSet* ClearCargoAvailable = AddLocalWrite(
        Blueprint, Graph, CargoAvailabilityLocal, -660, -120);
    SetDefault(ClearCargoAvailable, CargoAvailabilityLocal, CargoAction::Disabled);
    UK2Node_Self* CargoSelf = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, -660, -740);
    UK2Node_CallFunction* CargoComponents = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        -420, -740);
    Schema->TrySetDefaultObject(*RequirePin(CargoComponents, CargoAction::ComponentClass),
        UModuleSocketComponent::StaticClass());
    UK2Node_MacroInstance* CargoLoop = NewObject<UK2Node_MacroInstance>(Graph);
    CargoLoop->SetMacroGraph(StandardForEachLoop());
    FinishNode(CargoLoop, Graph, -180, -120);
    UK2Node_DynamicCast* CargoSocket = NewObject<UK2Node_DynamicCast>(Graph);
    CargoSocket->TargetType = UModuleSocketComponent::StaticClass();
    CargoSocket->SetPurity(false);
    FinishNode(CargoSocket, Graph, 40, -120);
    UK2Node_CallFunction* CargoValid = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 260, -740);
    UK2Node_IfThenElse* CargoValidBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 260, -120);
    UK2Node_VariableGet* CargoPort = AddRead(
        Graph, MooringLayout::PortProperty,
        UModuleSocketComponent::StaticClass(), 480, -740);
    UK2Node_BreakStruct* BreakCargoPort = NewObject<UK2Node_BreakStruct>(Graph);
    BreakCargoPort->StructType = FModuleSocketIOData::StaticStruct();
    BreakCargoPort->bMadeAfterOverridePinRemoval = true;
    FinishNode(BreakCargoPort, Graph, 700, -740);
    UK2Node_EnumEquality* CargoIsMooring = FinishNode(
        NewObject<UK2Node_EnumEquality>(Graph), Graph, 920, -740);
    UK2Node_IfThenElse* CargoMooringBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1140, -120);
    UK2Node_CallFunction* CargoConnected = AddCall(
        Graph, UModuleSocketComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UModuleSocketComponent, IsSocketConnectedOrHasCable),
        1360, -740);
    UK2Node_IfThenElse* CargoConnectedBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1360, -120);
    UK2Node_VariableSet* SetCargoAvailable = AddLocalWrite(
        Blueprint, Graph, CargoAvailabilityLocal, 1580, -120);
    SetDefault(SetCargoAvailable, CargoAvailabilityLocal, ProvidedActionDefaults::Enabled);

    UK2Node_Literal* CargoActionLiteral = FinishNode(
        NewObject<UK2Node_Literal>(Graph), Graph, 40, -980);
    CargoActionLiteral->SetObjectRef(DropCargoAction);
    UK2Node_VariableGet* CargoDescription = AddRead(
        Graph, CargoAction::ActionDescription, UInputAction::StaticClass(), 260, -980);
    UK2Node_MakeStruct* CargoRecord = NewObject<UK2Node_MakeStruct>(Graph);
    CargoRecord->StructType = FPlayerInputInterfaceAction::StaticStruct();
    CargoRecord->bMadeAfterOverridePinRemoval = true;
    FinishNode(CargoRecord, Graph, 480, -980);
    Schema->TrySetDefaultObject(
        *RequirePin(CargoRecord, PinNames::InputAction), DropCargoAction);
    Schema->TrySetDefaultValue(*RequirePin(CargoRecord, PinNames::Name),
        StableGyroAssetNames::DropCargoMappingName);
    Schema->TrySetDefaultValue(*RequirePin(CargoRecord, PinNames::Category),
        StableGyroAssetNames::VehicleInputCategory);
    Schema->TrySetDefaultValue(*RequirePin(CargoRecord, PinNames::Type),
        ProvidedActionDefaults::Type);
    UK2Node_VariableGet* CargoAvailable = AddLocalRead(
        Blueprint, Graph, CargoAvailabilityLocal, 480, -1160);
    UK2Node_VariableGet* CargoTriggerTime = AddRead(
        Graph, DropCargoTriggerTime, AVoyageVehicleGyroCopter::StaticClass(), 480, -1320);
    UK2Node_Self* CargoTriggerSelf = FinishNode(
        NewObject<UK2Node_Self>(Graph), Graph, 260, -1320);
    UK2Node_MakeArray* CargoArray = NewObject<UK2Node_MakeArray>(Graph);
    FinishNode(CargoArray, Graph, 700, -980);
    UK2Node_CallArrayFunction* AppendCargo = NewObject<UK2Node_CallArrayFunction>(Graph);
    AppendCargo->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Append)));
    FinishNode(AppendCargo, Graph, 920, -980);

    UK2Node_VariableGet* PitchHelperComponent = NewObject<UK2Node_VariableGet>(Graph);
    PitchHelperComponent->VariableReference.SetSelfMember(PitchHelperComponentName);
    FinishNode(PitchHelperComponent, Graph, -900, -420);
    UK2Node_VariableGet* GetPitchHelper = NewObject<UK2Node_VariableGet>(Graph);
    GetPitchHelper->VariableReference.SetExternalMember(
        BlueprintGraphNames::ActorProperties::ChildActor,
        UChildActorComponent::StaticClass());
    FinishNode(GetPitchHelper, Graph, -660, -420);
    UK2Node_DynamicCast* CastPitchHelper = NewObject<UK2Node_DynamicCast>(Graph);
    CastPitchHelper->TargetType = PitchHelperClass;
    CastPitchHelper->SetPurity(true);
    FinishNode(CastPitchHelper, Graph, -420, -420);
    UK2Node_VariableGet* PitchCorrectionEnabled = NewObject<UK2Node_VariableGet>(Graph);
    PitchCorrectionEnabled->VariableReference.SetExternalMember(
        Settings::EnablePitchCorrection, PitchHelperClass);
    FinishNode(PitchCorrectionEnabled, Graph, -180, -420);
    UK2Node_IfThenElse* EnabledBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, -660, -120);

    UK2Node_CallFunction* GetSettings = NewObject<UK2Node_CallFunction>(Graph);
    GetSettings->SetFromFunction(UGameUserSettings::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameUserSettings, GetGameUserSettings)));
    FinishNode(GetSettings, Graph, -660, -120);

    UK2Node_DynamicCast* CastSettings = NewObject<UK2Node_DynamicCast>(Graph);
    CastSettings->TargetType = UVoyageGameUserSettings::StaticClass();
    FinishNode(CastSettings, Graph, -660, 300);
    CastSettings->SetPurity(true);
    UK2Node_VariableGet* GetCustomSettings = NewObject<UK2Node_VariableGet>(Graph);
    GetCustomSettings->VariableReference.SetExternalMember(
        GET_MEMBER_NAME_CHECKED(UVoyageGameUserSettings, CustomSettings),
        UVoyageGameUserSettings::StaticClass());
    FinishNode(GetCustomSettings, Graph, -420, 300);
    UK2Node_BreakStruct* BreakSettings = NewObject<UK2Node_BreakStruct>(Graph);
    BreakSettings->StructType = FVoyageCustomGameUserSettings::StaticStruct();
    BreakSettings->bMadeAfterOverridePinRemoval = true;
    FinishNode(BreakSettings, Graph, -180, 300);
    UK2Node_EnumEquality* IsRussian = NewObject<UK2Node_EnumEquality>(Graph);
    FinishNode(IsRussian, Graph, 40, 300);

    UK2Node_CallFunction* SelectText = NewObject<UK2Node_CallFunction>(Graph);
    SelectText->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectText)));
    FinishNode(SelectText, Graph, 260, 300);
    Schema->TrySetDefaultText(*RequirePin(SelectText, SelectPins::WhenTrue),
        FText::FromString(StableGyroAssetNames::ResetRussianLabel));
    Schema->TrySetDefaultText(*RequirePin(SelectText, SelectPins::WhenFalse),
        FText::FromString(StableGyroAssetNames::ResetDisplayLabel));

    UK2Node_MakeStruct* Reset = NewObject<UK2Node_MakeStruct>(Graph);
    Reset->StructType = FPlayerInputInterfaceAction::StaticStruct();
    Reset->bMadeAfterOverridePinRemoval = true;
    FinishNode(Reset, Graph, 520, 80);
    Schema->TrySetDefaultObject(*RequirePin(Reset, PinNames::InputAction), ResetAction);
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Name),
        StableGyroAssetNames::ResetMappingName);
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Category),
        StableGyroAssetNames::VehicleInputCategory);
    Schema->TrySetDefaultText(*RequirePin(Reset, PinNames::Text),
        FText::FromString(StableGyroAssetNames::ResetDisplayLabel));
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Enabled), ProvidedActionDefaults::Enabled);
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Priority), ProvidedActionDefaults::Priority);
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Type), ProvidedActionDefaults::Type);

    UK2Node_MakeArray* ResetArray = NewObject<UK2Node_MakeArray>(Graph);
    FinishNode(ResetArray, Graph, 760, 80);
    UK2Node_CallArrayFunction* Append = NewObject<UK2Node_CallArrayFunction>(Graph);
    Append->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Append)));
    FinishNode(Append, Graph, 1020, -120);

    bool Ok = true;
    Ok &= Schema->TryCreateConnection(EntryThen, RequirePin(Parent, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(Parent, UEdGraphSchema_K2::PN_Then),
        RequirePin(FourPorts, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(FourPorts, UEdGraphSchema_K2::PN_Then),
        RequirePin(FourPortsBranch, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(FourPorts, P::ReturnValue),
        RequirePin(FourPortsBranch, P::Condition));
    Ok &= Schema->TryCreateConnection(RequirePin(FourPortsBranch, P::Then),
        RequirePin(ClearCargoAvailable, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(FourPortsBranch, P::Else),
        RequirePin(EnabledBranch, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(CargoSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(CargoComponents, P::FunctionTarget));
    Ok &= Schema->TryCreateConnection(RequirePin(ClearCargoAvailable, P::Then),
        RequireExecInputPin(CargoLoop));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoComponents, P::ReturnValue),
        RequirePin(CargoLoop, CargoAction::Array));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoLoop, P::LoopBody),
        RequirePin(CargoSocket, P::Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoLoop, CargoAction::ArrayElement),
        CargoSocket->GetCastSourcePin());
    Ok &= Schema->TryCreateConnection(CargoSocket->GetValidCastPin(),
        RequirePin(CargoValidBranch, P::Execute));
    Ok &= Schema->TryCreateConnection(CargoSocket->GetCastResultPin(),
        RequirePin(CargoValid, P::Object));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoValid, P::ReturnValue),
        RequirePin(CargoValidBranch, P::Condition));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoValidBranch, P::Then),
        RequirePin(CargoMooringBranch, P::Execute));
    Ok &= Schema->TryCreateConnection(CargoSocket->GetCastResultPin(),
        RequirePin(CargoPort, P::FunctionTarget));
    Ok &= Schema->TryCreateConnection(CargoPort->GetValuePin(), FindInputPin(BreakCargoPort));
    Ok &= Schema->TryCreateConnection(RequirePin(BreakCargoPort, CargoAction::ResourceType),
        CargoIsMooring->GetInput1Pin());
    Schema->TrySetDefaultValue(*CargoIsMooring->GetInput2Pin(),
        MooringLayout::MooringResourceEnum);
    Ok &= Schema->TryCreateConnection(CargoIsMooring->GetReturnValuePin(),
        RequirePin(CargoMooringBranch, P::Condition));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoMooringBranch, P::Then),
        RequirePin(CargoConnectedBranch, P::Execute));
    Ok &= Schema->TryCreateConnection(CargoSocket->GetCastResultPin(),
        RequirePin(CargoConnected, P::FunctionTarget));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoConnected, P::ReturnValue),
        RequirePin(CargoConnectedBranch, P::Condition));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoConnectedBranch, P::Then),
        RequirePin(SetCargoAvailable, P::Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoLoop, P::Completed),
        RequirePin(AppendCargo, P::Execute));
    Ok &= Schema->TryCreateConnection(CargoActionLiteral->GetValuePin(),
        RequirePin(CargoDescription, P::FunctionTarget));
    Ok &= Schema->TryCreateConnection(RequirePin(CargoDescription, CargoAction::ActionDescription),
        RequirePin(CargoRecord, PinNames::Text));
    Ok &= Schema->TryCreateConnection(CargoAvailable->GetValuePin(),
        RequirePin(CargoRecord, PinNames::Enabled));
    Ok &= Schema->TryCreateConnection(
        CargoTriggerSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(CargoTriggerTime, P::FunctionTarget));
    Ok &= Schema->TryCreateConnection(CargoTriggerTime->GetValuePin(),
        RequirePin(CargoRecord, PinNames::TriggerTime));
    Ok &= Schema->TryCreateConnection(FindOutputPin(CargoRecord),
        RequirePin(CargoArray, PinNames::FirstArrayElement));
    Ok &= Schema->TryCreateConnection(CargoArray->GetOutputPin(),
        RequirePin(AppendCargo, PinNames::SourceArray));
    Ok &= Schema->TryCreateConnection(RequirePin(Parent, PinNames::ReturnValue),
        RequirePin(AppendCargo, PinNames::TargetArray));
    Ok &= Schema->TryCreateConnection(RequirePin(AppendCargo, P::Then),
        RequirePin(EnabledBranch, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(
        RequirePin(EnabledBranch, BlueprintGraphNames::Pins::Condition),
        RequirePin(PitchCorrectionEnabled, Settings::EnablePitchCorrection));
    Ok &= Schema->TryCreateConnection(RequirePin(EnabledBranch, UEdGraphSchema_K2::PN_Then),
        RequirePin(GetSettings, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(EnabledBranch, UEdGraphSchema_K2::PN_Else),
        ResultExec);
    Ok &= Schema->TryCreateConnection(RequirePin(GetSettings, UEdGraphSchema_K2::PN_Then),
        RequirePin(Append, UEdGraphSchema_K2::PN_Execute));
    Ok &= Schema->TryCreateConnection(RequirePin(Append, UEdGraphSchema_K2::PN_Then), ResultExec);
    Ok &= Schema->TryCreateConnection(RequirePin(GetSettings, PinNames::ReturnValue),
        CastSettings->GetCastSourcePin());
    Ok &= Schema->TryCreateConnection(CastSettings->GetCastResultPin(),
        RequirePin(GetCustomSettings, UEdGraphSchema_K2::PN_Self));
    Ok &= Schema->TryCreateConnection(GetCustomSettings->GetValuePin(), FindInputPin(BreakSettings));
    Ok &= Schema->TryCreateConnection(RequirePin(BreakSettings, PinNames::LanguageType),
        IsRussian->GetInput1Pin());
    Schema->TrySetDefaultValue(*IsRussian->GetInput2Pin(), ProvidedActionDefaults::RussianLanguage);
    Ok &= Schema->TryCreateConnection(IsRussian->GetReturnValuePin(),
        RequirePin(SelectText, SelectPins::Condition));
    Ok &= Schema->TryCreateConnection(RequirePin(SelectText, PinNames::ReturnValue),
        RequirePin(Reset, PinNames::Text));
    Ok &= Schema->TryCreateConnection(FindOutputPin(Reset),
        RequirePin(ResetArray, PinNames::FirstArrayElement));
    Ok &= Schema->TryCreateConnection(ResetArray->GetOutputPin(),
        RequirePin(Append, PinNames::SourceArray));
    Ok &= Schema->TryCreateConnection(RequirePin(Parent, PinNames::ReturnValue),
        RequirePin(Append, PinNames::TargetArray));
    Ok &= Schema->TryCreateConnection(RequirePin(Parent, PinNames::ReturnValue), ResultValue);
    Ok &= Schema->TryCreateConnection(PitchHelperComponent->GetValuePin(),
        RequirePin(GetPitchHelper, BlueprintGraphNames::Pins::FunctionTarget));
    Ok &= Schema->TryCreateConnection(GetPitchHelper->GetValuePin(),
        CastPitchHelper->GetCastSourcePin());
    Ok &= Schema->TryCreateConnection(CastPitchHelper->GetCastResultPin(),
        RequirePin(PitchCorrectionEnabled, BlueprintGraphNames::Pins::FunctionTarget));
    return Ok;
}

bool AddHorizontalVelocityDamping(
    UEdGraph* Graph,
    UK2Node_Event* TickEvent,
    UK2Node_DynamicCast* Gyro,
    UEdGraphPin* StartExecution,
    UK2Node_IfThenElse* Continuation,
    UClass* GyroOwner)
{
    namespace P = BlueprintGraphNames::Pins;
    namespace Binary = BlueprintGraphNames::Pins::Binary;

    UK2Node_VariableGet* Root = AddRead(
        Graph, RootComponent, AActor::StaticClass(), 7810, 660);
    UK2Node_DynamicCast* PrimitiveRoot = NewObject<UK2Node_DynamicCast>(Graph);
    PrimitiveRoot->TargetType = UPrimitiveComponent::StaticClass();
    PrimitiveRoot->SetPurity(false);
    FinishNode(PrimitiveRoot, Graph, 8040, 660);
    UK2Node_CallFunction* LinearVelocity = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetPhysicsLinearVelocity), 8270, 660);
    SetDefault(LinearVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    UK2Node_CallFunction* BreakVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 8500, 920);
    UK2Node_CallFunction* HorizontalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 8730, 920);
    SetDefault(HorizontalVelocity, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);

    UK2Node_CallFunction* ActorForward = AddCall(
        Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetActorForwardVector), 8270, 1120);
    UK2Node_CallFunction* BreakActorForward = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 8500, 1120);
    UK2Node_CallFunction* HorizontalActorForward = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 8730, 1120);
    SetDefault(HorizontalActorForward, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);
    UK2Node_CallFunction* NormalizedActorForward = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Vector_Normal2D), 8960, 1120);
    UK2Node_CallFunction* ActorRight = AddCall(
        Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetActorRightVector), 8270, 1320);
    UK2Node_CallFunction* BreakActorRight = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 8500, 1320);
    UK2Node_CallFunction* HorizontalActorRight = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 8730, 1320);
    SetDefault(HorizontalActorRight, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);
    UK2Node_CallFunction* NormalizedActorRight = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Vector_Normal2D), 8960, 1320);

    UK2Node_VariableGet* ForwardTilt = AddRead(
        Graph, TiltForwardInput, GyroOwner, 7810, 1540);
    UK2Node_VariableGet* SideTilt = AddRead(
        Graph, TiltInput, GyroOwner, 7810, 1660);
    UK2Node_CallFunction* ControlTilt = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 8040, 1540);
    SetDefault(ControlTilt, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);
    UK2Node_CallFunction* ControlIsNeutral = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Vector_IsNearlyZero), 8270, 1540);
    SetDefault(ControlIsNeutral, ArcadeHorizontal::Tolerance, ArcadeHorizontal::NeutralTolerance);
    UK2Node_CallFunction* ForwardIntent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat), 9190, 1120);
    UK2Node_CallFunction* SideIntent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat), 9190, 1320);
    UK2Node_CallFunction* CombinedIntent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector), 9420, 1220);
    UK2Node_CallFunction* NormalizedIntent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Vector_Normal2D), 9650, 1220);
    UK2Node_CallFunction* ZeroVector = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 8500, 1660);
    SetDefault(ZeroVector, ArcadeAltitude::VectorX, ArcadeAltitude::Zero);
    SetDefault(ZeroVector, ArcadeAltitude::VectorY, ArcadeAltitude::Zero);
    SetDefault(ZeroVector, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);
    UK2Node_CallFunction* EffectiveDirection = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectVector), 9880, 1420);

    UK2Node_CallFunction* AlongTiltSpeed = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Dot_VectorVector), 9190, 920);
    UK2Node_CallFunction* PositiveAlongTiltSpeed = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), 9420, 920);
    SetDefault(PositiveAlongTiltSpeed, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* PreservedVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat), 9650, 920);
    UK2Node_CallFunction* VelocityToDamp = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), 9880, 920);
    UK2Node_CallFunction* DampedVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VInterpTo_Constant), 10110, 920);
    UK2Node_VariableGet* HorizontalAcceleration = AddRead(
        Graph, Settings::HorizontalVelocityDecayAcceleration, nullptr, 9880, 1260);
    UK2Node_CallFunction* NewHorizontalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector), 10340, 920);
    UK2Node_CallFunction* VelocityCorrection = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), 10570, 920);
    UK2Node_CallFunction* SetVelocity = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetPhysicsLinearVelocity), 10800, 660);
    SetDefault(SetVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    SetDefault(SetVelocity, ArcadeAltitude::AddToCurrent, ArcadeAltitude::Enabled);

    UK2Node_VariableGet* PreviousVelocity = AddRead(
        Graph, PreviousCorrectedHorizontalVelocity, nullptr, 8730, 1880);
    UK2Node_VariableGet* SampleValid = AddRead(
        Graph, HorizontalVelocitySampleValid, nullptr, 8730, 2020);
    UK2Node_CallFunction* PreviousForComparison = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectVector), 8960, 1880);
    UK2Node_CallFunction* ObservedSpeed = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize), 9190, 2160);
    UK2Node_CallFunction* PreviousSpeed = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize), 9420, 2160);
    UK2Node_CallFunction* PreviousSpeedWithTolerance = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::AddDouble, 9650, 2320);
    SetDefault(PreviousSpeedWithTolerance,
        Binary::RightOperand, ArcadeHorizontal::GrowthTolerance);
    UK2Node_CallFunction* GrowthDetected = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 9880, 2160);
    UK2Node_CallFunction* ValidGrowthDetected = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 10110, 2160);
    UK2Node_CallFunction* NeutralGrowthDetected = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 10340, 2160);
    UK2Node_CallFunction* VelocityAfterGrowthGuard = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectVector), 10570, 1880);
    UK2Node_VariableSet* StoreCorrectedVelocity = AddWrite(
        Graph, PreviousCorrectedHorizontalVelocity, 11260, 660);
    UK2Node_VariableSet* MarkSampleValid = AddWrite(
        Graph, HorizontalVelocitySampleValid, 11490, 660);
    SetDefault(MarkSampleValid, HorizontalVelocitySampleValid, ArcadeAltitude::Enabled);

    bool Ok = true;
    Ok &= Connect(StartExecution, RequirePin(PrimitiveRoot, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Root, P::FunctionTarget));
    Ok &= Connect(RequirePin(Root, RootComponent), PrimitiveRoot->GetCastSourcePin());
    Ok &= Connect(PrimitiveRoot->GetValidCastPin(), RequirePin(LinearVelocity, P::Execute));
    Ok &= Connect(PrimitiveRoot->GetCastResultPin(), RequirePin(LinearVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(LinearVelocity, P::Then), RequirePin(SetVelocity, P::Execute));
    Ok &= Connect(RequirePin(LinearVelocity, P::ReturnValue),
        RequirePin(BreakVelocity, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakVelocity, ArcadeAltitude::VectorX),
        RequirePin(HorizontalVelocity, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(BreakVelocity, ArcadeAltitude::VectorY),
        RequirePin(HorizontalVelocity, ArcadeAltitude::VectorY));

    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(ActorForward, P::FunctionTarget));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(ActorRight, P::FunctionTarget));
    Ok &= Connect(RequirePin(ActorForward, P::ReturnValue),
        RequirePin(BreakActorForward, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakActorForward, ArcadeAltitude::VectorX),
        RequirePin(HorizontalActorForward, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(BreakActorForward, ArcadeAltitude::VectorY),
        RequirePin(HorizontalActorForward, ArcadeAltitude::VectorY));
    Ok &= Connect(RequirePin(HorizontalActorForward, P::ReturnValue),
        RequirePin(NormalizedActorForward, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ActorRight, P::ReturnValue),
        RequirePin(BreakActorRight, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakActorRight, ArcadeAltitude::VectorX),
        RequirePin(HorizontalActorRight, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(BreakActorRight, ArcadeAltitude::VectorY),
        RequirePin(HorizontalActorRight, ArcadeAltitude::VectorY));
    Ok &= Connect(RequirePin(HorizontalActorRight, P::ReturnValue),
        RequirePin(NormalizedActorRight, Binary::LeftOperand));

    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(ForwardTilt, P::FunctionTarget));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(SideTilt, P::FunctionTarget));
    Ok &= Connect(RequirePin(ForwardTilt, TiltForwardInput),
        RequirePin(ControlTilt, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(SideTilt, TiltInput),
        RequirePin(ControlTilt, ArcadeAltitude::VectorY));
    Ok &= Connect(RequirePin(ControlTilt, P::ReturnValue),
        RequirePin(ControlIsNeutral, Binary::LeftOperand));
    Ok &= Connect(RequirePin(NormalizedActorForward, P::ReturnValue),
        RequirePin(ForwardIntent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ForwardTilt, TiltForwardInput),
        RequirePin(ForwardIntent, Binary::RightOperand));
    Ok &= Connect(RequirePin(NormalizedActorRight, P::ReturnValue),
        RequirePin(SideIntent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(SideTilt, TiltInput),
        RequirePin(SideIntent, Binary::RightOperand));
    Ok &= Connect(RequirePin(ForwardIntent, P::ReturnValue),
        RequirePin(CombinedIntent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(SideIntent, P::ReturnValue),
        RequirePin(CombinedIntent, Binary::RightOperand));
    Ok &= Connect(RequirePin(CombinedIntent, P::ReturnValue),
        RequirePin(NormalizedIntent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ZeroVector, P::ReturnValue),
        RequirePin(EffectiveDirection, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(NormalizedIntent, P::ReturnValue),
        RequirePin(EffectiveDirection, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(ControlIsNeutral, P::ReturnValue),
        RequirePin(EffectiveDirection, SelectPins::Condition));

    Ok &= Connect(RequirePin(VelocityAfterGrowthGuard, P::ReturnValue),
        RequirePin(AlongTiltSpeed, Binary::LeftOperand));
    Ok &= Connect(RequirePin(EffectiveDirection, P::ReturnValue),
        RequirePin(AlongTiltSpeed, Binary::RightOperand));
    Ok &= Connect(RequirePin(AlongTiltSpeed, P::ReturnValue),
        RequirePin(PositiveAlongTiltSpeed, Binary::LeftOperand));
    Ok &= Connect(RequirePin(EffectiveDirection, P::ReturnValue),
        RequirePin(PreservedVelocity, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PositiveAlongTiltSpeed, P::ReturnValue),
        RequirePin(PreservedVelocity, Binary::RightOperand));
    Ok &= Connect(RequirePin(VelocityAfterGrowthGuard, P::ReturnValue),
        RequirePin(VelocityToDamp, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreservedVelocity, P::ReturnValue),
        RequirePin(VelocityToDamp, Binary::RightOperand));
    Ok &= Connect(RequirePin(VelocityToDamp, P::ReturnValue),
        RequirePin(DampedVelocity, ArcadeHorizontal::Current));
    Ok &= Connect(RequirePin(ZeroVector, P::ReturnValue),
        RequirePin(DampedVelocity, ArcadeHorizontal::Target));
    Ok &= Connect(RequirePin(TickEvent, P::DeltaSeconds),
        RequirePin(DampedVelocity, ArcadeHorizontal::DeltaTime));
    Ok &= Connect(RequirePin(HorizontalAcceleration,
        Settings::HorizontalVelocityDecayAcceleration),
        RequirePin(DampedVelocity, ArcadeHorizontal::InterpSpeed));
    Ok &= Connect(RequirePin(PreservedVelocity, P::ReturnValue),
        RequirePin(NewHorizontalVelocity, Binary::LeftOperand));
    Ok &= Connect(RequirePin(DampedVelocity, P::ReturnValue),
        RequirePin(NewHorizontalVelocity, Binary::RightOperand));
    Ok &= Connect(RequirePin(NewHorizontalVelocity, P::ReturnValue),
        RequirePin(VelocityCorrection, Binary::LeftOperand));
    Ok &= Connect(RequirePin(HorizontalVelocity, P::ReturnValue),
        RequirePin(VelocityCorrection, Binary::RightOperand));
    Ok &= Connect(PrimitiveRoot->GetCastResultPin(), RequirePin(SetVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(VelocityCorrection, P::ReturnValue),
        RequirePin(SetVelocity, ArcadeAltitude::NewVelocity));

    Ok &= Connect(RequirePin(PreviousVelocity, PreviousCorrectedHorizontalVelocity),
        RequirePin(PreviousForComparison, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(HorizontalVelocity, P::ReturnValue),
        RequirePin(PreviousForComparison, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(SampleValid, HorizontalVelocitySampleValid),
        RequirePin(PreviousForComparison, SelectPins::Condition));
    Ok &= Connect(RequirePin(HorizontalVelocity, P::ReturnValue),
        RequirePin(ObservedSpeed, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreviousForComparison, P::ReturnValue),
        RequirePin(PreviousSpeed, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreviousSpeed, P::ReturnValue),
        RequirePin(PreviousSpeedWithTolerance, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ObservedSpeed, P::ReturnValue),
        RequirePin(GrowthDetected, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreviousSpeedWithTolerance, P::ReturnValue),
        RequirePin(GrowthDetected, Binary::RightOperand));
    Ok &= Connect(RequirePin(SampleValid, HorizontalVelocitySampleValid),
        RequirePin(ValidGrowthDetected, Binary::LeftOperand));
    Ok &= Connect(RequirePin(GrowthDetected, P::ReturnValue),
        RequirePin(ValidGrowthDetected, Binary::RightOperand));
    Ok &= Connect(RequirePin(ControlIsNeutral, P::ReturnValue),
        RequirePin(NeutralGrowthDetected, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ValidGrowthDetected, P::ReturnValue),
        RequirePin(NeutralGrowthDetected, Binary::RightOperand));
    Ok &= Connect(RequirePin(PreviousForComparison, P::ReturnValue),
        RequirePin(VelocityAfterGrowthGuard, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(HorizontalVelocity, P::ReturnValue),
        RequirePin(VelocityAfterGrowthGuard, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(NeutralGrowthDetected, P::ReturnValue),
        RequirePin(VelocityAfterGrowthGuard, SelectPins::Condition));
    Ok &= Connect(RequirePin(NewHorizontalVelocity, P::ReturnValue),
        RequirePin(StoreCorrectedVelocity, PreviousCorrectedHorizontalVelocity));
    Ok &= Connect(RequirePin(StoreCorrectedVelocity, P::Then),
        RequirePin(MarkSampleValid, P::Execute));
    Ok &= Connect(RequirePin(SetVelocity, P::Then),
        RequirePin(StoreCorrectedVelocity, P::Execute));
    Ok &= Connect(RequirePin(MarkSampleValid, P::Then),
        RequirePin(Continuation, P::Execute));
    return Ok;
}

bool AddArcadeAltitudeHold(
    UBlueprint* Blueprint,
    UClass* GyroOwner)
{
    namespace P = BlueprintGraphNames::Pins;
    namespace Binary = BlueprintGraphNames::Pins::Binary;

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !GyroOwner) return false;

    UK2Node_Event* TickEvent = NewObject<UK2Node_Event>(Graph);
    TickEvent->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    TickEvent->bOverrideFunction = true;
    FinishNode(TickEvent, Graph, 0, 0);

    UK2Node_CallFunction* GetParent = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor), 220, 180);
    UK2Node_DynamicCast* Gyro = NewObject<UK2Node_DynamicCast>(Graph);
    Gyro->TargetType = GyroOwner;
    Gyro->SetPurity(false);
    FinishNode(Gyro, Graph, 450, 0);
    UK2Node_CallFunction* Controlled = AddCall(Graph, APawn::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), 680, 0);
    UK2Node_IfThenElse* ControlledBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 910, 0);

    UK2Node_VariableGet* MeasuredRotorVelocity = AddRead(
        Graph, PropellerVelocity, GyroOwner, 1140, 240);
    UK2Node_VariableGet* MinimumRotorVelocity = AddRead(
        Graph, Settings::StabilizationMinimumPropellerVelocity, nullptr, 1370, 360);
    UK2Node_CallFunction* RotorIsFastEnough = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 1600, 240);
    UK2Node_VariableGet* IsEngineRunning = AddRead(
        Graph, EngineRunning, GyroOwner, 1140, 420);
    UK2Node_VariableGet* Throttle = AddRead(
        Graph, CurrentThrottle, GyroOwner, 1140, 540);
    UK2Node_CallFunction* HasPositiveThrottle = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 1370, 540);
    SetDefault(HasPositiveThrottle, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* HasPropulsion = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 1600, 480);
    UK2Node_CallFunction* StabilizationEligible = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 1830, 360);
    UK2Node_IfThenElse* RotorGateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2060, 0);

    UK2Node_Self* TickSelf = FinishNode(
        NewObject<UK2Node_Self>(Graph), Graph, 2060, 260);
    UK2Node_CallFunction* CurrentGameTime = AddCall(
        Graph, UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetTimeSeconds), 2290, 260);
    UK2Node_VariableGet* ReleaseDeadline = AddRead(
        Graph, CollisionReleaseUntilTime, nullptr, 2290, 420);
    UK2Node_CallFunction* CollisionReleasePending = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 2520, 360);
    UK2Node_IfThenElse* CollisionReleaseBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2290, 0);

    UK2Node_VariableGet* CollectiveValue = AddRead(
        Graph, Collective, GyroOwner, 2060, 620);
    UK2Node_VariableGet* MinimumThrottlePercent = AddRead(
        Graph, Settings::AltitudeStabilizationMinimumThrottlePercent, nullptr, 2060, 760);
    UK2Node_CallFunction* MinimumCollective = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble), 2290, 700);
    SetDefault(MinimumCollective, Binary::RightOperand, ArcadeAltitude::PercentScale);
    UK2Node_CallFunction* CollectiveReady = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 2520, 620);
    UK2Node_IfThenElse* VerticalThrottleGateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2750, 620);

    UK2Node_VariableSet* ClearWhenBelowMinimumThrottle = AddWrite(
        Graph, AltitudeLockActive, 2980, 620);
    SetDefault(ClearWhenBelowMinimumThrottle,
        AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenBelowMinimumThrottle = AddWrite(
        Graph, AltitudeExactHoldActive, 3210, 620);
    SetDefault(ClearExactWhenBelowMinimumThrottle,
        AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenBelowMinimumThrottle = AddWrite(
        Graph, SpaceWasHeld, 3440, 620);
    SetDefault(ClearSpaceWhenBelowMinimumThrottle,
        SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearDescentWhenBelowMinimumThrottle = AddWrite(
        Graph, DescentWasHeld, 3670, 620);
    SetDefault(ClearDescentWhenBelowMinimumThrottle,
        DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingWhenBelowMinimumThrottle = AddWrite(
        Graph, AltitudeBrakingActive, 3900, 620);
    SetDefault(StopBrakingWhenBelowMinimumThrottle,
        AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingWhenBelowMinimumThrottle = AddWrite(
        Graph, AltitudeDescentBrakingActive, 4130, 620);
    SetDefault(StopDescentBrakingWhenBelowMinimumThrottle,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenBelowMinimumThrottle = AddWrite(
        Graph, VerticalVelocitySampleValid, 4360, 620);
    SetDefault(ResetVerticalSampleWhenBelowMinimumThrottle,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);

    UK2Node_CallFunction* Controller = AddCall(Graph, APawn::StaticClass(),
        BlueprintGraphNames::ActorFunctions::GetController, 2060, 1100);
    UK2Node_DynamicCast* PlayerController = NewObject<UK2Node_DynamicCast>(Graph);
    PlayerController->TargetType = APlayerController::StaticClass();
    PlayerController->SetPurity(true);
    FinishNode(PlayerController, Graph, 2290, 1100);
    UK2Node_CallFunction* SpaceHeld = AddCall(Graph, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, IsInputKeyDown), 2520, 1040);
    SetDefault(SpaceHeld, P::Key, ArcadeAltitude::SpaceKey);
    UK2Node_CallFunction* DescentHeld = AddCall(Graph, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, IsInputKeyDown), 2520, 1220);
    SetDefault(DescentHeld, P::Key, ArcadeAltitude::DescentKey);
    UK2Node_IfThenElse* DescentBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 4130, 700);

    UK2Node_VariableSet* ClearWhenUncontrolled = AddWrite(
        Graph, AltitudeLockActive, 1600, -220);
    SetDefault(ClearWhenUncontrolled, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenUncontrolled = AddWrite(
        Graph, AltitudeExactHoldActive, 1830, -220);
    SetDefault(ClearExactWhenUncontrolled, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenUncontrolled = AddWrite(
        Graph, SpaceWasHeld, 2060, -220);
    SetDefault(ClearSpaceWhenUncontrolled, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearDescentWhenUncontrolled = AddWrite(
        Graph, DescentWasHeld, 2290, -220);
    SetDefault(ClearDescentWhenUncontrolled, DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingWhenUncontrolled = AddWrite(
        Graph, AltitudeBrakingActive, 2520, -220);
    SetDefault(StopBrakingWhenUncontrolled, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingWhenUncontrolled = AddWrite(
        Graph, AltitudeDescentBrakingActive, 2750, -220);
    SetDefault(StopDescentBrakingWhenUncontrolled,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetHorizontalSampleWhenUncontrolled = AddWrite(
        Graph, HorizontalVelocitySampleValid, 2980, -220);
    SetDefault(ResetHorizontalSampleWhenUncontrolled,
        HorizontalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenUncontrolled = AddWrite(
        Graph, VerticalVelocitySampleValid, 3210, -220);
    SetDefault(ResetVerticalSampleWhenUncontrolled,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearWhenDescending = AddWrite(
        Graph, AltitudeLockActive, 1830, 580);
    SetDefault(ClearWhenDescending, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenDescending = AddWrite(
        Graph, AltitudeExactHoldActive, 2060, 580);
    SetDefault(ClearExactWhenDescending, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenDescending = AddWrite(
        Graph, SpaceWasHeld, 2290, 580);
    SetDefault(ClearSpaceWhenDescending, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* MarkDescentHeld = AddWrite(
        Graph, DescentWasHeld, 2520, 580);
    SetDefault(MarkDescentHeld, DescentWasHeld, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* StopBrakingWhenDescending = AddWrite(
        Graph, AltitudeBrakingActive, 2750, 580);
    SetDefault(StopBrakingWhenDescending, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingWhenDescending = AddWrite(
        Graph, AltitudeDescentBrakingActive, 2980, 580);
    SetDefault(StopDescentBrakingWhenDescending,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenDescending = AddWrite(
        Graph, VerticalVelocitySampleValid, 3210, 580);
    SetDefault(ResetVerticalSampleWhenDescending,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearWhenRotorSlow = AddWrite(
        Graph, AltitudeLockActive, 8730, 180);
    SetDefault(ClearWhenRotorSlow, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenRotorSlow = AddWrite(
        Graph, AltitudeExactHoldActive, 8960, 180);
    SetDefault(ClearExactWhenRotorSlow,
        AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenRotorSlow = AddWrite(
        Graph, SpaceWasHeld, 9190, 180);
    SetDefault(ClearSpaceWhenRotorSlow, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearDescentWhenRotorSlow = AddWrite(
        Graph, DescentWasHeld, 9420, 180);
    SetDefault(ClearDescentWhenRotorSlow, DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingWhenRotorSlow = AddWrite(
        Graph, AltitudeBrakingActive, 9650, 180);
    SetDefault(StopBrakingWhenRotorSlow,
        AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingWhenRotorSlow = AddWrite(
        Graph, AltitudeDescentBrakingActive, 9880, 180);
    SetDefault(StopDescentBrakingWhenRotorSlow,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetHorizontalSampleWhenRotorSlow = AddWrite(
        Graph, HorizontalVelocitySampleValid, 10110, 180);
    SetDefault(ResetHorizontalSampleWhenRotorSlow,
        HorizontalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenRotorSlow = AddWrite(
        Graph, VerticalVelocitySampleValid, 10340, 180);
    SetDefault(ResetVerticalSampleWhenRotorSlow,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);

    UK2Node_VariableSet* ClearWhenCollisionReleased = AddWrite(
        Graph, AltitudeLockActive, 2750, 300);
    SetDefault(ClearWhenCollisionReleased, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenCollisionReleased = AddWrite(
        Graph, AltitudeExactHoldActive, 2980, 300);
    SetDefault(ClearExactWhenCollisionReleased,
        AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenCollisionReleased = AddWrite(
        Graph, SpaceWasHeld, 3210, 300);
    SetDefault(ClearSpaceWhenCollisionReleased, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearDescentWhenCollisionReleased = AddWrite(
        Graph, DescentWasHeld, 3440, 300);
    SetDefault(ClearDescentWhenCollisionReleased,
        DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingWhenCollisionReleased = AddWrite(
        Graph, AltitudeBrakingActive, 3670, 300);
    SetDefault(StopBrakingWhenCollisionReleased,
        AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingWhenCollisionReleased = AddWrite(
        Graph, AltitudeDescentBrakingActive, 3900, 300);
    SetDefault(StopDescentBrakingWhenCollisionReleased,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetHorizontalSampleWhenCollisionReleased = AddWrite(
        Graph, HorizontalVelocitySampleValid, 4130, 300);
    SetDefault(ResetHorizontalSampleWhenCollisionReleased,
        HorizontalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenCollisionReleased = AddWrite(
        Graph, VerticalVelocitySampleValid, 4360, 300);
    SetDefault(ResetVerticalSampleWhenCollisionReleased,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);

    UK2Node_VariableGet* IsLockActive = AddRead(
        Graph, AltitudeLockActive, nullptr, 1830, 940);
    UK2Node_IfThenElse* LockStateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2060, 700);
    UK2Node_IfThenElse* SpaceModeBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2290, 700);
    UK2Node_VariableGet* IsExactHoldActive = AddRead(
        Graph, AltitudeExactHoldActive, nullptr, 2520, 1740);
    UK2Node_IfThenElse* ExactHoldStateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2750, 1740);
    UK2Node_VariableSet* ClearExactForSpace = AddWrite(
        Graph, AltitudeExactHoldActive, 2520, 940);
    SetDefault(ClearExactForSpace, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* MarkSpaceHeld = AddWrite(
        Graph, SpaceWasHeld, 2520, 820);
    SetDefault(MarkSpaceHeld, SpaceWasHeld, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* ClearDescentForSpace = AddWrite(
        Graph, DescentWasHeld, 2750, 820);
    SetDefault(ClearDescentForSpace, DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingForSpace = AddWrite(
        Graph, AltitudeBrakingActive, 2980, 820);
    SetDefault(StopBrakingForSpace, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingForSpace = AddWrite(
        Graph, AltitudeDescentBrakingActive, 3210, 820);
    SetDefault(StopDescentBrakingForSpace,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleForSpace = AddWrite(
        Graph, VerticalVelocitySampleValid, 3440, 820);
    SetDefault(ResetVerticalSampleForSpace,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableGet* WasSpaceHeld = AddRead(
        Graph, SpaceWasHeld, nullptr, 2980, 1740);
    UK2Node_IfThenElse* ReleaseStateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 3210, 1740);
    UK2Node_VariableSet* ClearSpaceAfterRelease = AddWrite(
        Graph, SpaceWasHeld, 3440, 1740);
    SetDefault(ClearSpaceAfterRelease, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StartBrakingAfterRelease = AddWrite(
        Graph, AltitudeBrakingActive, 3670, 1740);
    SetDefault(StartBrakingAfterRelease, AltitudeBrakingActive, ArcadeAltitude::Enabled);
    UK2Node_VariableGet* IsBrakingActive = AddRead(
        Graph, AltitudeBrakingActive, nullptr, 3440, 1980);
    UK2Node_CallFunction* ShouldBrake = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), 3210, 1980);

    UK2Node_CallFunction* CaptureLocation = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), 2060, 1100);
    UK2Node_CallFunction* BreakCaptureLocation = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 2290, 1100);
    UK2Node_VariableSet* InitializeLockedAltitude = AddWrite(
        Graph, LockedAltitude, 2290, 840);
    UK2Node_VariableSet* ActivateLock = AddWrite(
        Graph, AltitudeLockActive, 2520, 840);
    SetDefault(ActivateLock, AltitudeLockActive, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* ClearExactAfterLock = AddWrite(
        Graph, AltitudeExactHoldActive, 2750, 840);
    SetDefault(ClearExactAfterLock, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingAfterLock = AddWrite(
        Graph, AltitudeBrakingActive, 2980, 840);
    SetDefault(StopBrakingAfterLock, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableGet* WasDescentHeld = AddRead(
        Graph, DescentWasHeld, nullptr, 2980, 960);
    UK2Node_VariableSet* SelectDescentBrakingAfterLock = AddWrite(
        Graph, AltitudeDescentBrakingActive, 3210, 840);
    UK2Node_VariableSet* ClearDescentAfterLock = AddWrite(
        Graph, DescentWasHeld, 3440, 840);
    SetDefault(ClearDescentAfterLock, DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* PrimeReleaseAfterLock = AddWrite(
        Graph, SpaceWasHeld, 3670, 840);
    SetDefault(PrimeReleaseAfterLock, SpaceWasHeld, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* CaptureExactAltitude = AddWrite(
        Graph, LockedAltitude, 3900, 1740);
    UK2Node_VariableSet* EnableExactHold = AddWrite(
        Graph, AltitudeExactHoldActive, 4130, 1740);
    SetDefault(EnableExactHold, AltitudeExactHoldActive, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* StopBrakingAfterExactActivation = AddWrite(
        Graph, AltitudeBrakingActive, 4360, 1740);
    SetDefault(StopBrakingAfterExactActivation, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingAfterExactActivation = AddWrite(
        Graph, AltitudeDescentBrakingActive, 4590, 1740);
    SetDefault(StopDescentBrakingAfterExactActivation,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleForExact = AddWrite(
        Graph, VerticalVelocitySampleValid, 3670, 1740);
    SetDefault(ResetVerticalSampleForExact,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);

    UK2Node_VariableGet* BrakingRoot = AddRead(
        Graph, RootComponent, AActor::StaticClass(), 3900, 2100);
    UK2Node_DynamicCast* BrakingPrimitiveRoot = NewObject<UK2Node_DynamicCast>(Graph);
    BrakingPrimitiveRoot->TargetType = UPrimitiveComponent::StaticClass();
    BrakingPrimitiveRoot->SetPurity(false);
    FinishNode(BrakingPrimitiveRoot, Graph, 4130, 1980);
    UK2Node_CallFunction* BrakingLinearVelocity = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetPhysicsLinearVelocity), 4360, 1980);
    SetDefault(BrakingLinearVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    UK2Node_CallFunction* BreakBrakingVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 4590, 2100);
    UK2Node_VariableGet* VerticalDeceleration = AddRead(
        Graph, Settings::AltitudeStabilizationVerticalDeceleration, nullptr, 4820, 2280);
    UK2Node_VariableGet* DescentVerticalDeceleration = AddRead(
        Graph, Settings::AltitudeStabilizationDescentDeceleration, nullptr, 4820, 2400);
    UK2Node_VariableGet* IsDescentBraking = AddRead(
        Graph, AltitudeDescentBrakingActive, nullptr, 4820, 2160);
    UK2Node_CallFunction* SelectedVerticalDeceleration = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 5050, 2280);
    UK2Node_CallFunction* VerticalDampingDisabled = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble), 5050, 2700);
    SetDefault(VerticalDampingDisabled, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* HasObservedAscent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 5050, 2820);
    SetDefault(HasObservedAscent, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* HasObservedDescent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble), 5050, 2940);
    SetDefault(HasObservedDescent, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* DescentDampingDisabled = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 5280, 2820);
    UK2Node_CallFunction* AscentDampingDisabled = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 5280, 2940);
    UK2Node_CallFunction* NotDescentBraking = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool), 5050, 3060);
    UK2Node_CallFunction* ObservedInBrakingDirection = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), 5510, 2940);
    UK2Node_CallFunction* SkipPositiveVerticalDamping = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 5740, 2820);
    UK2Node_IfThenElse* PositiveVerticalDampingGate = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 5510, 1980);
    UK2Node_VariableSet* ResetVerticalSampleWhenDampingDisabled = AddWrite(
        Graph, VerticalVelocitySampleValid, 5740, 1800);
    SetDefault(ResetVerticalSampleWhenDampingDisabled,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_CallFunction* BrakingStep = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble), 5050, 2280);
    UK2Node_VariableGet* PreviousVerticalVelocity = AddRead(
        Graph, PreviousCorrectedVerticalVelocity, nullptr, 4820, 2460);
    UK2Node_VariableGet* VerticalSampleValid = AddRead(
        Graph, VerticalVelocitySampleValid, nullptr, 4820, 2580);
    UK2Node_CallFunction* ObservedOrPreviousMinimum = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin), 5050, 2460);
    UK2Node_CallFunction* ObservedOrPreviousMaximum = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), 5050, 2580);
    UK2Node_CallFunction* DirectionalObservedOrPrevious = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 5280, 2580);
    UK2Node_CallFunction* VerticalVelocityBeforeBraking = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 5510, 2460);
    UK2Node_CallFunction* ReducedVerticalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble), 5740, 2160);
    UK2Node_CallFunction* IncreasedVerticalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), 5740, 2280);
    UK2Node_CallFunction* CorrectedVerticalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 5970, 2220);
    UK2Node_CallFunction* IsStillAscending = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 6200, 2100);
    SetDefault(IsStillAscending, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* IsStillDescending = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble), 6200, 2220);
    SetDefault(IsStillDescending, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* ContinuingDescent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 6430, 2220);
    UK2Node_CallFunction* ContinuingAscent = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), 6430, 2340);
    UK2Node_CallFunction* StillMovingVertically = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), 6660, 2280);
    UK2Node_IfThenElse* RemainingAscentBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 6890, 1980);
    UK2Node_CallFunction* VerticalBrakingDelta = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble), 6890, 2220);
    UK2Node_CallFunction* ReducedVelocity = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 7120, 2220);
    SetDefault(ReducedVelocity, ArcadeAltitude::VectorX, ArcadeAltitude::Zero);
    SetDefault(ReducedVelocity, ArcadeAltitude::VectorY, ArcadeAltitude::Zero);
    UK2Node_CallFunction* SetReducedVelocity = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetPhysicsLinearVelocity), 7120, 1980);
    SetDefault(SetReducedVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    SetDefault(SetReducedVelocity, ArcadeAltitude::AddToCurrent, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* StoreCorrectedVerticalVelocity = AddWrite(
        Graph, PreviousCorrectedVerticalVelocity, 7350, 1980);
    UK2Node_VariableSet* MarkVerticalSampleValid = AddWrite(
        Graph, VerticalVelocitySampleValid, 7580, 1980);
    SetDefault(MarkVerticalSampleValid,
        VerticalVelocitySampleValid, ArcadeAltitude::Enabled);

    UK2Node_VariableGet* SpaceRoot = AddRead(
        Graph, RootComponent, AActor::StaticClass(), 3210, 980);
    UK2Node_DynamicCast* SpacePrimitiveRoot = NewObject<UK2Node_DynamicCast>(Graph);
    SpacePrimitiveRoot->TargetType = UPrimitiveComponent::StaticClass();
    SpacePrimitiveRoot->SetPurity(false);
    FinishNode(SpacePrimitiveRoot, Graph, 3440, 820);
    UK2Node_CallFunction* SpaceLinearVelocity = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetPhysicsLinearVelocity), 3670, 820);
    SetDefault(SpaceLinearVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    UK2Node_CallFunction* BreakSpaceVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 3900, 980);
    UK2Node_CallFunction* HasPositiveVerticalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 4130, 980);
    SetDefault(HasPositiveVerticalVelocity, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_IfThenElse* PositiveVerticalVelocityBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 3210, 1120);

    UK2Node_CallFunction* CurrentLocation = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), 2290, 1280);
    UK2Node_CallFunction* BreakCurrentLocation = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 2520, 1280);
    UK2Node_VariableGet* TargetAltitude = AddRead(
        Graph, LockedAltitude, nullptr, 2520, 1460);
    UK2Node_CallFunction* HeightIsRising = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 2750, 1460);
    UK2Node_IfThenElse* RisingBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2980, 940);
    UK2Node_VariableSet* RaiseAltitudeFloor = AddWrite(
        Graph, LockedAltitude, 3210, 940);
    UK2Node_CallFunction* LockedLocation = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 2750, 1280);
    UK2Node_CallFunction* SetLocation = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorLocation), 2980, 700);
    SetDefault(SetLocation, ArcadeAltitude::Sweep, ArcadeAltitude::Enabled);
    SetDefault(SetLocation, ArcadeAltitude::Teleport, ArcadeAltitude::Disabled);

    UK2Node_CallFunction* BreakSweepHit = AddCall(
        Graph, UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult), 3210, 520);
    UK2Node_CallFunction* SweepCollisionDetected = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), 3440, 520);
    UK2Node_IfThenElse* SweepCollisionBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 3210, 700);
    UK2Node_CallFunction* SweepDeadline = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), 3670, 420);
    SetDefault(SweepDeadline, Binary::RightOperand, CollisionRelease::GraceSeconds);
    UK2Node_VariableSet* StoreSweepDeadline = AddWrite(
        Graph, CollisionReleaseUntilTime, 3670, 700);
    UK2Node_VariableSet* ClearAfterSweepHit = AddWrite(
        Graph, AltitudeLockActive, 3900, 700);
    SetDefault(ClearAfterSweepHit, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactAfterSweepHit = AddWrite(
        Graph, AltitudeExactHoldActive, 4130, 700);
    SetDefault(ClearExactAfterSweepHit, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceAfterSweepHit = AddWrite(
        Graph, SpaceWasHeld, 4360, 700);
    SetDefault(ClearSpaceAfterSweepHit, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearDescentAfterSweepHit = AddWrite(
        Graph, DescentWasHeld, 4590, 700);
    SetDefault(ClearDescentAfterSweepHit, DescentWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingAfterSweepHit = AddWrite(
        Graph, AltitudeBrakingActive, 4820, 700);
    SetDefault(StopBrakingAfterSweepHit, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopDescentBrakingAfterSweepHit = AddWrite(
        Graph, AltitudeDescentBrakingActive, 5050, 700);
    SetDefault(StopDescentBrakingAfterSweepHit,
        AltitudeDescentBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetHorizontalSampleAfterSweepHit = AddWrite(
        Graph, HorizontalVelocitySampleValid, 5280, 700);
    SetDefault(ResetHorizontalSampleAfterSweepHit,
        HorizontalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleAfterSweepHit = AddWrite(
        Graph, VerticalVelocitySampleValid, 5510, 700);
    SetDefault(ResetVerticalSampleAfterSweepHit,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);

    UK2Node_VariableGet* Root = AddRead(
        Graph, RootComponent, AActor::StaticClass(), 2980, 1160);
    UK2Node_DynamicCast* PrimitiveRoot = NewObject<UK2Node_DynamicCast>(Graph);
    PrimitiveRoot->TargetType = UPrimitiveComponent::StaticClass();
    PrimitiveRoot->SetPurity(false);
    FinishNode(PrimitiveRoot, Graph, 3210, 700);
    UK2Node_CallFunction* LinearVelocity = AddCall(Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetPhysicsLinearVelocity), 3210, 1160);
    SetDefault(LinearVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    UK2Node_CallFunction* BreakVelocity = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector), 3440, 1160);
    UK2Node_CallFunction* ZeroVerticalVelocityDelta = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble), 3670, 1340);
    SetDefault(ZeroVerticalVelocityDelta, Binary::LeftOperand, ArcadeAltitude::Zero);
    UK2Node_CallFunction* VerticalVelocityCorrection = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 3670, 1160);
    SetDefault(VerticalVelocityCorrection, ArcadeAltitude::VectorX, ArcadeAltitude::Zero);
    SetDefault(VerticalVelocityCorrection, ArcadeAltitude::VectorY, ArcadeAltitude::Zero);
    SetDefault(VerticalVelocityCorrection, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);
    UK2Node_CallFunction* SetVelocity = AddCall(Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetPhysicsLinearVelocity), 3900, 700);
    SetDefault(SetVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    SetDefault(SetVelocity, ArcadeAltitude::AddToCurrent, ArcadeAltitude::Enabled);

    bool Ok = true;
    Ok &= Connect(RequirePin(TickEvent, P::Then), RequirePin(Gyro, P::Execute));
    Ok &= Connect(RequirePin(GetParent, P::ReturnValue), Gyro->GetCastSourcePin());
    Ok &= Connect(Gyro->GetValidCastPin(), RequirePin(ControlledBranch, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Controlled, P::FunctionTarget));
    Ok &= Connect(RequirePin(Controlled, P::ReturnValue),
        RequirePin(ControlledBranch, P::Condition));
    Ok &= Connect(RequirePin(ControlledBranch, P::Then),
        RequirePin(RotorGateBranch, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(),
        RequirePin(MeasuredRotorVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(MeasuredRotorVelocity, PropellerVelocity),
        RequirePin(RotorIsFastEnough, Binary::LeftOperand));
    Ok &= Connect(RequirePin(MinimumRotorVelocity,
        Settings::StabilizationMinimumPropellerVelocity),
        RequirePin(RotorIsFastEnough, Binary::RightOperand));
    Ok &= Connect(Gyro->GetCastResultPin(),
        RequirePin(IsEngineRunning, P::FunctionTarget));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Throttle, P::FunctionTarget));
    Ok &= Connect(RequirePin(Throttle, CurrentThrottle),
        RequirePin(HasPositiveThrottle, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsEngineRunning, EngineRunning),
        RequirePin(HasPropulsion, Binary::LeftOperand));
    Ok &= Connect(RequirePin(HasPositiveThrottle, P::ReturnValue),
        RequirePin(HasPropulsion, Binary::RightOperand));
    Ok &= Connect(RequirePin(HasPropulsion, P::ReturnValue),
        RequirePin(StabilizationEligible, Binary::LeftOperand));
    Ok &= Connect(RequirePin(RotorIsFastEnough, P::ReturnValue),
        RequirePin(StabilizationEligible, Binary::RightOperand));
    Ok &= Connect(RequirePin(StabilizationEligible, P::ReturnValue),
        RequirePin(RotorGateBranch, P::Condition));
    Ok &= Connect(RequirePin(RotorGateBranch, P::Then),
        RequirePin(CollisionReleaseBranch, P::Execute));
    Ok &= Connect(TickSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(CurrentGameTime, CollisionRelease::WorldContextObject));
    Ok &= Connect(RequirePin(ReleaseDeadline, CollisionReleaseUntilTime),
        RequirePin(CollisionReleasePending, Binary::LeftOperand));
    Ok &= Connect(RequirePin(CurrentGameTime, P::ReturnValue),
        RequirePin(CollisionReleasePending, Binary::RightOperand));
    Ok &= Connect(RequirePin(CollisionReleasePending, P::ReturnValue),
        RequirePin(CollisionReleaseBranch, P::Condition));
    Ok &= AddHorizontalVelocityDamping(
        Graph, TickEvent, Gyro, RequirePin(CollisionReleaseBranch, P::Else),
        VerticalThrottleGateBranch, GyroOwner);
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(CollectiveValue, P::FunctionTarget));
    Ok &= Connect(RequirePin(MinimumThrottlePercent,
        Settings::AltitudeStabilizationMinimumThrottlePercent),
        RequirePin(MinimumCollective, Binary::LeftOperand));
    Ok &= Connect(RequirePin(CollectiveValue, Collective),
        RequirePin(CollectiveReady, Binary::LeftOperand));
    Ok &= Connect(RequirePin(MinimumCollective, P::ReturnValue),
        RequirePin(CollectiveReady, Binary::RightOperand));
    Ok &= Connect(RequirePin(CollectiveReady, P::ReturnValue),
        RequirePin(VerticalThrottleGateBranch, P::Condition));
    Ok &= Connect(RequirePin(VerticalThrottleGateBranch, P::Then),
        RequirePin(DescentBranch, P::Execute));
    Ok &= Connect(RequirePin(VerticalThrottleGateBranch, P::Else),
        RequirePin(ClearWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenBelowMinimumThrottle, P::Then),
        RequirePin(ClearExactWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenBelowMinimumThrottle, P::Then),
        RequirePin(ClearSpaceWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenBelowMinimumThrottle, P::Then),
        RequirePin(ClearDescentWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentWhenBelowMinimumThrottle, P::Then),
        RequirePin(StopBrakingWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenBelowMinimumThrottle, P::Then),
        RequirePin(StopDescentBrakingWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingWhenBelowMinimumThrottle, P::Then),
        RequirePin(ResetVerticalSampleWhenBelowMinimumThrottle, P::Execute));
    Ok &= Connect(RequirePin(CollisionReleaseBranch, P::Then),
        RequirePin(ClearWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenCollisionReleased, P::Then),
        RequirePin(ClearExactWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenCollisionReleased, P::Then),
        RequirePin(ClearSpaceWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenCollisionReleased, P::Then),
        RequirePin(ClearDescentWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentWhenCollisionReleased, P::Then),
        RequirePin(StopBrakingWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenCollisionReleased, P::Then),
        RequirePin(StopDescentBrakingWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingWhenCollisionReleased, P::Then),
        RequirePin(ResetHorizontalSampleWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(ResetHorizontalSampleWhenCollisionReleased, P::Then),
        RequirePin(ResetVerticalSampleWhenCollisionReleased, P::Execute));
    Ok &= Connect(RequirePin(RotorGateBranch, P::Else),
        RequirePin(ClearWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenRotorSlow, P::Then),
        RequirePin(ClearExactWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenRotorSlow, P::Then),
        RequirePin(ClearSpaceWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenRotorSlow, P::Then),
        RequirePin(ClearDescentWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentWhenRotorSlow, P::Then),
        RequirePin(StopBrakingWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenRotorSlow, P::Then),
        RequirePin(StopDescentBrakingWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingWhenRotorSlow, P::Then),
        RequirePin(ResetHorizontalSampleWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ResetHorizontalSampleWhenRotorSlow, P::Then),
        RequirePin(ResetVerticalSampleWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ControlledBranch, P::Else), RequirePin(ClearWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenUncontrolled, P::Then), RequirePin(ClearExactWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenUncontrolled, P::Then), RequirePin(ClearSpaceWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenUncontrolled, P::Then), RequirePin(ClearDescentWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentWhenUncontrolled, P::Then), RequirePin(StopBrakingWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenUncontrolled, P::Then),
        RequirePin(StopDescentBrakingWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingWhenUncontrolled, P::Then),
        RequirePin(ResetHorizontalSampleWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ResetHorizontalSampleWhenUncontrolled, P::Then),
        RequirePin(ResetVerticalSampleWhenUncontrolled, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Controller, P::FunctionTarget));
    Ok &= Connect(RequirePin(Controller, P::ReturnValue), PlayerController->GetCastSourcePin());
    Ok &= Connect(PlayerController->GetCastResultPin(), RequirePin(SpaceHeld, P::FunctionTarget));
    Ok &= Connect(PlayerController->GetCastResultPin(), RequirePin(DescentHeld, P::FunctionTarget));
    Ok &= Connect(RequirePin(DescentHeld, P::ReturnValue), RequirePin(DescentBranch, P::Condition));
    Ok &= Connect(RequirePin(DescentBranch, P::Then), RequirePin(ClearWhenDescending, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenDescending, P::Then), RequirePin(ClearExactWhenDescending, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenDescending, P::Then), RequirePin(ClearSpaceWhenDescending, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenDescending, P::Then), RequirePin(MarkDescentHeld, P::Execute));
    Ok &= Connect(RequirePin(MarkDescentHeld, P::Then), RequirePin(StopBrakingWhenDescending, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenDescending, P::Then),
        RequirePin(StopDescentBrakingWhenDescending, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingWhenDescending, P::Then),
        RequirePin(ResetVerticalSampleWhenDescending, P::Execute));
    Ok &= Connect(RequirePin(DescentBranch, P::Else), RequirePin(LockStateBranch, P::Execute));
    Ok &= Connect(RequirePin(IsLockActive, AltitudeLockActive), RequirePin(LockStateBranch, P::Condition));
    Ok &= Connect(RequirePin(LockStateBranch, P::Else), RequirePin(InitializeLockedAltitude, P::Execute));
    Ok &= Connect(RequirePin(LockStateBranch, P::Then), RequirePin(SpaceModeBranch, P::Execute));
    Ok &= Connect(RequirePin(SpaceHeld, P::ReturnValue), RequirePin(SpaceModeBranch, P::Condition));
    Ok &= Connect(RequirePin(SpaceModeBranch, P::Then), RequirePin(MarkSpaceHeld, P::Execute));
    Ok &= Connect(RequirePin(MarkSpaceHeld, P::Then), RequirePin(ClearDescentForSpace, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentForSpace, P::Then), RequirePin(ClearExactForSpace, P::Execute));
    Ok &= Connect(RequirePin(ClearExactForSpace, P::Then), RequirePin(StopBrakingForSpace, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingForSpace, P::Then),
        RequirePin(StopDescentBrakingForSpace, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingForSpace, P::Then),
        RequirePin(ResetVerticalSampleForSpace, P::Execute));
    Ok &= Connect(RequirePin(ResetVerticalSampleForSpace, P::Then),
        RequirePin(SpacePrimitiveRoot, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(SpaceRoot, P::FunctionTarget));
    Ok &= Connect(RequirePin(SpaceRoot, RootComponent),
        SpacePrimitiveRoot->GetCastSourcePin());
    Ok &= Connect(SpacePrimitiveRoot->GetValidCastPin(),
        RequirePin(SpaceLinearVelocity, P::Execute));
    Ok &= Connect(SpacePrimitiveRoot->GetCastResultPin(),
        RequirePin(SpaceLinearVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(SpaceLinearVelocity, P::ReturnValue),
        RequirePin(BreakSpaceVelocity, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(SpaceLinearVelocity, P::Then),
        RequirePin(RisingBranch, P::Execute));
    Ok &= Connect(RequirePin(BreakSpaceVelocity, ArcadeAltitude::VectorZ),
        RequirePin(HasPositiveVerticalVelocity, Binary::LeftOperand));
    Ok &= Connect(RequirePin(HasPositiveVerticalVelocity, P::ReturnValue),
        RequirePin(PositiveVerticalVelocityBranch, P::Condition));
    Ok &= Connect(RequirePin(SpaceModeBranch, P::Else), RequirePin(ExactHoldStateBranch, P::Execute));
    Ok &= Connect(RequirePin(IsExactHoldActive, AltitudeExactHoldActive), RequirePin(ExactHoldStateBranch, P::Condition));
    Ok &= Connect(RequirePin(ExactHoldStateBranch, P::Then), RequirePin(SetLocation, P::Execute));
    Ok &= Connect(RequirePin(ExactHoldStateBranch, P::Else), RequirePin(ReleaseStateBranch, P::Execute));
    Ok &= Connect(RequirePin(WasSpaceHeld, SpaceWasHeld), RequirePin(ShouldBrake, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsBrakingActive, AltitudeBrakingActive), RequirePin(ShouldBrake, Binary::RightOperand));
    Ok &= Connect(RequirePin(ShouldBrake, P::ReturnValue), RequirePin(ReleaseStateBranch, P::Condition));
    Ok &= Connect(RequirePin(ReleaseStateBranch, P::Then), RequirePin(ClearSpaceAfterRelease, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceAfterRelease, P::Then), RequirePin(StartBrakingAfterRelease, P::Execute));
    Ok &= Connect(RequirePin(StartBrakingAfterRelease, P::Then), RequirePin(BrakingPrimitiveRoot, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(CaptureLocation, P::FunctionTarget));
    Ok &= Connect(RequirePin(CaptureLocation, P::ReturnValue), RequirePin(BreakCaptureLocation, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakCaptureLocation, ArcadeAltitude::VectorZ), RequirePin(InitializeLockedAltitude, LockedAltitude));
    Ok &= Connect(RequirePin(InitializeLockedAltitude, P::Then), RequirePin(ActivateLock, P::Execute));
    Ok &= Connect(RequirePin(ActivateLock, P::Then), RequirePin(ClearExactAfterLock, P::Execute));
    Ok &= Connect(RequirePin(ClearExactAfterLock, P::Then), RequirePin(StopBrakingAfterLock, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingAfterLock, P::Then), RequirePin(SelectDescentBrakingAfterLock, P::Execute));
    Ok &= Connect(RequirePin(WasDescentHeld, DescentWasHeld),
        RequirePin(SelectDescentBrakingAfterLock, AltitudeDescentBrakingActive));
    Ok &= Connect(RequirePin(SelectDescentBrakingAfterLock, P::Then),
        RequirePin(ClearDescentAfterLock, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentAfterLock, P::Then), RequirePin(PrimeReleaseAfterLock, P::Execute));
    Ok &= Connect(RequirePin(PrimeReleaseAfterLock, P::Then), RequirePin(SpaceModeBranch, P::Execute));
    Ok &= Connect(RequirePin(BreakCaptureLocation, ArcadeAltitude::VectorZ), RequirePin(CaptureExactAltitude, LockedAltitude));
    Ok &= Connect(RequirePin(CaptureExactAltitude, P::Then), RequirePin(EnableExactHold, P::Execute));
    Ok &= Connect(RequirePin(EnableExactHold, P::Then), RequirePin(StopBrakingAfterExactActivation, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingAfterExactActivation, P::Then), RequirePin(StopDescentBrakingAfterExactActivation, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingAfterExactActivation, P::Then), RequirePin(SetLocation, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(BrakingRoot, P::FunctionTarget));
    Ok &= Connect(RequirePin(BrakingRoot, RootComponent), BrakingPrimitiveRoot->GetCastSourcePin());
    Ok &= Connect(BrakingPrimitiveRoot->GetValidCastPin(), RequirePin(BrakingLinearVelocity, P::Execute));
    Ok &= Connect(BrakingPrimitiveRoot->GetCastResultPin(), RequirePin(BrakingLinearVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(BrakingLinearVelocity, P::ReturnValue), RequirePin(BreakBrakingVelocity, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BrakingLinearVelocity, P::Then),
        RequirePin(PositiveVerticalDampingGate, P::Execute));
    Ok &= Connect(RequirePin(DescentVerticalDeceleration,
        Settings::AltitudeStabilizationDescentDeceleration),
        RequirePin(SelectedVerticalDeceleration, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(VerticalDeceleration,
        Settings::AltitudeStabilizationVerticalDeceleration),
        RequirePin(SelectedVerticalDeceleration, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(IsDescentBraking, AltitudeDescentBrakingActive),
        RequirePin(SelectedVerticalDeceleration, SelectPins::Condition));
    Ok &= Connect(RequirePin(SelectedVerticalDeceleration, P::ReturnValue),
        RequirePin(VerticalDampingDisabled, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(HasObservedAscent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(HasObservedDescent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsDescentBraking, AltitudeDescentBrakingActive),
        RequirePin(DescentDampingDisabled, Binary::LeftOperand));
    Ok &= Connect(RequirePin(HasObservedDescent, P::ReturnValue),
        RequirePin(DescentDampingDisabled, Binary::RightOperand));
    Ok &= Connect(RequirePin(IsDescentBraking, AltitudeDescentBrakingActive),
        RequirePin(NotDescentBraking, Binary::LeftOperand));
    Ok &= Connect(RequirePin(NotDescentBraking, P::ReturnValue),
        RequirePin(AscentDampingDisabled, Binary::LeftOperand));
    Ok &= Connect(RequirePin(HasObservedAscent, P::ReturnValue),
        RequirePin(AscentDampingDisabled, Binary::RightOperand));
    Ok &= Connect(RequirePin(DescentDampingDisabled, P::ReturnValue),
        RequirePin(ObservedInBrakingDirection, Binary::LeftOperand));
    Ok &= Connect(RequirePin(AscentDampingDisabled, P::ReturnValue),
        RequirePin(ObservedInBrakingDirection, Binary::RightOperand));
    Ok &= Connect(RequirePin(VerticalDampingDisabled, P::ReturnValue),
        RequirePin(SkipPositiveVerticalDamping, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ObservedInBrakingDirection, P::ReturnValue),
        RequirePin(SkipPositiveVerticalDamping, Binary::RightOperand));
    Ok &= Connect(RequirePin(SkipPositiveVerticalDamping, P::ReturnValue),
        RequirePin(PositiveVerticalDampingGate, P::Condition));
    Ok &= Connect(RequirePin(PositiveVerticalDampingGate, P::Then),
        RequirePin(ResetVerticalSampleWhenDampingDisabled, P::Execute));
    Ok &= Connect(RequirePin(PositiveVerticalDampingGate, P::Else),
        RequirePin(RemainingAscentBranch, P::Execute));
    Ok &= Connect(RequirePin(SelectedVerticalDeceleration, P::ReturnValue),
        RequirePin(BrakingStep, Binary::LeftOperand));
    Ok &= Connect(RequirePin(TickEvent, P::DeltaSeconds), RequirePin(BrakingStep, Binary::RightOperand));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(ObservedOrPreviousMinimum, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreviousVerticalVelocity, PreviousCorrectedVerticalVelocity),
        RequirePin(ObservedOrPreviousMinimum, Binary::RightOperand));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(ObservedOrPreviousMaximum, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreviousVerticalVelocity, PreviousCorrectedVerticalVelocity),
        RequirePin(ObservedOrPreviousMaximum, Binary::RightOperand));
    Ok &= Connect(RequirePin(ObservedOrPreviousMaximum, P::ReturnValue),
        RequirePin(DirectionalObservedOrPrevious, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(ObservedOrPreviousMinimum, P::ReturnValue),
        RequirePin(DirectionalObservedOrPrevious, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(IsDescentBraking, AltitudeDescentBrakingActive),
        RequirePin(DirectionalObservedOrPrevious, SelectPins::Condition));
    Ok &= Connect(RequirePin(DirectionalObservedOrPrevious, P::ReturnValue),
        RequirePin(VerticalVelocityBeforeBraking, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(VerticalVelocityBeforeBraking, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(VerticalSampleValid, VerticalVelocitySampleValid),
        RequirePin(VerticalVelocityBeforeBraking, SelectPins::Condition));
    Ok &= Connect(RequirePin(VerticalVelocityBeforeBraking, P::ReturnValue),
        RequirePin(ReducedVerticalVelocity, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BrakingStep, P::ReturnValue), RequirePin(ReducedVerticalVelocity, Binary::RightOperand));
    Ok &= Connect(RequirePin(VerticalVelocityBeforeBraking, P::ReturnValue),
        RequirePin(IncreasedVerticalVelocity, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BrakingStep, P::ReturnValue),
        RequirePin(IncreasedVerticalVelocity, Binary::RightOperand));
    Ok &= Connect(RequirePin(IncreasedVerticalVelocity, P::ReturnValue),
        RequirePin(CorrectedVerticalVelocity, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(ReducedVerticalVelocity, P::ReturnValue),
        RequirePin(CorrectedVerticalVelocity, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(IsDescentBraking, AltitudeDescentBrakingActive),
        RequirePin(CorrectedVerticalVelocity, SelectPins::Condition));
    Ok &= Connect(RequirePin(CorrectedVerticalVelocity, P::ReturnValue),
        RequirePin(IsStillAscending, Binary::LeftOperand));
    Ok &= Connect(RequirePin(CorrectedVerticalVelocity, P::ReturnValue),
        RequirePin(IsStillDescending, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsDescentBraking, AltitudeDescentBrakingActive),
        RequirePin(ContinuingDescent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsStillDescending, P::ReturnValue),
        RequirePin(ContinuingDescent, Binary::RightOperand));
    Ok &= Connect(RequirePin(NotDescentBraking, P::ReturnValue),
        RequirePin(ContinuingAscent, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsStillAscending, P::ReturnValue),
        RequirePin(ContinuingAscent, Binary::RightOperand));
    Ok &= Connect(RequirePin(ContinuingDescent, P::ReturnValue),
        RequirePin(StillMovingVertically, Binary::LeftOperand));
    Ok &= Connect(RequirePin(ContinuingAscent, P::ReturnValue),
        RequirePin(StillMovingVertically, Binary::RightOperand));
    Ok &= Connect(RequirePin(StillMovingVertically, P::ReturnValue),
        RequirePin(RemainingAscentBranch, P::Condition));
    Ok &= Connect(RequirePin(RemainingAscentBranch, P::Else),
        RequirePin(ResetVerticalSampleForExact, P::Execute));
    Ok &= Connect(RequirePin(ResetVerticalSampleForExact, P::Then),
        RequirePin(CaptureExactAltitude, P::Execute));
    Ok &= Connect(RequirePin(RemainingAscentBranch, P::Then), RequirePin(SetReducedVelocity, P::Execute));
    Ok &= Connect(RequirePin(CorrectedVerticalVelocity, P::ReturnValue),
        RequirePin(VerticalBrakingDelta, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(VerticalBrakingDelta, Binary::RightOperand));
    Ok &= Connect(RequirePin(VerticalBrakingDelta, P::ReturnValue),
        RequirePin(ReducedVelocity, ArcadeAltitude::VectorZ));
    Ok &= Connect(BrakingPrimitiveRoot->GetCastResultPin(), RequirePin(SetReducedVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(ReducedVelocity, P::ReturnValue), RequirePin(SetReducedVelocity, ArcadeAltitude::NewVelocity));
    Ok &= Connect(RequirePin(SetReducedVelocity, P::Then),
        RequirePin(StoreCorrectedVerticalVelocity, P::Execute));
    Ok &= Connect(RequirePin(CorrectedVerticalVelocity, P::ReturnValue),
        RequirePin(StoreCorrectedVerticalVelocity, PreviousCorrectedVerticalVelocity));
    Ok &= Connect(RequirePin(StoreCorrectedVerticalVelocity, P::Then),
        RequirePin(MarkVerticalSampleValid, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(CurrentLocation, P::FunctionTarget));
    Ok &= Connect(RequirePin(CurrentLocation, P::ReturnValue), RequirePin(BreakCurrentLocation, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakCurrentLocation, ArcadeAltitude::VectorX), RequirePin(LockedLocation, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(BreakCurrentLocation, ArcadeAltitude::VectorY), RequirePin(LockedLocation, ArcadeAltitude::VectorY));
    Ok &= Connect(RequirePin(TargetAltitude, LockedAltitude), RequirePin(LockedLocation, ArcadeAltitude::VectorZ));
    Ok &= Connect(RequirePin(BreakCurrentLocation, ArcadeAltitude::VectorZ), RequirePin(HeightIsRising, Binary::LeftOperand));
    Ok &= Connect(RequirePin(TargetAltitude, LockedAltitude), RequirePin(HeightIsRising, Binary::RightOperand));
    Ok &= Connect(RequirePin(HeightIsRising, P::ReturnValue), RequirePin(RisingBranch, P::Condition));
    Ok &= Connect(RequirePin(RisingBranch, P::Then), RequirePin(RaiseAltitudeFloor, P::Execute));
    Ok &= Connect(RequirePin(BreakCurrentLocation, ArcadeAltitude::VectorZ), RequirePin(RaiseAltitudeFloor, LockedAltitude));
    Ok &= Connect(RequirePin(RisingBranch, P::Else),
        RequirePin(PositiveVerticalVelocityBranch, P::Execute));
    Ok &= Connect(RequirePin(PositiveVerticalVelocityBranch, P::Else),
        RequirePin(SetLocation, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(SetLocation, P::FunctionTarget));
    Ok &= Connect(RequirePin(LockedLocation, P::ReturnValue), RequirePin(SetLocation, ArcadeAltitude::NewLocation));
    Ok &= Connect(RequirePin(SetLocation, P::Then),
        RequirePin(SweepCollisionBranch, P::Execute));
    Ok &= Connect(RequirePin(SetLocation, ArcadeAltitude::SweepHitResult),
        RequirePin(BreakSweepHit, CollisionRelease::Hit));
    Ok &= Connect(RequirePin(BreakSweepHit, ArcadeAltitude::BlockingHit),
        RequirePin(SweepCollisionDetected, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BreakSweepHit, ArcadeAltitude::InitialOverlap),
        RequirePin(SweepCollisionDetected, Binary::RightOperand));
    Ok &= Connect(RequirePin(SweepCollisionDetected, P::ReturnValue),
        RequirePin(SweepCollisionBranch, P::Condition));
    Ok &= Connect(RequirePin(SweepCollisionBranch, P::Else),
        RequirePin(PrimitiveRoot, P::Execute));
    Ok &= Connect(RequirePin(SweepCollisionBranch, P::Then),
        RequirePin(StoreSweepDeadline, P::Execute));
    Ok &= Connect(RequirePin(CurrentGameTime, P::ReturnValue),
        RequirePin(SweepDeadline, Binary::LeftOperand));
    Ok &= Connect(RequirePin(SweepDeadline, P::ReturnValue),
        RequirePin(StoreSweepDeadline, CollisionReleaseUntilTime));
    Ok &= Connect(RequirePin(StoreSweepDeadline, P::Then),
        RequirePin(ClearAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(ClearAfterSweepHit, P::Then),
        RequirePin(ClearExactAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(ClearExactAfterSweepHit, P::Then),
        RequirePin(ClearSpaceAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceAfterSweepHit, P::Then),
        RequirePin(ClearDescentAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(ClearDescentAfterSweepHit, P::Then),
        RequirePin(StopBrakingAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingAfterSweepHit, P::Then),
        RequirePin(StopDescentBrakingAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(StopDescentBrakingAfterSweepHit, P::Then),
        RequirePin(ResetHorizontalSampleAfterSweepHit, P::Execute));
    Ok &= Connect(RequirePin(ResetHorizontalSampleAfterSweepHit, P::Then),
        RequirePin(ResetVerticalSampleAfterSweepHit, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Root, P::FunctionTarget));
    Ok &= Connect(RequirePin(Root, RootComponent), PrimitiveRoot->GetCastSourcePin());
    Ok &= Connect(PrimitiveRoot->GetCastResultPin(), RequirePin(LinearVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(LinearVelocity, P::ReturnValue), RequirePin(BreakVelocity, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakVelocity, ArcadeAltitude::VectorZ),
        RequirePin(ZeroVerticalVelocityDelta, Binary::RightOperand));
    Ok &= Connect(RequirePin(ZeroVerticalVelocityDelta, P::ReturnValue),
        RequirePin(VerticalVelocityCorrection, ArcadeAltitude::VectorZ));
    Ok &= Connect(PrimitiveRoot->GetValidCastPin(), RequirePin(LinearVelocity, P::Execute));
    Ok &= Connect(RequirePin(LinearVelocity, P::Then), RequirePin(SetVelocity, P::Execute));
    Ok &= Connect(PrimitiveRoot->GetCastResultPin(), RequirePin(SetVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(VerticalVelocityCorrection, P::ReturnValue),
        RequirePin(SetVelocity, ArcadeAltitude::NewVelocity));
    return Ok;
}

bool SaveBlueprint(
    UPackage* Package,
    UBlueprint* Blueprint,
    const ETickingGroup TickGroup = TG_MAX)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) return false;
    if (TickGroup != TG_MAX)
    {
        AActor* ActorDefaults = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
        if (!ActorDefaults) return false;
        ActorDefaults->PrimaryActorTick.TickGroup = TickGroup;
    }
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Blueprint, *Filename, Args);
}

bool ValidateReflectedOrdinals(
    const UStruct* Owner,
    const TArray<FName>& ExpectedNames)
{
    TArray<FName> ActualNames;
    for (TFieldIterator<FProperty> Property(Owner, EFieldIteratorFlags::ExcludeSuper);
        Property; ++Property)
    {
        for (int32 ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ++ArrayIndex)
        {
            ActualNames.Add(Property->GetFName());
        }
    }
    if (ActualNames != ExpectedNames)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Reflected ordinal mismatch for %s: expected %d entries, found %d"),
            *Owner->GetPathName(), ExpectedNames.Num(), ActualNames.Num());
        return false;
    }
    return true;
}

bool ValidateMooringSerializationContracts()
{
    namespace Fields = MooringSerializationFields;
    const TArray<FName> SocketProperties = {
        Fields::ItemTransferTimer, Fields::SocketId, Fields::LeakMultiplier,
        Fields::AddModuleRequirement, Fields::CustomRequirement,
        Fields::AutoInitialize, MooringLayout::PortProperty, Fields::AcceptsSignals,
        Fields::OwningModule, Fields::Standalone, Fields::IsVirtual,
        Fields::IsVirtualGrouped, Fields::IsVirtualShareToGroup,
        Fields::ConnectSound, Fields::DisconnectSound, Fields::SwitchDirectionSound,
        Fields::SocketConnectionChanged, Fields::ResourcesTransferredLastFrame,
        Fields::ResourcesTransferredLastFrameAbsolute,
        Fields::ResourcesTransferredLastFrameType, Fields::ResourceTransferPerSecond,
        Fields::IntegratedCable, Fields::IntegratedCableLength,
        Fields::IntegratedCablePlaceholderMesh, Fields::ConnectedToSocket,
        Fields::ModuleOwner, Fields::ModuleOwnerOriginal, Fields::ConnectionCable,
        Fields::SocketCollisionBox, Fields::NiagaraFx, Fields::NiagaraFxLeak,
        Fields::NiagaraFxLeakSystem, Fields::NiagaraFxSystem,
        Fields::ConnectedSocketsCache, Fields::ConnectedSocketsCache,
        Fields::ConnectedSocketsCache
    };
    const TArray<FName> ViewProperties = {
        Fields::Socket, Fields::Mesh, Fields::DataAsset, Fields::MultiplyScale
    };
    const TArray<FName> PortProperties = {
        Fields::Type, CargoAction::ResourceType, Fields::Bandwidth,
        Fields::IgnoreCableBandwidth, Fields::DefaultDirection,
        Fields::BalanceResourceLevels, Fields::MatchByResourceCategory,
        Fields::ResourceCategory, Fields::AdditionalAcceptedResourceTypes
    };
    return ValidateReflectedOrdinals(UModuleSocketComponent::StaticClass(), SocketProperties) &&
        ValidateReflectedOrdinals(UVoyageModuleSocketViewComponent::StaticClass(), ViewProperties) &&
        ValidateReflectedOrdinals(FModuleSocketIOData::StaticStruct(), PortProperties);
}

UVoyageItemCable* CreateHookCableReferenceStub()
{
    if (FPackageName::DoesPackageExist(HookCablePackage))
    {
        UE_LOG(LogTemp, Error, TEXT("Hook Cable source stub already exists"));
        return nullptr;
    }
    UPackage* Package = CreatePackage(HookCablePackage);
    UVoyageItemCable* Cable = NewObject<UVoyageItemCable>(
        Package, MooringLayout::HookCableAssetName, RF_Public | RF_Standalone);
    if (!Cable) return nullptr;
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Cable, *Filename, Args) ? Cable : nullptr;
}

bool AddRearMooringSocket(
    UBlueprint* Blueprint,
    const FName ComponentName,
    const uint32 SocketId,
    const FVector& Location,
    const FRotator& Rotation,
    UVoyageItemCable* HookCable)
{
    auto* Socket = NewObject<UVoyageModuleSocketViewComponent>(
        Blueprint->GeneratedClass, ComponentName, RF_ArchetypeObject | RF_Public);
    if (!Socket) return false;

    Socket->SetRelativeLocationAndRotation(Location, Rotation);
    Socket->SetAbsolute(false, false, true);
    Socket->DataAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(MooringSocketDataPath));
    Socket->SocketID = SocketId;
    Socket->bAutoInitialize = false;
    Socket->Port.Type = EModuleSocketType::ST_Any;
    Socket->Port.ResourceType = EModuleResourceType::Mooring;
    Socket->Port.ResourceCategory = EModuleResourceCategory::Mooring;
    Socket->ConnectSound = TSoftObjectPtr<UObject>(FSoftObjectPath(CableConnectSoundPath));
    Socket->DisconnectSound = TSoftObjectPtr<UObject>(FSoftObjectPath(CableDisconnectSoundPath));
    Socket->SwitchDirectionSound = TSoftObjectPtr<UObject>(
        FSoftObjectPath(SwitchDirectionSoundPath));
    Socket->IntegratedCable = HookCable;
    Socket->IntegratedCableLength = MooringLayout::IntegratedCableLength;
    Blueprint->ComponentTemplates.Add(Socket);
    return true;
}

UMaterialInstanceConstant* CreateMooringMaterialReferenceStub()
{
    if (FPackageName::DoesPackageExist(MooringMaterialPackage)) return nullptr;
    UPackage* Package = CreatePackage(MooringMaterialPackage);
    auto* Material = NewObject<UMaterialInstanceConstant>(
        Package, MooringLayout::MaterialAssetName, RF_Public | RF_Standalone);
    if (!Material || Material->GetPathName() != MooringMaterialObjectPath) return nullptr;
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Material, *Filename, Args) ? Material : nullptr;
}

UStaticMesh* ImportMooringHalf(
    const FString& Source,
    const TCHAR* ExpectedPackage,
    UMaterialInterface* StockMaterial,
    const int32 ExpectedTriangles)
{
    if (!FPaths::FileExists(Source) || FPackageName::DoesPackageExist(ExpectedPackage)) return nullptr;
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return nullptr;
    TArray<UObject*> Imported;
    FImportAssetParameters Parameters;
    Parameters.bIsAutomated = true;
    Parameters.bReplaceExisting = false;
    Parameters.ImportLevel = World->PersistentLevel;
    Parameters.OnAssetDoneNative.BindLambda([&Imported](UObject* Asset) { Imported.Add(Asset); });
    auto& Manager = UInterchangeManager::GetInterchangeManager();
    if (!Manager.ImportScene(MooringLayout::ImportDestination,
        UInterchangeManager::CreateSourceData(Source), Parameters)) return nullptr;
    TArray<UStaticMesh*> Meshes;
    for (UObject* Asset : Imported)
    {
        if (auto* Mesh = Cast<UStaticMesh>(Asset)) Meshes.AddUnique(Mesh);
    }
    if (Meshes.Num() != 1)
    {
        UE_LOG(LogTemp, Error, TEXT("Expected one imported mooring mesh, found %d"), Meshes.Num());
        return nullptr;
    }
    UStaticMesh* Mesh = Meshes[0];
    if (Mesh->GetOutermost()->GetName() != ExpectedPackage ||
        Mesh->GetNumUVChannels(0) < 4 || !Mesh->GetMeshDescription(0))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Mooring mesh contract: package=%s expected=%s uv=%d description=%d"),
            *Mesh->GetOutermost()->GetName(), ExpectedPackage, Mesh->GetNumUVChannels(0),
            Mesh->GetMeshDescription(0) ? 1 : 0);
        return nullptr;
    }
    const FMeshDescription* Description = Mesh->GetMeshDescription(0);
    if (Description->Triangles().Num() != ExpectedTriangles)
    {
        UE_LOG(LogTemp, Error, TEXT("Mooring triangle count %d expected %d"),
            Description->Triangles().Num(), ExpectedTriangles);
        return nullptr;
    }
    FMeshBuildSettings& BuildSettings = Mesh->GetSourceModel(0).BuildSettings;
    BuildSettings.bGenerateLightmapUVs = false;
    BuildSettings.bRemoveDegenerates = false;
    BuildSettings.bRecomputeNormals = false;
    BuildSettings.bRecomputeTangents = true;
    Mesh->GetNaniteSettings().bEnabled = false;
    Mesh->Build(false);
    if (Mesh->GetNaniteSettings().bEnabled ||
        Mesh->GetNumTriangles(0) != MooringLayout::RuntimeTriangleCountPerSide ||
        Mesh->GetNumUVChannels(0) < 4)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Mooring runtime mesh contract: package=%s triangles=%d expected=%d uv=%d nanite=%d"),
            *Mesh->GetOutermost()->GetName(), Mesh->GetNumTriangles(0),
            MooringLayout::RuntimeTriangleCountPerSide, Mesh->GetNumUVChannels(0),
            Mesh->GetNaniteSettings().bEnabled ? 1 : 0);
        return nullptr;
    }
    Mesh->GetStaticMaterials().Reset();
    Mesh->GetStaticMaterials().Add(FStaticMaterial(
        StockMaterial, MooringLayout::MaterialSlotName, MooringLayout::MaterialSlotName));
    if (!Mesh->GetBodySetup()) Mesh->CreateBodySetup();
    Mesh->GetBodySetup()->RemoveSimpleCollision();
    Mesh->GetBodySetup()->CollisionTraceFlag = CTF_UseDefault;
    Mesh->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Mesh->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Mesh->GetOutermost(), Mesh, *Filename, Args) ? Mesh : nullptr;
}

bool AddMooringPlate(
    UBlueprint* Blueprint,
    const FName ComponentName,
    UStaticMesh* Mesh,
    UMaterialInterface* Material,
    const FVector& Location)
{
    auto* Component = NewObject<UStaticMeshComponent>(
        Blueprint->GeneratedClass, ComponentName, RF_ArchetypeObject | RF_Public);
    if (!Component) return false;
    Component->SetStaticMesh(Mesh);
    Component->SetMaterial(0, Material);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeRotation(FRotator::ZeroRotator);
    Component->SetRelativeScale3D(FVector::OneVector);
    Component->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    Component->SetGenerateOverlapEvents(false);
    Component->SetMobility(EComponentMobility::Movable);
    Blueprint->ComponentTemplates.Add(Component);
    return true;
}

bool AddDisabledMooringState(UBlueprint* Blueprint)
{
    FEdGraphPinType SocketType;
    SocketType.PinCategory = UEdGraphSchema_K2::PC_Object;
    SocketType.PinSubCategoryObject = UVoyageModuleSocketViewComponent::StaticClass();
    FEdGraphPinType BoolType;
    BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint, CachedRearLeftMooringSocket, SocketType) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Blueprint, CachedRearRightMooringSocket, SocketType) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Blueprint, DisabledMooringCleanupQueued, BoolType, CargoAction::Disabled))
        return false;
    for (const FName Field : {
        CachedRearLeftMooringSocket, CachedRearRightMooringSocket,
        DisabledMooringCleanupQueued})
    {
        FBPVariableDescription* Variable = Blueprint->NewVariables.FindByPredicate(
            [&Field](const FBPVariableDescription& Candidate)
            {
                return Candidate.VarName == Field;
            });
        if (!Variable) return false;
        Variable->PropertyFlags |= CPF_Transient;
    }
    return true;
}

bool AddDisableOwnedMooringObject(UBlueprint* Blueprint)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddVoidFunction(
        Blueprint, DisableOwnedMooringObjectFunction, Graph);
    if (!Entry) return false;
    FEdGraphPinType ObjectType;
    ObjectType.PinCategory = UEdGraphSchema_K2::PC_Object;
    ObjectType.PinSubCategoryObject = UObject::StaticClass();
    if (!Entry->CreateUserDefinedPin(
            MooringLayout::TargetObject, ObjectType, EGPD_Output)) return false;

    auto* Component = NewObject<UK2Node_DynamicCast>(Graph);
    Component->TargetType = UActorComponent::StaticClass();
    Component->SetPurity(false);
    FinishNode(Component, Graph, 220, 0);
    auto* Owner = AddCall(Graph, UActorComponent::StaticClass(),
        FName(TEXT("GetOwner")), 450, 180);
    auto* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 450, 360);
    auto* IsOwned = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), 680, 180);
    auto* OwnedBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 910, 0);
    auto* Deactivate = AddCall(Graph, UActorComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UActorComponent, Deactivate), 1140, 0);
    auto* Scene = NewObject<UK2Node_DynamicCast>(Graph);
    Scene->TargetType = USceneComponent::StaticClass();
    Scene->SetPurity(false);
    FinishNode(Scene, Graph, 1370, 0);
    auto* HideVisibility = AddCall(Graph, USceneComponent::StaticClass(),
        MooringLayout::SetVisibilityFunction, 1600, 0);
    SetDefault(HideVisibility, MooringLayout::NewVisibility, CargoAction::Disabled);
    SetDefault(HideVisibility, MooringLayout::PropagateVisibilityToChildren,
        CargoAction::Disabled);
    auto* HideInGame = AddCall(Graph, USceneComponent::StaticClass(),
        MooringLayout::SetHiddenInGameFunction, 1830, 0);
    SetDefault(HideInGame, MooringLayout::NewHidden, CargoAction::Enabled);
    SetDefault(HideInGame, MooringLayout::PropagateVisibilityToChildren,
        CargoAction::Disabled);
    auto* Primitive = NewObject<UK2Node_DynamicCast>(Graph);
    Primitive->TargetType = UPrimitiveComponent::StaticClass();
    Primitive->SetPurity(false);
    FinishNode(Primitive, Graph, 2060, 0);
    auto* NoCollision = AddCall(Graph, UPrimitiveComponent::StaticClass(),
        MooringLayout::SetCollisionEnabledFunction, 2290, 0);
    SetDefault(NoCollision, MooringLayout::CollisionEnabledType, TEXT("NoCollision"));
    auto* NoOverlap = AddCall(Graph, UPrimitiveComponent::StaticClass(),
        MooringLayout::SetGenerateOverlapEventsFunction, 2520, 0);
    SetDefault(NoOverlap, MooringLayout::GenerateOverlapEvents, CargoAction::Disabled);

    bool Ok = true;
    Ok &= Connect(RequirePin(Entry, P::Then), RequirePin(Component, P::Execute));
    Ok &= Connect(RequirePin(Entry, MooringLayout::TargetObject),
        Component->GetCastSourcePin());
    Ok &= Connect(Component->GetValidCastPin(), RequirePin(OwnedBranch, P::Execute));
    Ok &= Connect(Component->GetCastResultPin(), RequirePin(Owner, P::FunctionTarget));
    Ok &= Connect(RequirePin(Owner, P::ReturnValue),
        RequirePin(IsOwned, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(IsOwned, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(RequirePin(IsOwned, P::ReturnValue), RequirePin(OwnedBranch, P::Condition));
    Ok &= Connect(RequirePin(OwnedBranch, P::Then), RequirePin(Deactivate, P::Execute));
    Ok &= Connect(Component->GetCastResultPin(), RequirePin(Deactivate, P::FunctionTarget));
    Ok &= Connect(RequirePin(Deactivate, P::Then), RequirePin(Scene, P::Execute));
    Ok &= Connect(Component->GetCastResultPin(), Scene->GetCastSourcePin());
    Ok &= Connect(Scene->GetValidCastPin(), RequirePin(HideVisibility, P::Execute));
    Ok &= Connect(Scene->GetCastResultPin(), RequirePin(HideVisibility, P::FunctionTarget));
    Ok &= Connect(RequirePin(HideVisibility, P::Then), RequirePin(HideInGame, P::Execute));
    Ok &= Connect(Scene->GetCastResultPin(), RequirePin(HideInGame, P::FunctionTarget));
    Ok &= Connect(RequirePin(HideInGame, P::Then), RequirePin(Primitive, P::Execute));
    Ok &= Connect(Scene->GetCastResultPin(), Primitive->GetCastSourcePin());
    Ok &= Connect(Primitive->GetValidCastPin(), RequirePin(NoCollision, P::Execute));
    Ok &= Connect(Primitive->GetCastResultPin(), RequirePin(NoCollision, P::FunctionTarget));
    Ok &= Connect(RequirePin(NoCollision, P::Then), RequirePin(NoOverlap, P::Execute));
    Ok &= Connect(Primitive->GetCastResultPin(), RequirePin(NoOverlap, P::FunctionTarget));
    return Ok;
}

bool AddRunDisabledMooringCleanup(UBlueprint* Blueprint)
{
    UFunction* Disable = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(DisableOwnedMooringObjectFunction)
        : nullptr;
    if (!Disable) return false;
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddVoidFunction(
        Blueprint, RunDisabledMooringCleanupFunction, Graph);
    if (!Entry) return false;
    FEdGraphPinType SceneComponentArrayType;
    SceneComponentArrayType.PinCategory = UEdGraphSchema_K2::PC_Object;
    SceneComponentArrayType.PinSubCategoryObject = USceneComponent::StaticClass();
    SceneComponentArrayType.ContainerType = EPinContainerType::Array;
    for (const FName ChildrenLocal : {
        DisabledMooringLeftChildrenLocal, DisabledMooringRightChildrenLocal})
    {
        if (!FBlueprintEditorUtils::AddLocalVariable(
                Blueprint, Graph, ChildrenLocal, SceneComponentArrayType))
            return false;
    }
    auto* CleanupSequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 220, 0);
    auto* ClearQueued = AddWrite(Graph, DisabledMooringCleanupQueued, 1140, 1980);
    SetDefault(ClearQueued, DisabledMooringCleanupQueued, CargoAction::Disabled);
    auto* FourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, 450, 0);
    auto* DisabledBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 680, 0);
    auto* SocketSequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 910, 0);

    bool Ok = true;
    Ok &= Connect(RequirePin(Entry, P::Then), RequirePin(CleanupSequence, P::Execute));
    Ok &= Connect(CleanupSequence->GetThenPinGivenIndex(0), RequirePin(FourPorts, P::Execute));
    Ok &= Connect(CleanupSequence->GetThenPinGivenIndex(1), RequirePin(ClearQueued, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::Then), RequirePin(DisabledBranch, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::ReturnValue), RequirePin(DisabledBranch, P::Condition));
    Ok &= Connect(RequirePin(DisabledBranch, P::Else), RequirePin(SocketSequence, P::Execute));

    const TArray<FName> SocketFields = {
        CachedRearLeftMooringSocket, CachedRearRightMooringSocket};
    int32 SocketIndex = 0;
    for (const FName SocketField : SocketFields)
    {
        const FName ChildrenLocal = SocketIndex == 0
            ? DisabledMooringLeftChildrenLocal : DisabledMooringRightChildrenLocal;
        const int32 Y = SocketIndex * 900;
        auto* Socket = AddRead(Graph, SocketField, nullptr, 1140, Y + 180);
        auto* Valid = AddCall(Graph, UKismetSystemLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 1140, Y);
        auto* ValidBranch = FinishNode(
            NewObject<UK2Node_IfThenElse>(Graph), Graph, 1370, Y);
        auto* Disconnect = AddCall(
            Graph, UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(
                UVoyageMiscBlueprintFunctionLibrary, DisconnectCableFromSocket), 1600, Y);
        SetDefault(Disconnect, CargoAction::Destroy, CargoAction::DestroyCable);
        auto* DisableSocket = AddCall(
            Graph, Blueprint->GeneratedClass, DisableOwnedMooringObjectFunction, 1830, Y);
        auto* GetChildren = AddCall(Graph, USceneComponent::StaticClass(),
            MooringLayout::GetChildrenComponentsFunction, 2060, Y);
        SetDefault(GetChildren, MooringLayout::IncludeAllDescendants, CargoAction::Enabled);
        auto* SaveChildren = AddLocalWrite(Blueprint, Graph, ChildrenLocal, 2290, Y);
        auto* SavedChildren = AddLocalRead(Blueprint, Graph, ChildrenLocal, 2290, Y + 180);
        auto* ChildLoop = NewObject<UK2Node_MacroInstance>(Graph);
        ChildLoop->SetMacroGraph(StandardForEachLoop());
        FinishNode(ChildLoop, Graph, 2520, Y);
        auto* DisableChild = AddCall(
            Graph, Blueprint->GeneratedClass, DisableOwnedMooringObjectFunction, 2750, Y);

        Ok &= Connect(SocketSequence->GetThenPinGivenIndex(SocketIndex),
            RequirePin(ValidBranch, P::Execute));
        Ok &= Connect(RequirePin(Socket, SocketField), RequirePin(Valid, P::Object));
        Ok &= Connect(RequirePin(Valid, P::ReturnValue), RequirePin(ValidBranch, P::Condition));
        Ok &= Connect(RequirePin(ValidBranch, P::Then), RequirePin(Disconnect, P::Execute));
        Ok &= Connect(RequirePin(Socket, SocketField), RequirePin(Disconnect, CargoAction::Socket));
        Ok &= Connect(RequirePin(Disconnect, P::Then), RequirePin(DisableSocket, P::Execute));
        Ok &= Connect(RequirePin(Socket, SocketField),
            RequirePin(DisableSocket, MooringLayout::TargetObject));
        Ok &= Connect(RequirePin(DisableSocket, P::Then), RequirePin(SaveChildren, P::Execute));
        Ok &= Connect(RequirePin(Socket, SocketField), RequirePin(GetChildren, P::FunctionTarget));
        Ok &= Connect(RequirePin(GetChildren, MooringLayout::Children),
            RequirePin(SaveChildren, ChildrenLocal));
        Ok &= Connect(RequirePin(SaveChildren, P::Then), RequireExecInputPin(ChildLoop));
        Ok &= Connect(SavedChildren->GetValuePin(), RequirePin(ChildLoop, CargoAction::Array));
        Ok &= Connect(RequirePin(ChildLoop, P::LoopBody), RequirePin(DisableChild, P::Execute));
        Ok &= Connect(RequirePin(ChildLoop, CargoAction::ArrayElement),
            RequirePin(DisableChild, MooringLayout::TargetObject));

        UEdGraphPin* Tail = RequirePin(ChildLoop, P::Completed);
        for (const FName ObjectField : {
            MooringSerializationFields::Socket, MooringSerializationFields::Mesh,
            MooringSerializationFields::SocketCollisionBox,
            MooringSerializationFields::IntegratedCablePlaceholderMesh})
        {
            UClass* OwnerClass =
                (ObjectField == MooringSerializationFields::Socket ||
                 ObjectField == MooringSerializationFields::Mesh)
                    ? UVoyageModuleSocketViewComponent::StaticClass()
                    : UModuleSocketComponent::StaticClass();
            auto* Object = AddRead(Graph, ObjectField, OwnerClass, 2980, Y + 180);
            auto* DisableObject = AddCall(
                Graph, Blueprint->GeneratedClass, DisableOwnedMooringObjectFunction, 3210, Y);
            Ok &= Connect(RequirePin(Socket, SocketField),
                RequirePin(Object, P::FunctionTarget));
            Ok &= Connect(Tail, RequirePin(DisableObject, P::Execute));
            Ok &= Connect(RequirePin(Object, ObjectField),
                RequirePin(DisableObject, MooringLayout::TargetObject));
            Tail = RequirePin(DisableObject, P::Then);
        }
        ++SocketIndex;
    }
    return Ok;
}

bool AddQueueDisabledMooringCleanup(UBlueprint* Blueprint)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddVoidFunction(
        Blueprint, QueueDisabledMooringCleanupFunction, Graph);
    if (!Entry) return false;
    auto* FourPorts = AddCall(Graph, Blueprint->GeneratedClass, GetFourPortSetting, 220, 0);
    auto* DisabledBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 450, 0);
    auto* Queued = AddRead(Graph, DisabledMooringCleanupQueued, nullptr, 680, 180);
    auto* QueuedBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 680, 0);
    auto* MarkQueued = AddWrite(Graph, DisabledMooringCleanupQueued, 910, 0);
    SetDefault(MarkQueued, DisabledMooringCleanupQueued, CargoAction::Enabled);
    auto* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 1140, 180);
    auto* Timer = AddCall(Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, K2_SetTimerForNextTick), 1140, 0);
    SetDefault(Timer, MooringLayout::TimerFunctionName,
        *RunDisabledMooringCleanupFunction.ToString());

    bool Ok = true;
    Ok &= Connect(RequirePin(Entry, P::Then), RequirePin(FourPorts, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::Then), RequirePin(DisabledBranch, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::ReturnValue), RequirePin(DisabledBranch, P::Condition));
    Ok &= Connect(RequirePin(DisabledBranch, P::Else), RequirePin(QueuedBranch, P::Execute));
    Ok &= Connect(RequirePin(Queued, DisabledMooringCleanupQueued),
        RequirePin(QueuedBranch, P::Condition));
    Ok &= Connect(RequirePin(QueuedBranch, P::Else), RequirePin(MarkQueued, P::Execute));
    Ok &= Connect(RequirePin(MarkQueued, P::Then), RequirePin(Timer, P::Execute));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(Timer, MooringLayout::TimerObject));
    return Ok;
}

bool AddDisabledMooringConnectionCallback(UBlueprint* Blueprint)
{
    FMulticastDelegateProperty* DelegateProperty = SocketConnectionDelegateProperty();
    UFunction* Queue = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(QueueDisabledMooringCleanupFunction)
        : nullptr;
    if (!DelegateProperty || !Queue) return false;
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, DisabledMooringConnectionCallback, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, Graph, true, DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    if (!Entry) return false;
    RequirePin(Entry, P::Then)->BreakAllPinLinks();
    auto* QueueCall = AddCall(
        Graph, Blueprint->GeneratedClass, QueueDisabledMooringCleanupFunction, 220, 0);
    return Connect(RequirePin(Entry, P::Then), RequirePin(QueueCall, P::Execute));
}

bool AddDisabledMooringBeginPlayBinding(UBlueprint* Blueprint)
{
    FMulticastDelegateProperty* DelegateProperty = SocketConnectionDelegateProperty();
    if (!DelegateProperty || !Blueprint->GeneratedClass ||
        !Blueprint->GeneratedClass->FindFunctionByName(DisabledMooringConnectionCallback) ||
        !Blueprint->GeneratedClass->FindFunctionByName(QueueDisabledMooringCleanupFunction))
        return false;
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph) return false;
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        auto* Event = Cast<UK2Node_Event>(Node);
        if (Event && Event->EventReference.GetMemberName() ==
            BlueprintGraphNames::Events::ActorReceiveBeginPlay) BeginPlay = Event;
    }
    if (!BeginPlay) return false;
    UEdGraphPin* BeginTail = RequirePin(BeginPlay, P::Then);
    if (BeginTail->LinkedTo.Num() != 1) return false;
    UEdGraphPin* ExistingWork = BeginTail->LinkedTo[0];
    BeginTail->BreakAllPinLinks();

    auto* BeginSequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 100, -1560);
    auto* FourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, 220, -1560);
    auto* DisabledBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 450, -1560);
    auto* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, 450, -1340);
    auto* Components = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        680, -1560);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(Components, CargoAction::ComponentClass),
        UVoyageModuleSocketViewComponent::StaticClass());
    auto* Loop = NewObject<UK2Node_MacroInstance>(Graph);
    Loop->SetMacroGraph(StandardForEachLoop());
    FinishNode(Loop, Graph, 910, -1560);
    auto* Socket = NewObject<UK2Node_DynamicCast>(Graph);
    Socket->TargetType = UVoyageModuleSocketViewComponent::StaticClass();
    Socket->SetPurity(false);
    FinishNode(Socket, Graph, 1140, -1560);
    auto* ObjectName = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName), 1370, -1340);
    auto* IsLeft = AddCall(
        Graph, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr), 1600, -1340);
    SetDefault(IsLeft, BlueprintGraphNames::Pins::Binary::RightOperand,
        *RearLeftMooringComponentName.ToString());
    auto* LeftBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1830, -1560);
    auto* StoreLeft = AddWrite(Graph, CachedRearLeftMooringSocket, 2060, -1680);
    auto* IsRight = AddCall(
        Graph, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr), 2060, -1340);
    SetDefault(IsRight, BlueprintGraphNames::Pins::Binary::RightOperand,
        *RearRightMooringComponentName.ToString());
    auto* RightBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2290, -1440);
    auto* StoreRight = AddWrite(Graph, CachedRearRightMooringSocket, 2520, -1440);
    auto* BindSequence = FinishNode(
        NewObject<UK2Node_ExecutionSequence>(Graph), Graph, 1140, -1080);
    auto* Callback = FinishNode(
        NewObject<UK2Node_CreateDelegate>(Graph), Graph, 1370, -860);
    Callback->SetFunction(DisabledMooringConnectionCallback);

    bool Ok = true;
    Ok &= Connect(BeginTail, RequirePin(BeginSequence, P::Execute));
    Ok &= Connect(BeginSequence->GetThenPinGivenIndex(0), ExistingWork);
    Ok &= Connect(BeginSequence->GetThenPinGivenIndex(1), RequirePin(FourPorts, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::Then), RequirePin(DisabledBranch, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::ReturnValue), RequirePin(DisabledBranch, P::Condition));
    Ok &= Connect(RequirePin(DisabledBranch, P::Else), RequireExecInputPin(Loop));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(Components, P::FunctionTarget));
    Ok &= Connect(RequirePin(Components, P::ReturnValue), RequirePin(Loop, CargoAction::Array));
    Ok &= Connect(RequirePin(Loop, P::LoopBody), RequirePin(Socket, P::Execute));
    Ok &= Connect(RequirePin(Loop, CargoAction::ArrayElement), Socket->GetCastSourcePin());
    Ok &= Connect(Socket->GetValidCastPin(), RequirePin(LeftBranch, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(ObjectName, P::Object));
    Ok &= Connect(RequirePin(ObjectName, P::ReturnValue),
        RequirePin(IsLeft, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsLeft, P::ReturnValue), RequirePin(LeftBranch, P::Condition));
    Ok &= Connect(RequirePin(LeftBranch, P::Then), RequirePin(StoreLeft, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(StoreLeft, CachedRearLeftMooringSocket));
    Ok &= Connect(RequirePin(LeftBranch, P::Else), RequirePin(RightBranch, P::Execute));
    Ok &= Connect(RequirePin(ObjectName, P::ReturnValue),
        RequirePin(IsRight, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsRight, P::ReturnValue), RequirePin(RightBranch, P::Condition));
    Ok &= Connect(RequirePin(RightBranch, P::Then), RequirePin(StoreRight, P::Execute));
    Ok &= Connect(Socket->GetCastResultPin(), RequirePin(StoreRight, CachedRearRightMooringSocket));
    Ok &= Connect(RequirePin(Loop, P::Completed), RequirePin(BindSequence, P::Execute));

    const TArray<FName> SocketFields = {
        CachedRearLeftMooringSocket, CachedRearRightMooringSocket};
    int32 BindIndex = 0;
    for (const FName SocketField : SocketFields)
    {
        const int32 Y = -1080 + BindIndex * 300;
        auto* Cached = AddRead(Graph, SocketField, nullptr, 1370, Y);
        auto* Valid = AddCall(Graph, UKismetSystemLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 1600, Y - 120);
        auto* ValidBranch = FinishNode(
            NewObject<UK2Node_IfThenElse>(Graph), Graph, 1830, Y);
        auto* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
        Remove->SetFromProperty(DelegateProperty, false, UModuleSocketComponent::StaticClass());
        FinishNode(Remove, Graph, 2060, Y);
        auto* Add = NewObject<UK2Node_AddDelegate>(Graph);
        Add->SetFromProperty(DelegateProperty, false, UModuleSocketComponent::StaticClass());
        FinishNode(Add, Graph, 2290, Y);
        Ok &= Connect(BindSequence->GetThenPinGivenIndex(BindIndex),
            RequirePin(ValidBranch, P::Execute));
        Ok &= Connect(RequirePin(Cached, SocketField), RequirePin(Valid, P::Object));
        Ok &= Connect(RequirePin(Valid, P::ReturnValue), RequirePin(ValidBranch, P::Condition));
        Ok &= Connect(RequirePin(ValidBranch, P::Then), RequirePin(Remove, P::Execute));
        Ok &= Connect(RequirePin(Cached, SocketField), RequirePin(Remove, P::FunctionTarget));
        Ok &= Connect(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
        Ok &= Connect(RequirePin(Remove, P::Then), RequirePin(Add, P::Execute));
        Ok &= Connect(RequirePin(Cached, SocketField), RequirePin(Add, P::FunctionTarget));
        Ok &= Connect(Callback->GetDelegateOutPin(), Add->GetDelegatePin());
        ++BindIndex;
    }
    BindSequence->AddInputPin();
    auto* Queue = AddCall(
        Graph, Blueprint->GeneratedClass, QueueDisabledMooringCleanupFunction, 1370, -480);
    Ok &= Connect(BindSequence->GetThenPinGivenIndex(2), RequirePin(Queue, P::Execute));
    return Ok;
}

bool AddMooringLayoutConstruction(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindUserConstructionScript(Blueprint);
    if (!Graph) return false;
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    if (!Entry) return false;
    UEdGraphPin* EntryThen = RequirePin(Entry, P::Then);
    EntryThen->BreakAllPinLinks();
    FEdGraphPinType SceneComponentArrayType;
    SceneComponentArrayType.PinCategory = UEdGraphSchema_K2::PC_Object;
    SceneComponentArrayType.PinSubCategoryObject = USceneComponent::StaticClass();
    SceneComponentArrayType.ContainerType = EPinContainerType::Array;
    if (!FBlueprintEditorUtils::AddLocalVariable(
            Blueprint, Graph, MooringVehicleComponentsLocal, SceneComponentArrayType))
        return false;
    FEdGraphPinType SceneComponentType;
    SceneComponentType.PinCategory = UEdGraphSchema_K2::PC_Object;
    SceneComponentType.PinSubCategoryObject = USceneComponent::StaticClass();
    if (!FBlueprintEditorUtils::AddLocalVariable(
            Blueprint, Graph, MooringVehicleMeshLocal, SceneComponentType))
        return false;
    UFunction* ConstructionFunction = AActor::StaticClass()->FindFunctionByName(
        MooringLayout::UserConstructionScriptFunction);
    if (!ConstructionFunction) return false;
    auto* Parent = NewObject<UK2Node_CallParentFunction>(Graph);
    Parent->SetFromFunction(ConstructionFunction);
    FinishNode(Parent, Graph, 1600, -440);
    auto* FourPorts = AddCall(
        Graph, Blueprint->GeneratedClass, GetFourPortSetting, -900, -440);
    auto* ModeBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, -680, -440);
    auto* Self = FinishNode(NewObject<UK2Node_Self>(Graph), Graph, -900, -220);
    auto* ComponentsForVehicleMesh = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        -680, -220);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(ComponentsForVehicleMesh, CargoAction::ComponentClass),
        USceneComponent::StaticClass());
    auto* SaveVehicleComponents = AddLocalWrite(
        Blueprint, Graph, MooringVehicleComponentsLocal, -450, -440);
    auto* SavedVehicleComponents = AddLocalRead(
        Blueprint, Graph, MooringVehicleComponentsLocal, -450, -660);
    auto* VehicleLoop = NewObject<UK2Node_MacroInstance>(Graph);
    VehicleLoop->SetMacroGraph(StandardForEachLoop());
    FinishNode(VehicleLoop, Graph, -450, -220);
    auto* VehicleScene = NewObject<UK2Node_DynamicCast>(Graph);
    VehicleScene->TargetType = USceneComponent::StaticClass();
    VehicleScene->SetPurity(false);
    FinishNode(VehicleScene, Graph, -220, -220);
    auto* VehicleObjectName = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName), 0, -220);
    auto* IsVehicleMesh = AddCall(
        Graph, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr), 220, -220);
    SetDefault(IsVehicleMesh, BlueprintGraphNames::Pins::Binary::RightOperand,
        *VehicleMeshComponentName.ToString());
    auto* VehicleBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 440, -220);
    auto* SaveVehicleMesh = AddLocalWrite(
        Blueprint, Graph, MooringVehicleMeshLocal, 660, -440);
    auto* SavedVehicleMesh = AddLocalRead(
        Blueprint, Graph, MooringVehicleMeshLocal, 660, -660);
    auto* VehicleIsValid = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid), 660, -220);
    auto* VehicleFoundBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 880, -220);

    struct FConditionalComponent
    {
        FName Name;
        FTransform RelativeTransform;
        bool bFourPortOnly;
    };
    const TArray<FConditionalComponent> ComponentsToCreate = {
        { RearLeftMooringComponentName,
          FTransform(MooringLayout::RearLeftSocketRotation,
              MooringLayout::RearLeftSocketLocation, FVector::OneVector), false },
        { RearRightMooringComponentName,
          FTransform(MooringLayout::RearRightSocketRotation,
              MooringLayout::RearRightSocketLocation, FVector::OneVector), false },
        { FrontPositiveMooringPlateName,
          FTransform(FRotator::ZeroRotator,
              MooringLayout::FrontPositivePlateLocation, FVector::OneVector), true },
        { FrontNegativeMooringPlateName,
          FTransform(FRotator::ZeroRotator,
              MooringLayout::FrontNegativePlateLocation, FVector::OneVector), true },
        { RearPositiveMooringPlateName,
          FTransform(FRotator::ZeroRotator,
              MooringLayout::RearPositivePlateLocation, FVector::OneVector), true },
        { RearNegativeMooringPlateName,
          FTransform(FRotator::ZeroRotator,
              MooringLayout::RearNegativePlateLocation, FVector::OneVector), true }
    };
    UEdGraphPin* CreateThen = RequirePin(VehicleFoundBranch, P::Then);
    int32 CreateX = 1100;
    bool Ok = true;
    bool RoutedThroughModeBranch = false;
    for (const FConditionalComponent& ComponentToCreate : ComponentsToCreate)
    {
        if (ComponentToCreate.bFourPortOnly && !RoutedThroughModeBranch)
        {
            Ok &= Connect(CreateThen, RequirePin(ModeBranch, P::Execute));
            CreateThen = RequirePin(ModeBranch, P::Then);
            RoutedThroughModeBranch = true;
        }
        const FVector Location = ComponentToCreate.RelativeTransform.GetLocation();
        const FRotator Rotation = ComponentToCreate.RelativeTransform.Rotator();
        const FVector Scale = ComponentToCreate.RelativeTransform.GetScale3D();
        const FString LocationDefault = FString::Printf(
            MooringLayout::VectorTripleFormat, Location.X, Location.Y, Location.Z);
        const FString RotationDefault = FString::Printf(
            MooringLayout::VectorTripleFormat,
            Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
        const FString ScaleDefault = FString::Printf(
            MooringLayout::VectorTripleFormat, Scale.X, Scale.Y, Scale.Z);
        auto* MakeTransform = AddCall(
            Graph, UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform),
            CreateX, -520);
        SetDefault(MakeTransform, MooringLayout::LocationPin, *LocationDefault);
        SetDefault(MakeTransform, MooringLayout::RotationPin, *RotationDefault);
        SetDefault(MakeTransform, MooringLayout::ScalePin, *ScaleDefault);
        auto* Add = AddCall(
            Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, AddComponent),
            CreateX, -220);
        SetDefault(Add, MooringLayout::TemplateNamePin, *ComponentToCreate.Name.ToString());
        SetDefault(Add, MooringLayout::ManualAttachmentPin, ArcadeAltitude::Enabled);
        SetDefault(Add, MooringLayout::DeferredFinishPin, ArcadeAltitude::Enabled);
        auto* AddedScene = NewObject<UK2Node_DynamicCast>(Graph);
        AddedScene->TargetType = USceneComponent::StaticClass();
        AddedScene->SetPurity(false);
        FinishNode(AddedScene, Graph, CreateX + 220, -220);
        auto* Attach = AddCall(
            Graph, USceneComponent::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_AttachToComponent),
            CreateX + 440, -220);
        SetDefault(Attach, MooringLayout::SocketNamePin, MooringLayout::NoSocket);
        SetDefault(Attach, MooringLayout::LocationRulePin, MooringLayout::KeepRelativeRule);
        SetDefault(Attach, MooringLayout::RotationRulePin, MooringLayout::KeepRelativeRule);
        SetDefault(Attach, MooringLayout::ScaleRulePin, MooringLayout::KeepRelativeRule);
        SetDefault(Attach, MooringLayout::WeldBodiesPin, ArcadeAltitude::Disabled);
        auto* Finish = AddCall(
            Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, FinishAddComponent),
            CreateX + 660, -220);
        SetDefault(Finish, MooringLayout::ManualAttachmentPin, ArcadeAltitude::Enabled);

        Ok &= Connect(CreateThen, RequirePin(Add, P::Execute));
        Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
            RequirePin(Add, P::FunctionTarget));
        Ok &= Connect(RequirePin(MakeTransform, P::ReturnValue),
            RequirePin(Add, MooringLayout::RelativeTransformPin));
        Ok &= Connect(RequirePin(Add, P::Then), RequirePin(AddedScene, P::Execute));
        Ok &= Connect(RequirePin(Add, P::ReturnValue), AddedScene->GetCastSourcePin());
        Ok &= Connect(AddedScene->GetValidCastPin(), RequirePin(Attach, P::Execute));
        Ok &= Connect(AddedScene->GetCastResultPin(), RequirePin(Attach, P::FunctionTarget));
        Ok &= Connect(SavedVehicleMesh->GetValuePin(),
            RequirePin(Attach, MooringLayout::ParentPin));
        Ok &= Connect(RequirePin(Attach, P::Then), RequirePin(Finish, P::Execute));
        Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
            RequirePin(Finish, P::FunctionTarget));
        Ok &= Connect(RequirePin(Add, P::ReturnValue),
            RequirePin(Finish, MooringLayout::ComponentPin));
        Ok &= Connect(RequirePin(MakeTransform, P::ReturnValue),
            RequirePin(Finish, MooringLayout::RelativeTransformPin));
        CreateThen = RequirePin(Finish, P::Then);
        CreateX += 900;
    }

    auto* AfterParentModeBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1820, -440);
    auto* LayoutComponents = AddCall(
        Graph, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass),
        1820, -220);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *RequirePin(LayoutComponents, CargoAction::ComponentClass), USceneComponent::StaticClass());
    auto* LayoutLoop = NewObject<UK2Node_MacroInstance>(Graph);
    LayoutLoop->SetMacroGraph(StandardForEachLoop());
    FinishNode(LayoutLoop, Graph, 2040, -220);
    auto* LayoutScene = NewObject<UK2Node_DynamicCast>(Graph);
    LayoutScene->TargetType = USceneComponent::StaticClass();
    LayoutScene->SetPurity(false);
    FinishNode(LayoutScene, Graph, 2260, -220);
    auto* LayoutObjectName = AddCall(
        Graph, UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName), 2480, -220);

    const auto AddNameTest = [&](const TCHAR* Name, int32 X, int32 Y)
    {
        auto* Equals = AddCall(
            Graph, UKismetStringLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr), X, Y - 180);
        SetDefault(Equals, BlueprintGraphNames::Pins::Binary::RightOperand, Name);
        auto* Branch = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, X + 220, Y);
        Ok &= Connect(RequirePin(LayoutObjectName, P::ReturnValue),
            RequirePin(Equals, BlueprintGraphNames::Pins::Binary::LeftOperand));
        Ok &= Connect(RequirePin(Equals, P::ReturnValue), RequirePin(Branch, P::Condition));
        return Branch;
    };

    auto* PlateBranch = AddNameTest(MooringLayout::StockPlateObjectName, 2700, 0);
    auto* HidePlate = AddCall(
        Graph, USceneComponent::StaticClass(),
        MooringLayout::SetVisibilityFunction, 2920, -220);
    SetDefault(HidePlate, MooringLayout::NewVisibility, CargoAction::Disabled);
    SetDefault(HidePlate, MooringLayout::PropagateVisibilityToChildren,
        CargoAction::Disabled);
    auto* PositiveBranch = AddNameTest(
        MooringLayout::StockPositiveSocketObjectName, 2920, 220);
    auto* SetPositive = AddCall(
        Graph, USceneComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetRelativeLocation), 3140, 120);
    SetDefault(SetPositive, ArcadeAltitude::NewLocation,
        MooringLayout::StockPositiveSocketLocationDefault);
    SetDefault(SetPositive, ArcadeAltitude::Sweep, CargoAction::Disabled);
    SetDefault(SetPositive, ArcadeAltitude::Teleport, CargoAction::Disabled);
    auto* NegativeBranch = AddNameTest(
        MooringLayout::StockNegativeSocketObjectName, 3140, 440);
    auto* SetNegative = AddCall(
        Graph, USceneComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetRelativeLocation), 3360, 340);
    SetDefault(SetNegative, ArcadeAltitude::NewLocation,
        MooringLayout::StockNegativeSocketLocationDefault);
    SetDefault(SetNegative, ArcadeAltitude::Sweep, CargoAction::Disabled);
    SetDefault(SetNegative, ArcadeAltitude::Teleport, CargoAction::Disabled);

    Ok &= Connect(EntryThen, RequirePin(FourPorts, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::ReturnValue), RequirePin(ModeBranch, P::Condition));
    Ok &= Connect(RequirePin(FourPorts, P::Then), RequirePin(SaveVehicleComponents, P::Execute));
    Ok &= Connect(RequirePin(ModeBranch, P::Else), RequirePin(Parent, P::Execute));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(ComponentsForVehicleMesh, P::FunctionTarget));
    Ok &= Connect(RequirePin(ComponentsForVehicleMesh, P::ReturnValue),
        RequirePin(SaveVehicleComponents, MooringVehicleComponentsLocal));
    Ok &= Connect(RequirePin(SaveVehicleComponents, P::Then), RequireExecInputPin(VehicleLoop));
    Ok &= Connect(SavedVehicleComponents->GetValuePin(),
        RequirePin(VehicleLoop, CargoAction::Array));
    Ok &= Connect(RequirePin(VehicleLoop, P::LoopBody), RequirePin(VehicleScene, P::Execute));
    Ok &= Connect(RequirePin(VehicleLoop, CargoAction::ArrayElement),
        VehicleScene->GetCastSourcePin());
    Ok &= Connect(VehicleScene->GetValidCastPin(), RequirePin(VehicleBranch, P::Execute));
    Ok &= Connect(VehicleScene->GetCastResultPin(), RequirePin(VehicleObjectName, P::Object));
    Ok &= Connect(RequirePin(VehicleObjectName, P::ReturnValue),
        RequirePin(IsVehicleMesh, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsVehicleMesh, P::ReturnValue), RequirePin(VehicleBranch, P::Condition));
    Ok &= Connect(RequirePin(VehicleBranch, P::Then), RequirePin(SaveVehicleMesh, P::Execute));
    Ok &= Connect(VehicleScene->GetCastResultPin(),
        RequirePin(SaveVehicleMesh, MooringVehicleMeshLocal));
    Ok &= Connect(RequirePin(VehicleLoop, P::Completed), RequirePin(VehicleFoundBranch, P::Execute));
    Ok &= Connect(SavedVehicleMesh->GetValuePin(), RequirePin(VehicleIsValid, P::Object));
    Ok &= Connect(RequirePin(VehicleIsValid, P::ReturnValue),
        RequirePin(VehicleFoundBranch, P::Condition));
    Ok &= Connect(RequirePin(VehicleFoundBranch, P::Else), RequirePin(Parent, P::Execute));
    Ok &= Connect(CreateThen, RequirePin(Parent, P::Execute));
    Ok &= Connect(RequirePin(Parent, P::Then), RequirePin(AfterParentModeBranch, P::Execute));
    Ok &= Connect(RequirePin(FourPorts, P::ReturnValue), RequirePin(AfterParentModeBranch, P::Condition));
    Ok &= Connect(RequirePin(AfterParentModeBranch, P::Then), RequireExecInputPin(LayoutLoop));
    Ok &= Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        RequirePin(LayoutComponents, P::FunctionTarget));
    Ok &= Connect(RequirePin(LayoutComponents, P::ReturnValue),
        RequirePin(LayoutLoop, CargoAction::Array));
    Ok &= Connect(RequirePin(LayoutLoop, P::LoopBody), RequirePin(LayoutScene, P::Execute));
    Ok &= Connect(RequirePin(LayoutLoop, CargoAction::ArrayElement), LayoutScene->GetCastSourcePin());
    Ok &= Connect(LayoutScene->GetValidCastPin(), RequirePin(PlateBranch, P::Execute));
    Ok &= Connect(LayoutScene->GetCastResultPin(), RequirePin(LayoutObjectName, P::Object));
    Ok &= Connect(RequirePin(PlateBranch, P::Then), RequirePin(HidePlate, P::Execute));
    Ok &= Connect(LayoutScene->GetCastResultPin(), RequirePin(HidePlate, P::FunctionTarget));
    Ok &= Connect(RequirePin(PlateBranch, P::Else), RequirePin(PositiveBranch, P::Execute));
    Ok &= Connect(RequirePin(PositiveBranch, P::Then), RequirePin(SetPositive, P::Execute));
    Ok &= Connect(LayoutScene->GetCastResultPin(), RequirePin(SetPositive, P::FunctionTarget));
    Ok &= Connect(RequirePin(PositiveBranch, P::Else), RequirePin(NegativeBranch, P::Execute));
    Ok &= Connect(RequirePin(NegativeBranch, P::Then), RequirePin(SetNegative, P::Execute));
    Ok &= Connect(LayoutScene->GetCastResultPin(), RequirePin(SetNegative, P::FunctionTarget));
    return Ok;
}

bool TestFourPortSettingsAndLayout(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->GeneratedClass) return false;
    UClass* Class = Blueprint->GeneratedClass;
    UObject* Defaults = Class->GetDefaultObject();
    FBoolProperty* EnabledProperty = FindFProperty<FBoolProperty>(
        Class, Settings::EnableFourMooringPorts);
    FBoolProperty* LoadedProperty = FindFProperty<FBoolProperty>(
        Class, FourPortSettingsLoaded);
    UFunction* Getter = Class->FindFunctionByName(
        GetFourPortSetting, EIncludeSuperFlag::ExcludeSuper);
    UFunction* Construction = Class->FindFunctionByName(
        MooringLayout::UserConstructionScriptFunction);
    if (!Defaults || !EnabledProperty || !LoadedProperty || !Getter || !Construction ||
        !EnabledProperty->HasAnyPropertyFlags(CPF_Transient) ||
        !LoadedProperty->HasAnyPropertyFlags(CPF_Transient)) return false;

    const FString SettingsPath = FPaths::Combine(
        FPaths::ProjectContentDir(), Settings::RelativePath);
    const bool HadSettings = FPaths::FileExists(SettingsPath);
    TArray<uint8> OriginalSettings;
    if (HadSettings && !FFileHelper::LoadFileToArray(OriginalSettings, *SettingsPath))
        return false;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SettingsPath), true);

    bool Passed = true;
    auto WriteSettings = [&SettingsPath](const TCHAR* Text)
    {
        return FFileHelper::SaveStringToFile(
            Text, *SettingsPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    };
    auto ResetDefaults = [&](const bool Enabled)
    {
        EnabledProperty->SetPropertyValue_InContainer(Defaults, Enabled);
        LoadedProperty->SetPropertyValue_InContainer(Defaults, false);
    };
    auto ReadSetting = [&](UObject* Target)
    {
        struct FGetterResult { bool ReturnValue = false; } Result;
        Target->ProcessEvent(Getter, &Result);
        return Result.ReturnValue;
    };

    Passed &= WriteSettings(TEXT("EnableFourMooringPorts=false\n"));
    ResetDefaults(true);
    Passed &= !ReadSetting(Defaults);
    Passed &= WriteSettings(TEXT("EnableFourMooringPorts=true\n"));
    Passed &= !ReadSetting(Defaults);
    LoadedProperty->SetPropertyValue_InContainer(Defaults, false);
    Passed &= ReadSetting(Defaults);
    IFileManager::Get().Delete(*SettingsPath, false, true);
    ResetDefaults(false);
    Passed &= ReadSetting(Defaults);
    Passed &= WriteSettings(TEXT("EnableFourMooringPorts=invalid\n"));
    ResetDefaults(false);
    Passed &= ReadSetting(Defaults);

    const TArray<FName> ModComponentNames = {
        RearLeftMooringComponentName, RearRightMooringComponentName,
        FrontPositiveMooringPlateName, FrontNegativeMooringPlateName,
        RearPositiveMooringPlateName, RearNegativeMooringPlateName};
    for (const FName Name : ModComponentNames)
    {
        int32 TemplateCount = 0;
        for (const UActorComponent* Component : Blueprint->ComponentTemplates)
            if (Component && Component->GetFName() == Name) ++TemplateCount;
        Passed &= TemplateCount == 1;
        Passed &= Blueprint->SimpleConstructionScript->FindSCSNode(Name) == nullptr;
    }

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (World)
        World->InitializeActorsForPlay(FURL());

    auto SpawnActor = [&](const bool Enabled)
    {
        if (!World) return static_cast<AActor*>(nullptr);
        EnabledProperty->SetPropertyValue_InContainer(Defaults, Enabled);
        LoadedProperty->SetPropertyValue_InContainer(Defaults, true);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        return World->SpawnActor<AActor>(Class, FTransform::Identity, SpawnParameters);
    };
    auto CountModComponents = [&ModComponentNames](AActor* Actor)
    {
        if (!Actor) return -1;
        int32 Count = 0;
        TInlineComponentArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (!IsValid(Component)) continue;
            if (ModComponentNames.Contains(Component->GetFName()))
                ++Count;
        }
        return Count;
    };
    struct FLayoutFixture
    {
        USceneComponent* VehicleMesh = nullptr;
        UStaticMeshComponent* StockPlate = nullptr;
        USceneComponent* StockPositive = nullptr;
        USceneComponent* StockNegative = nullptr;
    };
    auto AddFixtureComponents = [](AActor* Actor)
    {
        FLayoutFixture Fixture;
        if (!Actor) return Fixture;
        Fixture.VehicleMesh = NewObject<USceneComponent>(Actor, VehicleMeshComponentName);
        Fixture.StockPlate = NewObject<UStaticMeshComponent>(
            Actor, FName(MooringLayout::StockPlateObjectName));
        Fixture.StockPositive = NewObject<USceneComponent>(
            Actor, FName(MooringLayout::StockPositiveSocketObjectName));
        Fixture.StockNegative = NewObject<USceneComponent>(
            Actor, FName(MooringLayout::StockNegativeSocketObjectName));
        TArray<UActorComponent*> Components = {
            Fixture.VehicleMesh, Fixture.StockPlate,
            Fixture.StockPositive, Fixture.StockNegative};
        for (UActorComponent* Component : Components)
        {
            Actor->AddInstanceComponent(Component);
            Component->RegisterComponent();
        }
        Fixture.StockPlate->SetVisibility(true, false);
        Fixture.StockPositive->SetRelativeLocation(FVector::ZeroVector);
        Fixture.StockNegative->SetRelativeLocation(FVector::ZeroVector);
        return Fixture;
    };

    AActor* StockActor = SpawnActor(false);
    FLayoutFixture StockFixture = AddFixtureComponents(StockActor);
    if (StockActor) StockActor->ProcessEvent(Construction, nullptr);
    Passed &= StockActor != nullptr;
    Passed &= CountModComponents(StockActor) == 2;
    Passed &= StockFixture.StockPlate && StockFixture.StockPlate->IsVisible();
    Passed &= StockFixture.StockPositive &&
        StockFixture.StockPositive->GetRelativeLocation().IsNearlyZero();
    Passed &= StockFixture.StockNegative &&
        StockFixture.StockNegative->GetRelativeLocation().IsNearlyZero();

    AActor* FourPortActor = SpawnActor(true);
    FLayoutFixture FourPortFixture = AddFixtureComponents(FourPortActor);
    if (FourPortActor) FourPortActor->ProcessEvent(Construction, nullptr);
    Passed &= FourPortActor != nullptr;
    Passed &= CountModComponents(FourPortActor) == 6;
    Passed &= FourPortFixture.StockPlate && !FourPortFixture.StockPlate->IsVisible();
    Passed &= FourPortFixture.StockPositive &&
        FourPortFixture.StockPositive->GetRelativeLocation().Equals(
            FVector(78.782426, 50.862349, -32.989642), UE_KINDA_SMALL_NUMBER);
    Passed &= FourPortFixture.StockNegative &&
        FourPortFixture.StockNegative->GetRelativeLocation().Equals(
            FVector(78.783752, -50.714545, -32.805209), UE_KINDA_SMALL_NUMBER);

    auto ValidateRearSockets = [&](AActor* Actor, USceneComponent* VehicleMesh)
    {
        auto* RearLeft = Actor ? FindObject<UVoyageModuleSocketViewComponent>(
            Actor, *RearLeftMooringComponentName.ToString()) : nullptr;
        auto* RearRight = Actor ? FindObject<UVoyageModuleSocketViewComponent>(
            Actor, *RearRightMooringComponentName.ToString()) : nullptr;
        auto* RearLeftTemplate = Cast<USceneComponent>(
            Blueprint->FindTemplateByName(RearLeftMooringComponentName));
        auto* RearRightTemplate = Cast<USceneComponent>(
            Blueprint->FindTemplateByName(RearRightMooringComponentName));
        Passed &= RearLeft && RearLeft->SocketID == MooringLayout::RearLeftSocketId &&
            RearLeft->GetAttachParent() == VehicleMesh && RearLeftTemplate &&
            RearLeft->GetRelativeTransform().Equals(
                RearLeftTemplate->GetRelativeTransform(), UE_KINDA_SMALL_NUMBER);
        Passed &= RearRight && RearRight->SocketID == MooringLayout::RearRightSocketId &&
            RearRight->GetAttachParent() == VehicleMesh && RearRightTemplate &&
            RearRight->GetRelativeTransform().Equals(
                RearRightTemplate->GetRelativeTransform(), UE_KINDA_SMALL_NUMBER);
    };
    ValidateRearSockets(StockActor, StockFixture.VehicleMesh);
    ValidateRearSockets(FourPortActor, FourPortFixture.VehicleMesh);

    FObjectProperty* CachedLeftProperty = FindFProperty<FObjectProperty>(
        Class, CachedRearLeftMooringSocket);
    FObjectProperty* CachedRightProperty = FindFProperty<FObjectProperty>(
        Class, CachedRearRightMooringSocket);
    FBoolProperty* CleanupQueuedProperty = FindFProperty<FBoolProperty>(
        Class, DisabledMooringCleanupQueued);
    UFunction* Cleanup = Class->FindFunctionByName(RunDisabledMooringCleanupFunction);
    UFunction* ConnectionCallback = Class->FindFunctionByName(
        DisabledMooringConnectionCallback);
    Passed &= CachedLeftProperty && CachedRightProperty && CleanupQueuedProperty &&
        Cleanup && ConnectionCallback;

    auto FindRearSocket = [](AActor* Actor, const FName Name)
    {
        return Actor ? FindObject<UVoyageModuleSocketViewComponent>(
            Actor, *Name.ToString()) : nullptr;
    };
    auto AddTestPrimitive = [](AActor* Owner, USceneComponent* Parent, const FName Name)
    {
        auto* Primitive = Owner ? NewObject<UBoxComponent>(Owner, Name) : nullptr;
        if (!Primitive) return Primitive;
        Owner->AddInstanceComponent(Primitive);
        if (Parent) Primitive->SetupAttachment(Parent);
        Primitive->RegisterComponent();
        Primitive->SetVisibility(true, false);
        Primitive->SetHiddenInGame(false, false);
        Primitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Primitive->SetGenerateOverlapEvents(true);
        Primitive->Activate(true);
        return Primitive;
    };
    auto IsDisabledPrimitive = [](const UPrimitiveComponent* Primitive)
    {
        return Primitive && !Primitive->IsActive() && !Primitive->IsVisible() &&
            Primitive->bHiddenInGame &&
            Primitive->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
            !Primitive->GetGenerateOverlapEvents();
    };
    auto IsEnabledPrimitive = [](const UPrimitiveComponent* Primitive)
    {
        return Primitive && Primitive->IsActive() && Primitive->IsVisible() &&
            !Primitive->bHiddenInGame &&
            Primitive->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics &&
            Primitive->GetGenerateOverlapEvents();
    };

    auto* StockRearLeft = FindRearSocket(StockActor, RearLeftMooringComponentName);
    auto* StockRearRight = FindRearSocket(StockActor, RearRightMooringComponentName);
    auto* OwnedChild = AddTestPrimitive(
        StockActor, StockRearLeft, TEXT("StableGyroFixtureOwnedChild"));
    auto* OwnedCollision = AddTestPrimitive(
        StockActor, nullptr, TEXT("StableGyroFixtureOwnedCollision"));
    auto* OwnedPlaceholder = AddTestPrimitive(
        StockActor, nullptr, TEXT("StableGyroFixtureOwnedPlaceholder"));
    AActor* ForeignOwner = World ? World->SpawnActor<AActor>() : nullptr;
    auto* ForeignMesh = AddTestPrimitive(
        ForeignOwner, nullptr, TEXT("StableGyroFixtureForeignMesh"));
    if (StockRearLeft)
    {
        StockRearLeft->SocketCollisionBox = OwnedCollision;
        StockRearLeft->IntegratedCablePlaceholderMesh = OwnedPlaceholder;
        StockRearLeft->mesh = ForeignMesh;
    }
    if (StockActor && CachedLeftProperty && CachedRightProperty && CleanupQueuedProperty &&
        ConnectionCallback && Cleanup)
    {
        EnabledProperty->SetPropertyValue_InContainer(Defaults, false);
        LoadedProperty->SetPropertyValue_InContainer(Defaults, true);
        CachedLeftProperty->SetObjectPropertyValue_InContainer(StockActor, StockRearLeft);
        CachedRightProperty->SetObjectPropertyValue_InContainer(StockActor, StockRearRight);
        struct FConnectionEvent { bool bSocketConnection = false; } Event;
        CleanupQueuedProperty->SetPropertyValue_InContainer(StockActor, false);
        StockActor->ProcessEvent(ConnectionCallback, &Event);
        Passed &= CleanupQueuedProperty->GetPropertyValue_InContainer(StockActor);
        Event.bSocketConnection = true;
        StockActor->ProcessEvent(ConnectionCallback, &Event);
        Passed &= CleanupQueuedProperty->GetPropertyValue_InContainer(StockActor);
        StockActor->ProcessEvent(Cleanup, nullptr);
        Passed &= !CleanupQueuedProperty->GetPropertyValue_InContainer(StockActor);
        StockActor->ProcessEvent(ConnectionCallback, &Event);
        Passed &= CleanupQueuedProperty->GetPropertyValue_InContainer(StockActor);
        Event.bSocketConnection = false;
        StockActor->ProcessEvent(ConnectionCallback, &Event);
        Passed &= CleanupQueuedProperty->GetPropertyValue_InContainer(StockActor);
        StockActor->ProcessEvent(Cleanup, nullptr);
        Passed &= !CleanupQueuedProperty->GetPropertyValue_InContainer(StockActor);
    }
    Passed &= IsDisabledPrimitive(StockRearLeft);
    Passed &= IsDisabledPrimitive(StockRearRight);
    Passed &= IsDisabledPrimitive(OwnedChild);
    Passed &= IsDisabledPrimitive(OwnedCollision);
    Passed &= IsDisabledPrimitive(OwnedPlaceholder);
    Passed &= IsEnabledPrimitive(ForeignMesh);
    Passed &= StockFixture.StockPlate && StockFixture.StockPlate->IsVisible();
    Passed &= StockFixture.StockPositive &&
        StockFixture.StockPositive->GetRelativeLocation().IsNearlyZero();
    Passed &= StockFixture.StockNegative &&
        StockFixture.StockNegative->GetRelativeLocation().IsNearlyZero();

    auto* FourPortRearLeft = FindRearSocket(FourPortActor, RearLeftMooringComponentName);
    auto* FourPortRearRight = FindRearSocket(FourPortActor, RearRightMooringComponentName);
    auto* FourPortChild = AddTestPrimitive(
        FourPortActor, FourPortRearLeft, TEXT("StableGyroFixtureFourPortChild"));
    struct FPrimitiveState
    {
        bool bActive = false;
        bool bVisible = false;
        bool bHiddenInGame = false;
        bool bGenerateOverlapEvents = false;
        ECollisionEnabled::Type Collision = ECollisionEnabled::NoCollision;
    };
    auto CapturePrimitiveState = [](const UPrimitiveComponent* Primitive)
    {
        FPrimitiveState State;
        if (Primitive)
        {
            State.bActive = Primitive->IsActive();
            State.bVisible = Primitive->IsVisible();
            State.bHiddenInGame = Primitive->bHiddenInGame;
            State.bGenerateOverlapEvents = Primitive->GetGenerateOverlapEvents();
            State.Collision = Primitive->GetCollisionEnabled();
        }
        return State;
    };
    auto MatchesPrimitiveState = [](const UPrimitiveComponent* Primitive,
        const FPrimitiveState& State)
    {
        return Primitive && Primitive->IsActive() == State.bActive &&
            Primitive->IsVisible() == State.bVisible &&
            Primitive->bHiddenInGame == State.bHiddenInGame &&
            Primitive->GetGenerateOverlapEvents() == State.bGenerateOverlapEvents &&
            Primitive->GetCollisionEnabled() == State.Collision;
    };
    const FPrimitiveState FourPortLeftBefore = CapturePrimitiveState(FourPortRearLeft);
    const FPrimitiveState FourPortRightBefore = CapturePrimitiveState(FourPortRearRight);
    if (FourPortActor && CachedLeftProperty && CachedRightProperty && Cleanup)
    {
        EnabledProperty->SetPropertyValue_InContainer(Defaults, true);
        LoadedProperty->SetPropertyValue_InContainer(Defaults, true);
        CachedLeftProperty->SetObjectPropertyValue_InContainer(FourPortActor, FourPortRearLeft);
        CachedRightProperty->SetObjectPropertyValue_InContainer(FourPortActor, FourPortRearRight);
        FourPortActor->ProcessEvent(Cleanup, nullptr);
    }
    Passed &= MatchesPrimitiveState(FourPortRearLeft, FourPortLeftBefore);
    Passed &= MatchesPrimitiveState(FourPortRearRight, FourPortRightBefore);
    Passed &= IsEnabledPrimitive(FourPortChild);

    if (FourPortActor)
    {
        TInlineComponentArray<UActorComponent*> Components;
        FourPortActor->GetComponents(Components);
        for (const FName Name : ModComponentNames)
        {
            UActorComponent* const* Match = Components.FindByPredicate(
                [&Name](const UActorComponent* Component)
                {
                    return IsValid(Component) && Component->GetFName() == Name;
                });
            auto* SceneComponent = Match ? Cast<USceneComponent>(*Match) : nullptr;
            auto* TemplateScene = Cast<USceneComponent>(Blueprint->FindTemplateByName(Name));
            Passed &= SceneComponent &&
                SceneComponent->GetAttachParent() == FourPortFixture.VehicleMesh;
            Passed &= SceneComponent && TemplateScene &&
                SceneComponent->GetRelativeTransform().Equals(
                    TemplateScene->GetRelativeTransform(), UE_KINDA_SMALL_NUMBER);
        }
    }

    if (World)
    {
        World->DestroyWorld(false);
    }
    if (HadSettings)
        Passed &= FFileHelper::SaveArrayToFile(OriginalSettings, *SettingsPath);
    else
        Passed &= IFileManager::Get().Delete(*SettingsPath, false, true);
    ResetDefaults(true);
    Passed &= EnabledProperty->GetPropertyValue_InContainer(Defaults);
    Passed &= !LoadedProperty->GetPropertyValue_InContainer(Defaults);
    UE_LOG(LogTemp, Display, TEXT("StableGyro four-port cache/layout fixture: %s"),
        Passed ? TEXT("passed") : TEXT("failed"));
    return Passed;
}

}

UGenerateStableGyroInheritanceCommandlet::UGenerateStableGyroInheritanceCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateStableGyroInheritanceCommandlet::Main(const FString& Params)
{
    FString PositiveMooringGlb;
    FString NegativeMooringGlb;
    if (!FParse::Value(*Params, MooringLayout::PositiveGlbParameter, PositiveMooringGlb) ||
        !FParse::Value(*Params, MooringLayout::NegativeGlbParameter, NegativeMooringGlb))
    {
        UE_LOG(LogTemp, Error, TEXT("Both generated mooring-half GLB paths are required"));
        return 1;
    }
    if (FPackageName::DoesPackageExist(BasePackage) ||
        FPackageName::DoesPackageExist(ChildPackage))
    {
        UE_LOG(LogTemp, Error, TEXT("Gyro inheritance assets already exist"));
        return 1;
    }
    UBlueprint* StabilizationHelper = LoadObject<UBlueprint>(nullptr, StabilizationHelperPath);
    UBlueprint* PitchHelper = LoadObject<UBlueprint>(nullptr, PitchHelperPath);
    if (!StabilizationHelper || !StabilizationHelper->GeneratedClass ||
        !PitchHelper || !PitchHelper->GeneratedClass ||
        !ValidateNativeAltitudeOwnershipContracts() ||
        !ValidateHoverPitchResetContracts() ||
        !ValidateMooringSerializationContracts()) return 1;
    UVoyageInputAction* ResetAction = LoadObject<UVoyageInputAction>(
        nullptr, StableGyroAssetNames::ResetActionObjectPath);
    UInputAction* DropCargoAction = LoadObject<UInputAction>(
        nullptr, StableGyroAssetNames::DropCargoActionObjectPath);
    if (!ResetAction || !DropCargoAction) return 1;
    UVoyageItemCable* HookCable = CreateHookCableReferenceStub();
    if (!HookCable || HookCable->GetPathName() != HookCableObjectPath) return 1;
    UMaterialInstanceConstant* MooringMaterial = CreateMooringMaterialReferenceStub();
    if (!MooringMaterial) return 1;
    UStaticMesh* PositiveMooringMesh = ImportMooringHalf(
        PositiveMooringGlb, PositiveMooringMeshPackage, MooringMaterial,
        MooringLayout::PositiveSourceTriangleCount);
    UStaticMesh* NegativeMooringMesh = ImportMooringHalf(
        NegativeMooringGlb, NegativeMooringMeshPackage, MooringMaterial,
        MooringLayout::NegativeSourceTriangleCount);
    if (!PositiveMooringMesh || !NegativeMooringMesh) return 1;

    UPackage* ParentPackage = CreatePackage(BasePackage);
    UBlueprint* Parent = FKismetEditorUtilities::CreateBlueprint(
        AVoyageVehicleGyroCopter::StaticClass(), ParentPackage, FName(BaseAssetName),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        BaseBlueprintName);
    if (!Parent || !Parent->SimpleConstructionScript) return 1;
    FEdGraphPinType DoubleType;
    DoubleType.PinCategory = UEdGraphSchema_K2::PC_Real;
    DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    if (!SaveBlueprint(ParentPackage, Parent)) return 1;

    FEdGraphPinType BoolType;
    BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    FEdGraphPinType VectorType;
    VectorType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    VectorType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    FEdGraphPinType PrimitiveArrayType;
    PrimitiveArrayType.PinCategory = UEdGraphSchema_K2::PC_Object;
    PrimitiveArrayType.PinSubCategoryObject = UActorComponent::StaticClass();
    PrimitiveArrayType.ContainerType = EPinContainerType::Array;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, AltitudeLockActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, AltitudeExactHoldActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, SpaceWasHeld, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, DescentWasHeld, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, AltitudeBrakingActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, AltitudeDescentBrakingActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, LockedAltitude, DoubleType, ArcadeAltitude::Zero) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, PreviousCorrectedHorizontalVelocity,
            VectorType, ArcadeHorizontal::ZeroVectorDefault) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, HorizontalVelocitySampleValid, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, PreviousCorrectedVerticalVelocity,
            DoubleType, ArcadeAltitude::Zero) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, VerticalVelocitySampleValid,
            BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, CollisionReleaseUntilTime,
            DoubleType, ArcadeAltitude::Zero) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            StabilizationHelper, OwnedPrimitiveComponents, PrimitiveArrayType) ||
        !AddControllerChangedCallback(StabilizationHelper, false) ||
        !AddCollisionHitCallback(StabilizationHelper)) return 1;
    FBPVariableDescription* PrimitiveComponentsVariable =
        StabilizationHelper->NewVariables.FindByPredicate(
            [](const FBPVariableDescription& Candidate)
            {
                return Candidate.VarName == OwnedPrimitiveComponents;
            });
    if (!PrimitiveComponentsVariable) return 1;
    PrimitiveComponentsVariable->PropertyFlags |= CPF_Transient;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(StabilizationHelper);
    FKismetEditorUtilities::CompileBlueprint(StabilizationHelper);
    if (StabilizationHelper->Status == BS_Error ||
        !AddControllerChangedBinding(StabilizationHelper) ||
        !AddCollisionHitBinding(StabilizationHelper) ||
        !AddArcadeAltitudeHold(StabilizationHelper, AVoyageVehicleGyroCopter::StaticClass()) ||
        !SaveBlueprint(StabilizationHelper->GetOutermost(), StabilizationHelper, TG_PostPhysics)) return 1;

    FEdGraphPinType GyroObjectType;
    GyroObjectType.PinCategory = UEdGraphSchema_K2::PC_Object;
    GyroObjectType.PinSubCategoryObject = AVoyageVehicleGyroCopter::StaticClass();
    FEdGraphPinType StabilizationObjectType;
    StabilizationObjectType.PinCategory = UEdGraphSchema_K2::PC_Object;
    StabilizationObjectType.PinSubCategoryObject = StabilizationHelper->GeneratedClass;
    FEdGraphPinType ChildComponentArrayType;
    ChildComponentArrayType.PinCategory = UEdGraphSchema_K2::PC_Object;
    ChildComponentArrayType.PinSubCategoryObject = UActorComponent::StaticClass();
    ChildComponentArrayType.ContainerType = EPinContainerType::Array;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            PitchHelper, CachedGyro, GyroObjectType) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            PitchHelper, CachedStabilizationHelper, StabilizationObjectType) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            PitchHelper, OwnedChildActorComponents, ChildComponentArrayType) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            PitchHelper, NativeAltitudeOwnershipActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            PitchHelper, OriginalAutoHoverCollective, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            PitchHelper, OriginalAltitudeAssist, BoolType, ArcadeAltitude::Disabled)) return 1;
    for (const FName TransientField :
        {CachedGyro, CachedStabilizationHelper, OwnedChildActorComponents,
         NativeAltitudeOwnershipActive, OriginalAutoHoverCollective, OriginalAltitudeAssist})
    {
        FBPVariableDescription* Variable = PitchHelper->NewVariables.FindByPredicate(
            [&TransientField](const FBPVariableDescription& Candidate)
            {
                return Candidate.VarName == TransientField;
            });
        if (!Variable) return 1;
        Variable->PropertyFlags |= CPF_Transient;
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(PitchHelper);
    FKismetEditorUtilities::CompileBlueprint(PitchHelper);
    if (PitchHelper->Status == BS_Error ||
        !AddRestoreNativeAltitudeOwnership(PitchHelper)) return 1;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(PitchHelper);
    FKismetEditorUtilities::CompileBlueprint(PitchHelper);
    if (PitchHelper->Status == BS_Error ||
        !AddUpdateNativeAltitudeOwnership(
            PitchHelper, StabilizationHelper->GeneratedClass) ||
        !AddControllerChangedCallback(PitchHelper, true)) return 1;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(PitchHelper);
    FKismetEditorUtilities::CompileBlueprint(PitchHelper);
    if (PitchHelper->Status == BS_Error ||
        !AddControllerChangedBinding(PitchHelper) ||
        !AddNativeAltitudeOwnershipCacheBinding(
            PitchHelper, StabilizationHelper->GeneratedClass) ||
        !AddNativeAltitudeOwnershipTickCall(PitchHelper) ||
        !AddNativeAltitudeOwnershipEndPlay(PitchHelper) ||
        !SaveBlueprint(PitchHelper->GetOutermost(), PitchHelper, TG_PrePhysics)) return 1;

    UPackage* ReplacementPackage = CreatePackage(ChildPackage);
    UBlueprint* Replacement = FKismetEditorUtilities::CreateBlueprint(
        Parent->GeneratedClass, ReplacementPackage, FName(ChildAssetName),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        ChildBlueprintName);
    if (!Replacement || !Replacement->SimpleConstructionScript) return 1;
    USCS_Node* StabilizationNode = Replacement->SimpleConstructionScript->CreateNode(
        UChildActorComponent::StaticClass(), StabilizationHelperComponentName);
    UChildActorComponent* StabilizationTemplate =
        Cast<UChildActorComponent>(StabilizationNode->ComponentTemplate);
    USCS_Node* PitchNode = Replacement->SimpleConstructionScript->CreateNode(
        UChildActorComponent::StaticClass(), PitchHelperComponentName);
    UChildActorComponent* PitchTemplate = Cast<UChildActorComponent>(PitchNode->ComponentTemplate);
    if (!StabilizationTemplate || !PitchTemplate ||
        !StabilizationHelper->GeneratedClass->IsChildOf(AActor::StaticClass()) ||
        !PitchHelper->GeneratedClass->IsChildOf(AActor::StaticClass())) return 1;
    AActor* StabilizationDefaults =
        Cast<AActor>(StabilizationHelper->GeneratedClass->GetDefaultObject());
    AActor* PitchDefaults = Cast<AActor>(PitchHelper->GeneratedClass->GetDefaultObject());
    if (!StabilizationDefaults || !PitchDefaults) return 1;
    StabilizationTemplate->SetChildActorClass(
        TSubclassOf<AActor>(StabilizationHelper->GeneratedClass.Get()), StabilizationDefaults);
    PitchTemplate->SetChildActorClass(
        TSubclassOf<AActor>(PitchHelper->GeneratedClass.Get()), PitchDefaults);
    AActor* StabilizationChildDefaults = StabilizationTemplate->GetChildActorTemplate();
    AActor* PitchChildDefaults = PitchTemplate->GetChildActorTemplate();
    if (!StabilizationChildDefaults || !PitchChildDefaults) return 1;
    StabilizationChildDefaults->PrimaryActorTick.TickGroup = TG_PostPhysics;
    StabilizationChildDefaults->PrimaryActorTick.bCanEverTick = true;
    StabilizationChildDefaults->PrimaryActorTick.bStartWithTickEnabled = true;
    PitchChildDefaults->PrimaryActorTick.TickGroup = TG_PrePhysics;
    PitchChildDefaults->PrimaryActorTick.bCanEverTick = true;
    PitchChildDefaults->PrimaryActorTick.bStartWithTickEnabled = true;
    Replacement->SimpleConstructionScript->AddNode(StabilizationNode);
    Replacement->SimpleConstructionScript->AddNode(PitchNode);
    Replacement->SimpleConstructionScript->ValidateSceneRootNodes();
    if (!AddRearMooringSocket(
            Replacement,
            RearLeftMooringComponentName,
            MooringLayout::RearLeftSocketId,
            MooringLayout::RearLeftSocketLocation,
            MooringLayout::RearLeftSocketRotation,
            HookCable) ||
        !AddRearMooringSocket(
            Replacement,
            RearRightMooringComponentName,
            MooringLayout::RearRightSocketId,
            MooringLayout::RearRightSocketLocation,
            MooringLayout::RearRightSocketRotation,
            HookCable) ||
        !AddMooringPlate(
            Replacement, FrontPositiveMooringPlateName, PositiveMooringMesh,
            MooringMaterial, MooringLayout::FrontPositivePlateLocation) ||
        !AddMooringPlate(
            Replacement, FrontNegativeMooringPlateName, NegativeMooringMesh,
            MooringMaterial, MooringLayout::FrontNegativePlateLocation) ||
        !AddMooringPlate(
            Replacement, RearPositiveMooringPlateName, PositiveMooringMesh,
            MooringMaterial, MooringLayout::RearPositivePlateLocation) ||
        !AddMooringPlate(
            Replacement, RearNegativeMooringPlateName, NegativeMooringMesh,
            MooringMaterial, MooringLayout::RearNegativePlateLocation) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Replacement, CargoPressArmed, BoolType, CargoAction::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Replacement, CargoPressFired, BoolType, CargoAction::Disabled)) return 1;

    for (const FName TransientField : {CargoPressArmed, CargoPressFired})
    {
        FBPVariableDescription* Variable = Replacement->NewVariables.FindByPredicate(
            [&TransientField](const FBPVariableDescription& Candidate)
            {
                return Candidate.VarName == TransientField;
            });
        if (!Variable) return 1;
        Variable->PropertyFlags |= CPF_Transient;
    }

    if (!AddFourPortSettingsCache(Replacement) ||
        !AddDisabledMooringState(Replacement)) return 1;

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Replacement);
    FKismetEditorUtilities::CompileBlueprint(Replacement);
    if (Replacement->Status == BS_Error ||
        !AddDisableOwnedMooringObject(Replacement)) return 1;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Replacement);
    FKismetEditorUtilities::CompileBlueprint(Replacement);
    if (Replacement->Status == BS_Error ||
        !AddRunDisabledMooringCleanup(Replacement)) return 1;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Replacement);
    FKismetEditorUtilities::CompileBlueprint(Replacement);
    if (Replacement->Status == BS_Error ||
        !AddQueueDisabledMooringCleanup(Replacement)) return 1;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Replacement);
    FKismetEditorUtilities::CompileBlueprint(Replacement);
    if (Replacement->Status == BS_Error ||
        !AddDisabledMooringConnectionCallback(Replacement) ||
        !AddCargoControllerChangedCallback(Replacement)) return 1;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Replacement);
    FKismetEditorUtilities::CompileBlueprint(Replacement);
    if (Replacement->Status == BS_Error ||
        !AddMooringLayoutConstruction(Replacement) ||
        !AddCargoOwnershipGraph(Replacement, DropCargoAction) ||
        !AddCargoControllerChangedBinding(Replacement) ||
        !AddDisabledMooringBeginPlayBinding(Replacement) ||
        !AddProvidedActionsBpOverride(
            Replacement, ResetAction, DropCargoAction,
            PitchHelper->GeneratedClass)) return 1;

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Replacement);
    FKismetEditorUtilities::CompileBlueprint(Replacement);
    if (Replacement->Status == BS_Error ||
        !TestFourPortSettingsAndLayout(Replacement)) return 1;

    if (!SaveBlueprint(ReplacementPackage, Replacement)) return 1;
    UE_LOG(LogTemp, Display, TEXT("Generated Gyro parent and replacement child"));
    return 0;
}

#endif
