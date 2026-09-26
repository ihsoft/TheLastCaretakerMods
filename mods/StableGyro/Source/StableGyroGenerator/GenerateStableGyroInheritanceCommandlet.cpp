// HAND-WRITTEN BUILD TOOL SOURCE: generates the Gyro replacement child.
// Contracts validated for Steam build 25191271 / executable SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// The generator is editor-only and never ships in the mod container.

#include "GenerateStableGyroInheritanceCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/GameUserSettings.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SavePackage.h"
#include "StableGyroAssetNames.h"
#include "StableGyroSettings.h"
#include "VoyageGameUserSettings.h"
#include "VoyageInputAction.h"
#include "VoyageVehicleGyroCopter.h"
#include "VoyageVehiclePawn.h"

namespace
{
constexpr TCHAR BasePackage[] = TEXT("/Game/Blueprints/Vehicles/BP_GyroCopter_Possessable");
constexpr TCHAR ChildPackage[] = TEXT("/Game/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot");
constexpr TCHAR BaseAssetName[] = TEXT("BP_GyroCopter_Possessable");
constexpr TCHAR ChildAssetName[] = TEXT("BP_GyroCopter_StablePilot");
constexpr TCHAR HelperPath[] = TEXT("/Game/Mods/StableGyro/ModActor.ModActor");
const FName BaseBlueprintName(TEXT("GenerateStableGyroInheritanceBase"));
const FName ChildBlueprintName(TEXT("GenerateStableGyroInheritanceChild"));
const FName HelperComponentName(TEXT("StableGyroHelper"));
const FName ProvidedActionsFunctionName(TEXT("GetProvidedActionsBP"));
const FName CurrentThrottle(TEXT("CurrentThrottle"));
const FName AltitudeLockActive(TEXT("AltitudeLockActive"));
const FName AltitudeExactHoldActive(TEXT("AltitudeExactHoldActive"));
const FName SpaceWasHeld(TEXT("SpaceWasHeld"));
const FName AltitudeBrakingActive(TEXT("AltitudeBrakingActive"));
const FName LockedAltitude(TEXT("LockedAltitude"));
const FName RootComponent(TEXT("RootComponent"));
const FName TiltForwardInput(TEXT("TiltForwardInput"));
const FName TiltInput(TEXT("TiltInput"));
const FName PropellerVelocity(TEXT("PropellerVelocity"));
const FName PreviousCorrectedHorizontalVelocity(TEXT("PreviousCorrectedHorizontalVelocity"));
const FName HorizontalVelocitySampleValid(TEXT("HorizontalVelocitySampleValid"));
const FName PreviousCorrectedVerticalVelocity(TEXT("PreviousCorrectedVerticalVelocity"));
const FName VerticalVelocitySampleValid(TEXT("VerticalVelocitySampleValid"));
namespace Settings = StableGyroSettings;

namespace ArcadeAltitude
{
constexpr TCHAR Disabled[] = TEXT("false");
constexpr TCHAR Enabled[] = TEXT("true");
constexpr TCHAR Zero[] = TEXT("0.0");
constexpr TCHAR FullThrottle[] = TEXT("1.0");
constexpr TCHAR SpaceKey[] = TEXT("SpaceBar");
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
const FName Type(TEXT("Type"));
const FName LanguageType(TEXT("LanguageType"));
const FName TargetArray(TEXT("TargetArray"));
const FName SourceArray(TEXT("SourceArray"));
}
namespace SelectPins = BlueprintGraphNames::Pins::Select;

namespace ProvidedActionDefaults
{
constexpr TCHAR Enabled[] = TEXT("true");
constexpr TCHAR Priority[] = TEXT("10");
constexpr TCHAR Type[] = TEXT("EPlayerInputInterfaceActionType::Central");
constexpr TCHAR RussianLanguage[] = TEXT("EVoyageLanguageType::Russian");
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

bool AddEmptyProvidedActionsOverride(UBlueprint* Blueprint)
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
    if (Entry && !Result)
    {
        Result = FinishNode(NewObject<UK2Node_FunctionResult>(Graph), Graph, 400, 0);
    }
    if (!Entry || !Result)
    {
        UE_LOG(LogTemp, Error, TEXT("GetProvidedActionsBP stub entry/result nodes were not generated"));
        return false;
    }
    UEdGraphPin* EntryThen = RequirePin(Entry, UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* ResultExec = RequirePin(Result, UEdGraphSchema_K2::PN_Execute);
    UEdGraphPin* ResultValue = RequirePin(Result, PinNames::ReturnValue);
    EntryThen->BreakAllPinLinks();
    ResultExec->BreakAllPinLinks();
    ResultValue->BreakAllPinLinks();
    UK2Node_MakeArray* EmptyArray = NewObject<UK2Node_MakeArray>(Graph);
    FinishNode(EmptyArray, Graph, 200, 100);
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    return Schema->TryCreateConnection(EntryThen, ResultExec) &&
        Schema->TryCreateConnection(EmptyArray->GetOutputPin(), ResultValue);
}

bool AddProvidedActionsBpOverride(UBlueprint* Blueprint, UInputAction* ResetAction)
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
    if (!Entry || !Result) return false;
    UEdGraphPin* EntryThen = RequirePin(Entry, UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* ResultExec = RequirePin(Result, UEdGraphSchema_K2::PN_Execute);
    UEdGraphPin* ResultValue = RequirePin(Result, PinNames::ReturnValue);
    EntryThen->BreakAllPinLinks();
    ResultExec->BreakAllPinLinks();
    ResultValue->BreakAllPinLinks();
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

    UFunction* ProvidedFunction = Blueprint->ParentClass->FindFunctionByName(
        ProvidedActionsFunctionName);
    if (!ProvidedFunction || ProvidedFunction->GetOuterUClass() != Blueprint->ParentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Dummy Gyro parent does not own GetProvidedActionsBP"));
        return false;
    }
    UK2Node_CallParentFunction* Parent = NewObject<UK2Node_CallParentFunction>(Graph);
    Parent->SetFromFunction(ProvidedFunction);
    FinishNode(Parent, Graph, -900, -120);

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
        RequirePin(GetSettings, UEdGraphSchema_K2::PN_Execute));
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
    UClass* ThrottleOwner)
{
    namespace P = BlueprintGraphNames::Pins;
    namespace Binary = BlueprintGraphNames::Pins::Binary;

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !ThrottleOwner) return false;

    UK2Node_Event* TickEvent = nullptr;
    UK2Node_VariableSet* ControlledTail = nullptr;
    UK2Node_VariableSet* ClearNativeTail = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
        {
            if (EventNode->EventReference.GetMemberName() ==
                BlueprintGraphNames::Events::ActorReceiveTick)
                TickEvent = EventNode;
        }
        if (UK2Node_VariableSet* SetNode = Cast<UK2Node_VariableSet>(Node))
        {
            if (SetNode->VariableReference.GetMemberName() == TiltForwardInput)
            {
                if (SetNode->NodePosY < 0)
                    ClearNativeTail = SetNode;
                else
                    ControlledTail = SetNode;
            }
        }
    }
    if (!TickEvent || !ControlledTail || !ClearNativeTail)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find helper Tick insertion points"));
        return false;
    }

    UK2Node_CallFunction* GetParent = AddCall(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor), 7350, 1500);
    UK2Node_DynamicCast* Gyro = NewObject<UK2Node_DynamicCast>(Graph);
    Gyro->TargetType = ThrottleOwner;
    Gyro->SetPurity(false);
    FinishNode(Gyro, Graph, 7580, 680);

    UK2Node_VariableGet* MeasuredRotorVelocity = AddRead(
        Graph, PropellerVelocity, ThrottleOwner, 7810, 420);
    UK2Node_VariableGet* MinimumRotorVelocity = AddRead(
        Graph, Settings::StabilizationMinimumPropellerVelocity, nullptr, 8040, 540);
    UK2Node_CallFunction* RotorIsFastEnough = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 8270, 420);
    UK2Node_IfThenElse* RotorGateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 8500, 420);

    UK2Node_VariableGet* Throttle = AddRead(
        Graph, CurrentThrottle, ThrottleOwner, 7580, 940);
    UK2Node_CallFunction* AtFullThrottle = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), 910, 940);
    SetDefault(AtFullThrottle, Binary::RightOperand, ArcadeAltitude::FullThrottle);
    UK2Node_CallFunction* Controller = AddCall(Graph, UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController), 680, 1100);
    SetDefault(Controller, P::PlayerIndex, TEXT("0"));
    UK2Node_CallFunction* SpaceHeld = AddCall(Graph, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, IsInputKeyDown), 910, 1100);
    SetDefault(SpaceHeld, P::Key, ArcadeAltitude::SpaceKey);
    UK2Node_IfThenElse* EligibilityBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1600, 700);

    UK2Node_VariableSet* ClearWhenUncontrolled = AddWrite(
        Graph, AltitudeLockActive, 1600, -220);
    SetDefault(ClearWhenUncontrolled, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenUncontrolled = AddWrite(
        Graph, AltitudeExactHoldActive, 1830, -220);
    SetDefault(ClearExactWhenUncontrolled, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenUncontrolled = AddWrite(
        Graph, SpaceWasHeld, 2060, -220);
    SetDefault(ClearSpaceWhenUncontrolled, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingWhenUncontrolled = AddWrite(
        Graph, AltitudeBrakingActive, 2290, -220);
    SetDefault(StopBrakingWhenUncontrolled, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetHorizontalSampleWhenUncontrolled = AddWrite(
        Graph, HorizontalVelocitySampleValid, 2520, -220);
    SetDefault(ResetHorizontalSampleWhenUncontrolled,
        HorizontalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenUncontrolled = AddWrite(
        Graph, VerticalVelocitySampleValid, 2750, -220);
    SetDefault(ResetVerticalSampleWhenUncontrolled,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearWhenIneligible = AddWrite(
        Graph, AltitudeLockActive, 1830, 580);
    SetDefault(ClearWhenIneligible, AltitudeLockActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearExactWhenIneligible = AddWrite(
        Graph, AltitudeExactHoldActive, 2060, 580);
    SetDefault(ClearExactWhenIneligible, AltitudeExactHoldActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ClearSpaceWhenIneligible = AddWrite(
        Graph, SpaceWasHeld, 2290, 580);
    SetDefault(ClearSpaceWhenIneligible, SpaceWasHeld, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StopBrakingWhenIneligible = AddWrite(
        Graph, AltitudeBrakingActive, 2520, 580);
    SetDefault(StopBrakingWhenIneligible, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenIneligible = AddWrite(
        Graph, VerticalVelocitySampleValid, 2750, 580);
    SetDefault(ResetVerticalSampleWhenIneligible,
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
    UK2Node_VariableSet* StopBrakingWhenRotorSlow = AddWrite(
        Graph, AltitudeBrakingActive, 9420, 180);
    SetDefault(StopBrakingWhenRotorSlow,
        AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetHorizontalSampleWhenRotorSlow = AddWrite(
        Graph, HorizontalVelocitySampleValid, 9650, 180);
    SetDefault(ResetHorizontalSampleWhenRotorSlow,
        HorizontalVelocitySampleValid, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleWhenRotorSlow = AddWrite(
        Graph, VerticalVelocitySampleValid, 9880, 180);
    SetDefault(ResetVerticalSampleWhenRotorSlow,
        VerticalVelocitySampleValid, ArcadeAltitude::Disabled);

    UK2Node_VariableGet* IsLockActive = AddRead(
        Graph, AltitudeLockActive, nullptr, 1830, 940);
    UK2Node_IfThenElse* LockStateBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2060, 700);
    UK2Node_IfThenElse* SpaceModeBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2290, 700);
    UK2Node_IfThenElse* InactiveSpaceBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 2060, 1740);
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
    UK2Node_VariableSet* StopBrakingForSpace = AddWrite(
        Graph, AltitudeBrakingActive, 2750, 820);
    SetDefault(StopBrakingForSpace, AltitudeBrakingActive, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* ResetVerticalSampleForSpace = AddWrite(
        Graph, VerticalVelocitySampleValid, 2980, 820);
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
    UK2Node_VariableSet* CaptureExactAltitude = AddWrite(
        Graph, LockedAltitude, 3900, 1740);
    UK2Node_VariableSet* EnableExactHold = AddWrite(
        Graph, AltitudeExactHoldActive, 4130, 1740);
    SetDefault(EnableExactHold, AltitudeExactHoldActive, ArcadeAltitude::Enabled);
    UK2Node_VariableSet* StopBrakingAfterExactActivation = AddWrite(
        Graph, AltitudeBrakingActive, 4360, 1740);
    SetDefault(StopBrakingAfterExactActivation, AltitudeBrakingActive, ArcadeAltitude::Disabled);
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
    UK2Node_CallFunction* BrakingStep = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble), 5050, 2280);
    UK2Node_VariableGet* PreviousVerticalVelocity = AddRead(
        Graph, PreviousCorrectedVerticalVelocity, nullptr, 4820, 2460);
    UK2Node_VariableGet* VerticalSampleValid = AddRead(
        Graph, VerticalVelocitySampleValid, nullptr, 4820, 2580);
    UK2Node_CallFunction* ObservedOrPreviousMinimum = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin), 5050, 2460);
    UK2Node_CallFunction* VerticalVelocityBeforeBraking = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 5280, 2460);
    UK2Node_CallFunction* ReducedVerticalVelocity = AddCall(
        Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble), 5510, 2160);
    UK2Node_CallFunction* IsStillAscending = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), 5740, 1980);
    SetDefault(IsStillAscending, Binary::RightOperand, ArcadeAltitude::Zero);
    UK2Node_IfThenElse* RemainingAscentBranch = FinishNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 5970, 1980);
    UK2Node_CallFunction* ReducedVelocity = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 5970, 2220);
    UK2Node_CallFunction* SetReducedVelocity = AddCall(
        Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetPhysicsLinearVelocity), 6200, 1980);
    SetDefault(SetReducedVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    SetDefault(SetReducedVelocity, ArcadeAltitude::AddToCurrent, ArcadeAltitude::Disabled);
    UK2Node_VariableSet* StoreCorrectedVerticalVelocity = AddWrite(
        Graph, PreviousCorrectedVerticalVelocity, 6430, 1980);
    UK2Node_VariableSet* MarkVerticalSampleValid = AddWrite(
        Graph, VerticalVelocitySampleValid, 6660, 1980);
    SetDefault(MarkVerticalSampleValid,
        VerticalVelocitySampleValid, ArcadeAltitude::Enabled);

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
    SetDefault(SetLocation, ArcadeAltitude::Sweep, TEXT("false"));
    SetDefault(SetLocation, ArcadeAltitude::Teleport, TEXT("false"));

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
    UK2Node_CallFunction* HorizontalVelocity = AddCall(Graph, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector), 3670, 1160);
    SetDefault(HorizontalVelocity, ArcadeAltitude::VectorZ, ArcadeAltitude::Zero);
    UK2Node_CallFunction* SetVelocity = AddCall(Graph, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetPhysicsLinearVelocity), 3900, 700);
    SetDefault(SetVelocity, ArcadeAltitude::BoneName, ArcadeAltitude::NoBone);
    SetDefault(SetVelocity, ArcadeAltitude::AddToCurrent, TEXT("false"));

    bool Ok = true;
    Ok &= Connect(RequirePin(ControlledTail, P::Then), RequirePin(Gyro, P::Execute));
    Ok &= Connect(RequirePin(GetParent, P::ReturnValue), Gyro->GetCastSourcePin());
    Ok &= Connect(Gyro->GetValidCastPin(), RequirePin(RotorGateBranch, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(),
        RequirePin(MeasuredRotorVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(MeasuredRotorVelocity, PropellerVelocity),
        RequirePin(RotorIsFastEnough, Binary::LeftOperand));
    Ok &= Connect(RequirePin(MinimumRotorVelocity,
        Settings::StabilizationMinimumPropellerVelocity),
        RequirePin(RotorIsFastEnough, Binary::RightOperand));
    Ok &= Connect(RequirePin(RotorIsFastEnough, P::ReturnValue),
        RequirePin(RotorGateBranch, P::Condition));
    Ok &= AddHorizontalVelocityDamping(
        Graph, TickEvent, Gyro, RequirePin(RotorGateBranch, P::Then),
        EligibilityBranch, ThrottleOwner);
    Ok &= Connect(RequirePin(RotorGateBranch, P::Else),
        RequirePin(ClearWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenRotorSlow, P::Then),
        RequirePin(ClearExactWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenRotorSlow, P::Then),
        RequirePin(ClearSpaceWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenRotorSlow, P::Then),
        RequirePin(StopBrakingWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenRotorSlow, P::Then),
        RequirePin(ResetHorizontalSampleWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ResetHorizontalSampleWhenRotorSlow, P::Then),
        RequirePin(ResetVerticalSampleWhenRotorSlow, P::Execute));
    Ok &= Connect(RequirePin(ClearNativeTail, P::Then), RequirePin(ClearWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenUncontrolled, P::Then), RequirePin(ClearExactWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenUncontrolled, P::Then), RequirePin(ClearSpaceWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenUncontrolled, P::Then), RequirePin(StopBrakingWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenUncontrolled, P::Then),
        RequirePin(ResetHorizontalSampleWhenUncontrolled, P::Execute));
    Ok &= Connect(RequirePin(ResetHorizontalSampleWhenUncontrolled, P::Then),
        RequirePin(ResetVerticalSampleWhenUncontrolled, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Throttle, P::FunctionTarget));
    Ok &= Connect(RequirePin(Throttle, CurrentThrottle), RequirePin(AtFullThrottle, Binary::LeftOperand));
    Ok &= Connect(RequirePin(Controller, P::ReturnValue), RequirePin(SpaceHeld, P::FunctionTarget));
    Ok &= Connect(RequirePin(AtFullThrottle, P::ReturnValue), RequirePin(EligibilityBranch, P::Condition));
    Ok &= Connect(RequirePin(EligibilityBranch, P::Else), RequirePin(ClearWhenIneligible, P::Execute));
    Ok &= Connect(RequirePin(ClearWhenIneligible, P::Then), RequirePin(ClearExactWhenIneligible, P::Execute));
    Ok &= Connect(RequirePin(ClearExactWhenIneligible, P::Then), RequirePin(ClearSpaceWhenIneligible, P::Execute));
    Ok &= Connect(RequirePin(ClearSpaceWhenIneligible, P::Then), RequirePin(StopBrakingWhenIneligible, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingWhenIneligible, P::Then),
        RequirePin(ResetVerticalSampleWhenIneligible, P::Execute));
    Ok &= Connect(RequirePin(EligibilityBranch, P::Then), RequirePin(LockStateBranch, P::Execute));
    Ok &= Connect(RequirePin(IsLockActive, AltitudeLockActive), RequirePin(LockStateBranch, P::Condition));
    Ok &= Connect(RequirePin(LockStateBranch, P::Else), RequirePin(InactiveSpaceBranch, P::Execute));
    Ok &= Connect(RequirePin(SpaceHeld, P::ReturnValue), RequirePin(InactiveSpaceBranch, P::Condition));
    Ok &= Connect(RequirePin(InactiveSpaceBranch, P::Then), RequirePin(InitializeLockedAltitude, P::Execute));
    Ok &= Connect(RequirePin(LockStateBranch, P::Then), RequirePin(SpaceModeBranch, P::Execute));
    Ok &= Connect(RequirePin(SpaceHeld, P::ReturnValue), RequirePin(SpaceModeBranch, P::Condition));
    Ok &= Connect(RequirePin(SpaceModeBranch, P::Then), RequirePin(MarkSpaceHeld, P::Execute));
    Ok &= Connect(RequirePin(MarkSpaceHeld, P::Then), RequirePin(ClearExactForSpace, P::Execute));
    Ok &= Connect(RequirePin(ClearExactForSpace, P::Then), RequirePin(StopBrakingForSpace, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingForSpace, P::Then),
        RequirePin(ResetVerticalSampleForSpace, P::Execute));
    Ok &= Connect(RequirePin(ResetVerticalSampleForSpace, P::Then),
        RequirePin(RisingBranch, P::Execute));
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
    Ok &= Connect(RequirePin(StopBrakingAfterLock, P::Then), RequirePin(SpaceModeBranch, P::Execute));
    Ok &= Connect(RequirePin(BreakCaptureLocation, ArcadeAltitude::VectorZ), RequirePin(CaptureExactAltitude, LockedAltitude));
    Ok &= Connect(RequirePin(CaptureExactAltitude, P::Then), RequirePin(EnableExactHold, P::Execute));
    Ok &= Connect(RequirePin(EnableExactHold, P::Then), RequirePin(StopBrakingAfterExactActivation, P::Execute));
    Ok &= Connect(RequirePin(StopBrakingAfterExactActivation, P::Then), RequirePin(SetLocation, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(BrakingRoot, P::FunctionTarget));
    Ok &= Connect(RequirePin(BrakingRoot, RootComponent), BrakingPrimitiveRoot->GetCastSourcePin());
    Ok &= Connect(BrakingPrimitiveRoot->GetValidCastPin(), RequirePin(BrakingLinearVelocity, P::Execute));
    Ok &= Connect(BrakingPrimitiveRoot->GetCastResultPin(), RequirePin(BrakingLinearVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(BrakingLinearVelocity, P::ReturnValue), RequirePin(BreakBrakingVelocity, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BrakingLinearVelocity, P::Then), RequirePin(RemainingAscentBranch, P::Execute));
    Ok &= Connect(RequirePin(VerticalDeceleration, Settings::AltitudeStabilizationVerticalDeceleration), RequirePin(BrakingStep, Binary::LeftOperand));
    Ok &= Connect(RequirePin(TickEvent, P::DeltaSeconds), RequirePin(BrakingStep, Binary::RightOperand));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(ObservedOrPreviousMinimum, Binary::LeftOperand));
    Ok &= Connect(RequirePin(PreviousVerticalVelocity, PreviousCorrectedVerticalVelocity),
        RequirePin(ObservedOrPreviousMinimum, Binary::RightOperand));
    Ok &= Connect(RequirePin(ObservedOrPreviousMinimum, P::ReturnValue),
        RequirePin(VerticalVelocityBeforeBraking, SelectPins::WhenTrue));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorZ),
        RequirePin(VerticalVelocityBeforeBraking, SelectPins::WhenFalse));
    Ok &= Connect(RequirePin(VerticalSampleValid, VerticalVelocitySampleValid),
        RequirePin(VerticalVelocityBeforeBraking, SelectPins::Condition));
    Ok &= Connect(RequirePin(VerticalVelocityBeforeBraking, P::ReturnValue),
        RequirePin(ReducedVerticalVelocity, Binary::LeftOperand));
    Ok &= Connect(RequirePin(BrakingStep, P::ReturnValue), RequirePin(ReducedVerticalVelocity, Binary::RightOperand));
    Ok &= Connect(RequirePin(ReducedVerticalVelocity, P::ReturnValue), RequirePin(IsStillAscending, Binary::LeftOperand));
    Ok &= Connect(RequirePin(IsStillAscending, P::ReturnValue), RequirePin(RemainingAscentBranch, P::Condition));
    Ok &= Connect(RequirePin(RemainingAscentBranch, P::Else),
        RequirePin(ResetVerticalSampleForExact, P::Execute));
    Ok &= Connect(RequirePin(ResetVerticalSampleForExact, P::Then),
        RequirePin(CaptureExactAltitude, P::Execute));
    Ok &= Connect(RequirePin(RemainingAscentBranch, P::Then), RequirePin(SetReducedVelocity, P::Execute));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorX), RequirePin(ReducedVelocity, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(BreakBrakingVelocity, ArcadeAltitude::VectorY), RequirePin(ReducedVelocity, ArcadeAltitude::VectorY));
    Ok &= Connect(RequirePin(ReducedVerticalVelocity, P::ReturnValue), RequirePin(ReducedVelocity, ArcadeAltitude::VectorZ));
    Ok &= Connect(BrakingPrimitiveRoot->GetCastResultPin(), RequirePin(SetReducedVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(ReducedVelocity, P::ReturnValue), RequirePin(SetReducedVelocity, ArcadeAltitude::NewVelocity));
    Ok &= Connect(RequirePin(SetReducedVelocity, P::Then),
        RequirePin(StoreCorrectedVerticalVelocity, P::Execute));
    Ok &= Connect(RequirePin(ReducedVerticalVelocity, P::ReturnValue),
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
    Ok &= Connect(RequirePin(RisingBranch, P::Else), RequirePin(SetLocation, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(SetLocation, P::FunctionTarget));
    Ok &= Connect(RequirePin(LockedLocation, P::ReturnValue), RequirePin(SetLocation, ArcadeAltitude::NewLocation));
    Ok &= Connect(RequirePin(SetLocation, P::Then), RequirePin(PrimitiveRoot, P::Execute));
    Ok &= Connect(Gyro->GetCastResultPin(), RequirePin(Root, P::FunctionTarget));
    Ok &= Connect(RequirePin(Root, RootComponent), PrimitiveRoot->GetCastSourcePin());
    Ok &= Connect(PrimitiveRoot->GetCastResultPin(), RequirePin(LinearVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(LinearVelocity, P::ReturnValue), RequirePin(BreakVelocity, ArcadeAltitude::InVector));
    Ok &= Connect(RequirePin(BreakVelocity, ArcadeAltitude::VectorX), RequirePin(HorizontalVelocity, ArcadeAltitude::VectorX));
    Ok &= Connect(RequirePin(BreakVelocity, ArcadeAltitude::VectorY), RequirePin(HorizontalVelocity, ArcadeAltitude::VectorY));
    Ok &= Connect(PrimitiveRoot->GetValidCastPin(), RequirePin(LinearVelocity, P::Execute));
    Ok &= Connect(RequirePin(LinearVelocity, P::Then), RequirePin(SetVelocity, P::Execute));
    Ok &= Connect(PrimitiveRoot->GetCastResultPin(), RequirePin(SetVelocity, P::FunctionTarget));
    Ok &= Connect(RequirePin(HorizontalVelocity, P::ReturnValue), RequirePin(SetVelocity, ArcadeAltitude::NewVelocity));
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
    if (FPackageName::DoesPackageExist(BasePackage) ||
        FPackageName::DoesPackageExist(ChildPackage))
    {
        UE_LOG(LogTemp, Error, TEXT("Gyro inheritance assets already exist"));
        return 1;
    }
    UBlueprint* Helper = LoadObject<UBlueprint>(nullptr, HelperPath);
    if (!Helper || !Helper->GeneratedClass) return 1;
    UVoyageInputAction* ResetAction = LoadObject<UVoyageInputAction>(
        nullptr, StableGyroAssetNames::ResetActionObjectPath);
    if (!ResetAction) return 1;

    UPackage* ParentPackage = CreatePackage(BasePackage);
    UBlueprint* Parent = FKismetEditorUtilities::CreateBlueprint(
        AVoyageVehicleGyroCopter::StaticClass(), ParentPackage, FName(BaseAssetName),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        BaseBlueprintName);
    if (!Parent || !Parent->SimpleConstructionScript) return 1;
    FEdGraphPinType DoubleType;
    DoubleType.PinCategory = UEdGraphSchema_K2::PC_Real;
    DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Parent, CurrentThrottle, DoubleType, ArcadeAltitude::Zero)) return 1;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Parent, PropellerVelocity, DoubleType, ArcadeAltitude::Zero)) return 1;
    if (!AddEmptyProvidedActionsOverride(Parent)) return 1;
    if (!SaveBlueprint(ParentPackage, Parent)) return 1;

    FEdGraphPinType BoolType;
    BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    FEdGraphPinType VectorType;
    VectorType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    VectorType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Helper, AltitudeLockActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, AltitudeExactHoldActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, SpaceWasHeld, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, AltitudeBrakingActive, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, LockedAltitude, DoubleType, ArcadeAltitude::Zero) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, PreviousCorrectedHorizontalVelocity,
            VectorType, ArcadeHorizontal::ZeroVectorDefault) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, HorizontalVelocitySampleValid, BoolType, ArcadeAltitude::Disabled) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, PreviousCorrectedVerticalVelocity,
            DoubleType, ArcadeAltitude::Zero) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Helper, VerticalVelocitySampleValid,
            BoolType, ArcadeAltitude::Disabled) ||
        !AddArcadeAltitudeHold(Helper, Parent->GeneratedClass) ||
        !SaveBlueprint(Helper->GetOutermost(), Helper, TG_PostPhysics)) return 1;

    UPackage* ReplacementPackage = CreatePackage(ChildPackage);
    UBlueprint* Replacement = FKismetEditorUtilities::CreateBlueprint(
        Parent->GeneratedClass, ReplacementPackage, FName(ChildAssetName),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        ChildBlueprintName);
    if (!Replacement || !Replacement->SimpleConstructionScript) return 1;
    USCS_Node* HelperNode = Replacement->SimpleConstructionScript->CreateNode(
        UChildActorComponent::StaticClass(), HelperComponentName);
    UChildActorComponent* Template = Cast<UChildActorComponent>(HelperNode->ComponentTemplate);
    if (!Template || !Helper->GeneratedClass->IsChildOf(AActor::StaticClass())) return 1;
    Template->SetChildActorClass(TSubclassOf<AActor>(Helper->GeneratedClass.Get()));
    Replacement->SimpleConstructionScript->AddNode(HelperNode);
    Replacement->SimpleConstructionScript->ValidateSceneRootNodes();

    if (!AddProvidedActionsBpOverride(Replacement, ResetAction)) return 1;

    if (!SaveBlueprint(ReplacementPackage, Replacement)) return 1;
    UE_LOG(LogTemp, Display, TEXT("Generated Gyro parent and replacement child"));
    return 0;
}

#endif
