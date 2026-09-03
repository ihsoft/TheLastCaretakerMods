// HAND-WRITTEN BUILD TOOL SOURCE: generates the forklift replacement Blueprint
// from version-bound Voyage contracts. The C++ tool itself is never shipped.
// Contracts validated against Steam build 25056839, exe SHA-256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Revalidate per ../../GAME_DERIVED_SOURCES.md after a game update.

#include "GenerateDonkLiftInheritanceCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "DonkLiftAssetNames.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Components/ChildActorComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameUserSettings.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "VoyageGameUserSettings.h"
#include "VoyageInputAction.h"
#include "VoyageVehicleForkliftPawn.h"
#include "VoyageVehiclePawn.h"

namespace
{
constexpr TCHAR BasePackageName[] = TEXT("/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original");
constexpr TCHAR ChildPackageName[] = TEXT("/Game/Blueprints/Vehicles/BP_Forklift_Possesable");
constexpr TCHAR ForkliftAssetName[] = TEXT("BP_Forklift_Possesable");
constexpr TCHAR HelperObjectPath[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/ModActor.ModActor");
const FName BaseBlueprintName(TEXT("GenerateDonkLiftInheritanceBase"));
const FName ChildBlueprintName(TEXT("GenerateDonkLiftInheritanceChild"));
const FName HelperComponentName(TEXT("DonkLiftKeyboardHelper"));
const FName ProvidedActionsFunctionName(TEXT("GetProvidedActionsBP"));

namespace PinNames
{
using BlueprintGraphNames::Pins::FirstArrayElement;
using BlueprintGraphNames::Pins::ReturnValue;
using BlueprintGraphNames::Pins::SecondArrayElement;
const FName InputAction(TEXT("InputAction"));
const FName Name(TEXT("Name"));
const FName Category(TEXT("Category"));
const FName Text(TEXT("Text"));
const FName Enabled(TEXT("bEnabled"));
const FName Priority(TEXT("Priority"));
const FName Type(TEXT("Type"));
const FName LanguageType(TEXT("LanguageType"));
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
        if (Pin && Pin->Direction == EGPD_Output)
        {
            return Pin;
        }
    }
    return nullptr;
}

UEdGraphPin* FindInputPin(UEdGraphNode* Node)
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input)
        {
            return Pin;
        }
    }
    return nullptr;
}

UK2Node_MakeStruct* AddProvidedAction(
    UEdGraph* Graph,
    UInputAction* Action,
    const TCHAR* Name,
    const TCHAR* Label,
    int32 Y)
{
    UK2Node_MakeStruct* Node = NewObject<UK2Node_MakeStruct>(Graph);
    Node->StructType = FPlayerInputInterfaceAction::StaticStruct();
    Node->bMadeAfterOverridePinRemoval = true;
    FinishNode(Node, Graph, 300, Y);

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    Schema->TrySetDefaultObject(*RequirePin(Node, PinNames::InputAction), Action);
    Schema->TrySetDefaultValue(*RequirePin(Node, PinNames::Name), Name);
    Schema->TrySetDefaultValue(*RequirePin(Node, PinNames::Category), DonkLiftAssetNames::VehicleInputCategory);
    Schema->TrySetDefaultText(*RequirePin(Node, PinNames::Text), FText::FromString(Label));
    Schema->TrySetDefaultValue(*RequirePin(Node, PinNames::Enabled), ProvidedActionDefaults::Enabled);
    Schema->TrySetDefaultValue(*RequirePin(Node, PinNames::Priority), ProvidedActionDefaults::Priority);
    Schema->TrySetDefaultValue(
        *RequirePin(Node, PinNames::Type),
        ProvidedActionDefaults::Type);
    return Node;
}

bool AddProvidedActionsBpOverride(
    UBlueprint* Blueprint,
    UInputAction* BrakeAction,
    UInputAction* CenterAction)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        ProvidedActionsFunctionName,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint,
        Graph,
        false,
        AVoyageVehiclePawn::StaticClass());

    UK2Node_FunctionResult* Result = nullptr;
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Node))
        {
            Entry = Candidate;
        }
        if (UK2Node_FunctionResult* Candidate = Cast<UK2Node_FunctionResult>(Node))
        {
            Result = Candidate;
        }
    }
    if (!Entry || !Result)
    {
        UE_LOG(LogTemp, Error, TEXT("GetProvidedActionsBP entry/result nodes were not generated"));
        return false;
    }

    UEdGraphPin* EntryThen = RequirePin(Entry, UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* ResultExec = RequirePin(Result, UEdGraphSchema_K2::PN_Execute);
    EntryThen->BreakAllPinLinks();
    ResultExec->BreakAllPinLinks();
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

    UK2Node_CallFunction* GetSettings = NewObject<UK2Node_CallFunction>(Graph);
    GetSettings->SetFromFunction(UGameUserSettings::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameUserSettings, GetGameUserSettings)));
    FinishNode(GetSettings, Graph, -900, -240);

    if (!Schema->TryCreateConnection(
            EntryThen,
            RequirePin(GetSettings, UEdGraphSchema_K2::PN_Execute)) ||
        !Schema->TryCreateConnection(
            RequirePin(GetSettings, UEdGraphSchema_K2::PN_Then),
            ResultExec))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect GetProvidedActionsBP execution pins"));
        return false;
    }

    UK2Node_DynamicCast* CastSettings = NewObject<UK2Node_DynamicCast>(Graph);
    CastSettings->TargetType = UVoyageGameUserSettings::StaticClass();
    FinishNode(CastSettings, Graph, -660, 260);
    CastSettings->SetPurity(true);

    UK2Node_VariableGet* GetCustomSettings = NewObject<UK2Node_VariableGet>(Graph);
    GetCustomSettings->VariableReference.SetExternalMember(
        GET_MEMBER_NAME_CHECKED(UVoyageGameUserSettings, CustomSettings),
        UVoyageGameUserSettings::StaticClass());
    FinishNode(GetCustomSettings, Graph, -420, 260);

    UK2Node_BreakStruct* BreakSettings = NewObject<UK2Node_BreakStruct>(Graph);
    BreakSettings->StructType = FVoyageCustomGameUserSettings::StaticStruct();
    BreakSettings->bMadeAfterOverridePinRemoval = true;
    FinishNode(BreakSettings, Graph, -180, 260);

    UK2Node_EnumEquality* IsRussian = NewObject<UK2Node_EnumEquality>(Graph);
    FinishNode(IsRussian, Graph, 40, 260);

    UK2Node_CallFunction* SelectBrakeText = NewObject<UK2Node_CallFunction>(Graph);
    SelectBrakeText->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectText)));
    FinishNode(SelectBrakeText, Graph, 260, -120);
    Schema->TrySetDefaultText(
        *RequirePin(SelectBrakeText, SelectPins::WhenTrue),
        FText::FromString(DonkLiftAssetNames::BrakeRussianLabel));
    Schema->TrySetDefaultText(
        *RequirePin(SelectBrakeText, SelectPins::WhenFalse),
        FText::FromString(DonkLiftAssetNames::BrakeDisplayLabel));

    UK2Node_CallFunction* SelectCenterText = NewObject<UK2Node_CallFunction>(Graph);
    SelectCenterText->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectText)));
    FinishNode(SelectCenterText, Graph, 260, 500);
    Schema->TrySetDefaultText(
        *RequirePin(SelectCenterText, SelectPins::WhenTrue),
        FText::FromString(DonkLiftAssetNames::CenterRussianLabel));
    Schema->TrySetDefaultText(
        *RequirePin(SelectCenterText, SelectPins::WhenFalse),
        FText::FromString(DonkLiftAssetNames::CenterDisplayLabel));

    UK2Node_MakeStruct* Brake = AddProvidedAction(
        Graph,
        BrakeAction,
        DonkLiftAssetNames::BrakeMappingName,
        DonkLiftAssetNames::BrakeDisplayLabel,
        0);
    UK2Node_MakeStruct* Center = AddProvidedAction(
        Graph,
        CenterAction,
        DonkLiftAssetNames::CenterMappingName,
        DonkLiftAssetNames::CenterDisplayLabel,
        360);

    UK2Node_MakeArray* Actions = NewObject<UK2Node_MakeArray>(Graph);
    FinishNode(Actions, Graph, 720, 180);
    Actions->AddInputPin();

    UEdGraphPin* ReturnValue = RequirePin(Result, PinNames::ReturnValue);
    ReturnValue->BreakAllPinLinks();

    UEdGraphPin* BrakeOutput = FindOutputPin(Brake);
    UEdGraphPin* CenterOutput = FindOutputPin(Center);
    UEdGraphPin* BreakSettingsInput = FindInputPin(BreakSettings);
    if (!Schema->TryCreateConnection(
            RequirePin(BreakSettings, PinNames::LanguageType),
            IsRussian->GetInput1Pin()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect the Voyage language enum"));
        return false;
    }
    Schema->TrySetDefaultValue(
        *IsRussian->GetInput2Pin(),
        ProvidedActionDefaults::RussianLanguage);

    if (!BrakeOutput || !CenterOutput || !BreakSettingsInput ||
        !Schema->TryCreateConnection(
            RequirePin(GetSettings, PinNames::ReturnValue),
            CastSettings->GetCastSourcePin()) ||
        !Schema->TryCreateConnection(
            CastSettings->GetCastResultPin(),
            RequirePin(GetCustomSettings, UEdGraphSchema_K2::PN_Self)) ||
        !Schema->TryCreateConnection(
            GetCustomSettings->GetValuePin(),
            BreakSettingsInput) ||
        !Schema->TryCreateConnection(
            IsRussian->GetReturnValuePin(),
            RequirePin(SelectBrakeText, SelectPins::Condition)) ||
        !Schema->TryCreateConnection(
            IsRussian->GetReturnValuePin(),
            RequirePin(SelectCenterText, SelectPins::Condition)) ||
        !Schema->TryCreateConnection(
            RequirePin(SelectBrakeText, PinNames::ReturnValue),
            RequirePin(Brake, PinNames::Text)) ||
        !Schema->TryCreateConnection(
            RequirePin(SelectCenterText, PinNames::ReturnValue),
            RequirePin(Center, PinNames::Text)) ||
        !Schema->TryCreateConnection(BrakeOutput, RequirePin(Actions, PinNames::FirstArrayElement)) ||
        !Schema->TryCreateConnection(CenterOutput, RequirePin(Actions, PinNames::SecondArrayElement)) ||
        !Schema->TryCreateConnection(Actions->GetOutputPin(), ReturnValue))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect GetProvidedActionsBP data pins"));
        return false;
    }
    return true;
}

bool SaveBlueprint(UPackage* Package, UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Compilation failed for %s"), *Blueprint->GetPathName());
        return false;
    }

    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(),
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
}
}

UGenerateDonkLiftInheritanceCommandlet::UGenerateDonkLiftInheritanceCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateDonkLiftInheritanceCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(BasePackageName) ||
        FPackageName::DoesPackageExist(ChildPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Inheritance experiment assets already exist"));
        return 1;
    }

    UBlueprint* Helper = LoadObject<UBlueprint>(nullptr, HelperObjectPath);
    if (!Helper || !Helper->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Helper Blueprint was not found: %s"), HelperObjectPath);
        return 1;
    }

    UVoyageInputAction* BrakeAction = LoadObject<UVoyageInputAction>(nullptr, DonkLiftAssetNames::BrakeActionObjectPath);
    UVoyageInputAction* CenterAction = LoadObject<UVoyageInputAction>(nullptr, DonkLiftAssetNames::CenterActionObjectPath);
    if (!BrakeAction || !CenterAction)
    {
        UE_LOG(LogTemp, Error, TEXT("DonkLift input actions were not found"));
        return 1;
    }

    UPackage* BasePackage = CreatePackage(BasePackageName);
    UBlueprint* BaseBlueprint = FKismetEditorUtilities::CreateBlueprint(
        AVoyageVehicleForkliftPawn::StaticClass(),
        BasePackage,
        FName(ForkliftAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        BaseBlueprintName);
    if (!BaseBlueprint || !SaveBlueprint(BasePackage, BaseBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the temporary parent Blueprint"));
        return 1;
    }

    UPackage* ChildPackage = CreatePackage(ChildPackageName);
    UBlueprint* ChildBlueprint = FKismetEditorUtilities::CreateBlueprint(
        BaseBlueprint->GeneratedClass,
        ChildPackage,
        FName(ForkliftAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        ChildBlueprintName);
    if (!ChildBlueprint || !ChildBlueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the child Blueprint"));
        return 1;
    }

    USCS_Node* HelperNode = ChildBlueprint->SimpleConstructionScript->CreateNode(
        UChildActorComponent::StaticClass(),
        HelperComponentName);
    UChildActorComponent* HelperTemplate = Cast<UChildActorComponent>(HelperNode->ComponentTemplate);
    if (!HelperTemplate)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the helper component template"));
        return 1;
    }
    UClass* HelperClass = Helper->GeneratedClass.Get();
    if (!HelperClass->IsChildOf(AActor::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("Helper Blueprint is not an Actor class: %s"), *HelperClass->GetPathName());
        return 1;
    }
    HelperTemplate->SetChildActorClass(TSubclassOf<AActor>(HelperClass));
    ChildBlueprint->SimpleConstructionScript->AddNode(HelperNode);

    if (!AddProvidedActionsBpOverride(ChildBlueprint, BrakeAction, CenterAction))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add standard provided-action hints"));
        return 1;
    }

    if (!SaveBlueprint(ChildPackage, ChildBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save the child Blueprint"));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated temporary parent: %s.%s"),
        BasePackageName, ForkliftAssetName);
    UE_LOG(LogTemp, Display, TEXT("Generated child override: %s.%s"),
        ChildPackageName, ForkliftAssetName);
    UE_LOG(LogTemp, Display, TEXT("Child helper class: %s"), *Helper->GeneratedClass->GetPathName());
    return 0;
}

#endif
