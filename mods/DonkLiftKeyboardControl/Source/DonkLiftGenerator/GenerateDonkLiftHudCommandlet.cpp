// HAND-WRITTEN BUILD TOOL SOURCE: generates the standard-HUD replacement from
// version-bound Voyage contracts. The C++ tool itself is never shipped.
// Contracts validated against Steam build 23962331, exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D.
// Revalidate per ../../GAME_DERIVED_SOURCES.md after a game update.

#include "GenerateDonkLiftHudCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "DonkLiftAssetNames.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Containers/UnrealString.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "InteractIndicator.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "VoyageButtonInfoContainerWidget.h"
#include "VoyageDynamicPlayerInputWidget.h"
#include "VoyageIngameForkliftWidget.h"
#include "WidgetBlueprint.h"

namespace
{
constexpr TCHAR ParentPackageName[] = TEXT("/Game/Mods/DonkLift/HUD_Forklift_Original");
constexpr TCHAR ChildPackageName[] = TEXT("/Game/UI/Game/HUD/BP_VoyageIngameForklift");
constexpr TCHAR AssetName[] = TEXT("BP_VoyageIngameForklift");
constexpr TCHAR BottomWidgetName[] = TEXT("BP_DynamicPlayerInputHorizontalWidget_Bottom");
constexpr TCHAR BrakeWidgetName[] = TEXT("DonkLiftBrakeWidget");
constexpr TCHAR CenterWidgetName[] = TEXT("DonkLiftCenterWidget");
constexpr TCHAR ParentBlueprintName[] = TEXT("GenerateDonkLiftHudParent");
constexpr TCHAR ChildBlueprintName[] = TEXT("GenerateDonkLiftHudChild");
namespace PinNames = BlueprintGraphNames::Pins;
namespace BinaryPins = BlueprintGraphNames::Pins::Binary;

namespace HudDefaults
{
constexpr double ReorderDelaySeconds = 0.25;
constexpr int32 LastIndexOffset = 1;
constexpr int32 FirstIndex = 0;
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

bool SaveWidgetBlueprint(UPackage* Package, UWidgetBlueprint* Blueprint)
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

UGenerateDonkLiftHudCommandlet::UGenerateDonkLiftHudCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateDonkLiftHudCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(ParentPackageName) ||
        FPackageName::DoesPackageExist(ChildPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("DonkLift HUD assets already exist"));
        return 1;
    }

    UPackage* ParentPackage = CreatePackage(ParentPackageName);
    UWidgetBlueprint* ParentBlueprint = CastChecked<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            UVoyageIngameForkliftWidget::StaticClass(),
            ParentPackage,
            FName(AssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            FName(ParentBlueprintName)));

    FEdGraphPinType BottomWidgetType;
    BottomWidgetType.PinCategory = UEdGraphSchema_K2::PC_Object;
    BottomWidgetType.PinSubCategoryObject = UVoyageDynamicPlayerInputWidget::StaticClass();
    if (!FBlueprintEditorUtils::AddMemberVariable(
            ParentBlueprint,
            FName(BottomWidgetName),
            BottomWidgetType))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add the bottom input widget placeholder"));
        return 1;
    }

    if (!SaveWidgetBlueprint(ParentPackage, ParentBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the temporary HUD parent"));
        return 1;
    }

    UPackage* ChildPackage = CreatePackage(ChildPackageName);
    UWidgetBlueprint* ChildBlueprint = CastChecked<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            ParentBlueprint->GeneratedClass,
            ChildPackage,
            FName(AssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            FName(ChildBlueprintName)));

    FEdGraphPinType ActionWidgetType;
    ActionWidgetType.PinCategory = UEdGraphSchema_K2::PC_Object;
    ActionWidgetType.PinSubCategoryObject = UWidget::StaticClass();
    if (!FBlueprintEditorUtils::AddMemberVariable(
            ChildBlueprint,
            FName(BrakeWidgetName),
            ActionWidgetType) ||
        !FBlueprintEditorUtils::AddMemberVariable(
            ChildBlueprint,
            FName(CenterWidgetName),
            ActionWidgetType))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add DonkLift action widget references"));
        return 1;
    }

    UInputAction* BrakeAction = LoadObject<UInputAction>(nullptr, DonkLiftAssetNames::BrakeActionObjectPath);
    UInputAction* CenterAction = LoadObject<UInputAction>(nullptr, DonkLiftAssetNames::CenterActionObjectPath);
    if (!BrakeAction || !CenterAction)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load DonkLift input actions"));
        return 1;
    }

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(ChildBlueprint);
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("HUD child has no event graph"));
        return 1;
    }

    UK2Node_Event* Construct = NewObject<UK2Node_Event>(Graph);
    Construct->EventReference.SetExternalMember(BlueprintGraphNames::Events::WidgetConstruct, UUserWidget::StaticClass());
    Construct->bOverrideFunction = true;
    FinishNode(Construct, Graph, 0, 0);

    UK2Node_CallFunction* Delay = NewObject<UK2Node_CallFunction>(Graph);
    Delay->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Delay)));
    FinishNode(Delay, Graph, 240, 0);
    RequirePin(Delay, PinNames::Duration)->DefaultValue = LexToString(HudDefaults::ReorderDelaySeconds);

    UK2Node_VariableGet* GetBottom = NewObject<UK2Node_VariableGet>(Graph);
    GetBottom->VariableReference.SetSelfMember(FName(BottomWidgetName));
    FinishNode(GetBottom, Graph, 520, 180);

    UK2Node_VariableGet* GetActionsRoot = NewObject<UK2Node_VariableGet>(Graph);
    GetActionsRoot->VariableReference.SetExternalMember(
        GET_MEMBER_NAME_CHECKED(UVoyageDynamicPlayerInputWidget, ContextInputActionsRoot),
        UVoyageDynamicPlayerInputWidget::StaticClass());
    FinishNode(GetActionsRoot, Graph, 800, 180);

    UK2Node_CallFunction* GetChildrenCount = NewObject<UK2Node_CallFunction>(Graph);
    GetChildrenCount->SetFromFunction(UPanelWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UPanelWidget, GetChildrenCount)));
    FinishNode(GetChildrenCount, Graph, 1040, 180);

    UK2Node_CallFunction* LastIndex = NewObject<UK2Node_CallFunction>(Graph);
    LastIndex->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt)));
    FinishNode(LastIndex, Graph, 1260, 180);
    RequirePin(LastIndex, BinaryPins::RightOperand)->DefaultValue = LexToString(HudDefaults::LastIndexOffset);

    UBlueprint* StandardMacros = LoadObject<UBlueprint>(
        nullptr,
        BlueprintGraphNames::EngineAssets::StandardMacrosObjectPath);
    UEdGraph* ForLoopGraph = nullptr;
    if (StandardMacros)
    {
        const TObjectPtr<UEdGraph>* Match = StandardMacros->MacroGraphs.FindByPredicate([](const UEdGraph* Candidate)
        {
            return Candidate && Candidate->GetFName() == BlueprintGraphNames::Macros::ForLoop;
        });
        ForLoopGraph = Match ? *Match : nullptr;
    }
    if (!ForLoopGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load the standard ForLoop macro"));
        return 1;
    }

    UK2Node_MacroInstance* ForLoop = NewObject<UK2Node_MacroInstance>(Graph);
    ForLoop->SetMacroGraph(ForLoopGraph);
    FinishNode(ForLoop, Graph, 1500, 0);
    RequirePin(ForLoop, PinNames::FirstIndex)->DefaultValue = LexToString(HudDefaults::FirstIndex);

    UK2Node_CallFunction* GetChild = NewObject<UK2Node_CallFunction>(Graph);
    GetChild->SetFromFunction(UPanelWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UPanelWidget, GetChildAt)));
    FinishNode(GetChild, Graph, 1760, 180);

    UK2Node_DynamicCast* CastIndicator = NewObject<UK2Node_DynamicCast>(Graph);
    CastIndicator->TargetType = UInteractIndicator::StaticClass();
    FinishNode(CastIndicator, Graph, 2000, 0);
    CastIndicator->SetPurity(false);

    UK2Node_VariableGet* GetButtonInfoContainer = NewObject<UK2Node_VariableGet>(Graph);
    GetButtonInfoContainer->VariableReference.SetExternalMember(
        GET_MEMBER_NAME_CHECKED(UInteractIndicator, ButtonInfoContainer),
        UInteractIndicator::StaticClass());
    FinishNode(GetButtonInfoContainer, Graph, 2240, 260);

    UK2Node_VariableGet* GetInputActions = NewObject<UK2Node_VariableGet>(Graph);
    GetInputActions->VariableReference.SetExternalMember(
        GET_MEMBER_NAME_CHECKED(UVoyageButtonInfoContainerWidget, InputActions),
        UVoyageButtonInfoContainerWidget::StaticClass());
    FinishNode(GetInputActions, Graph, 2480, 260);

    UK2Node_CallArrayFunction* ContainsBrake = NewObject<UK2Node_CallArrayFunction>(Graph);
    ContainsBrake->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Contains)));
    FinishNode(ContainsBrake, Graph, 2720, 180);

    UK2Node_CallArrayFunction* ContainsCenter = NewObject<UK2Node_CallArrayFunction>(Graph);
    ContainsCenter->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Contains)));
    FinishNode(ContainsCenter, Graph, 2720, 420);

    UK2Node_IfThenElse* IsBrake = NewObject<UK2Node_IfThenElse>(Graph);
    FinishNode(IsBrake, Graph, 3000, 0);

    UK2Node_VariableSet* SaveBrake = NewObject<UK2Node_VariableSet>(Graph);
    SaveBrake->VariableReference.SetSelfMember(FName(BrakeWidgetName));
    FinishNode(SaveBrake, Graph, 3260, -80);

    UK2Node_IfThenElse* IsCenter = NewObject<UK2Node_IfThenElse>(Graph);
    FinishNode(IsCenter, Graph, 3260, 160);

    UK2Node_VariableSet* SaveCenter = NewObject<UK2Node_VariableSet>(Graph);
    SaveCenter->VariableReference.SetSelfMember(FName(CenterWidgetName));
    FinishNode(SaveCenter, Graph, 3520, 160);

    UK2Node_VariableGet* GetBrake = NewObject<UK2Node_VariableGet>(Graph);
    GetBrake->VariableReference.SetSelfMember(FName(BrakeWidgetName));
    FinishNode(GetBrake, Graph, 2020, 740);

    UK2Node_CallFunction* RemoveBrake = NewObject<UK2Node_CallFunction>(Graph);
    RemoveBrake->SetFromFunction(UPanelWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UPanelWidget, RemoveChild)));
    FinishNode(RemoveBrake, Graph, 2020, 560);

    UK2Node_IfThenElse* BrakeRemoved = NewObject<UK2Node_IfThenElse>(Graph);
    FinishNode(BrakeRemoved, Graph, 2260, 560);

    UK2Node_CallFunction* AppendBrake = NewObject<UK2Node_CallFunction>(Graph);
    AppendBrake->SetFromFunction(UPanelWidget::StaticClass()->FindFunctionByName(
        BlueprintGraphNames::WidgetFunctions::AddChild));
    FinishNode(AppendBrake, Graph, 2500, 520);

    UK2Node_VariableGet* GetCenter = NewObject<UK2Node_VariableGet>(Graph);
    GetCenter->VariableReference.SetSelfMember(FName(CenterWidgetName));
    FinishNode(GetCenter, Graph, 2740, 780);

    UK2Node_CallFunction* RemoveCenter = NewObject<UK2Node_CallFunction>(Graph);
    RemoveCenter->SetFromFunction(UPanelWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UPanelWidget, RemoveChild)));
    FinishNode(RemoveCenter, Graph, 2740, 600);

    UK2Node_IfThenElse* CenterRemoved = NewObject<UK2Node_IfThenElse>(Graph);
    FinishNode(CenterRemoved, Graph, 2980, 600);

    UK2Node_CallFunction* AppendCenter = NewObject<UK2Node_CallFunction>(Graph);
    AppendCenter->SetFromFunction(UPanelWidget::StaticClass()->FindFunctionByName(
        BlueprintGraphNames::WidgetFunctions::AddChild));
    FinishNode(AppendCenter, Graph, 3220, 600);

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    bool Connected = true;
    Connected &= Schema->TryCreateConnection(
        RequirePin(Construct, UEdGraphSchema_K2::PN_Then),
        RequirePin(Delay, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(Delay, UEdGraphSchema_K2::PN_Then),
        RequirePin(ForLoop, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        GetBottom->GetValuePin(),
        RequirePin(GetActionsRoot, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetActionsRoot->GetValuePin(),
        RequirePin(GetChildrenCount, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        RequirePin(GetChildrenCount, PinNames::ReturnValue),
        RequirePin(LastIndex, BinaryPins::LeftOperand));
    Connected &= Schema->TryCreateConnection(
        RequirePin(LastIndex, PinNames::ReturnValue),
        RequirePin(ForLoop, PinNames::LastIndex));
    Connected &= Schema->TryCreateConnection(
        GetActionsRoot->GetValuePin(),
        RequirePin(GetChild, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        RequirePin(ForLoop, PinNames::Index),
        RequirePin(GetChild, PinNames::Index));
    Connected &= Schema->TryCreateConnection(
        RequirePin(ForLoop, PinNames::LoopBody),
        CastIndicator->GetExecPin());
    Connected &= Schema->TryCreateConnection(
        RequirePin(GetChild, PinNames::ReturnValue),
        CastIndicator->GetCastSourcePin());
    Connected &= Schema->TryCreateConnection(
        CastIndicator->GetCastResultPin(),
        RequirePin(GetButtonInfoContainer, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetButtonInfoContainer->GetValuePin(),
        RequirePin(GetInputActions, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetInputActions->GetValuePin(),
        RequirePin(ContainsBrake, PinNames::TargetArray));
    Connected &= Schema->TryCreateConnection(
        GetInputActions->GetValuePin(),
        RequirePin(ContainsCenter, PinNames::TargetArray));
    Schema->TrySetDefaultObject(*RequirePin(ContainsBrake, PinNames::ItemToFind), BrakeAction);
    Schema->TrySetDefaultObject(*RequirePin(ContainsCenter, PinNames::ItemToFind), CenterAction);
    Connected &= Schema->TryCreateConnection(
        CastIndicator->GetValidCastPin(),
        RequirePin(IsBrake, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(ContainsBrake, PinNames::ReturnValue),
        RequirePin(IsBrake, UEdGraphSchema_K2::PN_Condition));
    Connected &= Schema->TryCreateConnection(
        RequirePin(IsBrake, UEdGraphSchema_K2::PN_Then),
        RequirePin(SaveBrake, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(GetChild, PinNames::ReturnValue),
        RequirePin(SaveBrake, FName(BrakeWidgetName)));
    Connected &= Schema->TryCreateConnection(
        RequirePin(IsBrake, UEdGraphSchema_K2::PN_Else),
        RequirePin(IsCenter, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(ContainsCenter, PinNames::ReturnValue),
        RequirePin(IsCenter, UEdGraphSchema_K2::PN_Condition));
    Connected &= Schema->TryCreateConnection(
        RequirePin(IsCenter, UEdGraphSchema_K2::PN_Then),
        RequirePin(SaveCenter, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(GetChild, PinNames::ReturnValue),
        RequirePin(SaveCenter, FName(CenterWidgetName)));

    Connected &= Schema->TryCreateConnection(
        RequirePin(ForLoop, PinNames::Completed),
        RequirePin(RemoveBrake, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        GetActionsRoot->GetValuePin(),
        RequirePin(RemoveBrake, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetBrake->GetValuePin(),
        RequirePin(RemoveBrake, PinNames::Content));
    Connected &= Schema->TryCreateConnection(
        RequirePin(RemoveBrake, PinNames::ReturnValue),
        RequirePin(BrakeRemoved, UEdGraphSchema_K2::PN_Condition));
    Connected &= Schema->TryCreateConnection(
        RequirePin(RemoveBrake, UEdGraphSchema_K2::PN_Then),
        RequirePin(BrakeRemoved, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(BrakeRemoved, UEdGraphSchema_K2::PN_Then),
        RequirePin(AppendBrake, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        GetActionsRoot->GetValuePin(),
        RequirePin(AppendBrake, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetBrake->GetValuePin(),
        RequirePin(AppendBrake, PinNames::Content));
    Connected &= Schema->TryCreateConnection(
        RequirePin(AppendBrake, UEdGraphSchema_K2::PN_Then),
        RequirePin(RemoveCenter, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(BrakeRemoved, UEdGraphSchema_K2::PN_Else),
        RequirePin(RemoveCenter, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        GetActionsRoot->GetValuePin(),
        RequirePin(RemoveCenter, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetCenter->GetValuePin(),
        RequirePin(RemoveCenter, PinNames::Content));
    Connected &= Schema->TryCreateConnection(
        RequirePin(RemoveCenter, PinNames::ReturnValue),
        RequirePin(CenterRemoved, UEdGraphSchema_K2::PN_Condition));
    Connected &= Schema->TryCreateConnection(
        RequirePin(RemoveCenter, UEdGraphSchema_K2::PN_Then),
        RequirePin(CenterRemoved, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        RequirePin(CenterRemoved, UEdGraphSchema_K2::PN_Then),
        RequirePin(AppendCenter, UEdGraphSchema_K2::PN_Execute));
    Connected &= Schema->TryCreateConnection(
        GetActionsRoot->GetValuePin(),
        RequirePin(AppendCenter, UEdGraphSchema_K2::PN_Self));
    Connected &= Schema->TryCreateConnection(
        GetCenter->GetValuePin(),
        RequirePin(AppendCenter, PinNames::Content));

    if (!Connected)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect bottom-row reorder graph"));
        return 1;
    }

    if (!SaveWidgetBlueprint(ChildPackage, ChildBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save HUD child override"));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated DonkLift HUD action-identity reorder: %s.%s"),
        ChildPackageName, AssetName);
    return 0;
}

#endif
