// HAND-WRITTEN BUILD TOOL SOURCE: BoatHUDTotalResources Blueprint generator validated for
// Steam build 25056839 (UE 5.8.1), VoyageSteam-Win64-Shipping.exe SHA-256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// The C++ tool is never shipped. Revalidate after every game update.

#include "GenerateBoatHUDTotalResourcesCommandlet.h"

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
constexpr TCHAR DieselReplacementPackageName[] = TEXT("/Game/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalDieselValue");
constexpr TCHAR ElectricityReplacementPackageName[] = TEXT("/Game/Mods/BoatHUDTotalResources/WBP_BoatHUDTotalElectricityValue");
constexpr TCHAR BoatHudAssetName[] = TEXT("BP_VoyageIngameBoatHud");
constexpr TCHAR DieselReplacementAssetName[] = TEXT("WBP_BoatHUDTotalDieselValue");
constexpr TCHAR ElectricityReplacementAssetName[] = TEXT("WBP_BoatHUDTotalElectricityValue");
constexpr int32 StockResourceFontSize = 10;
constexpr float StockResourceColor = 0.5f;
constexpr double EmptyResourceTotal = 0.0;
const FName BaseBlueprintName(TEXT("GenerateBoatHUDTotalResourcesBase"));
const FName ChildBlueprintName(TEXT("GenerateBoatHUDTotalResourcesChild"));
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
const FName ComponentClassInputPinName(TEXT("ComponentClass"));
const FName ContextObjectInputPinName(TEXT("ContextObject"));
const FName SubsystemClassInputPinName(TEXT("Class"));
const FName ModuleInputPinName(TEXT("Module"));
const FName OutModulesOutputPinName(TEXT("OutModules"));
const FName ResourceTypeInputPinName(TEXT("Type"));
const FName StringInputPinName(TEXT("InString"));
const FName TextInputPinName(TEXT("InText"));
const FName FontInfoInputPinName(TEXT("InFontInfo"));
const FName ColorAndOpacityInputPinName(TEXT("InColorAndOpacity"));
const FName TextTransformPolicyInputPinName(TEXT("InTransformPolicy"));
const FName NumberValueInputPinName(TEXT("Value"));
const FName UseGroupingInputPinName(TEXT("bUseGrouping"));
const FName MinimumFractionalDigitsInputPinName(TEXT("MinimumFractionalDigits"));
const FName MaximumFractionalDigitsInputPinName(TEXT("MaximumFractionalDigits"));
const FName ForEachLoopMacroName(TEXT("ForEachLoop"));
const FName ForEachExecInputPinName(TEXT("Exec"));
const FName ForEachArrayInputPinName(TEXT("Array"));
const FName ForEachLoopBodyPinName(TEXT("LoopBody"));
const FName ForEachArrayElementPinName(TEXT("Array Element"));
const FName ForEachCompletedPinName(TEXT("Completed"));
const FString CollapsedVisibilityValue(TEXT("Collapsed"));
PRAGMA_DISABLE_DEPRECATION_WARNINGS
const FName TextFontPropertyName = GET_MEMBER_NAME_CHECKED(UTextBlock, Font);
const FName TextColorPropertyName = GET_MEMBER_NAME_CHECKED(UTextBlock, ColorAndOpacity);
const FName TextTransformPolicyPropertyName =
    GET_MEMBER_NAME_CHECKED(UTextBlock, TextTransformPolicy);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
namespace PinNames = BlueprintGraphNames::Pins;
namespace BinaryPins = BlueprintGraphNames::Pins::Binary;

struct FResourceDisplaySpec
{
    const TCHAR* PackageName;
    const TCHAR* AssetName;
    FName BlueprintName;
    FName TextName;
    FName TotalVariableName;
    FName StockTextName;
    FString InitialText;
    FString UnitSuffix;
    FString ResourceEnumValue;
    double UnitDivisor;
    int32 FractionalDigits;
};

const FResourceDisplaySpec DieselDisplay {
    DieselReplacementPackageName,
    DieselReplacementAssetName,
    FName(TEXT("GenerateBoatHUDTotalResourcesValue")),
    FName(TEXT("DieselValueText")),
    FName(TEXT("DieselTotal")),
    GET_MEMBER_NAME_CHECKED(UVoyageInGameBoatWidget, PetrolTB),
    TEXT("-- L"),
    TEXT(" L"),
    TEXT("EModuleResourceType::Diesel"),
    1.0,
    0
};

const FResourceDisplaySpec ElectricityDisplay {
    ElectricityReplacementPackageName,
    ElectricityReplacementAssetName,
    FName(TEXT("GenerateBoatTotalElectricityValue")),
    FName(TEXT("ElectricityValueText")),
    FName(TEXT("ElectricityTotal")),
    GET_MEMBER_NAME_CHECKED(UVoyageInGameBoatWidget, BatteryTB),
    TEXT("--.- KWH"),
    TEXT(" KWH"),
    TEXT("EModuleResourceType::Electricity"),
    1000.0,
    1
};

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

bool AddResourceTick(UWidgetBlueprint* Blueprint, const FResourceDisplaySpec& Display)
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
        Graph, Display.TotalVariableName, -1060, 0);
    RequirePin(ResetTotal, Display.TotalVariableName)->DefaultValue = LexToString(EmptyResourceTotal);

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
        Graph, Display.TotalVariableName, 760, 240);
    UK2Node_CallFunction* GetResourceAmount = AddCall(
        Graph,
        UVoyageModuleComponent::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetResourceAmount)),
        760,
        80);
    RequirePin(GetResourceAmount, ResourceTypeInputPinName)->DefaultValue = Display.ResourceEnumValue;

    UK2Node_CallFunction* AddAmount = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::MathFunctions::AddDouble),
        1020,
        160);
    UK2Node_VariableSet* StoreTotal = AddVariableSet(
        Graph, Display.TotalVariableName, 1280, 80);

    UK2Node_VariableGet* FinalTotal = AddVariableGet(
        Graph, Display.TotalVariableName, 760, 500);
    UK2Node_CallFunction* ScaleTotal = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble)),
        1020,
        500);
    RequirePin(ScaleTotal, BinaryPins::RightOperand)->DefaultValue = LexToString(Display.UnitDivisor);
    UK2Node_CallFunction* TotalToText = AddCall(
        Graph,
        UKismetTextLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_DoubleToText)),
        1260,
        500);
    RequirePin(TotalToText, UseGroupingInputPinName)->DefaultValue = TEXT("false");
    RequirePin(TotalToText, MinimumFractionalDigitsInputPinName)->DefaultValue =
        LexToString(Display.FractionalDigits);
    RequirePin(TotalToText, MaximumFractionalDigitsInputPinName)->DefaultValue =
        LexToString(Display.FractionalDigits);
    UK2Node_CallFunction* TotalToString = AddCall(
        Graph,
        UKismetTextLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_TextToString)),
        1500,
        500);
    UK2Node_CallFunction* AddUnit = AddCall(
        Graph,
        UKismetStringLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr)),
        1740,
        500);
    RequirePin(AddUnit, BinaryPins::RightOperand)->DefaultValue = Display.UnitSuffix;
    UK2Node_CallFunction* StringToText = AddCall(
        Graph,
        UKismetTextLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText)),
        1980,
        500);
    UK2Node_VariableGet* GetResourceText = AddVariableGet(
        Graph, Display.TextName, 1980, 680);
    UK2Node_CallFunction* SetText = AddCall(
        Graph,
        UTextBlock::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText)),
        2220,
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
            RequirePin(StoreTotal, Display.TotalVariableName)) &&
        Connect(Schema,
            RequirePin(ForEachModule, ForEachCompletedPinName),
            RequirePin(SetText, UEdGraphSchema_K2::PN_Execute)) &&
        Connect(Schema,
            FinalTotal->GetValuePin(),
            RequirePin(ScaleTotal, BinaryPins::LeftOperand)) &&
        Connect(Schema,
            RequirePin(ScaleTotal, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(TotalToText, NumberValueInputPinName)) &&
        Connect(Schema,
            RequirePin(TotalToText, UEdGraphSchema_K2::PN_ReturnValue),
            RequirePin(TotalToString, TextInputPinName)) &&
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
            GetResourceText->GetValuePin(),
            RequirePin(SetText, UEdGraphSchema_K2::PN_Self));
    if (!Connected)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect resource aggregation Tick graph"));
    }
    return Connected;
}

UWidgetBlueprint* CreateReplacementWidget(const FResourceDisplaySpec& Display)
{
    UPackage* Package = CreatePackage(Display.PackageName);
    UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            UUserWidget::StaticClass(),
            Package,
            FName(Display.AssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            Display.BlueprintName));
    if (!Blueprint || !Blueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create a replacement resource-value widget"));
        return nullptr;
    }

    UTextBlock* ReplacementText = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),
        Display.TextName);
    ReplacementText->SetText(FText::FromString(Display.InitialText));
    ReplacementText->bIsVariable = true;
    FSlateFontInfo Font = ReplacementText->GetFont();
    Font.Size = StockResourceFontSize;
    Font.TypefaceFontName = FName(TEXT("Regular"));
    ReplacementText->SetFont(Font);
    ReplacementText->SetColorAndOpacity(FSlateColor(FLinearColor(
        StockResourceColor,
        StockResourceColor,
        StockResourceColor,
        1.0f)));
    ReplacementText->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
    ReplacementText->SetJustification(ETextJustify::Center);
    Blueprint->WidgetTree->RootWidget = ReplacementText;

    FEdGraphPinType ResourceTotalType;
    ResourceTotalType.PinCategory = UEdGraphSchema_K2::PC_Real;
    ResourceTotalType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            Display.TotalVariableName,
            ResourceTotalType,
            LexToString(EmptyResourceTotal)))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add resource total state"));
        return nullptr;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error || !AddResourceTick(Blueprint, Display))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to prepare a resource aggregation widget"));
        return nullptr;
    }

    if (!SaveBlueprint(Package, Blueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save a replacement resource-value widget"));
        return nullptr;
    }
    return Blueprint;
}

bool AddReplacementPreConstruct(
    UWidgetBlueprint* Blueprint,
    UClass* DieselReplacementClass,
    UClass* ElectricityReplacementClass)
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

    UFunction* SetVisibilityFunction = UWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
    UFunction* GetParentFunction = UWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidget, GetParent));
    UFunction* CreateWidgetFunction = UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    UFunction* AddChildFunction = UPanelWidget::StaticClass()->FindFunctionByName(
        AddChildFunctionName);
    UFunction* SetFontFunction = UTextBlock::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UTextBlock, SetFont));
    UFunction* SetColorFunction = UTextBlock::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UTextBlock, SetColorAndOpacity));
    UFunction* SetTransformPolicyFunction = UTextBlock::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UTextBlock, SetTextTransformPolicy));
    if (!SetVisibilityFunction || !GetParentFunction || !CreateWidgetFunction ||
        !AddChildFunction || !SetFontFunction || !SetColorFunction ||
        !SetTransformPolicyFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("One or more required UMG functions were not found"));
        return false;
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    struct FReplacementBinding
    {
        const FResourceDisplaySpec* Display;
        UClass* ReplacementClass;
    };
    const FReplacementBinding Bindings[] = {
        { &DieselDisplay, DieselReplacementClass },
        { &ElectricityDisplay, ElectricityReplacementClass }
    };

    UEdGraphPin* PreviousThen = RequirePin(PreConstructEvent, UEdGraphSchema_K2::PN_Then);
    for (int32 BindingIndex = 0; BindingIndex < UE_ARRAY_COUNT(Bindings); ++BindingIndex)
    {
        const FReplacementBinding& Binding = Bindings[BindingIndex];
        const int32 Y = BindingIndex * 520;

        UK2Node_VariableGet* GetStockText = NewObject<UK2Node_VariableGet>(Graph);
        GetStockText->VariableReference.SetSelfMember(Binding.Display->StockTextName);
        FinishNode(GetStockText, Graph, -260, Y + 180);

        UK2Node_CallFunction* SetVisibility = AddCall(
            Graph, SetVisibilityFunction, 20, Y);
        Schema->TrySetDefaultValue(
            *RequirePin(SetVisibility, VisibilityInputPinName),
            CollapsedVisibilityValue);

        UK2Node_CallFunction* GetParent = AddCall(
            Graph, GetParentFunction, 20, Y + 220);
        UK2Node_CallFunction* GetOwningPlayer = AddCall(
            Graph,
            UWidget::StaticClass()->FindFunctionByName(GetOwningPlayerFunctionName),
            280,
            Y + 180);
        UK2Node_CallFunction* CreateWidget = AddCall(
            Graph, CreateWidgetFunction, 520, Y);
        RequirePin(CreateWidget, WidgetTypeInputPinName)->DefaultObject =
            Binding.ReplacementClass;

        UK2Node_DynamicCast* CastReplacement = NewObject<UK2Node_DynamicCast>(Graph);
        CastReplacement->TargetType = Binding.ReplacementClass;
        FinishNode(CastReplacement, Graph, 780, Y);
        CastReplacement->SetPurity(false);

        UK2Node_VariableGet* GetReplacementText = NewObject<UK2Node_VariableGet>(Graph);
        GetReplacementText->VariableReference.SetExternalMember(
            Binding.Display->TextName,
            Binding.ReplacementClass);
        FinishNode(GetReplacementText, Graph, 1020, Y + 260);

        UK2Node_VariableGet* GetStockFont = NewObject<UK2Node_VariableGet>(Graph);
        GetStockFont->VariableReference.SetExternalMember(
            TextFontPropertyName,
            UTextBlock::StaticClass());
        FinishNode(GetStockFont, Graph, 1020, Y + 360);

        UK2Node_VariableGet* GetStockColor = NewObject<UK2Node_VariableGet>(Graph);
        GetStockColor->VariableReference.SetExternalMember(
            TextColorPropertyName,
            UTextBlock::StaticClass());
        FinishNode(GetStockColor, Graph, 1260, Y + 360);

        UK2Node_VariableGet* GetStockTransform = NewObject<UK2Node_VariableGet>(Graph);
        GetStockTransform->VariableReference.SetExternalMember(
            TextTransformPolicyPropertyName,
            UTextBlock::StaticClass());
        FinishNode(GetStockTransform, Graph, 1500, Y + 360);

        UK2Node_CallFunction* SetFont = AddCall(
            Graph, SetFontFunction, 1020, Y);
        UK2Node_CallFunction* SetColor = AddCall(
            Graph, SetColorFunction, 1260, Y);
        UK2Node_CallFunction* SetTransformPolicy = AddCall(
            Graph, SetTransformPolicyFunction, 1500, Y);
        UK2Node_CallFunction* AddChild = AddCall(
            Graph, AddChildFunction, 1740, Y);

        const bool Connected =
            Connect(Schema, PreviousThen,
                RequirePin(SetVisibility, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema,
                RequirePin(SetVisibility, UEdGraphSchema_K2::PN_Then),
                RequirePin(CreateWidget, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema, GetStockText->GetValuePin(),
                RequirePin(SetVisibility, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockText->GetValuePin(),
                RequirePin(GetParent, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockText->GetValuePin(),
                RequirePin(CreateWidget, WorldContextInputPinName)) &&
            Connect(Schema,
                RequirePin(GetOwningPlayer, UEdGraphSchema_K2::PN_ReturnValue),
                RequirePin(CreateWidget, OwningPlayerInputPinName)) &&
            Connect(Schema,
                RequirePin(CreateWidget, UEdGraphSchema_K2::PN_Then),
                RequirePin(CastReplacement, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema,
                RequirePin(CreateWidget, UEdGraphSchema_K2::PN_ReturnValue),
                CastReplacement->GetCastSourcePin()) &&
            Connect(Schema,
                CastReplacement->GetValidCastPin(),
                RequirePin(SetFont, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema,
                CastReplacement->GetCastResultPin(),
                RequirePin(GetReplacementText, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockText->GetValuePin(),
                RequirePin(GetStockFont, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockText->GetValuePin(),
                RequirePin(GetStockColor, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockText->GetValuePin(),
                RequirePin(GetStockTransform, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetReplacementText->GetValuePin(),
                RequirePin(SetFont, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockFont->GetValuePin(),
                RequirePin(SetFont, FontInfoInputPinName)) &&
            Connect(Schema,
                RequirePin(SetFont, UEdGraphSchema_K2::PN_Then),
                RequirePin(SetColor, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema, GetReplacementText->GetValuePin(),
                RequirePin(SetColor, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockColor->GetValuePin(),
                RequirePin(SetColor, ColorAndOpacityInputPinName)) &&
            Connect(Schema,
                RequirePin(SetColor, UEdGraphSchema_K2::PN_Then),
                RequirePin(SetTransformPolicy, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema, GetReplacementText->GetValuePin(),
                RequirePin(SetTransformPolicy, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema, GetStockTransform->GetValuePin(),
                RequirePin(SetTransformPolicy, TextTransformPolicyInputPinName)) &&
            Connect(Schema,
                RequirePin(SetTransformPolicy, UEdGraphSchema_K2::PN_Then),
                RequirePin(AddChild, UEdGraphSchema_K2::PN_Execute)) &&
            Connect(Schema,
                RequirePin(GetParent, UEdGraphSchema_K2::PN_ReturnValue),
                RequirePin(AddChild, UEdGraphSchema_K2::PN_Self)) &&
            Connect(Schema,
                RequirePin(CreateWidget, UEdGraphSchema_K2::PN_ReturnValue),
                RequirePin(AddChild, ChildContentInputPinName));
        if (!Connected)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to connect replacement-widget PreConstruct graph"));
            return false;
        }
        PreviousThen = RequirePin(AddChild, UEdGraphSchema_K2::PN_Then);
    }
    return true;
}
}

UGenerateBoatHUDTotalResourcesCommandlet::UGenerateBoatHUDTotalResourcesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateBoatHUDTotalResourcesCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(RelocatedParentPackageName) ||
        FPackageName::DoesPackageExist(ChildPackageName) ||
        FPackageName::DoesPackageExist(DieselReplacementPackageName) ||
        FPackageName::DoesPackageExist(ElectricityReplacementPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("BoatHUDTotalResources assets already exist; start from an empty Content directory"));
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

    UWidgetBlueprint* DieselReplacementBlueprint = CreateReplacementWidget(DieselDisplay);
    UWidgetBlueprint* ElectricityReplacementBlueprint =
        CreateReplacementWidget(ElectricityDisplay);
    if (!DieselReplacementBlueprint || !DieselReplacementBlueprint->GeneratedClass ||
        !ElectricityReplacementBlueprint ||
        !ElectricityReplacementBlueprint->GeneratedClass)
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
        !AddReplacementPreConstruct(
            ChildBlueprint,
            DieselReplacementBlueprint->GeneratedClass,
            ElectricityReplacementBlueprint->GeneratedClass) ||
        !SaveBlueprint(ChildPackage, ChildBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the Boat HUD replacement-slot child"));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated temporary parent: %s.%s"),
        RelocatedParentPackageName, BoatHudAssetName);
    UE_LOG(LogTemp, Display, TEXT("Generated child override: %s.%s"),
        ChildPackageName, BoatHudAssetName);
    UE_LOG(LogTemp, Display, TEXT("Diesel replacement widget: %s.%s"),
        DieselDisplay.PackageName, DieselDisplay.AssetName);
    UE_LOG(LogTemp, Display, TEXT("Electricity replacement widget: %s.%s"),
        ElectricityDisplay.PackageName, ElectricityDisplay.AssetName);
    return 0;
}

#endif
