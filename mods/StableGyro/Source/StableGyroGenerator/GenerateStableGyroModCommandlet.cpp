// HAND-WRITTEN BUILD TOOL SOURCE: generates the runtime pitch integrator.
// Game identities are revalidated for Steam build 25929985.
// The C++ generator is editor-only and never ships in the mod container.

#include "GenerateStableGyroModCommandlet.h"

#if WITH_EDITOR

#include "ActorScanGraphNames.h"
#include "BlueprintGraphNames.h"
#include "StableGyroAssetNames.h"
#include "StableGyroSettings.h"
#include "TextSettingsGraphNames.h"
#include "VoyageModSettingsGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/BlueprintPathsLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "PlayerMappableKeySettings.h"
#include "UObject/SavePackage.h"
#include "VoyageEditorBlueprintFunctionLibrary.h"
#include "VoyageInputAction.h"
#include "VoyageVehicleGyroCopter.h"

namespace
{
namespace P = BlueprintGraphNames::Pins;
namespace Binary = BlueprintGraphNames::Pins::Binary;
namespace Select = BlueprintGraphNames::Pins::Select;
namespace Settings = StableGyroSettings;

constexpr TCHAR StabilizationPackage[] = TEXT("/Game/Mods/StableGyro/ModActor");
constexpr TCHAR StabilizationAsset[] = TEXT("ModActor");
constexpr TCHAR PitchInputPackage[] = TEXT("/Game/Mods/StableGyro/PitchInputActor");
constexpr TCHAR PitchInputAsset[] = TEXT("PitchInputActor");
constexpr TCHAR GeneratorName[] = TEXT("GenerateStableGyroMod");
constexpr TCHAR PitchGeneratorName[] = TEXT("GenerateStableGyroPitchInput");
constexpr TCHAR Neutral[] = TEXT("0.0");
constexpr TCHAR Positive[] = TEXT("1.0");
constexpr TCHAR Negative[] = TEXT("-1.0");
constexpr TCHAR IntegratedMinimum[] = TEXT("-0.9999");
constexpr TCHAR IntegratedMaximum[] = TEXT("0.9999");
constexpr TCHAR ResetKey[] = TEXT("X");
constexpr TCHAR FalseText[] = TEXT("false");
constexpr TCHAR ZeroText[] = TEXT("0");
constexpr TCHAR LoopPackage[] = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
constexpr TCHAR GyroContextPackage[] = TEXT("/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard");
constexpr TCHAR GyroContextAsset[] = TEXT("IMC_GyroCopter_Keyboard");
constexpr TCHAR GyroContextDescription[] = TEXT("Vehicle keyboard");

struct InputActionContract
{
    const TCHAR* PackageName;
    const TCHAR* AssetName;
    bool IsVoyageAction;
    EInputActionValueType ValueType;
};

namespace GyroInputAssets
{
constexpr InputActionContract LookUp{TEXT("/Game/Game/Input/Character/IA_LookUp"), TEXT("IA_LookUp"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract LookRight{TEXT("/Game/Game/Input/Character/IA_LookRight"), TEXT("IA_LookRight"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Zoom{TEXT("/Game/Game/Input/Character/IA_Zoom"), TEXT("IA_Zoom"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract Exit{TEXT("/Game/Game/Input/Vehicle/IAV_VehicleExit"), TEXT("IAV_VehicleExit"), true, EInputActionValueType::Boolean};
constexpr InputActionContract SwitchCamera{TEXT("/Game/Game/Input/Vehicle/IAV_VehicleSwitchCamera"), TEXT("IAV_VehicleSwitchCamera"), true, EInputActionValueType::Boolean};
constexpr InputActionContract ThrottleUp{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterThrottleUp"), TEXT("IA_GyroCopterThrottleUp"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract ThrottleDown{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterThrottleDown"), TEXT("IA_GyroCopterThrottleDown"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract SteeringLeft{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterSteeringLeft"), TEXT("IA_GyroCopterSteeringLeft"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract SteeringRight{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterSteeringRight"), TEXT("IA_GyroCopterSteeringRight"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract TiltForward{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterTiltForward"), TEXT("IA_GyroCopterTiltForward"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract TiltBackward{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterTiltBackward"), TEXT("IA_GyroCopterTiltBackward"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract TiltLeft{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterTiltSideLeft"), TEXT("IA_GyroCopterTiltSideLeft"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract TiltRight{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterTiltSideRight"), TEXT("IA_GyroCopterTiltSideRight"), false, EInputActionValueType::Axis1D};
constexpr InputActionContract DropCargo{TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterDropCargo"), TEXT("IA_GyroCopterDropCargo"), false, EInputActionValueType::Boolean};
}

const FName IntegratedPitch(TEXT("IntegratedPitch"));
const FName TiltForwardInput(TEXT("TiltForwardInput"));
const FName ReloadSettings(TEXT("ReloadSettings"));
const FName DivideDouble(TEXT("Divide_DoubleDouble"));
const FName LoopGraphName(TEXT("ForEachLoop"));
const FName LoopArray(TEXT("Array"));
const FName LoopElement(TEXT("Array Element"));
const FName InputPath(TEXT("InPath"));

template <typename NodeType>
NodeType* AddNode(NodeType* Node, UEdGraph* Graph, int32 X, int32 Y)
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

UEdGraphPin* Pin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Result = Node->FindPin(Name);
    checkf(Result, TEXT("Missing pin '%s' on '%s'"), *Name.ToString(), *Node->GetName());
    return Result;
}

UK2Node_CallFunction* Call(UEdGraph* Graph, UClass* Owner, const FName Function, int32 X, int32 Y)
{
    UFunction* Reflected = Owner->FindFunctionByName(Function);
    checkf(Reflected, TEXT("Missing reflected function %s.%s"),
        *Owner->GetPathName(), *Function.ToString());
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Reflected);
    return AddNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* Read(UEdGraph* Graph, const FName Name, UClass* Owner, int32 X, int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    if (Owner) Node->VariableReference.SetExternalMember(Name, Owner);
    else Node->VariableReference.SetSelfMember(Name);
    return AddNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* Write(UEdGraph* Graph, const FName Name, UClass* Owner, int32 X, int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    if (Owner) Node->VariableReference.SetExternalMember(Name, Owner);
    else Node->VariableReference.SetSelfMember(Name);
    return AddNode(Node, Graph, X, Y);
}

bool Link(UEdGraphPin* From, UEdGraphPin* To)
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

void Default(UEdGraphNode* Node, const FName Name, const TCHAR* Value)
{
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Pin(Node, Name), Value);
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

bool SaveAsset(UPackage* Package, UObject* Asset, const TCHAR* LongPackageName)
{
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        LongPackageName, FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *Filename, Args);
}

UVoyageInputAction* CreateResetAction()
{
    UPackage* Package = CreatePackage(StableGyroAssetNames::ResetActionPackageName);
    UVoyageInputAction* Action = NewObject<UVoyageInputAction>(
        Package, FName(StableGyroAssetNames::ResetActionAssetName),
        RF_Public | RF_Standalone | RF_Transactional);
    if (!Action) return nullptr;
    const FText Label = FText::FromString(StableGyroAssetNames::ResetDisplayLabel);
    Action->Description = Label;
    Action->ActionDescription = Label;
    Action->ValueType = EInputActionValueType::Boolean;
    UPlayerMappableKeySettings* Settings = NewObject<UPlayerMappableKeySettings>(
        Action, NAME_None, RF_Public | RF_Transactional);
    Settings->Name = FName(StableGyroAssetNames::ResetMappingName);
    Settings->DisplayName = Label;
    Settings->DisplayCategory = FText::FromString(StableGyroAssetNames::VehicleInputCategory);
    Action->SetPlayerMappableKeySettings(Settings);
    return SaveAsset(Package, Action, StableGyroAssetNames::ResetActionPackageName)
        ? Action : nullptr;
}

UInputAction* CreatePlaceholder(const InputActionContract& Contract)
{
    UPackage* Package = CreatePackage(Contract.PackageName);
    UInputAction* Action = Contract.IsVoyageAction
        ? static_cast<UInputAction*>(NewObject<UVoyageInputAction>(
            Package, FName(Contract.AssetName), RF_Public | RF_Standalone | RF_Transactional))
        : NewObject<UInputAction>(
            Package, FName(Contract.AssetName), RF_Public | RF_Standalone | RF_Transactional);
    if (!Action) return nullptr;
    Action->ValueType = Contract.ValueType;
    return SaveAsset(Package, Action, Contract.PackageName) ? Action : nullptr;
}

void AddMapping(UInputMappingContext* Context, UInputAction* Action, const FKey Key,
    const bool Negate = false, const bool Pressed = false)
{
    FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
    if (Negate)
    {
        Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(
            Context, NAME_None, RF_Public | RF_Transactional));
    }
    if (Pressed)
    {
        Mapping.Triggers.Add(NewObject<UInputTriggerPressed>(
            Context, NAME_None, RF_Public | RF_Transactional));
    }
}

UInputMappingContext* CreateStableGyroContext(UVoyageInputAction* ResetAction)
{
    UInputAction* LookUp = CreatePlaceholder(GyroInputAssets::LookUp);
    UInputAction* LookRight = CreatePlaceholder(GyroInputAssets::LookRight);
    UInputAction* Zoom = CreatePlaceholder(GyroInputAssets::Zoom);
    UInputAction* Exit = CreatePlaceholder(GyroInputAssets::Exit);
    UInputAction* SwitchCamera = CreatePlaceholder(GyroInputAssets::SwitchCamera);
    UInputAction* ThrottleUp = CreatePlaceholder(GyroInputAssets::ThrottleUp);
    UInputAction* ThrottleDown = CreatePlaceholder(GyroInputAssets::ThrottleDown);
    UInputAction* SteeringLeft = CreatePlaceholder(GyroInputAssets::SteeringLeft);
    UInputAction* SteeringRight = CreatePlaceholder(GyroInputAssets::SteeringRight);
    UInputAction* TiltForward = CreatePlaceholder(GyroInputAssets::TiltForward);
    UInputAction* TiltBackward = CreatePlaceholder(GyroInputAssets::TiltBackward);
    UInputAction* TiltLeft = CreatePlaceholder(GyroInputAssets::TiltLeft);
    UInputAction* TiltRight = CreatePlaceholder(GyroInputAssets::TiltRight);
    UInputAction* DropCargo = CreatePlaceholder(GyroInputAssets::DropCargo);
    if (!LookUp || !LookRight || !Zoom || !Exit || !SwitchCamera || !ThrottleUp ||
        !ThrottleDown || !SteeringLeft || !SteeringRight || !TiltForward ||
        !TiltBackward || !TiltLeft || !TiltRight || !DropCargo || !ResetAction)
    {
        return nullptr;
    }

    UPackage* Package = CreatePackage(GyroContextPackage);
    UInputMappingContext* Context = NewObject<UInputMappingContext>(
        Package, FName(GyroContextAsset), RF_Public | RF_Standalone | RF_Transactional);
    if (!Context) return nullptr;
    AddMapping(Context, LookUp, EKeys::MouseY, true);
    AddMapping(Context, LookRight, EKeys::MouseX);
    AddMapping(Context, Zoom, EKeys::MouseWheelAxis);
    AddMapping(Context, Exit, EKeys::E, false, true);
    AddMapping(Context, SwitchCamera, EKeys::T, false, true);
    AddMapping(Context, ThrottleUp, EKeys::SpaceBar);
    AddMapping(Context, ThrottleDown, EKeys::LeftControl);
    AddMapping(Context, SteeringLeft, EKeys::Left);
    AddMapping(Context, SteeringRight, EKeys::Right);
    AddMapping(Context, TiltForward, EKeys::W);
    AddMapping(Context, TiltBackward, EKeys::S);
    AddMapping(Context, TiltLeft, EKeys::A);
    AddMapping(Context, TiltRight, EKeys::D);
    AddMapping(Context, DropCargo, EKeys::F);
    AddMapping(Context, ResetAction, EKeys::X, false, true);
    Context->ContextDescription = FText::FromString(GyroContextDescription);
    return SaveAsset(Package, Context, GyroContextPackage) ? Context : nullptr;
}

UK2Node_MacroInstance* AddForEachLoop(UEdGraph* Graph, int32 X, int32 Y)
{
    UBlueprint* Macros = LoadObject<UBlueprint>(nullptr, LoopPackage);
    check(Macros);
    UEdGraph* LoopGraph = nullptr;
    for (UEdGraph* Candidate : Macros->MacroGraphs)
    {
        if (Candidate->GetFName() == LoopGraphName) LoopGraph = Candidate;
    }
    check(LoopGraph);
    UK2Node_MacroInstance* Loop = NewObject<UK2Node_MacroInstance>(Graph);
    Loop->SetMacroGraph(LoopGraph);
    return AddNode(Loop, Graph, X, Y);
}

UEdGraphPin* FirstExecInput(UEdGraphNode* Node)
{
    for (UEdGraphPin* Candidate : Node->Pins)
    {
        if (Candidate->Direction == EGPD_Input &&
            Candidate->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            return Candidate;
        }
    }
    checkNoEntry();
    return nullptr;
}

bool AddSettingsFunction(UBlueprint* Blueprint, const bool PitchOnly)
{
    return VoyageModSettings::Graph::AddSettingsLoader(
        Blueprint, ReloadSettings, Settings::RelativePath,
        Settings::Settings, UE_ARRAY_COUNT(Settings::Settings),
        [PitchOnly](const VoyageModSettings::FSetting& Setting)
        {
            return Setting.Field != Settings::EnableFourMooringPorts &&
                (!PitchOnly || Setting.Field == Settings::PitchRampSeconds ||
                    Setting.Field == Settings::EnablePitchCorrection);
        });
}

bool AddBeginPlayGraph(UEdGraph* Graph, UBlueprint* Blueprint, const bool PitchOnly)
{
    UFunction* ReloadFunction = Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->FindFunctionByName(ReloadSettings) : nullptr;
    if (!ReloadFunction || ReloadFunction->GetOuterUClass() != Blueprint->GeneratedClass)
        return false;
    UK2Node_Event* BeginPlay = NewObject<UK2Node_Event>(Graph);
    BeginPlay->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::ActorReceiveBeginPlay, AActor::StaticClass());
    BeginPlay->bOverrideFunction = true;
    AddNode(BeginPlay, Graph, 0, -700);
    UK2Node_CallFunction* Reload = Call(
        Graph, Blueprint->GeneratedClass, ReloadSettings, 220, -700);
    UK2Node_CallFunction* GetParent = Call(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor), 450, -520);
    UK2Node_DynamicCast* Gyro = NewObject<UK2Node_DynamicCast>(Graph);
    Gyro->TargetType = AVoyageVehicleGyroCopter::StaticClass();
    Gyro->SetPurity(false);
    AddNode(Gyro, Graph, 680, -700);
    UK2Node_Self* Self = AddNode(NewObject<UK2Node_Self>(Graph), Graph, 910, -340);
    UK2Node_CallFunction* GyroAfterHelper = Call(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, AddTickPrerequisiteActor), 1370, -700);

    bool Ok = true;
    Ok &= Link(Pin(BeginPlay, P::Then), Pin(Reload, P::Execute));
    Ok &= Link(Pin(Reload, P::Then), Pin(Gyro, P::Execute));
    Ok &= Link(Pin(GetParent, P::ReturnValue), Gyro->GetCastSourcePin());
    if (PitchOnly)
    {
        UK2Node_CallFunction* Controller = Call(Graph, UGameplayStatics::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController), 910, -520);
        Default(Controller, P::PlayerIndex, TEXT("0"));
        UK2Node_CallFunction* HelperAfterController = Call(Graph, AActor::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(AActor, AddTickPrerequisiteActor), 1140, -700);
        Ok &= Link(Gyro->GetValidCastPin(), Pin(HelperAfterController, P::Execute));
        Ok &= Link(Pin(Controller, P::ReturnValue),
            Pin(HelperAfterController, P::PrerequisiteActor));
        Ok &= Link(Pin(HelperAfterController, P::Then), Pin(GyroAfterHelper, P::Execute));
    }
    else
    {
        Ok &= Link(Gyro->GetValidCastPin(), Pin(GyroAfterHelper, P::Execute));
        Ok &= Link(Gyro->GetCastResultPin(), Pin(GyroAfterHelper, P::PrerequisiteActor));
    }
    if (PitchOnly)
    {
        Ok &= Link(Gyro->GetCastResultPin(), Pin(GyroAfterHelper, P::FunctionTarget));
        Ok &= Link(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
            Pin(GyroAfterHelper, P::PrerequisiteActor));
    }
    return Ok;
}

bool AddTickGraph(UEdGraph* Graph)
{
    UK2Node_Event* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true;
    AddNode(Tick, Graph, 0, 0);
    UK2Node_CallFunction* GetParent = Call(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetParentActor), 220, 170);
    UK2Node_DynamicCast* Cast = NewObject<UK2Node_DynamicCast>(Graph);
    Cast->TargetType = AVoyageVehicleGyroCopter::StaticClass();
    Cast->SetPurity(false);
    AddNode(Cast, Graph, 450, 0);
    UK2Node_CallFunction* Controlled = Call(Graph, APawn::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), 680, 0);
    UK2Node_IfThenElse* Branch = AddNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, 910, 0);
    UK2Node_VariableGet* Enabled = Read(
        Graph, Settings::EnablePitchCorrection, nullptr, 910, -340);
    UK2Node_IfThenElse* EnabledBranch = AddNode(
        NewObject<UK2Node_IfThenElse>(Graph), Graph, 1140, 0);

    UK2Node_CallFunction* Controller = Call(Graph, APawn::StaticClass(),
        BlueprintGraphNames::ActorFunctions::GetController, 680, -180);
    UK2Node_CallFunction* HelperAfterController = Call(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, AddTickPrerequisiteActor), 910, -180);
    UK2Node_Self* Self = AddNode(NewObject<UK2Node_Self>(Graph), Graph, 1140, -400);
    UK2Node_CallFunction* GyroAfterHelper = Call(Graph, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, AddTickPrerequisiteActor), 1140, -180);

    UK2Node_VariableGet* Raw = Read(Graph, TiltForwardInput,
        AVoyageVehicleGyroCopter::StaticClass(), 680, 240);
    UK2Node_CallFunction* PositiveTest = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::EqualDouble, 910, 210);
    UK2Node_CallFunction* NegativeTest = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::EqualDouble, 910, 390);
    Default(PositiveTest, Binary::RightOperand, Positive);
    Default(NegativeTest, Binary::RightOperand, Negative);
    UK2Node_CallFunction* PositiveDirection = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 1140, 180);
    UK2Node_CallFunction* NegativeDirection = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 1140, 360);
    Default(PositiveDirection, Select::WhenTrue, Positive);
    Default(PositiveDirection, Select::WhenFalse, Neutral);
    Default(NegativeDirection, Select::WhenTrue, Negative);
    Default(NegativeDirection, Select::WhenFalse, Neutral);
    UK2Node_CallFunction* Direction = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::AddDouble, 1370, 270);
    UK2Node_CallFunction* PerSecond = Call(Graph, UKismetMathLibrary::StaticClass(),
        DivideDouble, 1600, 270);
    UK2Node_VariableGet* Ramp = Read(Graph, Settings::PitchRampSeconds, nullptr, 1370, 500);
    UK2Node_CallFunction* PerFrame = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::MultiplyDouble, 1830, 270);
    UK2Node_VariableGet* Current = Read(Graph, IntegratedPitch, nullptr, 1830, 500);
    UK2Node_CallFunction* Add = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::AddDouble, 2060, 330);
    UK2Node_CallFunction* Clamp = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::ClampFloat, 2290, 330);
    Default(Clamp, P::Min, IntegratedMinimum);
    Default(Clamp, P::Max, IntegratedMaximum);

    UK2Node_DynamicCast* PlayerController = NewObject<UK2Node_DynamicCast>(Graph);
    PlayerController->TargetType = APlayerController::StaticClass();
    PlayerController->SetPurity(true);
    AddNode(PlayerController, Graph, 1830, 80);
    UK2Node_CallFunction* XPressed = Call(Graph, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed), 2060, 80);
    Default(XPressed, P::Key, ResetKey);
    UK2Node_CallFunction* Reset = Call(Graph, UKismetMathLibrary::StaticClass(),
        BlueprintGraphNames::MathFunctions::SelectFloat, 2520, 330);
    Default(Reset, Select::WhenTrue, Neutral);
    UK2Node_VariableSet* SetIntegrated = Write(Graph, IntegratedPitch, nullptr, 2750, 0);
    UK2Node_VariableSet* SetNative = Write(Graph, TiltForwardInput,
        AVoyageVehicleGyroCopter::StaticClass(), 2980, 0);
    UK2Node_VariableGet* StoredIntegrated = Read(
        Graph, IntegratedPitch, nullptr, 2980, 180);

    UK2Node_VariableSet* ClearIntegrated = Write(Graph, IntegratedPitch, nullptr, 1140, -220);
    Default(ClearIntegrated, IntegratedPitch, Neutral);
    UK2Node_VariableSet* ClearNative = Write(Graph, TiltForwardInput,
        AVoyageVehicleGyroCopter::StaticClass(), 1370, -220);
    Default(ClearNative, TiltForwardInput, Neutral);
    UK2Node_VariableSet* ClearDisabledIntegrated = Write(
        Graph, IntegratedPitch, nullptr, 1370, -400);
    Default(ClearDisabledIntegrated, IntegratedPitch, Neutral);
    bool Ok = true;
    Ok &= Link(Pin(Tick, P::Then), Pin(Cast, P::Execute));
    Ok &= Link(Pin(GetParent, P::ReturnValue), Cast->GetCastSourcePin());
    Ok &= Link(Cast->GetValidCastPin(), Pin(HelperAfterController, P::Execute));
    Ok &= Link(Cast->GetCastResultPin(), Pin(Controller, P::FunctionTarget));
    Ok &= Link(Pin(Controller, P::ReturnValue), Pin(HelperAfterController, P::PrerequisiteActor));
    Ok &= Link(Pin(HelperAfterController, P::Then), Pin(GyroAfterHelper, P::Execute));
    Ok &= Link(Cast->GetCastResultPin(), Pin(GyroAfterHelper, P::FunctionTarget));
    Ok &= Link(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self),
        Pin(GyroAfterHelper, P::PrerequisiteActor));
    Ok &= Link(Pin(GyroAfterHelper, P::Then), Pin(EnabledBranch, P::Execute));
    Ok &= Link(Pin(Enabled, Settings::EnablePitchCorrection),
        Pin(EnabledBranch, P::Condition));
    Ok &= Link(Pin(EnabledBranch, P::Then), Pin(Branch, P::Execute));
    Ok &= Link(Pin(EnabledBranch, P::Else), Pin(ClearDisabledIntegrated, P::Execute));
    Ok &= Link(Cast->GetCastResultPin(), Pin(Controlled, P::FunctionTarget));
    Ok &= Link(Pin(Controlled, P::ReturnValue), Pin(Branch, P::Condition));
    Ok &= Link(Pin(Branch, P::Then), Pin(SetIntegrated, P::Execute));
    Ok &= Link(Pin(SetIntegrated, P::Then), Pin(SetNative, P::Execute));
    Ok &= Link(Pin(Branch, P::Else), Pin(ClearIntegrated, P::Execute));
    Ok &= Link(Pin(ClearIntegrated, P::Then), Pin(ClearNative, P::Execute));
    Ok &= Link(Cast->GetCastResultPin(), Pin(Raw, P::FunctionTarget));
    Ok &= Link(Cast->GetCastResultPin(), Pin(SetNative, P::FunctionTarget));
    Ok &= Link(Cast->GetCastResultPin(), Pin(ClearNative, P::FunctionTarget));
    Ok &= Link(Pin(Raw, TiltForwardInput), Pin(PositiveTest, Binary::LeftOperand));
    Ok &= Link(Pin(Raw, TiltForwardInput), Pin(NegativeTest, Binary::LeftOperand));
    Ok &= Link(Pin(PositiveTest, P::ReturnValue), Pin(PositiveDirection, Select::Condition));
    Ok &= Link(Pin(NegativeTest, P::ReturnValue), Pin(NegativeDirection, Select::Condition));
    Ok &= Link(Pin(PositiveDirection, P::ReturnValue), Pin(Direction, Binary::LeftOperand));
    Ok &= Link(Pin(NegativeDirection, P::ReturnValue), Pin(Direction, Binary::RightOperand));
    Ok &= Link(Pin(Direction, P::ReturnValue), Pin(PerSecond, Binary::LeftOperand));
    Ok &= Link(Pin(Ramp, Settings::PitchRampSeconds), Pin(PerSecond, Binary::RightOperand));
    Ok &= Link(Pin(PerSecond, P::ReturnValue), Pin(PerFrame, Binary::LeftOperand));
    Ok &= Link(Pin(Tick, P::DeltaSeconds), Pin(PerFrame, Binary::RightOperand));
    Ok &= Link(Pin(Current, IntegratedPitch), Pin(Add, Binary::LeftOperand));
    Ok &= Link(Pin(PerFrame, P::ReturnValue), Pin(Add, Binary::RightOperand));
    Ok &= Link(Pin(Add, P::ReturnValue), Pin(Clamp, P::Value));
    Ok &= Link(Pin(Controller, P::ReturnValue), PlayerController->GetCastSourcePin());
    Ok &= Link(PlayerController->GetCastResultPin(), Pin(XPressed, P::FunctionTarget));
    Ok &= Link(Pin(Clamp, P::ReturnValue), Pin(Reset, Select::WhenFalse));
    Ok &= Link(Pin(XPressed, P::ReturnValue), Pin(Reset, Select::Condition));
    Ok &= Link(Pin(Reset, P::ReturnValue), Pin(SetIntegrated, IntegratedPitch));
    Ok &= Link(Pin(StoredIntegrated, IntegratedPitch), Pin(SetNative, TiltForwardInput));
    return Ok;
}

bool AddSettingMembers(UBlueprint* Blueprint, const bool PitchOnly)
{
    return VoyageModSettings::Graph::AddSettingMembers(
        Blueprint, Settings::Settings, UE_ARRAY_COUNT(Settings::Settings),
        [PitchOnly](const VoyageModSettings::FSetting& Setting)
        {
            return Setting.Field != Settings::EnableFourMooringPorts &&
                (!PitchOnly || Setting.Field == Settings::PitchRampSeconds ||
                    Setting.Field == Settings::EnablePitchCorrection);
        });
}
}

UGenerateStableGyroModCommandlet::UGenerateStableGyroModCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateStableGyroModCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(StabilizationPackage) ||
        FPackageName::DoesPackageExist(PitchInputPackage))
    {
        UE_LOG(LogTemp, Error, TEXT("StableGyro helper asset already exists"));
        return 1;
    }
    UVoyageInputAction* ResetAction = CreateResetAction();
    UInputMappingContext* GyroContext = CreateStableGyroContext(ResetAction);
    if (!ResetAction || !GyroContext)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate Gyro reset action or keyboard context"));
        return 1;
    }
    UPackage* StabilizationPackageObject = CreatePackage(StabilizationPackage);
    UBlueprint* StabilizationBlueprint = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(), StabilizationPackageObject, FName(StabilizationAsset), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        FName(GeneratorName));
    if (!StabilizationBlueprint) return 1;

    FEdGraphPinType DoubleType;
    DoubleType.PinCategory = UEdGraphSchema_K2::PC_Real;
    DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    UEdGraph* StabilizationGraph = FBlueprintEditorUtils::FindEventGraph(StabilizationBlueprint);
    if (!StabilizationGraph || !AddSettingMembers(StabilizationBlueprint, false) ||
        !AddSettingsFunction(StabilizationBlueprint, false))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate StableGyro stabilization helper"));
        return 1;
    }
    if (StabilizationBlueprint->Status == BS_Error ||
        !AddBeginPlayGraph(StabilizationGraph, StabilizationBlueprint, false) ||
        !SaveBlueprint(StabilizationPackageObject, StabilizationBlueprint)) return 1;

    UPackage* PitchPackageObject = CreatePackage(PitchInputPackage);
    UBlueprint* PitchBlueprint = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(), PitchPackageObject, FName(PitchInputAsset), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        FName(PitchGeneratorName));
    if (!PitchBlueprint || !AddSettingMembers(PitchBlueprint, true) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            PitchBlueprint, IntegratedPitch, DoubleType, Neutral))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add Gyro pitch state"));
        return 1;
    }
    UEdGraph* PitchGraph = FBlueprintEditorUtils::FindEventGraph(PitchBlueprint);
    if (!PitchGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate Gyro pitch settings reload"));
        return 1;
    }
    if (!AddSettingsFunction(PitchBlueprint, true)) return 1;
    if (PitchBlueprint->Status == BS_Error ||
        !AddBeginPlayGraph(PitchGraph, PitchBlueprint, true) ||
        !AddTickGraph(PitchGraph) ||
        !SaveBlueprint(PitchPackageObject, PitchBlueprint, TG_PrePhysics))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate Gyro pitch helper"));
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("Generated %s and %s"),
        StabilizationPackage, PitchInputPackage);
    return 0;
}

#endif
