#include "GenerateContainerLidLabelCommandlet.h"

#if WITH_EDITOR

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "BlueprintEditorLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/EngineTypes.h"
#include "Engine/Font.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/BlueprintPathsLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "VoyageBaseInventoryComponent.h"
#include "VoyageEditorBlueprintFunctionLibrary.h"
#include "VoyageModSettingsGraph.h"
#include "VoyageModSettingsTests.h"
#include "ContainerLidLabelSettings.h"
#include "VoyageInventoryWeightLimitedComponent.h"
#include "VoyageMiscBlueprintFunctionLibrary.h"
#include "VoyageWidgetComponent.h"

namespace
{
constexpr TCHAR StockParentPackage[] = TEXT("/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox");
constexpr TCHAR StockParentAsset[] = TEXT("BP_Inventory_PlayerBox");
constexpr TCHAR StockContainerBasePackage[] = TEXT("/Game/Blueprints/Interactives/Containers/BP_Container_Base");
constexpr TCHAR StockContainerBaseAsset[] = TEXT("BP_Container_Base");
constexpr TCHAR StockInventoryParentPackage[] = TEXT("/Game/Blueprints/Interactives/Containers/BP_Inventory_Container_Base_WeightLimited");
constexpr TCHAR StockInventoryParentAsset[] = TEXT("BP_Inventory_Container_Base_WeightLimited");
constexpr TCHAR RelocatedLargePackage[] = TEXT("/Game/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large");
constexpr TCHAR StockLargePackage[] = TEXT("/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large");
constexpr TCHAR StockLargeAsset[] = TEXT("BP_Inventory_PlayerBox_Large");
constexpr TCHAR StockWidgetPackage[] = TEXT("/Game/UI/Game/Inventory/BP_PlayerBox_Text");
constexpr TCHAR StockWidgetAsset[] = TEXT("BP_PlayerBox_Text");
constexpr TCHAR StockWidgetClassPath[] = TEXT("/Game/UI/Game/Inventory/BP_PlayerBox_Text.BP_PlayerBox_Text_C");
constexpr TCHAR PlaneMeshPath[] = TEXT("/Engine/BasicShapes/Plane.Plane");
constexpr TCHAR StockFontPackage[] = TEXT("/Game/UI/VoyageRoboto");
constexpr TCHAR StockFontAsset[] = TEXT("VoyageRoboto");
constexpr TCHAR LoopPackage[] = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
constexpr TCHAR ConfigRelativePath[] = TEXT("Paks/ContainerLidLabel.ini");
constexpr TCHAR ConfigKey[] = TEXT("LargeContainerMaxWeightKg");
constexpr TCHAR ConfigSeparator[] = TEXT("=");
constexpr TCHAR FloatMaximumLiteral[] = TEXT("3.402823466e+38");

const FName ParentBlueprintContext(TEXT("GenerateContainerLidLabelStockParent"));
const FName ContainerBaseBlueprintContext(TEXT("GenerateContainerLidLabelContainerBase"));
const FName InventoryParentBlueprintContext(TEXT("GenerateContainerLidLabelInventoryParent"));
const FName RelocatedBlueprintContext(TEXT("GenerateContainerLidLabelRelocatedLarge"));
const FName ReplacementBlueprintContext(TEXT("GenerateContainerLidLabelReplacement"));
const FName WidgetBlueprintContext(TEXT("GenerateContainerLidLabelStockWidget"));

const FName InitTextWidget(TEXT("InitTextWidget"));
const FName UpdateTextWidget(TEXT("UpdateTextWidget"));
const FName UpdateIconMaterial(TEXT("UpdateIconMaterial"));
const FName ReceiveBeginPlay(TEXT("ReceiveBeginPlay"));
const FName OnPersistentActorPostLoad(TEXT("OnPersistentActorPostLoad"));
const FName PersistentVersion(TEXT("Version"));
const FName ApplyConfiguredCapacity(TEXT("ApplyConfiguredCapacity"));
const FName EnsureCachedSettings(TEXT("EnsureCachedSettings"));
const FName ReloadSettings(TEXT("ReloadContainerLidLabelSettings"));
const FName PlayerBoxLid(TEXT("PlayerBox_Lid"));
const FName ResourceMeter(TEXT("ResourceMeter"));
const FName LeftIconPlane(TEXT("LeftIconPlane"));
const FName RightIconPlane(TEXT("RightIconPlane"));
const FName LeftIconDynamicMaterial(TEXT("LeftIconDynamicMaterial"));
const FName RightIconDynamicMaterial(TEXT("RightIconDynamicMaterial"));
const FName Inventory(TEXT("VoyageInventoryWeightLimited"));
const FName InventoryName(TEXT("Name"));
const FName ConfiguredMaxWeightKg(TEXT("ConfiguredMaxWeightKg"));
const FName SettingsLoaded(TEXT("ContainerLidLabelSettingsLoaded"));

const FName LidWidget(TEXT("LidLabelWidget"));
const FName LidLeftIcon(TEXT("LidLeftIconPlane"));
const FName LidRightIcon(TEXT("LidRightIconPlane"));
const FName LidFont(TEXT("LidLabelFont"));
const FName TextBlock(TEXT("TextBlock"));
const FName TextBlockAlt(TEXT("TextBlock_1"));
const FName SetSizeFunction(TEXT("SetSize"));
const FName SetFontFunction(TEXT("SetFont"));
const FName SetTextFunction(TEXT("SetText"));
const FName SetVisibilityFunction(TEXT("SetVisibility"));
const FName IsSmallContainerParameter(TEXT("IsSmallContainer"));
const FName FontParameter(TEXT("Font"));
const FName TextParameter(TEXT("Text"));
const FName AutoWrapParameter(TEXT("InAutoTextWrap"));
const FName MaterialParameter(TEXT("Material"));
const FName MaterialIndexParameter(TEXT("ElementIndex"));
const FName VisibilityParameter(TEXT("bNewVisibility"));
const FName PropagateVisibilityParameter(TEXT("bPropagateToChildren"));
const FName BoldTypeface(TEXT("Bold"));
const FName LoopGraphName(TEXT("ForEachLoop"));
const FName LoopArray(TEXT("Array"));
const FName LoopElement(TEXT("Array Element"));
const FName LoopBody(TEXT("LoopBody"));
const FName LoopCompleted(TEXT("Completed"));
const FName SourceString(TEXT("SourceString"));
const FName SplitDelimiter(TEXT("InStr"));
const FName SplitLeft(TEXT("LeftS"));
const FName SplitRight(TEXT("RightS"));
const FName NumericString(TEXT("InString"));
const FName InputPath(TEXT("InPath"));
const FName BinaryLeft(TEXT("A"));
const FName BinaryRight(TEXT("B"));
const FName FloatInput(TEXT("InDouble"));
const FName DoubleInput(TEXT("InFloat"));
const FName ClassParameter(TEXT("Class"));
const FName NewDefaultMax(TEXT("NewDefaultMax"));
const FName ParameterName(TEXT("ParameterName"));
const FName ParameterValue(TEXT("ParameterValue"));
const FName ProgressLevel(TEXT("ProgressLevel"));

const FVector LidRowCenter(0.0, 66.4, 13.73);
const FVector LidLeftIconCenter(-17.0, 62.27709, 13.73);
const FVector LidRightIconCenter(17.0, 62.27709, 13.73);
constexpr double WidgetScale = 0.12;
constexpr double IconScale = 0.05;
constexpr int32 WidgetDrawWidth = 200;
constexpr int32 WidgetDrawHeight = 110;
constexpr int32 LidFontSize = 12;
constexpr TCHAR FalseLiteral[] = TEXT("false");
constexpr TCHAR ZeroLiteral[] = TEXT("0");

FEdGraphPinType ObjectPin(UClass* Class)
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Object;
    Type.PinSubCategoryObject = Class;
    return Type;
}

FEdGraphPinType StructPin(UScriptStruct* Struct)
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Struct;
    Type.PinSubCategoryObject = Struct;
    return Type;
}

FEdGraphPinType BoolPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    return Type;
}

FEdGraphPinType FloatPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Real;
    Type.PinSubCategory = UEdGraphSchema_K2::PC_Float;
    return Type;
}

FEdGraphPinType IntPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Int;
    return Type;
}

FEdGraphPinType TextPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Text;
    return Type;
}

template <typename T>
T* FinishNode(T* Node, UEdGraph* Graph, int32 X, int32 Y)
{
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, false, false);
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    Node->AllocateDefaultPins();
    return Node;
}

UEdGraphPin* Pin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Result = Node ? Node->FindPin(Name) : nullptr;
    checkf(Result, TEXT("Missing pin %s on %s"), *Name.ToString(), Node ? *Node->GetName() : TEXT("null"));
    return Result;
}

bool Link(UEdGraphPin* A, UEdGraphPin* B)
{
    check(A && B);
    return GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A, B);
}

UK2Node_CallFunction* Call(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
{
    check(Function);
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Function);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* ReadSelf(UEdGraph* Graph, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    Node->VariableReference.SetSelfMember(Name);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* WriteSelf(UEdGraph* Graph, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    Node->VariableReference.SetSelfMember(Name);
    return FinishNode(Node, Graph, X, Y);
}

void MarkVariableTransient(UBlueprint* Blueprint, const FName Name)
{
    FBPVariableDescription* Variable = Blueprint->NewVariables.FindByPredicate(
        [Name](const FBPVariableDescription& Candidate)
        {
            return Candidate.VarName == Name;
        });
    check(Variable);
    Variable->PropertyFlags |= CPF_Transient;
}

UK2Node_IfThenElse* Branch(UEdGraph* Graph, UEdGraphPin* Execute, UEdGraphPin* Condition, int32 X, int32 Y)
{
    UK2Node_IfThenElse* Node = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, X, Y);
    check(Link(Execute, Pin(Node, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Condition, Pin(Node, UEdGraphSchema_K2::PN_Condition)));
    return Node;
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
    return FinishNode(Loop, Graph, X, Y);
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

UEdGraphPin* Trimmed(UEdGraph* Graph, UEdGraphPin* Source, int32 X, int32 Y)
{
    UK2Node_CallFunction* Leading = Call(Graph,
        UKismetStringLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Trim)), X, Y);
    check(Link(Source, Pin(Leading, SourceString)));
    UK2Node_CallFunction* Trailing = Call(Graph,
        UKismetStringLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, TrimTrailing)), X + 220, Y);
    check(Link(Pin(Leading, UEdGraphSchema_K2::PN_ReturnValue), Pin(Trailing, SourceString)));
    return Pin(Trailing, UEdGraphSchema_K2::PN_ReturnValue);
}

UK2Node_VariableGet* ReadExternal(
    UEdGraph* Graph, const FName Name, UClass* Owner, UEdGraphPin* Target, int32 X, int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    Node->VariableReference.SetExternalMember(Name, Owner);
    FinishNode(Node, Graph, X, Y);
    check(Link(Target, Pin(Node, UEdGraphSchema_K2::PN_Self)));
    return Node;
}

UBlueprint* CreateActorBlueprint(
    const TCHAR* PackageName, const TCHAR* AssetName, UClass* ParentClass, const FName Context)
{
    UPackage* Package = CreatePackage(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass, Package, FName(AssetName), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), Context);
    check(Blueprint);
    return Blueprint;
}

void Compile(UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    checkf(Blueprint->Status != BS_Error, TEXT("Blueprint compilation failed: %s"), *Blueprint->GetPathName());
}

void Save(UBlueprint* Blueprint)
{
    Compile(Blueprint);
    UPackage* Package = Blueprint->GetOutermost();
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    checkf(UPackage::SavePackage(Package, Blueprint, *Filename, Args), TEXT("Save failed: %s"), *Filename);
}

void SaveObject(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    checkf(UPackage::SavePackage(Package, Asset, *Filename, Args), TEXT("Save failed: %s"), *Filename);
}

UK2Node_FunctionEntry* AddFunction(
    UBlueprint* Blueprint, const FName Name, UEdGraph*& Graph, const EFunctionFlags Flags)
{
    Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, Graph, false, static_cast<UClass*>(nullptr));
    TArray<UK2Node_FunctionEntry*> Entries;
    Graph->GetNodesOfClass(Entries);
    check(Entries.Num() == 1);
    Entries[0]->AddExtraFlags(Flags);
    return Entries[0];
}

void AddVoidFunction(UBlueprint* Blueprint, const FName Name, const EFunctionFlags Flags)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(Blueprint, Name, Graph, Flags);
    TArray<UK2Node_FunctionResult*> Results;
    Graph->GetNodesOfClass(Results);
    if (Results.Num() == 1)
    {
        check(Link(Pin(Entry, UEdGraphSchema_K2::PN_Then), Pin(Results[0], UEdGraphSchema_K2::PN_Execute)));
    }
}

void AddVoidFunctionWithInput(
    UBlueprint* Blueprint, const FName Name, const FName Parameter,
    const FEdGraphPinType& Type, const EFunctionFlags Flags)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(Blueprint, Name, Graph, Flags);
    check(Entry->CreateUserDefinedPin(Parameter, Type, EGPD_Output));
    TArray<UK2Node_FunctionResult*> Results;
    Graph->GetNodesOfClass(Results);
    if (Results.Num() == 1)
    {
        check(Link(Pin(Entry, UEdGraphSchema_K2::PN_Then), Pin(Results[0], UEdGraphSchema_K2::PN_Execute)));
    }
}

void AddBeginPlayPlaceholder(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    check(Graph);
    UK2Node_Event* Event = FinishNode(NewObject<UK2Node_Event>(Graph), Graph, 0, 0);
    Event->EventReference.SetExternalMember(ReceiveBeginPlay, AActor::StaticClass());
    Event->bOverrideFunction = true;
    Event->ReconstructNode();
}

void AddWidgetFunction(UBlueprint* Blueprint, const FName Name, const FName Parameter, const FEdGraphPinType& Type)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Blueprint, Name, Graph,
        FUNC_Public | FUNC_BlueprintCallable | FUNC_BlueprintEvent);
    check(Entry->CreateUserDefinedPin(Parameter, Type, EGPD_Output));
}

UWidgetBlueprint* CreateStockWidgetPlaceholder()
{
    UPackage* Package = CreatePackage(StockWidgetPackage);
    UWidgetBlueprint* Widget = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
        UUserWidget::StaticClass(), Package, FName(StockWidgetAsset), BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass(), WidgetBlueprintContext));
    check(FBlueprintEditorUtils::AddMemberVariable(Widget, TextBlock, ObjectPin(UTextBlock::StaticClass())));
    check(FBlueprintEditorUtils::AddMemberVariable(Widget, TextBlockAlt, ObjectPin(UTextBlock::StaticClass())));
    AddWidgetFunction(Widget, SetSizeFunction, IsSmallContainerParameter, BoolPin());
    AddWidgetFunction(Widget, SetFontFunction, FontParameter, StructPin(FSlateFontInfo::StaticStruct()));
    AddWidgetFunction(Widget, SetTextFunction, TextParameter, TextPin());
    Save(Widget);
    return Widget;
}

UBlueprint* CreateStockParentPlaceholder()
{
    UBlueprint* ContainerBase = CreateActorBlueprint(
        StockContainerBasePackage, StockContainerBaseAsset,
        AActor::StaticClass(), ContainerBaseBlueprintContext);
    AddVoidFunctionWithInput(
        ContainerBase, OnPersistentActorPostLoad, PersistentVersion, IntPin(),
        FUNC_Public | FUNC_BlueprintCallable | FUNC_BlueprintEvent);
    Save(ContainerBase);
    UBlueprint* InventoryParent = CreateActorBlueprint(
        StockInventoryParentPackage, StockInventoryParentAsset,
        ContainerBase->GeneratedClass, InventoryParentBlueprintContext);
    check(FBlueprintEditorUtils::AddMemberVariable(
        InventoryParent, Inventory, ObjectPin(UVoyageInventoryWeightLimitedComponent::StaticClass())));
    AddBeginPlayPlaceholder(InventoryParent);
    Save(InventoryParent);
    UBlueprint* Parent = CreateActorBlueprint(
        StockParentPackage, StockParentAsset, InventoryParent->GeneratedClass, ParentBlueprintContext);
    check(Parent->SimpleConstructionScript);
    USCS_Node* Lid = Parent->SimpleConstructionScript->CreateNode(
        UStaticMeshComponent::StaticClass(), PlayerBoxLid);
    Parent->SimpleConstructionScript->AddNode(Lid);
    USCS_Node* Meter = Parent->SimpleConstructionScript->CreateNode(
        UStaticMeshComponent::StaticClass(), ResourceMeter);
    Parent->SimpleConstructionScript->AddNode(Meter);
    check(FBlueprintEditorUtils::AddMemberVariable(Parent, LeftIconPlane, ObjectPin(UStaticMeshComponent::StaticClass())));
    check(FBlueprintEditorUtils::AddMemberVariable(Parent, RightIconPlane, ObjectPin(UStaticMeshComponent::StaticClass())));
    check(FBlueprintEditorUtils::AddMemberVariable(Parent, LeftIconDynamicMaterial, ObjectPin(UMaterialInstanceDynamic::StaticClass())));
    check(FBlueprintEditorUtils::AddMemberVariable(Parent, RightIconDynamicMaterial, ObjectPin(UMaterialInstanceDynamic::StaticClass())));
    AddVoidFunction(Parent, InitTextWidget, FUNC_Public | FUNC_BlueprintCallable | FUNC_BlueprintEvent);
    AddVoidFunction(Parent, UpdateTextWidget, FUNC_Public | FUNC_BlueprintCallable | FUNC_BlueprintEvent);
    AddVoidFunction(Parent, UpdateIconMaterial, FUNC_BlueprintCallable | FUNC_BlueprintEvent);
    AddBeginPlayPlaceholder(Parent);
    check(UBlueprintEditorLibrary::AddFunctionOverride(Parent, OnPersistentActorPostLoad));
    Save(Parent);
    return Parent;
}

struct FOverrideGraph
{
    UEdGraph* Graph = nullptr;
    UK2Node_CallParentFunction* Parent = nullptr;
};

FOverrideGraph AddOverride(UBlueprint* Blueprint, const FName Name)
{
    FOverrideGraph Result;
    Result.Graph = UBlueprintEditorLibrary::AddFunctionOverride(Blueprint, Name);
    check(Result.Graph);
    TArray<UK2Node_CallParentFunction*> Calls;
    Result.Graph->GetNodesOfClass(Calls);
    checkf(Calls.Num() == 1, TEXT("Expected one parent call in %s; found %d"), *Name.ToString(), Calls.Num());
    Result.Parent = Calls[0];
    return Result;
}

struct FWidgetCast
{
    UK2Node_DynamicCast* Cast = nullptr;
    UEdGraphPin* Widget = nullptr;
};

FWidgetCast AddLidWidgetCast(UEdGraph* Graph, UClass* WidgetClass, UEdGraphPin* Execute, int32 X, int32 Y)
{
    UK2Node_VariableGet* WidgetComponent = ReadSelf(Graph, LidWidget, X, Y + 180);
    UFunction* GetWidgetFunction = UWidgetComponent::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetComponent, GetWidget));
    UK2Node_CallFunction* GetWidget = Call(Graph, GetWidgetFunction, X + 220, Y + 180);
    check(Link(Pin(WidgetComponent, LidWidget), Pin(GetWidget, UEdGraphSchema_K2::PN_Self)));
    UK2Node_DynamicCast* Cast = NewObject<UK2Node_DynamicCast>(Graph);
    Cast->TargetType = WidgetClass;
    FinishNode(Cast, Graph, X + 440, Y);
    check(Link(Execute, Pin(Cast, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(GetWidget, UEdGraphSchema_K2::PN_ReturnValue), Cast->GetCastSourcePin()));
    return {Cast, Cast->GetCastResultPin()};
}

UK2Node_CallFunction* CallWidget(
    UEdGraph* Graph, UClass* WidgetClass, const FName Function,
    UEdGraphPin* Target, UEdGraphPin* Execute, int32 X, int32 Y)
{
    UK2Node_CallFunction* Node = Call(Graph, WidgetClass->FindFunctionByName(Function), X, Y);
    check(Link(Target, Pin(Node, UEdGraphSchema_K2::PN_Self)));
    check(Link(Execute, Pin(Node, UEdGraphSchema_K2::PN_Execute)));
    return Node;
}

void AddInitOverride(UBlueprint* Child, UClass* WidgetClass)
{
    FOverrideGraph Override = AddOverride(Child, InitTextWidget);
    FWidgetCast Widget = AddLidWidgetCast(
        Override.Graph, WidgetClass, Pin(Override.Parent, UEdGraphSchema_K2::PN_Then), 200, 0);
    UK2Node_CallFunction* SetSize = CallWidget(
        Override.Graph, WidgetClass, SetSizeFunction, Widget.Widget,
        Widget.Cast->GetValidCastPin(), 900, 0);
    Pin(SetSize, IsSmallContainerParameter)->DefaultValue = FalseLiteral;
    UK2Node_CallFunction* SetFont = CallWidget(
        Override.Graph, WidgetClass, SetFontFunction, Widget.Widget,
        Pin(SetSize, UEdGraphSchema_K2::PN_Then), 1150, 0);
    UK2Node_VariableGet* Font = ReadSelf(Override.Graph, LidFont, 900, 240);
    check(Link(Pin(Font, LidFont), Pin(SetFont, FontParameter)));

    UFunction* SetAutoWrap = UTextBlock::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UTextBlock, SetAutoWrapText));
    UEdGraphPin* Tail = Pin(SetFont, UEdGraphSchema_K2::PN_Then);
    int32 X = 1420;
    for (const FName TextName : {TextBlock, TextBlockAlt})
    {
        UK2Node_VariableGet* Text = ReadExternal(
            Override.Graph, TextName, WidgetClass, Widget.Widget, X, 220);
        UK2Node_CallFunction* DisableWrap = Call(Override.Graph, SetAutoWrap, X + 220, 0);
        check(Link(Pin(Text, TextName), Pin(DisableWrap, UEdGraphSchema_K2::PN_Self)));
        Pin(DisableWrap, AutoWrapParameter)->DefaultValue = FalseLiteral;
        check(Link(Tail, Pin(DisableWrap, UEdGraphSchema_K2::PN_Execute)));
        Tail = Pin(DisableWrap, UEdGraphSchema_K2::PN_Then);
        X += 500;
    }
}

void AddTextOverride(UBlueprint* Child, UClass* WidgetClass)
{
    FOverrideGraph Override = AddOverride(Child, UpdateTextWidget);
    FWidgetCast Widget = AddLidWidgetCast(
        Override.Graph, WidgetClass, Pin(Override.Parent, UEdGraphSchema_K2::PN_Then), 200, 0);
    UK2Node_CallFunction* SetText = CallWidget(
        Override.Graph, WidgetClass, SetTextFunction, Widget.Widget,
        Widget.Cast->GetValidCastPin(), 1100, 0);
    UK2Node_VariableGet* InventoryGet = ReadSelf(Override.Graph, Inventory, 600, 260);
    UK2Node_VariableGet* NameGet = ReadExternal(
        Override.Graph, InventoryName, UVoyageBaseInventoryComponent::StaticClass(),
        Pin(InventoryGet, Inventory), 820, 260);
    check(Link(Pin(NameGet, InventoryName), Pin(SetText, TextParameter)));
}

UK2Node_CallFunction* SetMaterial(
    UEdGraph* Graph, const FName ComponentName, const FName MaterialName,
    UEdGraphPin* Execute, int32 X, int32 Y)
{
    UFunction* Function = UPrimitiveComponent::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetMaterial));
    UK2Node_CallFunction* Node = Call(Graph, Function, X, Y);
    UK2Node_VariableGet* Component = ReadSelf(Graph, ComponentName, X - 220, Y + 160);
    UK2Node_VariableGet* Material = ReadSelf(Graph, MaterialName, X - 220, Y + 260);
    check(Link(Pin(Component, ComponentName), Pin(Node, UEdGraphSchema_K2::PN_Self)));
    check(Link(Pin(Material, MaterialName), Pin(Node, MaterialParameter)));
    Pin(Node, MaterialIndexParameter)->DefaultValue = ZeroLiteral;
    check(Link(Execute, Pin(Node, UEdGraphSchema_K2::PN_Execute)));
    return Node;
}

UK2Node_CallFunction* SetVisibility(
    UEdGraph* Graph, const FName ComponentName, UEdGraphPin* Visible,
    UEdGraphPin* Execute, int32 X, int32 Y)
{
    UFunction* Function = USceneComponent::StaticClass()->FindFunctionByName(
        SetVisibilityFunction);
    UK2Node_CallFunction* Node = Call(Graph, Function, X, Y);
    UK2Node_VariableGet* Component = ReadSelf(Graph, ComponentName, X - 220, Y + 160);
    check(Link(Pin(Component, ComponentName), Pin(Node, UEdGraphSchema_K2::PN_Self)));
    check(Link(Visible, Pin(Node, VisibilityParameter)));
    Pin(Node, PropagateVisibilityParameter)->DefaultValue = FalseLiteral;
    check(Link(Execute, Pin(Node, UEdGraphSchema_K2::PN_Execute)));
    return Node;
}

UEdGraphPin* IsVisible(UEdGraph* Graph, const FName ComponentName, int32 X, int32 Y)
{
    UFunction* Function = USceneComponent::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(USceneComponent, IsVisible));
    UK2Node_CallFunction* Node = Call(Graph, Function, X, Y);
    UK2Node_VariableGet* Component = ReadSelf(Graph, ComponentName, X - 220, Y + 120);
    check(Link(Pin(Component, ComponentName), Pin(Node, UEdGraphSchema_K2::PN_Self)));
    return Pin(Node, UEdGraphSchema_K2::PN_ReturnValue);
}

void AddIconOverride(UBlueprint* Child)
{
    FOverrideGraph Override = AddOverride(Child, UpdateIconMaterial);
    UK2Node_CallFunction* LeftMaterial = SetMaterial(
        Override.Graph, LidLeftIcon, LeftIconDynamicMaterial,
        Pin(Override.Parent, UEdGraphSchema_K2::PN_Then), 400, 0);
    UK2Node_CallFunction* RightMaterial = SetMaterial(
        Override.Graph, LidRightIcon, RightIconDynamicMaterial,
        Pin(LeftMaterial, UEdGraphSchema_K2::PN_Then), 700, 0);

    UEdGraphPin* LeftVisible = IsVisible(Override.Graph, LeftIconPlane, 600, 360);
    UEdGraphPin* RightVisible = IsVisible(Override.Graph, RightIconPlane, 900, 360);
    UK2Node_CallFunction* ShowLeft = SetVisibility(
        Override.Graph, LidLeftIcon, LeftVisible,
        Pin(RightMaterial, UEdGraphSchema_K2::PN_Then), 1000, 0);
    SetVisibility(
        Override.Graph, LidRightIcon, RightVisible,
        Pin(ShowLeft, UEdGraphSchema_K2::PN_Then), 1300, 0);
}

void AddCachedSettingsFunction(UBlueprint* Child)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Child, EnsureCachedSettings, Graph, FUNC_Public | FUNC_BlueprintCallable);
    TArray<UK2Node_FunctionResult*> Results;
    Graph->GetNodesOfClass(Results);
    check(Results.Num() <= 1);
    Pin(Entry, UEdGraphSchema_K2::PN_Then)->BreakAllPinLinks();

    UK2Node_VariableGet* Loaded = ReadSelf(Graph, SettingsLoaded, 220, -360);
    UK2Node_IfThenElse* NeedsLoad = Branch(
        Graph, Pin(Entry, UEdGraphSchema_K2::PN_Then), Pin(Loaded, SettingsLoaded), 450, -600);
    UFunction* Reload = Child->GeneratedClass->FindFunctionByName(
        ReloadSettings, EIncludeSuperFlag::ExcludeSuper);
    check(Reload && Reload->GetOwnerClass() == Child->GeneratedClass);
    UK2Node_CallFunction* ReloadCall = Call(Graph, Reload, 680, -480);
    check(Link(Pin(NeedsLoad, UEdGraphSchema_K2::PN_Else),
        Pin(ReloadCall, UEdGraphSchema_K2::PN_Execute)));
    UK2Node_VariableSet* MarkLoaded = WriteSelf(Graph, SettingsLoaded, 910, -480);
    Pin(MarkLoaded, SettingsLoaded)->DefaultValue = TEXT("true");
    check(Link(Pin(ReloadCall, UEdGraphSchema_K2::PN_Then),
        Pin(MarkLoaded, UEdGraphSchema_K2::PN_Execute)));
    if (Results.Num() == 1)
    {
        check(Link(Pin(MarkLoaded, UEdGraphSchema_K2::PN_Then), Pin(Results[0], UEdGraphSchema_K2::PN_Execute)));
    }
}

void AddCapacityFunction(UBlueprint* Child)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Child, ApplyConfiguredCapacity, Graph, FUNC_Public | FUNC_BlueprintCallable);
    TArray<UK2Node_FunctionResult*> Results;
    Graph->GetNodesOfClass(Results);
    check(Results.Num() <= 1);
    Pin(Entry, UEdGraphSchema_K2::PN_Then)->BreakAllPinLinks();

    UK2Node_CallFunction* GetDefaults = Call(Graph,
        UVoyageMiscBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary, GetClassDefaultObject)), 220, -200);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(
        *Pin(GetDefaults, ClassParameter), Child->GeneratedClass);
    check(Pin(GetDefaults, ClassParameter)->DefaultObject == Child->GeneratedClass);
    check(Link(Pin(Entry, UEdGraphSchema_K2::PN_Then), Pin(GetDefaults, UEdGraphSchema_K2::PN_Execute)));

    UK2Node_DynamicCast* DefaultsCast = NewObject<UK2Node_DynamicCast>(Graph);
    DefaultsCast->TargetType = Child->GeneratedClass;
    FinishNode(DefaultsCast, Graph, 450, -200);
    check(Link(Pin(GetDefaults, UEdGraphSchema_K2::PN_Then), Pin(DefaultsCast, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(GetDefaults, UEdGraphSchema_K2::PN_ReturnValue), DefaultsCast->GetCastSourcePin()));

    UFunction* Ensure = Child->GeneratedClass->FindFunctionByName(
        EnsureCachedSettings, EIncludeSuperFlag::ExcludeSuper);
    check(Ensure && Ensure->GetOwnerClass() == Child->GeneratedClass);
    UK2Node_CallFunction* EnsureCall = Call(Graph, Ensure, 680, -200);
    check(Link(DefaultsCast->GetValidCastPin(), Pin(EnsureCall, UEdGraphSchema_K2::PN_Execute)));
    check(Link(DefaultsCast->GetCastResultPin(), Pin(EnsureCall, UEdGraphSchema_K2::PN_Self)));

    UK2Node_VariableGet* InventoryGet = ReadSelf(Graph, Inventory, 680, 100);
    UK2Node_VariableGet* Configured = NewObject<UK2Node_VariableGet>(Graph);
    Configured->VariableReference.SetExternalMember(ConfiguredMaxWeightKg, Child->GeneratedClass);
    FinishNode(Configured, Graph, 900, 300);
    check(Link(DefaultsCast->GetCastResultPin(), Pin(Configured, UEdGraphSchema_K2::PN_Self)));
    UK2Node_CallFunction* SetLimit = Call(Graph,
        UVoyageInventoryWeightLimitedComponent::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UVoyageInventoryWeightLimitedComponent, SetMaxWeightLimit)), 1130, -40);
    check(Link(Pin(EnsureCall, UEdGraphSchema_K2::PN_Then), Pin(SetLimit, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(InventoryGet, Inventory), Pin(SetLimit, UEdGraphSchema_K2::PN_Self)));
    check(Link(Pin(Configured, ConfiguredMaxWeightKg), Pin(SetLimit, NewDefaultMax)));
    UK2Node_CallFunction* WeightUsed = Call(Graph,
        UVoyageInventoryWeightLimitedComponent::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UVoyageInventoryWeightLimitedComponent, GetPercentWeightUsed)), 1360, 220);
    check(Link(Pin(InventoryGet, Inventory), Pin(WeightUsed, UEdGraphSchema_K2::PN_Self)));
    UK2Node_VariableGet* Meter = ReadSelf(Graph, ResourceMeter, 1360, 360);
    UK2Node_CallFunction* RefreshMeter = Call(Graph,
        UMeshComponent::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UMeshComponent, SetScalarParameterValueOnMaterials)), 1590, -40);
    Pin(RefreshMeter, ParameterName)->DefaultValue = ProgressLevel.ToString();
    check(Link(Pin(SetLimit, UEdGraphSchema_K2::PN_Then), Pin(RefreshMeter, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Meter, ResourceMeter), Pin(RefreshMeter, UEdGraphSchema_K2::PN_Self)));
    check(Link(Pin(WeightUsed, UEdGraphSchema_K2::PN_ReturnValue), Pin(RefreshMeter, ParameterValue)));
    if (Results.Num() == 1)
    {
        check(Link(Pin(RefreshMeter, UEdGraphSchema_K2::PN_Then), Pin(Results[0], UEdGraphSchema_K2::PN_Execute)));
    }
}

void AddCapacityLifecycleOverride(UBlueprint* Child, const FName Function)
{
    FOverrideGraph Override = AddOverride(Child, Function);
    UFunction* Apply = Child->GeneratedClass->FindFunctionByName(
        ApplyConfiguredCapacity, EIncludeSuperFlag::ExcludeSuper);
    check(Apply && Apply->GetOwnerClass() == Child->GeneratedClass);
    UK2Node_CallFunction* ApplyCall = Call(Override.Graph, Apply, 400, 0);
    check(Link(
        Pin(Override.Parent, UEdGraphSchema_K2::PN_Then),
        Pin(ApplyCall, UEdGraphSchema_K2::PN_Execute)));
}

void AddBeginPlayCapacityOverride(UBlueprint* Child)
{
    check(Child && Child->GeneratedClass && Child->ParentClass);
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Child);
    check(Graph);
    UK2Node_Event* Event = FinishNode(NewObject<UK2Node_Event>(Graph), Graph, 0, 0);
    Event->EventReference.SetExternalMember(ReceiveBeginPlay, AActor::StaticClass());
    Event->bOverrideFunction = true;
    Event->ReconstructNode();

    UFunction* Inherited = Child->ParentClass->FindFunctionByName(ReceiveBeginPlay);
    check(Inherited && Inherited->GetOwnerClass()->GetPathName() ==
        FString(TEXT("/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox.BP_Inventory_PlayerBox_C")));
    UK2Node_CallParentFunction* Parent = NewObject<UK2Node_CallParentFunction>(Graph);
    Parent->SetFromFunction(Inherited);
    FinishNode(Parent, Graph, 240, 0);
    check(Link(Pin(Event, UEdGraphSchema_K2::PN_Then), Pin(Parent, UEdGraphSchema_K2::PN_Execute)));

    UFunction* Apply = Child->GeneratedClass->FindFunctionByName(
        ApplyConfiguredCapacity, EIncludeSuperFlag::ExcludeSuper);
    check(Apply && Apply->GetOwnerClass() == Child->GeneratedClass);
    UK2Node_CallFunction* ApplyCall = Call(Graph, Apply, 480, 0);
    check(Link(Pin(Parent, UEdGraphSchema_K2::PN_Then), Pin(ApplyCall, UEdGraphSchema_K2::PN_Execute)));
}

void ValidateGenerated(UBlueprint* Child)
{
    check(Child && Child->GeneratedClass && Child->SimpleConstructionScript);
    check(Child->ParentClass && Child->ParentClass->GetPathName() ==
        FString(TEXT("/Game/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large.BP_Inventory_PlayerBox_Large_C")));
    check(Child->SimpleConstructionScript->GetAllNodes().Num() == 3);
    for (const FName Function : {
        InitTextWidget, UpdateTextWidget, UpdateIconMaterial,
        ReceiveBeginPlay, OnPersistentActorPostLoad,
        EnsureCachedSettings, ApplyConfiguredCapacity})
    {
        UFunction* Owned = Child->GeneratedClass->FindFunctionByName(Function, EIncludeSuperFlag::ExcludeSuper);
        check(Owned && Owned->GetOwnerClass() == Child->GeneratedClass);
    }
    for (const FName Function : {InitTextWidget, UpdateTextWidget, UpdateIconMaterial})
    {
        UFunction* ParentFunction = Child->ParentClass->FindFunctionByName(Function);
        check(ParentFunction && ParentFunction->GetOwnerClass()->GetPathName() ==
            FString(TEXT("/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox.BP_Inventory_PlayerBox_C")));
    }
    UFunction* PostLoadParent = Child->ParentClass->FindFunctionByName(OnPersistentActorPostLoad);
    check(PostLoadParent && PostLoadParent->GetOwnerClass()->GetPathName() ==
        FString(TEXT("/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox.BP_Inventory_PlayerBox_C")));
}
}

UGenerateContainerLidLabelCommandlet::UGenerateContainerLidLabelCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateContainerLidLabelCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(StockWidgetPackage) ||
        FPackageName::DoesPackageExist(StockContainerBasePackage) ||
        FPackageName::DoesPackageExist(StockInventoryParentPackage) ||
        FPackageName::DoesPackageExist(StockParentPackage) ||
        FPackageName::DoesPackageExist(RelocatedLargePackage) ||
        FPackageName::DoesPackageExist(StockLargePackage))
    {
        UE_LOG(LogTemp, Error, TEXT("ContainerLidLabel generation paths already exist"));
        return 1;
    }

    UWidgetBlueprint* Widget = CreateStockWidgetPlaceholder();
    UPackage* FontPackage = CreatePackage(StockFontPackage);
    UFont* StockFont = NewObject<UFont>(
        FontPackage, FName(StockFontAsset), RF_Public | RF_Standalone);
    check(StockFont);
    SaveObject(StockFont);
    UBlueprint* StockParent = CreateStockParentPlaceholder();
    USCS_Node* LidNode = StockParent->SimpleConstructionScript->FindSCSNode(PlayerBoxLid);
    check(LidNode);

    UBlueprint* RelocatedLarge = CreateActorBlueprint(
        RelocatedLargePackage, StockLargeAsset, StockParent->GeneratedClass, RelocatedBlueprintContext);
    Save(RelocatedLarge);

    UBlueprint* Child = CreateActorBlueprint(
        StockLargePackage, StockLargeAsset, RelocatedLarge->GeneratedClass, ReplacementBlueprintContext);
    check(Child->SimpleConstructionScript);
    check(FBlueprintEditorUtils::AddMemberVariable(Child, ConfiguredMaxWeightKg, FloatPin()));
    check(FBlueprintEditorUtils::AddMemberVariable(Child, SettingsLoaded, BoolPin()));
    MarkVariableTransient(Child, ConfiguredMaxWeightKg);
    MarkVariableTransient(Child, SettingsLoaded);
    check(FBlueprintEditorUtils::AddMemberVariable(
        Child, LidFont, StructPin(FSlateFontInfo::StaticStruct())));

    USCS_Node* WidgetNode = Child->SimpleConstructionScript->CreateNode(
        UVoyageWidgetComponent::StaticClass(), LidWidget);
    WidgetNode->SetParent(LidNode);
    Child->SimpleConstructionScript->AddNode(WidgetNode);
    UVoyageWidgetComponent* WidgetTemplate = CastChecked<UVoyageWidgetComponent>(WidgetNode->ComponentTemplate);
    WidgetTemplate->SetWidgetClass(TSubclassOf<UUserWidget>(Widget->GeneratedClass.Get()));
    WidgetTemplate->SetDrawSize(FVector2D(WidgetDrawWidth, WidgetDrawHeight));
    WidgetTemplate->SetRelativeLocation(LidRowCenter);
    WidgetTemplate->SetRelativeRotation(FRotator(90.0, 90.0, 0.0));
    WidgetTemplate->SetRelativeScale3D(FVector(WidgetScale));
    WidgetTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WidgetTemplate->SetVisibility(true);

    UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, PlaneMeshPath);
    check(Plane);
    for (const FName Name : {LidLeftIcon, LidRightIcon})
    {
        USCS_Node* Node = Child->SimpleConstructionScript->CreateNode(UStaticMeshComponent::StaticClass(), Name);
        Node->SetParent(LidNode);
        Child->SimpleConstructionScript->AddNode(Node);
        UStaticMeshComponent* Template = CastChecked<UStaticMeshComponent>(Node->ComponentTemplate);
        Template->SetStaticMesh(Plane);
        Template->SetRelativeLocation(Name == LidLeftIcon
            ? LidLeftIconCenter
            : LidRightIconCenter);
        Template->SetRelativeRotation(FRotator::ZeroRotator);
        Template->SetRelativeScale3D(FVector(IconScale));
        Template->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Template->SetGenerateOverlapEvents(false);
        Template->SetVisibility(false);
    }

    Compile(Child);
    check(VoyageModSettings::Graph::AddSettingsLoader(
        Child, ReloadSettings, ContainerLidLabelSettings::RelativePath,
        ContainerLidLabelSettings::Settings, UE_ARRAY_COUNT(ContainerLidLabelSettings::Settings),
        [](const VoyageModSettings::FSetting&) { return true; }));
    VoyageModSettings::Graph::TestLineParser(Child);
    VoyageModSettings::Graph::TestNumberParser(Child);
    VoyageModSettings::Graph::TestStringParser(Child);
    VoyageModSettings::Tests::RunGeneratedLoaderTests();
    FStructProperty* FontProperty = FindFProperty<FStructProperty>(Child->GeneratedClass, LidFont);
    check(FontProperty);
    FSlateFontInfo* Font = FontProperty->ContainerPtrToValuePtr<FSlateFontInfo>(
        Child->GeneratedClass->GetDefaultObject());
    check(Font);
    Font->FontObject = StockFont;
    Font->TypefaceFontName = BoldTypeface;
    Font->Size = LidFontSize;

    AddCachedSettingsFunction(Child);
    Compile(Child);
    AddCapacityFunction(Child);
    Compile(Child);
    AddBeginPlayCapacityOverride(Child);
    AddCapacityLifecycleOverride(Child, OnPersistentActorPostLoad);
    AddInitOverride(Child, Widget->GeneratedClass);
    AddTextOverride(Child, Widget->GeneratedClass);
    AddIconOverride(Child);
    Compile(Child);
    ValidateGenerated(Child);
    Save(Child);

    UE_LOG(LogTemp, Display, TEXT("Generated ContainerLidLabel replacement: %s"), StockLargePackage);
    return 0;
}

#endif
