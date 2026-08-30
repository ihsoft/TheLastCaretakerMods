// HAND-WRITTEN BUILD TOOL SOURCE: generates helper/input Blueprint assets from
// version-bound Voyage contracts. The C++ tool itself is never shipped.
// Contracts validated against Steam build 23962331, exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D.
// Revalidate per ../../GAME_DERIVED_SOURCES.md after a game update.

#include "GenerateDonkLiftModCommandlet.h"

#if WITH_EDITOR

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
constexpr TCHAR BrakeActionPackageName[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/IAV_DonkLiftBrake");
constexpr TCHAR CenterActionPackageName[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/IAV_DonkLiftCenterSteering");

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

UFunction* MathFunction(const TCHAR* Name)
{
    return UKismetMathLibrary::StaticClass()->FindFunctionByName(FName(Name));
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
    Settings->DisplayCategory = FText::FromString(TEXT("Vehicle"));
    Action->SetPlayerMappableKeySettings(Settings);

    if (!SaveAssetPackage(Package, Action, LongPackageName))
    {
        return nullptr;
    }
    return Action;
}

UInputAction* CreateInputActionPlaceholder(
    const TCHAR* LongPackageName,
    const TCHAR* InputActionAssetName,
    const bool bVoyageAction,
    const EInputActionValueType ValueType)
{
    UPackage* Package = CreatePackage(LongPackageName);
    UInputAction* Action = bVoyageAction
        ? static_cast<UInputAction*>(NewObject<UVoyageInputAction>(
            Package,
            FName(InputActionAssetName),
            RF_Public | RF_Standalone | RF_Transactional))
        : NewObject<UInputAction>(
            Package,
            FName(InputActionAssetName),
            RF_Public | RF_Standalone | RF_Transactional);
    if (!Action)
    {
        return nullptr;
    }

    Action->ValueType = ValueType;
    if (!SaveAssetPackage(Package, Action, LongPackageName))
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
    UInputAction* Forward = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IA_VehicleForward"),
        TEXT("IA_VehicleForward"), false, EInputActionValueType::Axis1D);
    UInputAction* Backward = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IA_VehicleBackward"),
        TEXT("IA_VehicleBackward"), false, EInputActionValueType::Axis1D);
    UInputAction* Right = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IA_VehicleRight"),
        TEXT("IA_VehicleRight"), false, EInputActionValueType::Axis1D);
    UInputAction* Left = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IA_VehicleLeft"),
        TEXT("IA_VehicleLeft"), false, EInputActionValueType::Axis1D);
    UInputAction* LookUp = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Character/IA_LookUp"),
        TEXT("IA_LookUp"), false, EInputActionValueType::Axis1D);
    UInputAction* LookRight = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Character/IA_LookRight"),
        TEXT("IA_LookRight"), false, EInputActionValueType::Axis1D);
    UInputAction* Zoom = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Character/IA_Zoom"),
        TEXT("IA_Zoom"), false, EInputActionValueType::Axis1D);
    UInputAction* Exit = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IAV_VehicleExit"),
        TEXT("IAV_VehicleExit"), true, EInputActionValueType::Boolean);
    UInputAction* SwitchCamera = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IAV_VehicleSwitchCamera"),
        TEXT("IAV_VehicleSwitchCamera"), true, EInputActionValueType::Boolean);
    UInputAction* ForkUp = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkUp"),
        TEXT("IAV_Forklift_ForkUp"), true, EInputActionValueType::Axis1D);
    UInputAction* ForkDown = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkDown"),
        TEXT("IAV_Forklift_ForkDown"), true, EInputActionValueType::Axis1D);
    UInputAction* Horn = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IAV_VehicleHorn"),
        TEXT("IAV_VehicleHorn"), true, EInputActionValueType::Boolean);
    UInputAction* Handbrake = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/IAV_VehicleHandbrake"),
        TEXT("IAV_VehicleHandbrake"), true, EInputActionValueType::Boolean);
    UInputAction* ForkTiltUp = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkTiltUp"),
        TEXT("IAV_Forklift_ForkTiltUp"), true, EInputActionValueType::Axis1D);
    UInputAction* ForkTiltDown = CreateInputActionPlaceholder(
        TEXT("/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkTiltDown"),
        TEXT("IAV_Forklift_ForkTiltDown"), true, EInputActionValueType::Axis1D);

    if (!Forward || !Backward || !Right || !Left || !LookUp || !LookRight || !Zoom ||
        !Exit || !SwitchCamera || !ForkUp || !ForkDown || !Horn || !Handbrake ||
        !ForkTiltUp || !ForkTiltDown)
    {
        return nullptr;
    }

    UPackage* Package = CreatePackage(ForkliftContextPackageName);
    UInputMappingContext* Context = NewObject<UInputMappingContext>(
        Package,
        FName(TEXT("IMC_Forklift_Keyboard")),
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
    Context->ContextDescription = FText::FromString(TEXT("Vehicle keyboard"));

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
        BrakeActionPackageName,
        TEXT("IAV_DonkLiftBrake"),
        TEXT("DonkLiftBrake"),
        TEXT("Brake"));
    UVoyageInputAction* CenterAction = CreateVoyageInputAction(
        CenterActionPackageName,
        TEXT("IAV_DonkLiftCenterSteering"),
        TEXT("DonkLiftCenterSteering"),
        TEXT("Center"));
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
        FName(TEXT("GenerateDonkLiftMod")));

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
            FName(TEXT("IntegratedThrottle")),
            DoubleType,
            TEXT("0.0")))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add IntegratedThrottle"));
        return 1;
    }

    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            FName(TEXT("IntegratedSteering")),
            DoubleType,
            TEXT("0.0")) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            FName(TEXT("SteeringVelocity")),
            DoubleType,
            TEXT("0.0")))
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
    Tick->EventReference.SetExternalMember(FName(TEXT("ReceiveTick")), AActor::StaticClass());
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
        FName(TEXT("ThrottleInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        700,
        220);
    UK2Node_VariableGet* CurrentThrottle = AddVariableGet(
        Graph,
        FName(TEXT("IntegratedThrottle")),
        nullptr,
        1180,
        500);
    UK2Node_VariableGet* RawSteering = AddVariableGet(
        Graph,
        FName(TEXT("SteeringInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        700,
        720);
    UK2Node_VariableGet* CurrentSteering = AddVariableGet(
        Graph,
        FName(TEXT("IntegratedSteering")),
        nullptr,
        1180,
        1080);
    UK2Node_VariableGet* CurrentSteeringVelocity = AddVariableGet(
        Graph,
        FName(TEXT("SteeringVelocity")),
        nullptr,
        1180,
        1260);

    UK2Node_CallFunction* GetController = AddCall(
        Graph,
        UGameplayStatics::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)),
        700,
        -180);
    SetDefault(GetController, FName(TEXT("PlayerIndex")), TEXT("0"));

    UFunction* WasInputKeyJustPressed = APlayerController::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    UK2Node_CallFunction* WasXPressed = AddCall(Graph, WasInputKeyJustPressed, 940, -180);
    SetDefault(WasXPressed, FName(TEXT("Key")), TEXT("X"));
    UK2Node_CallFunction* WasCPressed = AddCall(Graph, WasInputKeyJustPressed, 3940, 1080);
    SetDefault(WasCPressed, FName(TEXT("Key")), TEXT("C"));

    UK2Node_CallFunction* IsForward = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 940, 120);
    UK2Node_CallFunction* IsReverse = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 940, 300);
    SetDefault(IsForward, FName(TEXT("B")), TEXT("1.0"));
    SetDefault(IsReverse, FName(TEXT("B")), TEXT("-1.0"));

    UK2Node_CallFunction* ForwardDirection = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 1190, 80);
    UK2Node_CallFunction* ReverseDirection = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 1190, 260);
    SetDefault(ForwardDirection, FName(TEXT("A")), TEXT("1.0"));
    SetDefault(ForwardDirection, FName(TEXT("B")), TEXT("0.0"));
    SetDefault(ReverseDirection, FName(TEXT("A")), TEXT("-1.0"));
    SetDefault(ReverseDirection, FName(TEXT("B")), TEXT("0.0"));

    UK2Node_CallFunction* AddDirection = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 1450, 170);
    UK2Node_CallFunction* ScaleRate = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 1680, 170);
    UK2Node_CallFunction* ScaleTime = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 1900, 170);
    UK2Node_CallFunction* AddCurrent = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 2130, 280);
    UK2Node_CallFunction* Clamp = AddCall(Graph, MathFunction(TEXT("FClamp")), 2360, 280);
    UK2Node_CallFunction* ResetThrottle = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 2500, 500);
    SetDefault(ScaleRate, FName(TEXT("B")), TEXT("0.3333333333333333"));
    SetDefault(Clamp, FName(TEXT("Min")), TEXT("-0.9999"));
    SetDefault(Clamp, FName(TEXT("Max")), TEXT("0.9999"));
    SetDefault(ResetThrottle, FName(TEXT("A")), TEXT("0.0"));

    UK2Node_VariableSet* SetIntegrated = AddVariableSet(
        Graph,
        FName(TEXT("IntegratedThrottle")),
        nullptr,
        2620,
        0);
    UK2Node_VariableSet* SetThrottle = AddVariableSet(
        Graph,
        FName(TEXT("ThrottleInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        2870,
        0);

    UK2Node_CallFunction* IsSteeringPositive = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 940, 700);
    UK2Node_CallFunction* IsSteeringNegative = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 940, 880);
    SetDefault(IsSteeringPositive, FName(TEXT("B")), TEXT("1.0"));
    SetDefault(IsSteeringNegative, FName(TEXT("B")), TEXT("-1.0"));

    UK2Node_CallFunction* PositiveSteeringDirection = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 1190, 660);
    UK2Node_CallFunction* NegativeSteeringDirection = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 1190, 840);
    SetDefault(PositiveSteeringDirection, FName(TEXT("A")), TEXT("1.0"));
    SetDefault(PositiveSteeringDirection, FName(TEXT("B")), TEXT("0.0"));
    SetDefault(NegativeSteeringDirection, FName(TEXT("A")), TEXT("-1.0"));
    SetDefault(NegativeSteeringDirection, FName(TEXT("B")), TEXT("0.0"));

    UK2Node_CallFunction* AddSteeringDirection = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 1450, 750);
    UK2Node_CallFunction* TargetSteeringVelocity = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 1680, 650);
    UK2Node_CallFunction* SteeringVelocityDirection = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 1680, 840);
    SetDefault(TargetSteeringVelocity, FName(TEXT("B")), TEXT("1.20"));

    UK2Node_CallFunction* IsReversingSteering = AddCall(Graph, MathFunction(TEXT("Less_DoubleDouble")), 1900, 840);
    SetDefault(IsReversingSteering, FName(TEXT("B")), TEXT("0.0"));
    UK2Node_CallFunction* SelectSteeringAcceleration = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 2130, 840);
    SetDefault(SelectSteeringAcceleration, FName(TEXT("A")), TEXT("5.00"));
    SetDefault(SelectSteeringAcceleration, FName(TEXT("B")), TEXT("2.50"));
    UK2Node_CallFunction* ScaleSteeringAccelerationTime = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 2360, 840);
    UK2Node_CallFunction* NegateSteeringMaximumChange = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 2580, 980);
    SetDefault(NegateSteeringMaximumChange, FName(TEXT("B")), TEXT("-1.0"));

    UK2Node_CallFunction* SteeringVelocityDifference = AddCall(Graph, MathFunction(TEXT("Subtract_DoubleDouble")), 2130, 650);
    UK2Node_CallFunction* ClampSteeringVelocityChange = AddCall(Graph, MathFunction(TEXT("FClamp")), 2800, 760);
    UK2Node_CallFunction* AddSteeringVelocity = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 3030, 760);
    UK2Node_CallFunction* IsSteeringReleased = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 1900, 1080);
    SetDefault(IsSteeringReleased, FName(TEXT("B")), TEXT("0.0"));
    UK2Node_CallFunction* StopReleasedSteering = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 3260, 760);
    SetDefault(StopReleasedSteering, FName(TEXT("A")), TEXT("0.0"));

    UK2Node_CallFunction* ResetSteeringVelocity = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 3490, 1080);
    SetDefault(ResetSteeringVelocity, FName(TEXT("A")), TEXT("0.0"));
    UK2Node_CallFunction* ScaleSteeringTime = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 3490, 760);
    UK2Node_CallFunction* AddCurrentSteering = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 3710, 860);
    UK2Node_CallFunction* ClampSteering = AddCall(Graph, MathFunction(TEXT("FClamp")), 3940, 860);
    SetDefault(ClampSteering, FName(TEXT("Min")), TEXT("-0.9999"));
    SetDefault(ClampSteering, FName(TEXT("Max")), TEXT("0.9999"));
    UK2Node_CallFunction* ResetSteering = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 4170, 860);
    SetDefault(ResetSteering, FName(TEXT("A")), TEXT("0.0"));

    UK2Node_VariableSet* SetSteeringVelocity = AddVariableSet(
        Graph,
        FName(TEXT("SteeringVelocity")),
        nullptr,
        3120,
        0);
    UK2Node_VariableGet* UpdatedSteeringVelocity = AddVariableGet(
        Graph,
        FName(TEXT("SteeringVelocity")),
        nullptr,
        3370,
        620);
    UK2Node_VariableSet* SetIntegratedSteering = AddVariableSet(
        Graph,
        FName(TEXT("IntegratedSteering")),
        nullptr,
        3370,
        0);
    UK2Node_VariableGet* UpdatedSteering = AddVariableGet(
        Graph,
        FName(TEXT("IntegratedSteering")),
        nullptr,
        4170,
        620);
    UK2Node_VariableSet* SetSteering = AddVariableSet(
        Graph,
        FName(TEXT("SteeringInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        3620,
        0);

    // Every forklift owns its helper through a ChildActorComponent. When the
    // pawn is no longer player-controlled, clear both our integrators and the
    // native input fields on that exact parent instead of losing the reference
    // through GetPlayerPawn and leaving a parked forklift under throttle.
    UK2Node_VariableSet* ClearIntegratedThrottle = AddVariableSet(
        Graph,
        FName(TEXT("IntegratedThrottle")),
        nullptr,
        1180,
        -420);
    SetDefault(ClearIntegratedThrottle, FName(TEXT("IntegratedThrottle")), TEXT("0.0"));
    UK2Node_VariableSet* ClearThrottleInput = AddVariableSet(
        Graph,
        FName(TEXT("ThrottleInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        1420,
        -420);
    SetDefault(ClearThrottleInput, FName(TEXT("ThrottleInput")), TEXT("0.0"));
    UK2Node_VariableSet* ClearSteeringVelocity = AddVariableSet(
        Graph,
        FName(TEXT("SteeringVelocity")),
        nullptr,
        1660,
        -420);
    SetDefault(ClearSteeringVelocity, FName(TEXT("SteeringVelocity")), TEXT("0.0"));
    UK2Node_VariableSet* ClearIntegratedSteering = AddVariableSet(
        Graph,
        FName(TEXT("IntegratedSteering")),
        nullptr,
        1900,
        -420);
    SetDefault(ClearIntegratedSteering, FName(TEXT("IntegratedSteering")), TEXT("0.0"));
    UK2Node_VariableSet* ClearSteeringInput = AddVariableSet(
        Graph,
        FName(TEXT("SteeringInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        2140,
        -420);
    SetDefault(ClearSteeringInput, FName(TEXT("SteeringInput")), TEXT("0.0"));

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
    Ok &= Connect(Schema, RawThrottle->GetValuePin(), RequirePin(IsForward, FName(TEXT("A"))));
    Ok &= Connect(Schema, RawThrottle->GetValuePin(), RequirePin(IsReverse, FName(TEXT("A"))));
    Ok &= Connect(Schema, IsForward->GetReturnValuePin(), RequirePin(ForwardDirection, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, IsReverse->GetReturnValuePin(), RequirePin(ReverseDirection, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, ForwardDirection->GetReturnValuePin(), RequirePin(AddDirection, FName(TEXT("A"))));
    Ok &= Connect(Schema, ReverseDirection->GetReturnValuePin(), RequirePin(AddDirection, FName(TEXT("B"))));
    Ok &= Connect(Schema, AddDirection->GetReturnValuePin(), RequirePin(ScaleRate, FName(TEXT("A"))));
    Ok &= Connect(Schema, ScaleRate->GetReturnValuePin(), RequirePin(ScaleTime, FName(TEXT("A"))));
    Ok &= Connect(Schema, RequirePin(Tick, FName(TEXT("DeltaSeconds"))), RequirePin(ScaleTime, FName(TEXT("B"))));
    Ok &= Connect(Schema, ScaleTime->GetReturnValuePin(), RequirePin(AddCurrent, FName(TEXT("B"))));
    Ok &= Connect(Schema, CurrentThrottle->GetValuePin(), RequirePin(AddCurrent, FName(TEXT("A"))));
    Ok &= Connect(Schema, AddCurrent->GetReturnValuePin(), RequirePin(Clamp, FName(TEXT("Value"))));
    Ok &= Connect(Schema, Clamp->GetReturnValuePin(), RequirePin(ResetThrottle, FName(TEXT("B"))));
    Ok &= Connect(Schema, WasXPressed->GetReturnValuePin(), RequirePin(ResetThrottle, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, ResetThrottle->GetReturnValuePin(), RequirePin(SetIntegrated, FName(TEXT("IntegratedThrottle"))));
    Ok &= Connect(Schema, ResetThrottle->GetReturnValuePin(), RequirePin(SetThrottle, FName(TEXT("ThrottleInput"))));

    Ok &= Connect(Schema, RawSteering->GetValuePin(), RequirePin(IsSteeringPositive, FName(TEXT("A"))));
    Ok &= Connect(Schema, RawSteering->GetValuePin(), RequirePin(IsSteeringNegative, FName(TEXT("A"))));
    Ok &= Connect(Schema, IsSteeringPositive->GetReturnValuePin(), RequirePin(PositiveSteeringDirection, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, IsSteeringNegative->GetReturnValuePin(), RequirePin(NegativeSteeringDirection, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, PositiveSteeringDirection->GetReturnValuePin(), RequirePin(AddSteeringDirection, FName(TEXT("A"))));
    Ok &= Connect(Schema, NegativeSteeringDirection->GetReturnValuePin(), RequirePin(AddSteeringDirection, FName(TEXT("B"))));
    Ok &= Connect(Schema, AddSteeringDirection->GetReturnValuePin(), RequirePin(TargetSteeringVelocity, FName(TEXT("A"))));
    Ok &= Connect(Schema, CurrentSteeringVelocity->GetValuePin(), RequirePin(SteeringVelocityDirection, FName(TEXT("A"))));
    Ok &= Connect(Schema, AddSteeringDirection->GetReturnValuePin(), RequirePin(SteeringVelocityDirection, FName(TEXT("B"))));
    Ok &= Connect(Schema, SteeringVelocityDirection->GetReturnValuePin(), RequirePin(IsReversingSteering, FName(TEXT("A"))));
    Ok &= Connect(Schema, IsReversingSteering->GetReturnValuePin(), RequirePin(SelectSteeringAcceleration, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, SelectSteeringAcceleration->GetReturnValuePin(), RequirePin(ScaleSteeringAccelerationTime, FName(TEXT("A"))));
    Ok &= Connect(Schema, RequirePin(Tick, FName(TEXT("DeltaSeconds"))), RequirePin(ScaleSteeringAccelerationTime, FName(TEXT("B"))));
    Ok &= Connect(Schema, ScaleSteeringAccelerationTime->GetReturnValuePin(), RequirePin(NegateSteeringMaximumChange, FName(TEXT("A"))));
    Ok &= Connect(Schema, TargetSteeringVelocity->GetReturnValuePin(), RequirePin(SteeringVelocityDifference, FName(TEXT("A"))));
    Ok &= Connect(Schema, CurrentSteeringVelocity->GetValuePin(), RequirePin(SteeringVelocityDifference, FName(TEXT("B"))));
    Ok &= Connect(Schema, SteeringVelocityDifference->GetReturnValuePin(), RequirePin(ClampSteeringVelocityChange, FName(TEXT("Value"))));
    Ok &= Connect(Schema, NegateSteeringMaximumChange->GetReturnValuePin(), RequirePin(ClampSteeringVelocityChange, FName(TEXT("Min"))));
    Ok &= Connect(Schema, ScaleSteeringAccelerationTime->GetReturnValuePin(), RequirePin(ClampSteeringVelocityChange, FName(TEXT("Max"))));
    Ok &= Connect(Schema, CurrentSteeringVelocity->GetValuePin(), RequirePin(AddSteeringVelocity, FName(TEXT("A"))));
    Ok &= Connect(Schema, ClampSteeringVelocityChange->GetReturnValuePin(), RequirePin(AddSteeringVelocity, FName(TEXT("B"))));
    Ok &= Connect(Schema, AddSteeringDirection->GetReturnValuePin(), RequirePin(IsSteeringReleased, FName(TEXT("A"))));
    Ok &= Connect(Schema, IsSteeringReleased->GetReturnValuePin(), RequirePin(StopReleasedSteering, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, AddSteeringVelocity->GetReturnValuePin(), RequirePin(StopReleasedSteering, FName(TEXT("B"))));
    Ok &= Connect(Schema, StopReleasedSteering->GetReturnValuePin(), RequirePin(ResetSteeringVelocity, FName(TEXT("B"))));
    Ok &= Connect(Schema, WasCPressed->GetReturnValuePin(), RequirePin(ResetSteeringVelocity, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, ResetSteeringVelocity->GetReturnValuePin(), RequirePin(SetSteeringVelocity, FName(TEXT("SteeringVelocity"))));
    Ok &= Connect(Schema, UpdatedSteeringVelocity->GetValuePin(), RequirePin(ScaleSteeringTime, FName(TEXT("A"))));
    Ok &= Connect(Schema, RequirePin(Tick, FName(TEXT("DeltaSeconds"))), RequirePin(ScaleSteeringTime, FName(TEXT("B"))));
    Ok &= Connect(Schema, CurrentSteering->GetValuePin(), RequirePin(AddCurrentSteering, FName(TEXT("A"))));
    Ok &= Connect(Schema, ScaleSteeringTime->GetReturnValuePin(), RequirePin(AddCurrentSteering, FName(TEXT("B"))));
    Ok &= Connect(Schema, AddCurrentSteering->GetReturnValuePin(), RequirePin(ClampSteering, FName(TEXT("Value"))));
    Ok &= Connect(Schema, ClampSteering->GetReturnValuePin(), RequirePin(ResetSteering, FName(TEXT("B"))));
    Ok &= Connect(Schema, WasCPressed->GetReturnValuePin(), RequirePin(ResetSteering, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, ResetSteering->GetReturnValuePin(), RequirePin(SetIntegratedSteering, FName(TEXT("IntegratedSteering"))));
    Ok &= Connect(Schema, UpdatedSteering->GetValuePin(), RequirePin(SetSteering, FName(TEXT("SteeringInput"))));

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
