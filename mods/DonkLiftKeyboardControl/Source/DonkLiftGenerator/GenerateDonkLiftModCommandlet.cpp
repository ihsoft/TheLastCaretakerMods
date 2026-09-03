// HAND-WRITTEN BUILD TOOL SOURCE: generates helper/input Blueprint assets from
// version-bound Voyage contracts. The C++ tool itself is never shipped.
// Contracts validated against Steam build 25056839, exe SHA-256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Revalidate per ../../GAME_DERIVED_SOURCES.md after a game update.

#include "GenerateDonkLiftModCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "DonkLiftAssetNames.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "InputModifiers.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Containers/UnrealString.h"
#include "Misc/PackageName.h"
#include "PlayerMappableKeySettings.h"
#include "UObject/SavePackage.h"
#include "VoyageInputAction.h"
#include "VoyageVehicleForkliftPawn.h"

namespace
{
constexpr TCHAR PackageName[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/ModActor");
constexpr TCHAR AssetName[] = TEXT("ModActor");
constexpr TCHAR ForkliftContextPackageName[] = TEXT("/Game/Game/Input/Vehicle/IMC_Forklift_Keyboard");
constexpr TCHAR ForkliftContextAssetName[] = TEXT("IMC_Forklift_Keyboard");
constexpr TCHAR ForkliftContextDescription[] = TEXT("Vehicle keyboard");
constexpr TCHAR GeneratorBlueprintName[] = TEXT("GenerateDonkLiftMod");

struct InputActionContract
{
    const TCHAR* PackageName;
    const TCHAR* AssetName;
    bool IsVoyageAction;
    EInputActionValueType ValueType;
};

namespace ForkliftInputAssets
{
constexpr InputActionContract Forward{
    TEXT("/Game/Game/Input/Vehicle/IA_VehicleForward"), TEXT("IA_VehicleForward"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Backward{
    TEXT("/Game/Game/Input/Vehicle/IA_VehicleBackward"), TEXT("IA_VehicleBackward"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Right{
    TEXT("/Game/Game/Input/Vehicle/IA_VehicleRight"), TEXT("IA_VehicleRight"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Left{
    TEXT("/Game/Game/Input/Vehicle/IA_VehicleLeft"), TEXT("IA_VehicleLeft"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract LookUp{
    TEXT("/Game/Game/Input/Character/IA_LookUp"), TEXT("IA_LookUp"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract LookRight{
    TEXT("/Game/Game/Input/Character/IA_LookRight"), TEXT("IA_LookRight"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Zoom{
    TEXT("/Game/Game/Input/Character/IA_Zoom"), TEXT("IA_Zoom"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Exit{
    TEXT("/Game/Game/Input/Vehicle/IAV_VehicleExit"), TEXT("IAV_VehicleExit"), true, EInputActionValueType::Boolean};
constexpr InputActionContract SwitchCamera{
    TEXT("/Game/Game/Input/Vehicle/IAV_VehicleSwitchCamera"), TEXT("IAV_VehicleSwitchCamera"), true, EInputActionValueType::Boolean};
constexpr InputActionContract ForkUp{
    TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkUp"), TEXT("IAV_Forklift_ForkUp"), true, EInputActionValueType::Axis1D};
constexpr InputActionContract ForkDown{
    TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkDown"), TEXT("IAV_Forklift_ForkDown"), true, EInputActionValueType::Axis1D};
constexpr InputActionContract Horn{
    TEXT("/Game/Game/Input/Vehicle/IAV_VehicleHorn"), TEXT("IAV_VehicleHorn"), true, EInputActionValueType::Boolean};
constexpr InputActionContract Handbrake{
    TEXT("/Game/Game/Input/Vehicle/IAV_VehicleHandbrake"), TEXT("IAV_VehicleHandbrake"), true, EInputActionValueType::Boolean};
constexpr InputActionContract ForkTiltUp{
    TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkTiltUp"), TEXT("IAV_Forklift_ForkTiltUp"), true, EInputActionValueType::Axis1D};
constexpr InputActionContract ForkTiltDown{
    TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkTiltDown"), TEXT("IAV_Forklift_ForkTiltDown"), true, EInputActionValueType::Axis1D};
}
namespace PinNames = BlueprintGraphNames::Pins;
namespace BinaryPins = BlueprintGraphNames::Pins::Binary;
namespace SelectPins = BlueprintGraphNames::Pins::Select;

namespace VariableNames
{
const FName IntegratedThrottle(TEXT("IntegratedThrottle"));
const FName IntegratedSteering(TEXT("IntegratedSteering"));
const FName SteeringVelocity(TEXT("SteeringVelocity"));
const FName ThrottleInput(TEXT("ThrottleInput"));
const FName SteeringInput(TEXT("SteeringInput"));
}

namespace InputDefaults
{
constexpr double Neutral = 0.0;
constexpr double DigitalPositive = 1.0;
constexpr double DigitalNegative = -1.0;
constexpr double ThrottleRate = 1.0 / 3.0;
constexpr double SteeringMaximumSpeed = 1.20;
constexpr double SteeringAcceleration = 2.50;
constexpr double SteeringReversalBraking = 5.00;
// The game's exact -1/+1 values are commands, so our integrated state stops short.
constexpr double IntegratedInputMinimum = -0.9999;
constexpr double IntegratedInputMaximum = 0.9999;
constexpr int32 PlayerIndex = 0;
constexpr TCHAR BrakeKey[] = TEXT("X");
constexpr TCHAR CenterSteeringKey[] = TEXT("C");
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

UK2Node_CallFunction* AddCall(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
{
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Function);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* AddVariableGet(
    UEdGraph* Graph,
    const FName VariableName,
    UClass* ExternalClass,
    int32 X,
    int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    if (ExternalClass)
    {
        Node->VariableReference.SetExternalMember(VariableName, ExternalClass);
    }
    else
    {
        Node->VariableReference.SetSelfMember(VariableName);
    }
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* AddVariableSet(
    UEdGraph* Graph,
    const FName VariableName,
    UClass* ExternalClass,
    int32 X,
    int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    if (ExternalClass)
    {
        Node->VariableReference.SetExternalMember(VariableName, ExternalClass);
    }
    else
    {
        Node->VariableReference.SetSelfMember(VariableName);
    }
    return FinishNode(Node, Graph, X, Y);
}

UEdGraphPin* RequirePin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Pin = Node->FindPin(Name);
    checkf(Pin, TEXT("Missing pin '%s' on node '%s'"), *Name.ToString(), *Node->GetName());
    return Pin;
}

void SetDefault(UEdGraphNode* Node, const FName Name, const TCHAR* Value)
{
    RequirePin(Node, Name)->DefaultValue = Value;
}

void SetDefault(UEdGraphNode* Node, const FName Name, const double Value)
{
    RequirePin(Node, Name)->DefaultValue = LexToString(Value);
}

void SetDefault(UEdGraphNode* Node, const FName Name, const int32 Value)
{
    RequirePin(Node, Name)->DefaultValue = LexToString(Value);
}

bool Connect(const UEdGraphSchema_K2* Schema, UEdGraphPin* A, UEdGraphPin* B)
{
    if (!Schema->TryCreateConnection(A, B))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not connect %s.%s to %s.%s"),
            *A->GetOwningNode()->GetName(), *A->PinName.ToString(),
            *B->GetOwningNode()->GetName(), *B->PinName.ToString());
        return false;
    }
    return true;
}

UFunction* MathFunction(const FName Name)
{
    return UKismetMathLibrary::StaticClass()->FindFunctionByName(Name);
}

bool SaveAssetPackage(UPackage* Package, UObject* Asset, const TCHAR* LongPackageName)
{
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        LongPackageName,
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
}

UVoyageInputAction* CreateVoyageInputAction(
    const TCHAR* LongPackageName,
    const TCHAR* InputActionAssetName,
    const TCHAR* MappingName,
    const TCHAR* Label)
{
    UPackage* Package = CreatePackage(LongPackageName);
    UVoyageInputAction* Action = NewObject<UVoyageInputAction>(
        Package,
        FName(InputActionAssetName),
        RF_Public | RF_Standalone | RF_Transactional);
    if (!Action)
    {
        return nullptr;
    }

    const FText LabelText = FText::FromString(Label);
    Action->Description = LabelText;
    Action->ActionDescription = LabelText;
    Action->ValueType = EInputActionValueType::Boolean;

    UPlayerMappableKeySettings* Settings = NewObject<UPlayerMappableKeySettings>(
        Action,
        NAME_None,
        RF_Public | RF_Transactional);
    Settings->Name = FName(MappingName);
    Settings->DisplayName = LabelText;
    Settings->DisplayCategory = FText::FromString(DonkLiftAssetNames::VehicleInputCategory);
    Action->SetPlayerMappableKeySettings(Settings);

    if (!SaveAssetPackage(Package, Action, LongPackageName))
    {
        return nullptr;
    }
    return Action;
}

UInputAction* CreateInputActionPlaceholder(const InputActionContract& Contract)
{
    UPackage* Package = CreatePackage(Contract.PackageName);
    UInputAction* Action = Contract.IsVoyageAction
        ? static_cast<UInputAction*>(NewObject<UVoyageInputAction>(
            Package,
            FName(Contract.AssetName),
            RF_Public | RF_Standalone | RF_Transactional))
        : NewObject<UInputAction>(
            Package,
            FName(Contract.AssetName),
            RF_Public | RF_Standalone | RF_Transactional);
    if (!Action)
    {
        return nullptr;
    }

    Action->ValueType = Contract.ValueType;
    if (!SaveAssetPackage(Package, Action, Contract.PackageName))
    {
        return nullptr;
    }
    return Action;
}

void AddContextMapping(
    UInputMappingContext* Context,
    UInputAction* Action,
    const FKey Key,
    const bool bNegate = false,
    const bool bPressed = false)
{
    FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
    if (bNegate)
    {
        Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(
            Context, NAME_None, RF_Public | RF_Transactional));
    }
    if (bPressed)
    {
        Mapping.Triggers.Add(NewObject<UInputTriggerPressed>(
            Context, NAME_None, RF_Public | RF_Transactional));
    }
}

UInputMappingContext* CreateForkliftKeyboardContext(
    UVoyageInputAction* BrakeAction,
    UVoyageInputAction* CenterAction)
{
    UInputAction* Forward = CreateInputActionPlaceholder(ForkliftInputAssets::Forward);
    UInputAction* Backward = CreateInputActionPlaceholder(ForkliftInputAssets::Backward);
    UInputAction* Right = CreateInputActionPlaceholder(ForkliftInputAssets::Right);
    UInputAction* Left = CreateInputActionPlaceholder(ForkliftInputAssets::Left);
    UInputAction* LookUp = CreateInputActionPlaceholder(ForkliftInputAssets::LookUp);
    UInputAction* LookRight = CreateInputActionPlaceholder(ForkliftInputAssets::LookRight);
    UInputAction* Zoom = CreateInputActionPlaceholder(ForkliftInputAssets::Zoom);
    UInputAction* Exit = CreateInputActionPlaceholder(ForkliftInputAssets::Exit);
    UInputAction* SwitchCamera = CreateInputActionPlaceholder(ForkliftInputAssets::SwitchCamera);
    UInputAction* ForkUp = CreateInputActionPlaceholder(ForkliftInputAssets::ForkUp);
    UInputAction* ForkDown = CreateInputActionPlaceholder(ForkliftInputAssets::ForkDown);
    UInputAction* Horn = CreateInputActionPlaceholder(ForkliftInputAssets::Horn);
    UInputAction* Handbrake = CreateInputActionPlaceholder(ForkliftInputAssets::Handbrake);
    UInputAction* ForkTiltUp = CreateInputActionPlaceholder(ForkliftInputAssets::ForkTiltUp);
    UInputAction* ForkTiltDown = CreateInputActionPlaceholder(ForkliftInputAssets::ForkTiltDown);

    if (!Forward || !Backward || !Right || !Left || !LookUp || !LookRight || !Zoom ||
        !Exit || !SwitchCamera || !ForkUp || !ForkDown || !Horn || !Handbrake ||
        !ForkTiltUp || !ForkTiltDown)
    {
        return nullptr;
    }

    UPackage* Package = CreatePackage(ForkliftContextPackageName);
    UInputMappingContext* Context = NewObject<UInputMappingContext>(
        Package,
        FName(ForkliftContextAssetName),
        RF_Public | RF_Standalone | RF_Transactional);
    if (!Context)
    {
        return nullptr;
    }

    AddContextMapping(Context, Forward, EKeys::W);
    AddContextMapping(Context, Backward, EKeys::S);
    AddContextMapping(Context, Right, EKeys::D);
    AddContextMapping(Context, Left, EKeys::A);
    AddContextMapping(Context, LookUp, EKeys::MouseY, true);
    AddContextMapping(Context, LookRight, EKeys::MouseX);
    AddContextMapping(Context, Zoom, EKeys::MouseWheelAxis);
    AddContextMapping(Context, Exit, EKeys::E, false, true);
    AddContextMapping(Context, SwitchCamera, EKeys::T, false, true);
    AddContextMapping(Context, ForkUp, EKeys::LeftMouseButton);
    AddContextMapping(Context, ForkDown, EKeys::RightMouseButton);
    AddContextMapping(Context, Horn, EKeys::H);
    AddContextMapping(Context, Handbrake, EKeys::SpaceBar);
    AddContextMapping(Context, ForkTiltUp, EKeys::R);
    AddContextMapping(Context, ForkTiltDown, EKeys::F);
    AddContextMapping(Context, BrakeAction, EKeys::X);
    AddContextMapping(Context, CenterAction, EKeys::C);
    Context->ContextDescription = FText::FromString(ForkliftContextDescription);

    if (!SaveAssetPackage(Package, Context, ForkliftContextPackageName))
    {
        return nullptr;
    }
    return Context;
}
}

UGenerateDonkLiftModCommandlet::UGenerateDonkLiftModCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateDonkLiftModCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(PackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Asset already exists: %s"), PackageName);
        return 1;
    }

    UVoyageInputAction* BrakeAction = CreateVoyageInputAction(
        DonkLiftAssetNames::BrakeActionPackageName,
        DonkLiftAssetNames::BrakeActionAssetName,
        DonkLiftAssetNames::BrakeMappingName,
        DonkLiftAssetNames::BrakeDisplayLabel);
    UVoyageInputAction* CenterAction = CreateVoyageInputAction(
        DonkLiftAssetNames::CenterActionPackageName,
        DonkLiftAssetNames::CenterActionAssetName,
        DonkLiftAssetNames::CenterMappingName,
        DonkLiftAssetNames::CenterDisplayLabel);
    UInputMappingContext* ForkliftContext = CreateForkliftKeyboardContext(BrakeAction, CenterAction);
    if (!BrakeAction || !CenterAction || !ForkliftContext)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate native input-action assets"));
        return 1;
    }

    UPackage* Package = CreatePackage(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(),
        Package,
        FName(AssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        FName(GeneratorBlueprintName));

    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create %s"), PackageName);
        return 1;
    }

    FEdGraphPinType DoubleType;
    DoubleType.PinCategory = UEdGraphSchema_K2::PC_Real;
    DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            VariableNames::IntegratedThrottle,
            DoubleType,
            LexToString(InputDefaults::Neutral)))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add IntegratedThrottle"));
        return 1;
    }

    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            VariableNames::IntegratedSteering,
            DoubleType,
            LexToString(InputDefaults::Neutral)) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            VariableNames::SteeringVelocity,
            DoubleType,
            LexToString(InputDefaults::Neutral)))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add steering state"));
        return 1;
    }

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("Event graph was not created"));
        return 1;
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

    UK2Node_Event* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true;
    FinishNode(Tick, Graph, 0, 0);

    UK2Node_CallFunction* GetParentActor = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor)),
        220,
        180);

    UK2Node_DynamicCast* CastPawn = NewObject<UK2Node_DynamicCast>(Graph);
    CastPawn->TargetType = AVoyageVehicleForkliftPawn::StaticClass();
    FinishNode(CastPawn, Graph, 450, 0);
    CastPawn->SetPurity(false);

    UK2Node_CallFunction* IsPlayerControlled = AddCall(
        Graph,
        APawn::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled)),
        700,
        20);

    UK2Node_IfThenElse* ControlledBranch = NewObject<UK2Node_IfThenElse>(Graph);
    FinishNode(ControlledBranch, Graph, 940, 0);

    UK2Node_VariableGet* RawThrottle = AddVariableGet(
        Graph,
        VariableNames::ThrottleInput,
        AVoyageVehicleForkliftPawn::StaticClass(),
        700,
        220);
    UK2Node_VariableGet* CurrentThrottle = AddVariableGet(
        Graph,
        VariableNames::IntegratedThrottle,
        nullptr,
        1180,
        500);
    UK2Node_VariableGet* RawSteering = AddVariableGet(
        Graph,
        VariableNames::SteeringInput,
        AVoyageVehicleForkliftPawn::StaticClass(),
        700,
        720);
    UK2Node_VariableGet* CurrentSteering = AddVariableGet(
        Graph,
        VariableNames::IntegratedSteering,
        nullptr,
        1180,
        1080);
    UK2Node_VariableGet* CurrentSteeringVelocity = AddVariableGet(
        Graph,
        VariableNames::SteeringVelocity,
        nullptr,
        1180,
        1260);

    UK2Node_CallFunction* GetController = AddCall(
        Graph,
        UGameplayStatics::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)),
        700,
        -180);
    SetDefault(GetController, PinNames::PlayerIndex, InputDefaults::PlayerIndex);

    UFunction* WasInputKeyJustPressed = APlayerController::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    UK2Node_CallFunction* WasXPressed = AddCall(Graph, WasInputKeyJustPressed, 940, -180);
    SetDefault(WasXPressed, PinNames::Key, InputDefaults::BrakeKey);
    UK2Node_CallFunction* WasCPressed = AddCall(Graph, WasInputKeyJustPressed, 3940, 1080);
    SetDefault(WasCPressed, PinNames::Key, InputDefaults::CenterSteeringKey);

    UK2Node_CallFunction* IsForward = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::EqualDouble), 940, 120);
    UK2Node_CallFunction* IsReverse = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::EqualDouble), 940, 300);
    SetDefault(IsForward, BinaryPins::RightOperand, InputDefaults::DigitalPositive);
    SetDefault(IsReverse, BinaryPins::RightOperand, InputDefaults::DigitalNegative);

    UK2Node_CallFunction* ForwardDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 1190, 80);
    UK2Node_CallFunction* ReverseDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 1190, 260);
    SetDefault(ForwardDirection, SelectPins::WhenTrue, InputDefaults::DigitalPositive);
    SetDefault(ForwardDirection, SelectPins::WhenFalse, InputDefaults::Neutral);
    SetDefault(ReverseDirection, SelectPins::WhenTrue, InputDefaults::DigitalNegative);
    SetDefault(ReverseDirection, SelectPins::WhenFalse, InputDefaults::Neutral);

    UK2Node_CallFunction* AddDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::AddDouble), 1450, 170);
    UK2Node_CallFunction* ScaleRate = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 1680, 170);
    UK2Node_CallFunction* ScaleTime = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 1900, 170);
    UK2Node_CallFunction* AddCurrent = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::AddDouble), 2130, 280);
    UK2Node_CallFunction* ClampThrottle = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::ClampFloat), 2360, 280);
    UK2Node_CallFunction* ResetThrottle = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 2500, 500);
    SetDefault(ScaleRate, BinaryPins::RightOperand, InputDefaults::ThrottleRate);
    SetDefault(ClampThrottle, PinNames::Min, InputDefaults::IntegratedInputMinimum);
    SetDefault(ClampThrottle, PinNames::Max, InputDefaults::IntegratedInputMaximum);
    SetDefault(ResetThrottle, SelectPins::WhenTrue, InputDefaults::Neutral);

    UK2Node_VariableSet* SetIntegrated = AddVariableSet(
        Graph,
        VariableNames::IntegratedThrottle,
        nullptr,
        2620,
        0);
    UK2Node_VariableSet* SetThrottle = AddVariableSet(
        Graph,
        VariableNames::ThrottleInput,
        AVoyageVehicleForkliftPawn::StaticClass(),
        2870,
        0);

    UK2Node_CallFunction* IsSteeringPositive = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::EqualDouble), 940, 700);
    UK2Node_CallFunction* IsSteeringNegative = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::EqualDouble), 940, 880);
    SetDefault(IsSteeringPositive, BinaryPins::RightOperand, InputDefaults::DigitalPositive);
    SetDefault(IsSteeringNegative, BinaryPins::RightOperand, InputDefaults::DigitalNegative);

    UK2Node_CallFunction* PositiveSteeringDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 1190, 660);
    UK2Node_CallFunction* NegativeSteeringDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 1190, 840);
    SetDefault(PositiveSteeringDirection, SelectPins::WhenTrue, InputDefaults::DigitalPositive);
    SetDefault(PositiveSteeringDirection, SelectPins::WhenFalse, InputDefaults::Neutral);
    SetDefault(NegativeSteeringDirection, SelectPins::WhenTrue, InputDefaults::DigitalNegative);
    SetDefault(NegativeSteeringDirection, SelectPins::WhenFalse, InputDefaults::Neutral);

    UK2Node_CallFunction* AddSteeringDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::AddDouble), 1450, 750);
    UK2Node_CallFunction* TargetSteeringVelocity = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 1680, 650);
    UK2Node_CallFunction* SteeringVelocityDirection = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 1680, 840);
    SetDefault(TargetSteeringVelocity, BinaryPins::RightOperand, InputDefaults::SteeringMaximumSpeed);

    UK2Node_CallFunction* IsReversingSteering = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::LessDouble), 1900, 840);
    SetDefault(IsReversingSteering, BinaryPins::RightOperand, InputDefaults::Neutral);
    UK2Node_CallFunction* SelectSteeringAcceleration = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 2130, 840);
    SetDefault(SelectSteeringAcceleration, SelectPins::WhenTrue, InputDefaults::SteeringReversalBraking);
    SetDefault(SelectSteeringAcceleration, SelectPins::WhenFalse, InputDefaults::SteeringAcceleration);
    UK2Node_CallFunction* ScaleSteeringAccelerationTime = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 2360, 840);
    UK2Node_CallFunction* NegateSteeringMaximumChange = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 2580, 980);
    SetDefault(NegateSteeringMaximumChange, BinaryPins::RightOperand, InputDefaults::DigitalNegative);

    UK2Node_CallFunction* SteeringVelocityDifference = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SubtractDouble), 2130, 650);
    UK2Node_CallFunction* ClampSteeringVelocityChange = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::ClampFloat), 2800, 760);
    UK2Node_CallFunction* AddSteeringVelocity = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::AddDouble), 3030, 760);
    UK2Node_CallFunction* IsSteeringReleased = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::EqualDouble), 1900, 1080);
    SetDefault(IsSteeringReleased, BinaryPins::RightOperand, InputDefaults::Neutral);
    UK2Node_CallFunction* StopReleasedSteering = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 3260, 760);
    SetDefault(StopReleasedSteering, SelectPins::WhenTrue, InputDefaults::Neutral);

    UK2Node_CallFunction* ResetSteeringVelocity = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 3490, 1080);
    SetDefault(ResetSteeringVelocity, SelectPins::WhenTrue, InputDefaults::Neutral);
    UK2Node_CallFunction* ScaleSteeringTime = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::MultiplyDouble), 3490, 760);
    UK2Node_CallFunction* AddCurrentSteering = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::AddDouble), 3710, 860);
    UK2Node_CallFunction* ClampSteering = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::ClampFloat), 3940, 860);
    SetDefault(ClampSteering, PinNames::Min, InputDefaults::IntegratedInputMinimum);
    SetDefault(ClampSteering, PinNames::Max, InputDefaults::IntegratedInputMaximum);
    UK2Node_CallFunction* ResetSteering = AddCall(Graph, MathFunction(BlueprintGraphNames::MathFunctions::SelectFloat), 4170, 860);
    SetDefault(ResetSteering, SelectPins::WhenTrue, InputDefaults::Neutral);

    UK2Node_VariableSet* SetSteeringVelocity = AddVariableSet(
        Graph,
        VariableNames::SteeringVelocity,
        nullptr,
        3120,
        0);
    UK2Node_VariableGet* UpdatedSteeringVelocity = AddVariableGet(
        Graph,
        VariableNames::SteeringVelocity,
        nullptr,
        3370,
        620);
    UK2Node_VariableSet* SetIntegratedSteering = AddVariableSet(
        Graph,
        VariableNames::IntegratedSteering,
        nullptr,
        3370,
        0);
    UK2Node_VariableGet* UpdatedSteering = AddVariableGet(
        Graph,
        VariableNames::IntegratedSteering,
        nullptr,
        4170,
        620);
    UK2Node_VariableSet* SetSteering = AddVariableSet(
        Graph,
        VariableNames::SteeringInput,
        AVoyageVehicleForkliftPawn::StaticClass(),
        3620,
        0);

    // Every forklift owns its helper through a ChildActorComponent. When the
    // pawn is no longer player-controlled, clear both our integrators and the
    // native input fields on that exact parent instead of losing the reference
    // through GetPlayerPawn and leaving a parked forklift under throttle.
    UK2Node_VariableSet* ClearIntegratedThrottle = AddVariableSet(
        Graph,
        VariableNames::IntegratedThrottle,
        nullptr,
        1180,
        -420);
    SetDefault(ClearIntegratedThrottle, VariableNames::IntegratedThrottle, InputDefaults::Neutral);
    UK2Node_VariableSet* ClearThrottleInput = AddVariableSet(
        Graph,
        VariableNames::ThrottleInput,
        AVoyageVehicleForkliftPawn::StaticClass(),
        1420,
        -420);
    SetDefault(ClearThrottleInput, VariableNames::ThrottleInput, InputDefaults::Neutral);
    UK2Node_VariableSet* ClearSteeringVelocity = AddVariableSet(
        Graph,
        VariableNames::SteeringVelocity,
        nullptr,
        1660,
        -420);
    SetDefault(ClearSteeringVelocity, VariableNames::SteeringVelocity, InputDefaults::Neutral);
    UK2Node_VariableSet* ClearIntegratedSteering = AddVariableSet(
        Graph,
        VariableNames::IntegratedSteering,
        nullptr,
        1900,
        -420);
    SetDefault(ClearIntegratedSteering, VariableNames::IntegratedSteering, InputDefaults::Neutral);
    UK2Node_VariableSet* ClearSteeringInput = AddVariableSet(
        Graph,
        VariableNames::SteeringInput,
        AVoyageVehicleForkliftPawn::StaticClass(),
        2140,
        -420);
    SetDefault(ClearSteeringInput, VariableNames::SteeringInput, InputDefaults::Neutral);

    bool Ok = true;
    Ok &= Connect(Schema, RequirePin(Tick, UEdGraphSchema_K2::PN_Then), RequirePin(CastPawn, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, GetParentActor->GetReturnValuePin(), CastPawn->GetCastSourcePin());
    Ok &= Connect(Schema, CastPawn->GetValidCastPin(), RequirePin(ControlledBranch, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(IsPlayerControlled, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, IsPlayerControlled->GetReturnValuePin(), RequirePin(ControlledBranch, UEdGraphSchema_K2::PN_Condition));
    Ok &= Connect(Schema, RequirePin(ControlledBranch, UEdGraphSchema_K2::PN_Then), RequirePin(SetIntegrated, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(SetIntegrated, UEdGraphSchema_K2::PN_Then), RequirePin(SetThrottle, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(SetThrottle, UEdGraphSchema_K2::PN_Then), RequirePin(SetSteeringVelocity, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(SetSteeringVelocity, UEdGraphSchema_K2::PN_Then), RequirePin(SetIntegratedSteering, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(SetIntegratedSteering, UEdGraphSchema_K2::PN_Then), RequirePin(SetSteering, UEdGraphSchema_K2::PN_Execute));

    Ok &= Connect(Schema, RequirePin(ControlledBranch, UEdGraphSchema_K2::PN_Else), RequirePin(ClearIntegratedThrottle, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(ClearIntegratedThrottle, UEdGraphSchema_K2::PN_Then), RequirePin(ClearThrottleInput, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(ClearThrottleInput, UEdGraphSchema_K2::PN_Then), RequirePin(ClearSteeringVelocity, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(ClearSteeringVelocity, UEdGraphSchema_K2::PN_Then), RequirePin(ClearIntegratedSteering, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(ClearIntegratedSteering, UEdGraphSchema_K2::PN_Then), RequirePin(ClearSteeringInput, UEdGraphSchema_K2::PN_Execute));

    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(RawThrottle, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(SetThrottle, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(RawSteering, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(SetSteering, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(ClearThrottleInput, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(ClearSteeringInput, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, GetController->GetReturnValuePin(), RequirePin(WasXPressed, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, GetController->GetReturnValuePin(), RequirePin(WasCPressed, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, RawThrottle->GetValuePin(), RequirePin(IsForward, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, RawThrottle->GetValuePin(), RequirePin(IsReverse, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, IsForward->GetReturnValuePin(), RequirePin(ForwardDirection, SelectPins::Condition));
    Ok &= Connect(Schema, IsReverse->GetReturnValuePin(), RequirePin(ReverseDirection, SelectPins::Condition));
    Ok &= Connect(Schema, ForwardDirection->GetReturnValuePin(), RequirePin(AddDirection, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, ReverseDirection->GetReturnValuePin(), RequirePin(AddDirection, BinaryPins::RightOperand));
    Ok &= Connect(Schema, AddDirection->GetReturnValuePin(), RequirePin(ScaleRate, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, ScaleRate->GetReturnValuePin(), RequirePin(ScaleTime, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, RequirePin(Tick, PinNames::DeltaSeconds), RequirePin(ScaleTime, BinaryPins::RightOperand));
    Ok &= Connect(Schema, ScaleTime->GetReturnValuePin(), RequirePin(AddCurrent, BinaryPins::RightOperand));
    Ok &= Connect(Schema, CurrentThrottle->GetValuePin(), RequirePin(AddCurrent, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, AddCurrent->GetReturnValuePin(), RequirePin(ClampThrottle, PinNames::Value));
    Ok &= Connect(Schema, ClampThrottle->GetReturnValuePin(), RequirePin(ResetThrottle, SelectPins::WhenFalse));
    Ok &= Connect(Schema, WasXPressed->GetReturnValuePin(), RequirePin(ResetThrottle, SelectPins::Condition));
    Ok &= Connect(Schema, ResetThrottle->GetReturnValuePin(), RequirePin(SetIntegrated, VariableNames::IntegratedThrottle));
    Ok &= Connect(Schema, ResetThrottle->GetReturnValuePin(), RequirePin(SetThrottle, VariableNames::ThrottleInput));

    Ok &= Connect(Schema, RawSteering->GetValuePin(), RequirePin(IsSteeringPositive, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, RawSteering->GetValuePin(), RequirePin(IsSteeringNegative, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, IsSteeringPositive->GetReturnValuePin(), RequirePin(PositiveSteeringDirection, SelectPins::Condition));
    Ok &= Connect(Schema, IsSteeringNegative->GetReturnValuePin(), RequirePin(NegativeSteeringDirection, SelectPins::Condition));
    Ok &= Connect(Schema, PositiveSteeringDirection->GetReturnValuePin(), RequirePin(AddSteeringDirection, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, NegativeSteeringDirection->GetReturnValuePin(), RequirePin(AddSteeringDirection, BinaryPins::RightOperand));
    Ok &= Connect(Schema, AddSteeringDirection->GetReturnValuePin(), RequirePin(TargetSteeringVelocity, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, CurrentSteeringVelocity->GetValuePin(), RequirePin(SteeringVelocityDirection, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, AddSteeringDirection->GetReturnValuePin(), RequirePin(SteeringVelocityDirection, BinaryPins::RightOperand));
    Ok &= Connect(Schema, SteeringVelocityDirection->GetReturnValuePin(), RequirePin(IsReversingSteering, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, IsReversingSteering->GetReturnValuePin(), RequirePin(SelectSteeringAcceleration, SelectPins::Condition));
    Ok &= Connect(Schema, SelectSteeringAcceleration->GetReturnValuePin(), RequirePin(ScaleSteeringAccelerationTime, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, RequirePin(Tick, PinNames::DeltaSeconds), RequirePin(ScaleSteeringAccelerationTime, BinaryPins::RightOperand));
    Ok &= Connect(Schema, ScaleSteeringAccelerationTime->GetReturnValuePin(), RequirePin(NegateSteeringMaximumChange, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, TargetSteeringVelocity->GetReturnValuePin(), RequirePin(SteeringVelocityDifference, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, CurrentSteeringVelocity->GetValuePin(), RequirePin(SteeringVelocityDifference, BinaryPins::RightOperand));
    Ok &= Connect(Schema, SteeringVelocityDifference->GetReturnValuePin(), RequirePin(ClampSteeringVelocityChange, PinNames::Value));
    Ok &= Connect(Schema, NegateSteeringMaximumChange->GetReturnValuePin(), RequirePin(ClampSteeringVelocityChange, PinNames::Min));
    Ok &= Connect(Schema, ScaleSteeringAccelerationTime->GetReturnValuePin(), RequirePin(ClampSteeringVelocityChange, PinNames::Max));
    Ok &= Connect(Schema, CurrentSteeringVelocity->GetValuePin(), RequirePin(AddSteeringVelocity, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, ClampSteeringVelocityChange->GetReturnValuePin(), RequirePin(AddSteeringVelocity, BinaryPins::RightOperand));
    Ok &= Connect(Schema, AddSteeringDirection->GetReturnValuePin(), RequirePin(IsSteeringReleased, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, IsSteeringReleased->GetReturnValuePin(), RequirePin(StopReleasedSteering, SelectPins::Condition));
    Ok &= Connect(Schema, AddSteeringVelocity->GetReturnValuePin(), RequirePin(StopReleasedSteering, SelectPins::WhenFalse));
    Ok &= Connect(Schema, StopReleasedSteering->GetReturnValuePin(), RequirePin(ResetSteeringVelocity, SelectPins::WhenFalse));
    Ok &= Connect(Schema, WasCPressed->GetReturnValuePin(), RequirePin(ResetSteeringVelocity, SelectPins::Condition));
    Ok &= Connect(Schema, ResetSteeringVelocity->GetReturnValuePin(), RequirePin(SetSteeringVelocity, VariableNames::SteeringVelocity));
    Ok &= Connect(Schema, UpdatedSteeringVelocity->GetValuePin(), RequirePin(ScaleSteeringTime, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, RequirePin(Tick, PinNames::DeltaSeconds), RequirePin(ScaleSteeringTime, BinaryPins::RightOperand));
    Ok &= Connect(Schema, CurrentSteering->GetValuePin(), RequirePin(AddCurrentSteering, BinaryPins::LeftOperand));
    Ok &= Connect(Schema, ScaleSteeringTime->GetReturnValuePin(), RequirePin(AddCurrentSteering, BinaryPins::RightOperand));
    Ok &= Connect(Schema, AddCurrentSteering->GetReturnValuePin(), RequirePin(ClampSteering, PinNames::Value));
    Ok &= Connect(Schema, ClampSteering->GetReturnValuePin(), RequirePin(ResetSteering, SelectPins::WhenFalse));
    Ok &= Connect(Schema, WasCPressed->GetReturnValuePin(), RequirePin(ResetSteering, SelectPins::Condition));
    Ok &= Connect(Schema, ResetSteering->GetReturnValuePin(), RequirePin(SetIntegratedSteering, VariableNames::IntegratedSteering));
    Ok &= Connect(Schema, UpdatedSteering->GetValuePin(), RequirePin(SetSteering, VariableNames::SteeringInput));

    if (!Ok)
    {
        return 1;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint compilation failed"));
        return 1;
    }

    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        PackageName,
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save %s"), *Filename);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated %s"), PackageName);
    return 0;
}

#endif
