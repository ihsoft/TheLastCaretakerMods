#pragma once
#include "../../RailgunModelContract.h"
#include "RailgunInventoryNames.h"

namespace RailgunInventory
{
using RailgunInventoryShared::ModuleObjectPath;
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
using RailgunInventoryShared::SyncVisuals;
inline const FName InventoryChangedCallback(TEXT("OnRailgunAmmoInventoryChanged"));
inline const FName OnInventoryChanged(TEXT("OnInventoryChanged"));
inline const FName OnPersistentActorPostLoad(TEXT("OnPersistentActorPostLoad"));
inline constexpr TCHAR InventoryDelegateSignaturePath[] =
    TEXT("/Script/Voyage.InventoryDelegate__DelegateSignature");
inline const FName VisualCount(TEXT("RailgunAmmoVisualCount"));
using RailgunInventoryShared::LastVisualCount;
inline const FName Items(TEXT("Items"));
inline const FName TargetMap(TEXT("TargetMap"));
inline const FName Values(TEXT("Values"));
inline const FName SerializedItem(TEXT("Item"));
inline const FName SerializedData(TEXT("Data"));
inline const FName ItemCount(TEXT("ItemCount"));
inline const FName NewHidden(TEXT("NewHidden"));
inline const FName PropagateToChildren(TEXT("bPropagateToChildren"));
inline const FName SetHiddenInGame(TEXT("SetHiddenInGame"));
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR InvalidVisualCount[] = TEXT("-1");
inline constexpr TCHAR MaximumVisualCount[] = TEXT("6");
inline constexpr int32 InventoryPartId = 100;
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

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
    G.Write(RailgunInventory::LastVisualCount, nullptr,
        RailgunInventory::InvalidVisualCount);

    auto* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        UVoyageBaseInventoryComponent::StaticClass(),
        RailgunInventory::OnInventoryChanged);
    check(DelegateProperty && DelegateProperty->SignatureFunction &&
        DelegateProperty->SignatureFunction->GetPathName() ==
            RailgunInventory::InventoryDelegateSignaturePath);
    auto* Callback = G.Node(NewObject<UK2Node_CreateDelegate>(Graph));
    auto* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false,
        UVoyageBaseInventoryComponent::StaticClass());
    G.Node(Remove);
    auto* Add = NewObject<UK2Node_AddDelegate>(Graph);
    Add->SetFromProperty(DelegateProperty, false,
        UVoyageBaseInventoryComponent::StaticClass());
    G.Node(Add);
    G.Link(Inventory, G.Pin(Remove, P::FunctionTarget));
    G.Link(Inventory, G.Pin(Add, P::FunctionTarget));
    G.Link(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    G.Link(Callback->GetDelegateOutPin(), Add->GetDelegatePin());
    Callback->SetFunction(RailgunInventory::InventoryChangedCallback);
    G.Exec(Remove);
    G.Exec(Add);
    auto* Sync = G.Call(BP->GeneratedClass, RailgunInventory::SyncVisuals);
    G.Exec(Sync);
}

void AddRailgunAmmoVisualSync(UBlueprint* BP)
{
    using namespace RailgunInventory;
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, SyncVisuals,
        UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        BP, Graph, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    check(Entry);
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();

    FGraph G(Graph, nullptr);
    G.Tail = G.Pin(Entry, P::Then);
    UEdGraphPin* Inventory = G.Read(InventoryComponent);
    G.Branch(G.Valid(Inventory));
    G.Write(VisualCount, nullptr, Zero);

    UEdGraphPin* InventoryItems = ReadNativeInputField(G, Inventory,
        UVoyageBaseInventoryComponent::StaticClass(), Items);
    auto* MapValues = G.Call(UBlueprintMapLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Values));
    G.Link(InventoryItems, G.Pin(MapValues, TargetMap));
    G.Exec(MapValues);
    auto* Loop = ContextLoop(G, G.Pin(MapValues, Values));
    auto* Record = NewObject<UK2Node_BreakStruct>(Graph);
    Record->StructType = FVoyageItemSerialize::StaticStruct();
    G.Node(Record);
    UEdGraphPin* RecordInput = nullptr;
    for (UEdGraphPin* Pin : Record->Pins)
        if (Pin->Direction == EGPD_Input) { check(!RecordInput); RecordInput = Pin; }
    check(RecordInput);
    G.Link(G.Pin(Loop, CE::ArrayElement), RecordInput);
    G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(Record, SerializedItem), G.Read(AcceptedAmmo)));
    auto* Data = NewObject<UK2Node_BreakStruct>(Graph);
    Data->StructType = FVoyageItemData::StaticStruct();
    G.Node(Data);
    UEdGraphPin* DataInput = nullptr;
    for (UEdGraphPin* Pin : Data->Pins)
        if (Pin->Direction == EGPD_Input) { check(!DataInput); DataInput = Pin; }
    check(DataInput);
    G.Link(G.Pin(Record, SerializedData), DataInput);
    G.Write(VisualCount, G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt),
        G.Read(VisualCount), G.Pin(Data, ItemCount)));

    G.Tail = G.Pin(Loop, CE::Completed);
    auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Clamp));
    G.Link(G.Read(VisualCount), G.Pin(Clamp, P::Value));
    G.Default(Clamp, P::Min, Zero);
    G.Default(Clamp, P::Max, MaximumVisualCount);
    UEdGraphPin* ClampedCount = G.Pin(Clamp, P::ReturnValue);
    G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_IntInt),
        ClampedCount, G.Read(LastVisualCount)));
    G.Write(LastVisualCount, ClampedCount);
    for (int32 Index = 0;
        Index < RailgunModelContract::AmmoCassetteRoots.Num(); ++Index)
    {
        auto* Hidden = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_IntInt));
        G.Link(ClampedCount, G.Pin(Hidden, P::Binary::LeftOperand));
        G.Default(Hidden, P::Binary::RightOperand, *FString::FromInt(Index));
        auto* SetHidden = G.Call(USceneComponent::StaticClass(),
            SetHiddenInGame);
        G.Link(G.Read(RailgunModelContract::AmmoCassetteRoots[Index]),
            G.Pin(SetHidden, P::FunctionTarget));
        G.Link(G.Pin(Hidden, P::ReturnValue), G.Pin(SetHidden, NewHidden));
        G.Default(SetHidden, PropagateToChildren, N::True);
        G.Exec(SetHidden);
    }
}

void AddRailgunAmmoVisualCallback(UBlueprint* BP)
{
    using namespace RailgunInventory;
    check(BP && BP->GeneratedClass);
    UFunction* SyncFunction = BP->GeneratedClass->FindFunctionByName(SyncVisuals);
    check(SyncFunction && SyncFunction->GetOuterUClass() == BP->GeneratedClass);
    auto* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        UVoyageBaseInventoryComponent::StaticClass(), OnInventoryChanged);
    check(DelegateProperty && DelegateProperty->SignatureFunction &&
        DelegateProperty->SignatureFunction->GetPathName() ==
            InventoryDelegateSignaturePath &&
        DelegateProperty->SignatureFunction->NumParms == 0);
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
        InventoryChangedCallback, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, true,
        DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    }
    check(Entry);
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph G(Graph, nullptr);
    G.Tail = G.Pin(Entry, P::Then);
    auto* Sync = G.Call(BP->GeneratedClass, SyncVisuals);
    G.Exec(Sync);
}

void AddRailgunAmmoVisualPostLoad(UBlueprint* BP)
{
    using namespace RailgunInventory;
    check(BP && BP->UbergraphPages.Num() == 1 && BP->GeneratedClass);
    UClass* Interface = UPersistentInterface::StaticClass();
    UFunction* Function = Interface->FindFunctionByName(OnPersistentActorPostLoad);
    check(Function && Function->GetOuterUClass() == Interface &&
        BP->ParentClass->ImplementsInterface(Interface));
    UEdGraph* Graph = BP->UbergraphPages[0];
    FGraph G(Graph, nullptr);
    auto* PostLoad = NewObject<UK2Node_Event>(Graph);
    PostLoad->EventReference.SetExternalMember(OnPersistentActorPostLoad, Interface);
    PostLoad->bOverrideFunction = true;
    G.Node(PostLoad);
    G.Tail = G.Pin(PostLoad, P::Then);
    auto* Delay = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, DelayUntilNextTick));
    G.Exec(Delay);
    G.Write(LastVisualCount, nullptr, InvalidVisualCount);
    auto* Sync = G.Call(BP->GeneratedClass, SyncVisuals);
    G.Exec(Sync);
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
    AddVariable(BP, VisualCount, UEdGraphSchema_K2::PC_Int);
    AddVariable(BP, LastVisualCount, UEdGraphSchema_K2::PC_Int);
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
    AddRailgunAmmoVisualSync(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);
    AddRailgunAmmoVisualCallback(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);
    AddRailgunInventoryLimitInitialization(BP);
    AddRailgunAmmoVisualPostLoad(BP);
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
