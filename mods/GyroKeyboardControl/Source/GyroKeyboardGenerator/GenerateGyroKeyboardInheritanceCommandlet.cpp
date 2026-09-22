// HAND-WRITTEN BUILD TOOL SOURCE: generates the Gyro replacement child.
// Contracts validated for Steam build 25191271 / executable SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// The generator is editor-only and never ships in the mod container.

#include "GenerateGyroKeyboardInheritanceCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "Components/ChildActorComponent.h"
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
#include "K2Node_EnumEquality.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "GyroKeyboardAssetNames.h"
#include "VoyageGameUserSettings.h"
#include "VoyageInputAction.h"
#include "VoyageVehicleGyroCopter.h"
#include "VoyageVehiclePawn.h"

namespace
{
constexpr TCHAR BasePackage[] = TEXT("/Game/Blueprints/Vehicles/BP_GyroCopter_Possessable");
constexpr TCHAR ChildPackage[] = TEXT("/Game/Mods/GyroKeyboardBP/BP_GyroCopter_KeyboardCtl");
constexpr TCHAR BaseAssetName[] = TEXT("BP_GyroCopter_Possessable");
constexpr TCHAR ChildAssetName[] = TEXT("BP_GyroCopter_KeyboardCtl");
constexpr TCHAR HelperPath[] = TEXT("/Game/Mods/GyroKeyboardControl/ModActor.ModActor");
const FName BaseBlueprintName(TEXT("GenerateGyroKeyboardInheritanceBase"));
const FName ChildBlueprintName(TEXT("GenerateGyroKeyboardInheritanceChild"));
const FName HelperComponentName(TEXT("GyroKeyboardControlHelper"));
const FName ProvidedActionsFunctionName(TEXT("GetProvidedActionsBP"));

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
    if (!Entry || !Result) return false;
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
        FText::FromString(GyroKeyboardAssetNames::ResetRussianLabel));
    Schema->TrySetDefaultText(*RequirePin(SelectText, SelectPins::WhenFalse),
        FText::FromString(GyroKeyboardAssetNames::ResetDisplayLabel));

    UK2Node_MakeStruct* Reset = NewObject<UK2Node_MakeStruct>(Graph);
    Reset->StructType = FPlayerInputInterfaceAction::StaticStruct();
    Reset->bMadeAfterOverridePinRemoval = true;
    FinishNode(Reset, Graph, 520, 80);
    Schema->TrySetDefaultObject(*RequirePin(Reset, PinNames::InputAction), ResetAction);
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Name),
        GyroKeyboardAssetNames::ResetMappingName);
    Schema->TrySetDefaultValue(*RequirePin(Reset, PinNames::Category),
        GyroKeyboardAssetNames::VehicleInputCategory);
    Schema->TrySetDefaultText(*RequirePin(Reset, PinNames::Text),
        FText::FromString(GyroKeyboardAssetNames::ResetDisplayLabel));
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

bool SaveBlueprint(UPackage* Package, UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) return false;
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

UGenerateGyroKeyboardInheritanceCommandlet::UGenerateGyroKeyboardInheritanceCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateGyroKeyboardInheritanceCommandlet::Main(const FString& Params)
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
        nullptr, GyroKeyboardAssetNames::ResetActionObjectPath);
    if (!ResetAction) return 1;

    UPackage* ParentPackage = CreatePackage(BasePackage);
    UBlueprint* Parent = FKismetEditorUtilities::CreateBlueprint(
        AVoyageVehicleGyroCopter::StaticClass(), ParentPackage, FName(BaseAssetName),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        BaseBlueprintName);
    if (!Parent || !Parent->SimpleConstructionScript) return 1;
    if (!AddEmptyProvidedActionsOverride(Parent)) return 1;
    if (!SaveBlueprint(ParentPackage, Parent)) return 1;

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
