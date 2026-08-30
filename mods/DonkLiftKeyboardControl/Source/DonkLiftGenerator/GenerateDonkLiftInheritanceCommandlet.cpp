// HAND-WRITTEN BUILD TOOL SOURCE: generates the forklift replacement Blueprint
// from version-bound Voyage contracts. The C++ tool itself is never shipped.
// Contracts validated against Steam build 23962331, exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D.
// Revalidate per ../../GAME_DERIVED_SOURCES.md after a game update.

#include "GenerateDonkLiftInheritanceCommandlet.h"

#if WITH_EDITOR

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
constexpr TCHAR BrakeActionObjectPath[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/IAV_DonkLiftBrake.IAV_DonkLiftBrake");
constexpr TCHAR CenterActionObjectPath[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/IAV_DonkLiftCenterSteering.IAV_DonkLiftCenterSteering");

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
    Schema->TrySetDefaultObject(*RequirePin(Node, FName(TEXT("InputAction"))), Action);
    Schema->TrySetDefaultValue(*RequirePin(Node, FName(TEXT("Name"))), Name);
    Schema->TrySetDefaultValue(*RequirePin(Node, FName(TEXT("Category"))), TEXT("Vehicle"));
    Schema->TrySetDefaultText(*RequirePin(Node, FName(TEXT("Text"))), FText::FromString(Label));
    Schema->TrySetDefaultValue(*RequirePin(Node, FName(TEXT("bEnabled"))), TEXT("true"));
    Schema->TrySetDefaultValue(*RequirePin(Node, FName(TEXT("Priority"))), TEXT("10"));
    Schema->TrySetDefaultValue(
        *RequirePin(Node, FName(TEXT("Type"))),
        TEXT("EPlayerInputInterfaceActionType::Central"));
    return Node;
}

bool AddProvidedActionsBpOverride(
    UBlueprint* Blueprint,
    UInputAction* BrakeAction,
    UInputAction* CenterAction)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        FName(TEXT("GetProvidedActionsBP")),
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
        *RequirePin(SelectBrakeText, FName(TEXT("A"))),
        FText::FromString(TEXT("Тормоз")));
    Schema->TrySetDefaultText(
        *RequirePin(SelectBrakeText, FName(TEXT("B"))),
        FText::FromString(TEXT("Brake")));

    UK2Node_CallFunction* SelectCenterText = NewObject<UK2Node_CallFunction>(Graph);
    SelectCenterText->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectText)));
    FinishNode(SelectCenterText, Graph, 260, 500);
    Schema->TrySetDefaultText(
        *RequirePin(SelectCenterText, FName(TEXT("A"))),
        FText::FromString(TEXT("Выровнять")));
    Schema->TrySetDefaultText(
        *RequirePin(SelectCenterText, FName(TEXT("B"))),
        FText::FromString(TEXT("Center")));

    UK2Node_MakeStruct* Brake = AddProvidedAction(
        Graph,
        BrakeAction,
        TEXT("DonkLiftBrake"),
        TEXT("Brake"),
        0);
    UK2Node_MakeStruct* Center = AddProvidedAction(
        Graph,
        CenterAction,
        TEXT("DonkLiftCenterSteering"),
        TEXT("Center"),
        360);

    UK2Node_MakeArray* Actions = NewObject<UK2Node_MakeArray>(Graph);
    FinishNode(Actions, Graph, 720, 180);
    Actions->AddInputPin();

    UEdGraphPin* ReturnValue = RequirePin(Result, FName(TEXT("ReturnValue")));
    ReturnValue->BreakAllPinLinks();

    UEdGraphPin* BrakeOutput = FindOutputPin(Brake);
    UEdGraphPin* CenterOutput = FindOutputPin(Center);
    UEdGraphPin* BreakSettingsInput = FindInputPin(BreakSettings);
    if (!Schema->TryCreateConnection(
            RequirePin(BreakSettings, FName(TEXT("LanguageType"))),
            IsRussian->GetInput1Pin()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect the Voyage language enum"));
        return false;
    }
    Schema->TrySetDefaultValue(
        *IsRussian->GetInput2Pin(),
        TEXT("EVoyageLanguageType::Russian"));

    if (!BrakeOutput || !CenterOutput || !BreakSettingsInput ||
        !Schema->TryCreateConnection(
            RequirePin(GetSettings, FName(TEXT("ReturnValue"))),
            CastSettings->GetCastSourcePin()) ||
        !Schema->TryCreateConnection(
            CastSettings->GetCastResultPin(),
            RequirePin(GetCustomSettings, UEdGraphSchema_K2::PN_Self)) ||
        !Schema->TryCreateConnection(
            GetCustomSettings->GetValuePin(),
            BreakSettingsInput) ||
        !Schema->TryCreateConnection(
            IsRussian->GetReturnValuePin(),
            RequirePin(SelectBrakeText, FName(TEXT("bPickA")))) ||
        !Schema->TryCreateConnection(
            IsRussian->GetReturnValuePin(),
            RequirePin(SelectCenterText, FName(TEXT("bPickA")))) ||
        !Schema->TryCreateConnection(
            RequirePin(SelectBrakeText, FName(TEXT("ReturnValue"))),
            RequirePin(Brake, FName(TEXT("Text")))) ||
        !Schema->TryCreateConnection(
            RequirePin(SelectCenterText, FName(TEXT("ReturnValue"))),
            RequirePin(Center, FName(TEXT("Text")))) ||
        !Schema->TryCreateConnection(BrakeOutput, RequirePin(Actions, FName(TEXT("[0]")))) ||
        !Schema->TryCreateConnection(CenterOutput, RequirePin(Actions, FName(TEXT("[1]")))) ||
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

    UVoyageInputAction* BrakeAction = LoadObject<UVoyageInputAction>(nullptr, BrakeActionObjectPath);
    UVoyageInputAction* CenterAction = LoadObject<UVoyageInputAction>(nullptr, CenterActionObjectPath);
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
        FName(TEXT("GenerateDonkLiftInheritanceBase")));
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
        FName(TEXT("GenerateDonkLiftInheritanceChild")));
    if (!ChildBlueprint || !ChildBlueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the child Blueprint"));
        return 1;
    }

    USCS_Node* HelperNode = ChildBlueprint->SimpleConstructionScript->CreateNode(
        UChildActorComponent::StaticClass(),
        FName(TEXT("DonkLiftKeyboardHelper")));
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
