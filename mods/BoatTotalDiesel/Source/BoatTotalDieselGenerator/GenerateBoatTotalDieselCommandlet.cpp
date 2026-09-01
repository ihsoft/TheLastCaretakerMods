// HAND-WRITTEN BUILD TOOL SOURCE: BoatTotalDiesel Blueprint generator validated for
// Steam build 24990438 (UE 5.8.1), VoyageSteam-Win64-Shipping.exe SHA-256
// D9BF4C9624C60615198E62C87DA7792A9888AB02F7905AAAF1C9B02C7A9E524F.
// The C++ tool is never shipped. Revalidate after every game update.

#include "GenerateBoatTotalDieselCommandlet.h"

#if WITH_EDITOR

#include "BlueprintGraphNames.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Subsystems/SubsystemBlueprintLibrary.h"
#include "UObject/SavePackage.h"
#include "VoyageInGameBoatWidget.h"
#include "VoyageModuleComponent.h"
#include "VoyageModuleSubsystem.h"
#include "WidgetBlueprint.h"

namespace
{
constexpr TCHAR RelocatedParentPackageName[] = TEXT("/Game/Mods/Boat/BP_VoyageIngameBoatHud_O");
constexpr TCHAR ChildPackageName[] = TEXT("/Game/UI/Game/HUD/BP_VoyageIngameBoatHud");
constexpr TCHAR ReplacementPackageName[] = TEXT("/Game/Mods/BoatTotalDiesel/WBP_BoatTotalDieselValue");
constexpr TCHAR BoatHudAssetName[] = TEXT("BP_VoyageIngameBoatHud");
constexpr TCHAR ReplacementAssetName[] = TEXT("WBP_BoatTotalDieselValue");
constexpr TCHAR InitialText[] = TEXT("-- L");
constexpr TCHAR LitreSuffix[] = TEXT(" L");
constexpr int32 DieselFontSize = 10;
constexpr double EmptyDieselTotal = 0.0;
const FName BaseBlueprintName(TEXT("GenerateBoatTotalDieselBase"));
const FName ChildBlueprintName(TEXT("GenerateBoatTotalDieselChild"));
const FName ReplacementBlueprintName(TEXT("GenerateBoatTotalDieselValue"));
const FName ReplacementTextName(TEXT("DieselValueText"));
const FName EventGraphName(TEXT("EventGraph"));
const FName VisibilityInputPinName(TEXT("InVisibility"));
const FName WorldContextInputPinName(TEXT("WorldContextObject"));
const FName WidgetTypeInputPinName(TEXT("WidgetType"));
const FName OwningPlayerInputPinName(TEXT("OwningPlayer"));
const FName ChildContentInputPinName(TEXT("Content"));
const FName AddChildFunctionName(TEXT("AddChild"));
const FName GetOwningPlayerPawnFunctionName(TEXT("GetOwningPlayerPawn"));
const FName GetComponentByClassFunctionName(TEXT("GetComponentByClass"));
const FName GetOwningPlayerFunctionName(TEXT("GetOwningPlayer"));
const FName DieselTotalVariableName(TEXT("DieselTotal"));
const FName ComponentClassInputPinName(TEXT("ComponentClass"));
const FName ContextObjectInputPinName(TEXT("ContextObject"));
const FName SubsystemClassInputPinName(TEXT("Class"));
const FName ModuleInputPinName(TEXT("Module"));
const FName OutModulesOutputPinName(TEXT("OutModules"));
const FName ResourceTypeInputPinName(TEXT("Type"));
const FName IntegerInputPinName(TEXT("InInt"));
const FName StringInputPinName(TEXT("InString"));
const FName TextInputPinName(TEXT("InText"));
const FName NumberInputPinName(TEXT("A"));
const FName ForEachLoopMacroName(TEXT("ForEachLoop"));
const FName ForEachExecInputPinName(TEXT("Exec"));
const FName ForEachArrayInputPinName(TEXT("Array"));
const FName ForEachLoopBodyPinName(TEXT("LoopBody"));
const FName ForEachArrayElementPinName(TEXT("Array Element"));
const FName ForEachCompletedPinName(TEXT("Completed"));
const FString CollapsedVisibilityValue(TEXT("Collapsed"));
const FString DieselResourceValue(TEXT("EModuleResourceType::Diesel"));
namespace PinNames = BlueprintGraphNames::Pins;
namespace BinaryPins = BlueprintGraphNames::Pins::Binary;

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

UK2Node_CallFunction* AddCall(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
{
    check(Function);
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Function);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* AddVariableGet(
    UEdGraph* Graph,
    const FName VariableName,
    int32 X,
    int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    Node->VariableReference.SetSelfMember(VariableName);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* AddVariableSet(
    UEdGraph* Graph,
    const FName VariableName,
    int32 X,
    int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    Node->VariableReference.SetSelfMember(VariableName);
    return FinishNode(Node, Graph, X, Y);
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

UEdGraph* FindStandardMacro(const FName MacroName)
{
    UBlueprint* StandardMacros = LoadObject<UBlueprint>(
        nullptr,
        BlueprintGraphNames::EngineAssets::StandardMacrosObjectPath);
    if (!StandardMacros)
    {
        return nullptr;
    }
    for (UEdGraph* Graph : StandardMacros->MacroGraphs)
    {
        if (Graph && Graph->GetFName() == MacroName)
        {
            return Graph;
        }
    }
    return nullptr;
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

bool AddDieselTick(UWidgetBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        EventGraphName,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);

    UEdGraph* ForEachLoopGraph = FindStandardMacro(ForEachLoopMacroName);
    if (!ForEachLoopGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("Standard ForEachLoop macro was not found"));
        return false;
    }

    UK2Node_Event* TickEvent = NewObject<UK2Node_Event>(Graph);
    TickEvent->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::WidgetTick,
        UUserWidget::StaticClass());
    TickEvent->bOverrideFunction = true;
    FinishNode(TickEvent, Graph, -1300, 0);

    UK2Node_VariableSet* ResetTotal = AddVariableSet(
        Graph, DieselTotalVariableName, -1060, 0);
    RequirePin(ResetTotal, DieselTotalVariableName)->DefaultValue = LexToString(EmptyDieselTotal);

    UK2Node_CallFunction* GetPawn = AddCall(
        Graph,
        UUserWidget::StaticClass()->FindFunctionByName(
            GetOwningPlayerPawnFunctionName),
        -820,
        0);

    UK2Node_CallFunction* GetModuleComponent = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GetComponentByClassFunctionName),
        -560,
        0);
    RequirePin(GetModuleComponent, ComponentClassInputPinName)->DefaultObject =
        UVoyageModuleComponent::StaticClass();

    UK2Node_DynamicCast* CastModule = NewObject<UK2Node_DynamicCast>(Graph);
    CastModule->TargetType = UVoyageModuleComponent::StaticClass();
    FinishNode(CastModule, Graph, -300, 0);
    CastModule->SetPurity(false);

    UK2Node_CallFunction* GetWorldSubsystem = AddCall(
        Graph,
        USubsystemBlueprintLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(USubsystemBlueprintLibrary, GetWorldSubsystem)),
        -560,
        260);
    RequirePin(GetWorldSubsystem, SubsystemClassInputPinName)->DefaultObject =
        UVoyageModuleSubsystem::StaticClass();

    UK2Node_DynamicCast* CastSubsystem = NewObject<UK2Node_DynamicCast>(Graph);
    CastSubsystem->TargetType = UVoyageModuleSubsystem::StaticClass();
    FinishNode(CastSubsystem, Graph, -40, 0);
    CastSubsystem->SetPurity(false);

    UK2Node_CallFunction* GetModules = AddCall(
        Graph,
        UVoyageModuleSubsystem::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UVoyageModuleSubsystem, GetModulesInSameGrid)),
        220,
        0);

    UK2Node_MacroInstance* ForEachModule = NewObject<UK2Node_MacroInstance>(Graph);
    ForEachModule->SetMacroGraph(ForEachLoopGraph);
    FinishNode(ForEachModule, Graph, 500, 0);

    UK2Node_VariableGet* CurrentTotal = AddVariableGet(
        Graph, DieselTotalVariableName, 760, 240);
    UK2Node_CallFunction* GetResourceAmount = AddCall(
        Graph,
        UVoyageModuleComponent::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetResourceAmount)),
        760,
        80);
    RequirePin(GetResourceAmount, ResourceTypeInputPinName)->DefaultValue = DieselResourceValue;

    UK2Node_CallFunction* AddAmount = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::MathFunctions::AddDouble),
        1020,
        160);
    UK2Node_VariableSet* StoreTotal = AddVariableSet(
        Graph, DieselTotalVariableName, 1280, 80);

    UK2Node_VariableGet* FinalTotal = AddVariableGet(
        Graph, DieselTotalVariableName, 760, 500);
    UK2Node_CallFunction* RoundTotal = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Round)),
        1020,
        500);
    UK2Node_CallFunction* TotalToString = AddCall(
        Graph,
        UKismetStringLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Conv_IntToString)),
        1260,
        500);
    UK2Node_CallFunction* AddUnit = AddCall(
        Graph,
        UKismetStringLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr)),
        1500,
        500);
    RequirePin(AddUnit, BinaryPins::RightOperand)->DefaultValue = LitreSuffix;
    UK2Node_CallFunction* StringToText = AddCall(
        Graph,
        UKismetTextLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText)),
        1740,
        500);
    UK2Node_VariableGet* GetDieselText = AddVariableGet(
        Graph, ReplacementTextName, 1740, 680);
    UK2Node_CallFunction* SetText = AddCall(
        Graph,
        UTextBlock::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText)),
        1980,
        500);

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    const bool Connected =
        Connect(Schema,
            RequirePin(TickEvent, UEdGraphSchema_K2::PN_Then),
            RequirePin(ResetTotal, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            RequirePin(ResetTotal, UEdGraphSchema_K2::PN_Then),
            RequirePin(CastModule, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            RequirePin(GetPawn, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(GetModuleComponent, UEdGraphSchema_K2::PN_Self)) &&
        Connect(Schema,
            RequirePin(GetPawn, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(GetWorldSubsystem, ContextObjectInputPinName)) &&
        Connect(Schema,
            RequirePin(GetModuleComponent, UEdGraphSchema_K2::PN_ReturnValue),
            CastModule->GetCastSourcePin()) &&
        Connect(Schema,
            CastModule->GetValidCastPin(),
            RequirePin(CastSubsystem, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            RequirePin(GetWorldSubsystem, UEdGraphSchema_K2::PN_ReturnValue),
            CastSubsystem->GetCastSourcePin()) &&
        Connect(Schema,
            CastSubsystem->GetValidCastPin(),
            RequirePin(GetModules, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            CastSubsystem->GetCastResultPin(),
            RequirePin(GetModules, UEdGraphSchema_K2::PN_Self)) &&
        Connect(Schema,
            CastModule->GetCastResultPin(),
            RequirePin(GetModules, ModuleInputPinName)) &&
        Connect(Schema,
            RequirePin(GetModules, UEdGraphSchema_K2::PN_Then),
            RequirePin(ForEachModule, ForEachExecInputPinName)) &&
        Connect(Schema,
            RequirePin(GetModules, OutModulesOutputPinName),
            RequirePin(ForEachModule, ForEachArrayInputPinName)) &&
        Connect(Schema,
            RequirePin(ForEachModule, ForEachLoopBodyPinName),
            RequirePin(StoreTotal, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            RequirePin(ForEachModule, ForEachArrayElementPinName),
            RequirePin(GetResourceAmount, UEdGraphSchema_K2::PN_Self)) &&
        Connect(Schema,
            CurrentTotal->GetValuePin(),
            RequirePin(AddAmount, BinaryPins::LeftOperand)) &&
        Connect(Schema,
            RequirePin(GetResourceAmount, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(AddAmount, BinaryPins::RightOperand)) &&
        Connect(Schema,
            RequirePin(AddAmount, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(StoreTotal, DieselTotalVariableName)) &&
        Connect(Schema,
            RequirePin(ForEachModule, ForEachCompletedPinName),
            RequirePin(SetText, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            FinalTotal->GetValuePin(),
            RequirePin(RoundTotal, NumberInputPinName)) &&
        Connect(Schema,
            RequirePin(RoundTotal, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(TotalToString, IntegerInputPinName)) &&
        Connect(Schema,
            RequirePin(TotalToString, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(AddUnit, BinaryPins::LeftOperand)) &&
        Connect(Schema,
            RequirePin(AddUnit, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(StringToText, StringInputPinName)) &&
        Connect(Schema,
            RequirePin(StringToText, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(SetText, TextInputPinName)) &&
        Connect(Schema,
            GetDieselText->GetValuePin(),
            RequirePin(SetText, UEdGraphSchema_K2::PN_Self));
    if (!Connected)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect diesel aggregation Tick graph"));
    }
    return Connected;
}

UWidgetBlueprint* CreateReplacementWidget()
{
    UPackage* Package = CreatePackage(ReplacementPackageName);
    UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            UUserWidget::StaticClass(),
            Package,
            FName(ReplacementAssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            ReplacementBlueprintName));
    if (!Blueprint || !Blueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the replacement diesel-value widget"));
        return nullptr;
    }

    UTextBlock* ReplacementText = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),
        ReplacementTextName);
    ReplacementText->SetText(FText::FromString(InitialText));
    ReplacementText->bIsVariable = true;
    FSlateFontInfo Font = ReplacementText->GetFont();
    Font.Size = DieselFontSize;
    ReplacementText->SetFont(Font);
    ReplacementText->SetJustification(ETextJustify::Center);
    Blueprint->WidgetTree->RootWidget = ReplacementText;

    FEdGraphPinType DieselTotalType;
    DieselTotalType.PinCategory = UEdGraphSchema_K2::PC_Real;
    DieselTotalType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            DieselTotalVariableName,
            DieselTotalType,
            LexToString(EmptyDieselTotal)))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add diesel total state"));
        return nullptr;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error || !AddDieselTick(Blueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to prepare the diesel aggregation widget"));
        return nullptr;
    }

    if (!SaveBlueprint(Package, Blueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save the replacement diesel-value widget"));
        return nullptr;
    }
    return Blueprint;
}

bool AddReplacementPreConstruct(UWidgetBlueprint* Blueprint, UClass* ReplacementClass)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        EventGraphName,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);

    UK2Node_Event* PreConstructEvent = NewObject<UK2Node_Event>(Graph);
    PreConstructEvent->EventReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UUserWidget, PreConstruct),
        UUserWidget::StaticClass());
    PreConstructEvent->bOverrideFunction = true;
    FinishNode(PreConstructEvent, Graph, -500, 0);

    UK2Node_VariableGet* GetPetrolText = NewObject<UK2Node_VariableGet>(Graph);
    GetPetrolText->VariableReference.SetSelfMember(
        GET_MEMBER_NAME_CHECKED(UVoyageInGameBoatWidget, PetrolTB));
    FinishNode(GetPetrolText, Graph, -260, 180);

    UFunction* SetVisibilityFunction = UWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
    UFunction* GetParentFunction = UWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidget, GetParent));
    UFunction* CreateWidgetFunction = UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    UFunction* AddChildFunction = UPanelWidget::StaticClass()->FindFunctionByName(
        AddChildFunctionName);
    if (!SetVisibilityFunction || !GetParentFunction || !CreateWidgetFunction || !AddChildFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("One or more required UMG functions were not found"));
        return false;
    }

    UK2Node_CallFunction* SetVisibility = NewObject<UK2Node_CallFunction>(Graph);
    SetVisibility->SetFromFunction(SetVisibilityFunction);
    FinishNode(SetVisibility, Graph, 20, 0);

    UK2Node_CallFunction* GetParent = NewObject<UK2Node_CallFunction>(Graph);
    GetParent->SetFromFunction(GetParentFunction);
    FinishNode(GetParent, Graph, 20, 220);

    UK2Node_CallFunction* GetOwningPlayer = AddCall(
        Graph,
        UWidget::StaticClass()->FindFunctionByName(
            GetOwningPlayerFunctionName),
        280,
        180);

    UK2Node_CallFunction* CreateWidget = NewObject<UK2Node_CallFunction>(Graph);
    CreateWidget->SetFromFunction(CreateWidgetFunction);
    FinishNode(CreateWidget, Graph, 520, 0);

    UK2Node_CallFunction* AddChild = NewObject<UK2Node_CallFunction>(Graph);
    AddChild->SetFromFunction(AddChildFunction);
    FinishNode(AddChild, Graph, 800, 0);

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    Schema->TrySetDefaultValue(
        *RequirePin(SetVisibility, VisibilityInputPinName),
        CollapsedVisibilityValue);
    RequirePin(CreateWidget, WidgetTypeInputPinName)->DefaultObject = ReplacementClass;

    if (!Schema->TryCreateConnection(
            RequirePin(PreConstructEvent, UEdGraphSchema_K2::PN_Then),
            RequirePin(SetVisibility, UEdGraphSchema_K2::PN_Execute)) ||
        !Schema->TryCreateConnection(
            RequirePin(SetVisibility, UEdGraphSchema_K2::PN_Then),
            RequirePin(CreateWidget, UEdGraphSchema_K2::PN_Execute)) ||
        !Schema->TryCreateConnection(
            GetPetrolText->GetValuePin(),
            RequirePin(SetVisibility, UEdGraphSchema_K2::PN_Self)) ||
        !Schema->TryCreateConnection(
            GetPetrolText->GetValuePin(),
            RequirePin(GetParent, UEdGraphSchema_K2::PN_Self)) ||
        !Schema->TryCreateConnection(
            GetPetrolText->GetValuePin(),
            RequirePin(CreateWidget, WorldContextInputPinName)) ||
        !Schema->TryCreateConnection(
            RequirePin(GetOwningPlayer, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(CreateWidget, OwningPlayerInputPinName)) ||
        !Schema->TryCreateConnection(
            RequirePin(CreateWidget, UEdGraphSchema_K2::PN_Then),
            RequirePin(AddChild, UEdGraphSchema_K2::PN_Execute)) ||
        !Schema->TryCreateConnection(
            RequirePin(GetParent, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(AddChild, UEdGraphSchema_K2::PN_Self)) ||
        !Schema->TryCreateConnection(
            RequirePin(CreateWidget, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(AddChild, ChildContentInputPinName)))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect replacement-widget PreConstruct graph"));
        return false;
    }
    return true;
}
}

UGenerateBoatTotalDieselCommandlet::UGenerateBoatTotalDieselCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateBoatTotalDieselCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(RelocatedParentPackageName) ||
        FPackageName::DoesPackageExist(ChildPackageName) ||
        FPackageName::DoesPackageExist(ReplacementPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("BoatTotalDiesel assets already exist; start from an empty Content directory"));
        return 1;
    }

    UPackage* BasePackage = CreatePackage(RelocatedParentPackageName);
    UWidgetBlueprint* BaseBlueprint = Cast<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            UVoyageInGameBoatWidget::StaticClass(),
            BasePackage,
            FName(BoatHudAssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            BaseBlueprintName));
    if (!BaseBlueprint || !SaveBlueprint(BasePackage, BaseBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the temporary Boat HUD parent"));
        return 1;
    }

    UWidgetBlueprint* ReplacementBlueprint = CreateReplacementWidget();
    if (!ReplacementBlueprint || !ReplacementBlueprint->GeneratedClass)
    {
        return 1;
    }

    UPackage* ChildPackage = CreatePackage(ChildPackageName);
    UWidgetBlueprint* ChildBlueprint = Cast<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            BaseBlueprint->GeneratedClass,
            ChildPackage,
            FName(BoatHudAssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            ChildBlueprintName));
    if (!ChildBlueprint ||
        !AddReplacementPreConstruct(ChildBlueprint, ReplacementBlueprint->GeneratedClass) ||
        !SaveBlueprint(ChildPackage, ChildBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the Boat HUD replacement-slot child"));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated temporary parent: %s.%s"),
        RelocatedParentPackageName, BoatHudAssetName);
    UE_LOG(LogTemp, Display, TEXT("Generated child override: %s.%s"),
        ChildPackageName, BoatHudAssetName);
    UE_LOG(LogTemp, Display, TEXT("Replacement widget: %s.%s"),
        ReplacementPackageName, ReplacementAssetName);
    UE_LOG(LogTemp, Display, TEXT("Replacement initial text: %s"), InitialText);
    return 0;
}

#endif
