#pragma once
#include "../../RailgunModelContract.h"

namespace RailgunInventory
{
inline constexpr TCHAR ModuleObjectPath[] =
    TEXT("/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun");
inline constexpr TCHAR ContainerOverlayPackage[] =
    TEXT("/Game/Data/UI/OverlayWidgets/DA_Widget_Container");
inline constexpr TCHAR ContainerOverlayAsset[] = TEXT("DA_Widget_Container");
inline constexpr TCHAR InventoryPartIdLiteral[] = TEXT("100");
inline const FName InventoryComponent(TEXT("RailgunAmmoInventory"));
inline const FName InteractionComponent(TEXT("RailgunAmmoInventoryInteraction"));
inline const FName InteractionQueryComponent(TEXT("RailgunAmmoInventoryQuery"));
inline const FName InteractionCollisionProfile(TEXT("Interactive"));
inline const FName AcceptedAmmo(TEXT("AcceptedRailgunAmmo"));
inline const FName ModuleComponent(TEXT("ModuleComponent"));
inline const FName ValidateItem(TEXT("ValidateItem"));
inline const FName InteractGetInventory(TEXT("InteractGetInventory"));
inline const FName ItemParameter(TEXT("Item"));
inline const FName IsValidParameter(TEXT("bIsValid"));
inline const FName PartIdParameter(TEXT("PartId"));
inline const FName NewMaxWeightLimitParameter(TEXT("NewMaxWeightLimit"));
inline constexpr int32 InventoryPartId = 100;
}

UEdGraph* AddRailgunInterfaceFunction(UBlueprint* BP, UClass* Interface,
    const FName FunctionName, bool RequireInheritedInterface)
{
    check(BP && Interface);
    UFunction* Function = Interface->FindFunctionByName(FunctionName);
    check(Function && Function->GetOuterUClass() == Interface);

    if (RequireInheritedInterface)
    {
        check(BP->ParentClass->ImplementsInterface(Interface));
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, FunctionName,
            UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false, Interface);
        return Graph;
    }

    check(!BP->ParentClass->ImplementsInterface(Interface));
    check(FBlueprintEditorUtils::ImplementNewInterface(BP, Interface->GetClassPathName()));
    for (const FBPInterfaceDescription& Description : BP->ImplementedInterfaces)
    {
        if (Description.Interface != Interface) continue;
        for (UEdGraph* Graph : Description.Graphs)
        {
            if (Graph->GetFName() == FunctionName) return Graph;
        }
    }
    checkNoEntry();
    return nullptr;
}

void FindFunctionTerminals(UEdGraph* Graph, UK2Node_FunctionEntry*& Entry,
    UK2Node_FunctionResult*& Result)
{
    Entry = nullptr;
    Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
        if (auto* Candidate = Cast<UK2Node_FunctionResult>(Node)) Result = Candidate;
    }
    check(Entry && Result);
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
    Result->FindPinChecked(P::Execute)->BreakAllPinLinks();
}

void AddRailgunInventoryLimitInitialization(UBlueprint* BP)
{
    check(BP && BP->UbergraphPages.Num() == 1);
    UEdGraph* Graph = BP->UbergraphPages[0];
    UK2Node_Event* BeginPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        auto* Event = Cast<UK2Node_Event>(Node);
        if (!Event || Event->EventReference.GetMemberName() !=
            TimerGraphNames::ActorBeginPlay) continue;
        check(!BeginPlay);
        BeginPlay = Event;
    }

    FGraph G(Graph, nullptr);
    if (!BeginPlay)
    {
        BeginPlay = NewObject<UK2Node_Event>(Graph);
        BeginPlay->EventReference.SetExternalMember(
            TimerGraphNames::ActorBeginPlay, AActor::StaticClass());
        BeginPlay->bOverrideFunction = true;
        G.Node(BeginPlay);
    }
    UEdGraphPin* BeginPlayTail = G.Pin(BeginPlay, P::Then);
    check(BeginPlayTail->LinkedTo.Num() <= 1);
    if (BeginPlayTail->LinkedTo.IsEmpty())
    {
        G.Tail = BeginPlayTail;
    }
    else
    {
        UEdGraphPin* ExistingBeginPlayWork = BeginPlayTail->LinkedTo[0];
        BeginPlayTail->BreakAllPinLinks();
        auto* BeginPlayWork = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
        G.Link(BeginPlayTail, G.Pin(BeginPlayWork, P::Execute));
        G.Link(BeginPlayWork->GetThenPinGivenIndex(0), ExistingBeginPlayWork);
        G.Tail = BeginPlayWork->GetThenPinGivenIndex(1);
    }

    UEdGraphPin* Inventory = G.Read(RailgunInventory::InventoryComponent);
    G.Branch(G.Valid(Inventory));
    const FName WeightLimit = GET_MEMBER_NAME_CHECKED(
        UVoyageInventoryWeightLimitedComponent, MaxWeightLimit);
    UEdGraphPin* AuthoredLimit = ReadNativeInputField(G, Inventory,
        UVoyageInventoryWeightLimitedComponent::StaticClass(), WeightLimit);
    auto* SetLimit = G.Call(UVoyageInventoryWeightLimitedComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageInventoryWeightLimitedComponent, SetMaxWeightLimit));
    G.Link(Inventory, G.Pin(SetLimit, P::FunctionTarget));
    G.Link(AuthoredLimit,
        G.Pin(SetLimit, RailgunInventory::NewMaxWeightLimitParameter));
    G.Exec(SetLimit);
}

void AddRailgunInventoryValidator(UBlueprint* BP)
{
    UEdGraph* Graph = AddRailgunInterfaceFunction(BP,
        UVoyageInventoryItemValidatorInterface::StaticClass(),
        RailgunInventory::ValidateItem, false);
    UK2Node_FunctionEntry* Entry;
    UK2Node_FunctionResult* Result;
    FindFunctionTerminals(Graph, Entry, Result);

    FGraph G(Graph, nullptr);
    G.Tail = G.Pin(Entry, P::Then);
    UEdGraphPin* CorrectItem = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(Entry, RailgunInventory::ItemParameter),
        G.Read(RailgunInventory::AcceptedAmmo));
    UEdGraphPin* Accepted = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Valid(G.Pin(Entry, RailgunInventory::ItemParameter)), CorrectItem);
    G.Link(Accepted, G.Pin(Result, RailgunInventory::IsValidParameter));
    G.Link(G.Tail, G.Pin(Result, P::Execute));
}

void AddRailgunInventoryInteraction(UBlueprint* BP)
{
    UEdGraph* Graph = AddRailgunInterfaceFunction(BP,
        UInteractiveInterface::StaticClass(), RailgunInventory::InteractGetInventory,
        true);
    UK2Node_FunctionEntry* Entry;
    UK2Node_FunctionResult* Result;
    FindFunctionTerminals(Graph, Entry, Result);

    FGraph G(Graph, nullptr);
    G.Tail = G.Pin(Entry, P::Then);
    UK2Node_IfThenElse* Part = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        G.Pin(Entry, RailgunInventory::PartIdParameter),
        RailgunInventory::InventoryPartIdLiteral));
    UEdGraphPin* Inventory = G.Read(RailgunInventory::ModuleComponent);
    auto* GetInventory = G.Call(UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetInternalInventory));
    G.Link(Inventory, G.Pin(GetInventory, P::FunctionTarget));
    UEdGraphPin* ReturnedInventory = G.Pin(GetInventory, P::ReturnValue);
    G.Link(ReturnedInventory, G.Pin(Result, P::ReturnValue));
    G.Link(G.Tail, G.Pin(Result, P::Execute));

    auto* EmptyResult = NewObject<UK2Node_FunctionResult>(Graph);
    EmptyResult->FunctionReference = Result->FunctionReference;
    G.Node(EmptyResult);
    G.Link(G.Pin(Part, P::Else), G.Pin(EmptyResult, P::Execute));
}

void ConfigureRailgunInventory(UVoyageItemAmmo* Ammo)
{
    using namespace RailgunInventory;
    check(Ammo && Ammo->Category == EVoyageItemCategory::Ammo && Ammo->CategoryAsset);
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr, ModuleObjectPath);
    check(BP && BP->ParentClass == AVoyageModuleActor::StaticClass());

    AddVariable(BP, AcceptedAmmo, UEdGraphSchema_K2::PC_Object,
        UVoyageItem::StaticClass());
    USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
    check(SCS);
    USCS_Node* InventoryReferenceNode = nullptr;
    for (USCS_Node* Node : SCS->GetAllNodes())
    {
        if (Node->GetVariableName() == RailgunModelContract::InventoryComponent)
        {
            check(!InventoryReferenceNode);
            InventoryReferenceNode = Node;
        }
    }
    check(InventoryReferenceNode);
    auto* InventoryReference = CastChecked<UBoxComponent>(
        InventoryReferenceNode->ComponentTemplate);
    check(InventoryReference->ComponentHasTag(RailgunModelContract::InventoryTag));
    USCS_Node* InventoryNode = SCS->CreateNode(
        UVoyageInventoryWeightLimitedComponent::StaticClass(), InventoryComponent);
    SCS->AddNode(InventoryNode);
    auto* Inventory = CastChecked<UVoyageInventoryWeightLimitedComponent>(
        InventoryNode->ComponentTemplate);
    Inventory->Type = EVoyageInventoryType::Container;
    Inventory->AcceptedItemCategories.Add(EVoyageItemCategory::Ammo);
    Inventory->Access = EVoyageInventoryAccessType::ReadWrite;
    Inventory->bAllowFiltering = false;
    Inventory->DepositAllCategoryFilter.Add(Ammo->CategoryAsset);
    Inventory->bAllowNearbyQueries = false;
    Inventory->bAutoCloseHudWhenEmpty = false;
    Inventory->MaxWeightLimit = RailgunAmmo::InventoryWeightLimit;
    Inventory->bAllowBeyondWeightLimit = false;

    USCS_Node* InteractionNode = SCS->CreateNode(
        UInteractiveObjectComponent::StaticClass(), InteractionComponent);
    InventoryReferenceNode->AddChildNode(InteractionNode);
    auto* Interaction = CastChecked<UInteractiveObjectComponent>(
        InteractionNode->ComponentTemplate);
    Interaction->InteractType = FVoyageInteractType::WidgetOverlay;
    Interaction->PartId = InventoryPartId;
    Interaction->OverlayWidget = CreateRailgunReference<UVoyageOverlayWidgetData>(
        ContainerOverlayPackage, ContainerOverlayAsset);
    check(Interaction->OverlayWidget);
    Interaction->SetRelativeLocation(FVector::ZeroVector);

    USCS_Node* QueryNode = SCS->CreateNode(
        UBoxComponent::StaticClass(), InteractionQueryComponent);
    InteractionNode->AddChildNode(QueryNode);
    auto* Query = CastChecked<UBoxComponent>(QueryNode->ComponentTemplate);
    Query->SetBoxExtent(InventoryReference->GetUnscaledBoxExtent());
    Query->SetCollisionProfileName(InteractionCollisionProfile);
    Query->SetGenerateOverlapEvents(false);
    Query->SetSimulatePhysics(false);

    AddRailgunInventoryValidator(BP);
    AddRailgunInventoryInteraction(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);
    AddRailgunInventoryLimitInitialization(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);
    auto* AcceptedAmmoProperty = FindFProperty<FObjectPropertyBase>(
        BP->GeneratedClass, AcceptedAmmo);
    check(AcceptedAmmoProperty);
    AcceptedAmmoProperty->SetObjectPropertyValue_InContainer(
        BP->GeneratedClass->GetDefaultObject(), Ammo);
    check(SaveDedicatedAsset(BP));
}
