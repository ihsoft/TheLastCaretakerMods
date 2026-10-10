#include "RailgunReload.h"

#include "DedicatedStationGenerator.h"
#include "DedicatedStationNames.h"
#include "ContextEntryNames.h"
#include "GraphCallHelpers.h"
#include "NativeVehicleGraphHelpers.h"
#include "RailgunInputNames.h"
#include "RailgunInventory.h"
#include "RailgunRuntimeGeneratorPrivate.h"
#include "RailgunWater.h"
#include "StationAttachmentGraph.h"
#include "StationEnergy.h"
#include "StationEntryGraph.h"
#include "StationOpticsGraph.h"

#include "VoyageBaseCharacter.h"

namespace Railgun::Runtime
{
namespace
{
FMulticastDelegateProperty* InventoryChangedDelegateProperty()
{
    auto* Property = FindFProperty<FMulticastDelegateProperty>(
        UVoyageBaseInventoryComponent::StaticClass(),
        RailgunInventory::OnInventoryChanged);
    check(Property && Property->SignatureFunction &&
        Property->SignatureFunction->GetPathName() ==
            Reload::InventoryDelegateSignaturePath &&
        Property->SignatureFunction->NumParms == 0);
    return Property;
}

void CountAcceptedAmmo(FGraph& G, UEdGraphPin* Inventory, FName CountField)
{
    G.Write(CountField, nullptr, Reload::Zero);
    UEdGraphPin* InventoryItems = ReadNativeInputField(G, Inventory,
        UVoyageBaseInventoryComponent::StaticClass(),
        RailgunInventory::Items);
    auto* MapValues = G.Call(UBlueprintMapLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Values));
    G.Link(InventoryItems, G.Pin(MapValues, RailgunInventory::TargetMap));
    G.Exec(MapValues);
    auto* Loop = ContextLoop(G, G.Pin(MapValues, RailgunInventory::Values));
    auto* Record = NewObject<UK2Node_BreakStruct>(G.Graph);
    Record->StructType = FVoyageItemSerialize::StaticStruct();
    G.Node(Record);
    UEdGraphPin* RecordInput = nullptr;
    for (UEdGraphPin* Pin : Record->Pins)
        if (Pin->Direction == EGPD_Input)
        {
            check(!RecordInput);
            RecordInput = Pin;
        }
    check(RecordInput);
    G.Link(G.Pin(Loop, CE::ArrayElement), RecordInput);
    G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject),
        G.Pin(Record, RailgunInventory::SerializedItem),
        G.Read(Reload::AcceptedAmmo)));
    auto* Data = NewObject<UK2Node_BreakStruct>(G.Graph);
    Data->StructType = FVoyageItemData::StaticStruct();
    G.Node(Data);
    UEdGraphPin* DataInput = nullptr;
    for (UEdGraphPin* Pin : Data->Pins)
        if (Pin->Direction == EGPD_Input)
        {
            check(!DataInput);
            DataInput = Pin;
        }
    check(DataInput);
    G.Link(G.Pin(Record, RailgunInventory::SerializedData), DataInput);
    G.Write(CountField, G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt),
        G.Read(CountField),
        G.Pin(Data, RailgunInventory::ItemCount)));
    G.Tail = G.Pin(Loop, CE::Completed);
}

UK2Node_CreateDelegate* ReloadCallback(FGraph& G)
{
    auto* Callback = G.Node(NewObject<UK2Node_CreateDelegate>(G.Graph));
    Callback->SetFunction(Reload::InventoryChanged);
    return Callback;
}

void RemoveInventoryDelegate(FGraph& G, UEdGraphPin* Inventory,
    UK2Node_CreateDelegate* Callback,
    FMulticastDelegateProperty* DelegateProperty)
{
    auto* Remove = NewObject<UK2Node_RemoveDelegate>(G.Graph);
    Remove->SetFromProperty(DelegateProperty, false,
        UVoyageBaseInventoryComponent::StaticClass());
    G.Node(Remove);
    G.Link(Inventory, G.Pin(Remove, P::FunctionTarget));
    G.Link(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    G.Exec(Remove);
}

void AddInventoryDelegate(FGraph& G, UEdGraphPin* Inventory,
    UK2Node_CreateDelegate* Callback,
    FMulticastDelegateProperty* DelegateProperty)
{
    auto* Add = NewObject<UK2Node_AddDelegate>(G.Graph);
    Add->SetFromProperty(DelegateProperty, false,
        UVoyageBaseInventoryComponent::StaticClass());
    G.Node(Add);
    G.Link(Inventory, G.Pin(Add, P::FunctionTarget));
    G.Link(Callback->GetDelegateOutPin(), Add->GetDelegatePin());
    G.Exec(Add);
}

FEdGraphPinType ReloadPinType(FName Category,
    EPinContainerType Container = EPinContainerType::None)
{
    FEdGraphPinType Type;
    Type.PinCategory = Category;
    Type.ContainerType = Container;
    return Type;
}

FBPVariableDescription AddReloadLocal(UK2Node_FunctionEntry* Entry,
    FName Name, const FEdGraphPinType& Type)
{
    FBPVariableDescription Description;
    Description.VarName = Name;
    Description.VarGuid = FGuid::NewGuid();
    Description.VarType = Type;
    Description.PropertyFlags = CPF_BlueprintVisible;
    Description.FriendlyName = FName::NameToDisplayString(Name.ToString(),
        Type.PinCategory == UEdGraphSchema_K2::PC_Boolean);
    Description.Category = UEdGraphSchema_K2::VR_DefaultCategory;
    Entry->LocalVariables.Add(Description);
    return Description;
}

UEdGraphPin* ReadReloadLocal(FGraph& G,
    const FBPVariableDescription& Local)
{
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetLocalMember(Local.VarName, G.Graph->GetName(),
        Local.VarGuid);
    G.Node(Get);
    return G.Pin(Get, Local.VarName);
}

void WriteReloadLocal(FGraph& G, const FBPVariableDescription& Local,
    UEdGraphPin* Value, const TCHAR* Literal = nullptr)
{
    auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
    Set->VariableReference.SetLocalMember(Local.VarName, G.Graph->GetName(),
        Local.VarGuid);
    G.Node(Set);
    if (Value)
        G.Link(Value, G.Pin(Set, Local.VarName));
    else
        G.Default(Set, Local.VarName, Literal);
    G.Exec(Set);
}
}

void AddRailgunReloadVariables(UBlueprint* BP)
{
    check(BP);
    AddVariable(BP, Reload::Available,
        UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Reload::SourceInventory,
        UEdGraphSchema_K2::PC_Object,
        UVoyageBaseInventoryComponent::StaticClass());
    AddVariable(BP, Reload::TargetInventory,
        UEdGraphSchema_K2::PC_Object,
        UVoyageBaseInventoryComponent::StaticClass());
    AddVariable(BP, Reload::AcceptedAmmo,
        UEdGraphSchema_K2::PC_Object, UVoyageItem::StaticClass());
    AddVariable(BP, Reload::SourceCount,
        UEdGraphSchema_K2::PC_Int);
    AddVariable(BP, Reload::TargetCount,
        UEdGraphSchema_K2::PC_Int);
    for (FName Field : {Reload::Available, Reload::SourceInventory,
        Reload::TargetInventory, Reload::AcceptedAmmo,
        Reload::SourceCount, Reload::TargetCount})
        MarkVariableTransient(BP, Field);
}

void AddRailgunReloadFunctions(UBlueprint* BP)
{
    check(BP && BP->GeneratedClass);
    auto Compile = [&]()
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FKismetEditorUtilities::CompileBlueprint(BP);
        check(BP->Status != BS_Error && BP->GeneratedClass);
    };
    auto AddFunction = [&](FName Name, auto Build)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, Name,
            UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        FGraph G(Graph);
        G.Tail = G.Pin(Entry, P::Then);
        Build(G);
    };

    AddFunction(Reload::Refresh, [&](FGraph& G)
    {
        G.Write(Reload::Available, nullptr, N::False);
        G.Branch(ObserveCall(G, APawn::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled),
            OpticalSelf(G)));
        G.Branch(G.Valid(G.Read(Reload::SourceInventory)));
        G.Branch(G.Valid(G.Read(Reload::TargetInventory)));
        G.Branch(G.Valid(G.Read(Reload::AcceptedAmmo)));
        G.Branch(G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                NotEqual_ObjectObject),
            G.Read(Reload::SourceInventory),
            G.Read(Reload::TargetInventory)));
        CountAcceptedAmmo(G, G.Read(Reload::SourceInventory),
            Reload::SourceCount);
        CountAcceptedAmmo(G, G.Read(Reload::TargetInventory),
            Reload::TargetCount);
        UEdGraphPin* HasSource = G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                Greater_IntInt),
            G.Read(Reload::SourceCount), Reload::Zero);
        UEdGraphPin* HasSpace = G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                Less_IntInt),
            G.Read(Reload::TargetCount), Reload::Capacity);
        G.Write(Reload::Available, G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
            HasSource, HasSpace));
    });
    Compile();
    check(BP->GeneratedClass->FindFunctionByName(Reload::Refresh));

    FMulticastDelegateProperty* DelegateProperty =
        InventoryChangedDelegateProperty();
    UEdGraph* CallbackGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        Reload::InventoryChanged, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, CallbackGraph, true,
        DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* CallbackEntry = nullptr;
    for (UEdGraphNode* Node : CallbackGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            CallbackEntry = Candidate;
    check(CallbackEntry);
    CallbackEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Callback(CallbackGraph);
    Callback.Tail = Callback.Pin(CallbackEntry, P::Then);
    auto* Refresh = Callback.Call(BP->GeneratedClass, Reload::Refresh);
    Callback.Exec(Refresh);
    Compile();
    check(BP->GeneratedClass->FindFunctionByName(Reload::InventoryChanged));

    AddFunction(Reload::Unbind, [&](FGraph& G)
    {
        auto* CallbackNode = ReloadCallback(G);
        auto* SourceValid = G.Branch(
            G.Valid(G.Read(Reload::SourceInventory)));
        RemoveInventoryDelegate(G, G.Read(Reload::SourceInventory),
            CallbackNode, DelegateProperty);
        UEdGraphPin* SourceRemoved = G.Tail;
        G.Tail = G.Pin(SourceValid, P::Else);
        StationMerge(G, {SourceRemoved, G.Tail});
        auto* TargetValid = G.Branch(
            G.Valid(G.Read(Reload::TargetInventory)));
        RemoveInventoryDelegate(G, G.Read(Reload::TargetInventory),
            CallbackNode, DelegateProperty);
        UEdGraphPin* TargetRemoved = G.Tail;
        G.Tail = G.Pin(TargetValid, P::Else);
        StationMerge(G, {TargetRemoved, G.Tail});
        G.Write(Reload::SourceInventory, nullptr);
        G.Write(Reload::TargetInventory, nullptr);
        G.Write(Reload::AcceptedAmmo, nullptr);
        G.Write(Reload::SourceCount, nullptr, Reload::Zero);
        G.Write(Reload::TargetCount, nullptr, Reload::Zero);
        G.Write(Reload::Available, nullptr, N::False);
    });
    Compile();
    check(BP->GeneratedClass->FindFunctionByName(Reload::Unbind));

    AddFunction(Reload::Bind, [&](FGraph& G)
    {
        auto* Unbind = G.Call(BP->GeneratedClass, Reload::Unbind);
        G.Exec(Unbind);
        auto* Character = NewObject<UK2Node_DynamicCast>(G.Graph);
        Character->TargetType = AVoyageBaseCharacter::StaticClass();
        Character->SetPurity(true);
        G.Node(Character);
        G.Link(G.Read(N::OriginalPawn), Character->GetCastSourcePin());
        G.Branch(G.Valid(Character->GetCastResultPin()));
        UEdGraphPin* Source = ReadNativeInputField(G,
            Character->GetCastResultPin(), AVoyageBaseCharacter::StaticClass(),
            Reload::WeightInventory);
        G.Branch(G.Valid(Source));
        G.Branch(G.Valid(G.Read(Charge::Module)));
        G.Branch(G.Valid(G.Read(S::Anchor)));
        UEdGraphPin* Shell = ObserveCall(G, UActorComponent::StaticClass(),
            OP::ComponentOwner, G.Read(S::Anchor));
        auto* ModuleBlueprint = LoadObject<UBlueprint>(nullptr,
            RailgunInventoryShared::ModuleObjectPath);
        check(ModuleBlueprint && ModuleBlueprint->GeneratedClass);
        UClass* ModuleClass = ModuleBlueprint->GeneratedClass;
        auto* ModuleActor = NewObject<UK2Node_DynamicCast>(G.Graph);
        ModuleActor->TargetType = ModuleClass;
        ModuleActor->SetPurity(false);
        G.Node(ModuleActor);
        G.Link(G.Tail, G.Pin(ModuleActor, P::Execute));
        G.Link(Shell, ModuleActor->GetCastSourcePin());
        G.Tail = ModuleActor->GetValidCastPin();
        UEdGraphPin* AcceptedAmmo = ReadNativeInputField(G,
            ModuleActor->GetCastResultPin(), ModuleClass,
            RailgunInventoryShared::AcceptedAmmo);
        G.Branch(G.Valid(AcceptedAmmo));
        auto* GetTarget = G.Call(UVoyageModuleComponent::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent,
                GetInternalInventory));
        G.Link(G.Read(Charge::Module),
            G.Pin(GetTarget, P::FunctionTarget));
        UEdGraphPin* Target = G.Pin(GetTarget, P::ReturnValue);
        G.Branch(G.Valid(Target));
        G.Branch(G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                NotEqual_ObjectObject), Source, Target));
        G.Write(Reload::SourceInventory, Source);
        G.Write(Reload::TargetInventory, Target);
        G.Write(Reload::AcceptedAmmo, AcceptedAmmo);
        auto* CallbackNode = ReloadCallback(G);
        RemoveInventoryDelegate(G, Source, CallbackNode, DelegateProperty);
        AddInventoryDelegate(G, Source, CallbackNode, DelegateProperty);
        RemoveInventoryDelegate(G, Target, CallbackNode, DelegateProperty);
        AddInventoryDelegate(G, Target, CallbackNode, DelegateProperty);
        auto* RefreshBound = G.Call(BP->GeneratedClass, Reload::Refresh);
        G.Exec(RefreshBound);
    });
    Compile();
    check(BP->GeneratedClass->FindFunctionByName(Reload::Bind));

    UClass* InventoryClass = UVoyageBaseInventoryComponent::StaticClass();
    UFunction* FindSlots = InventoryClass->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UVoyageBaseInventoryComponent,
            FindSlotsByItem));
    check(FindSlots && FindSlots->GetOuterUClass() == InventoryClass);
    check(FindFProperty<FObjectPropertyBase>(FindSlots,
        Reload::ItemParameter));
    check(FindFProperty<FArrayProperty>(FindSlots,
        Reload::OutSlotsParameter));
    UFunction* Transfer = InventoryClass->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UVoyageBaseInventoryComponent,
            TransferSlot));
    check(Transfer && Transfer->GetOuterUClass() == InventoryClass);
    check(FindFProperty<FObjectPropertyBase>(Transfer,
        Reload::SourceInventoryParameter));
    check(FindFProperty<FIntProperty>(Transfer,
        Reload::SourceSlotParameter));
    check(FindFProperty<FIntProperty>(Transfer,
        Reload::TargetSlotParameter));
    check(FindFProperty<FIntProperty>(Transfer,
        Reload::TransferAmountParameter));
    UFunction* GetSlot = InventoryClass->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UVoyageBaseInventoryComponent, GetSlot));
    check(GetSlot && GetSlot->GetOuterUClass() == InventoryClass);
    check(FindFProperty<FIntProperty>(GetSlot, Reload::SlotParameter));
    check(FindFProperty<FStructProperty>(GetSlot,
        Reload::OutItemDataParameter));
    UClass* ValidatorInterface =
        UVoyageInventoryItemValidatorInterface::StaticClass();
    UFunction* Validate = ValidatorInterface->FindFunctionByName(
        RailgunInventory::ValidateItem);
    check(Validate && Validate->GetOuterUClass() ==
        ValidatorInterface);
    check(FindFProperty<FObjectPropertyBase>(Validate,
        Reload::InventoryParameter));
    check(FindFProperty<FObjectPropertyBase>(Validate,
        Reload::ItemParameter));
    check(FindFProperty<FBoolProperty>(Validate,
        Reload::IsValidParameter));
    check(Transfer->GetReturnProperty() &&
        CastField<FIntProperty>(Transfer->GetReturnProperty()));
    check(GetSlot->GetReturnProperty() &&
        CastField<FBoolProperty>(GetSlot->GetReturnProperty()));
    AddFunction(Reload::Execute, [&](FGraph& G)
    {
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : G.Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        const FBPVariableDescription SlotSnapshot = AddReloadLocal(Entry,
            Reload::SlotSnapshot, ReloadPinType(UEdGraphSchema_K2::PC_Int,
                EPinContainerType::Array));
        const FEdGraphPinType IntType = ReloadPinType(
            UEdGraphSchema_K2::PC_Int);
        const FBPVariableDescription Remaining = AddReloadLocal(Entry,
            Reload::Remaining, IntType);
        const FBPVariableDescription Moved = AddReloadLocal(Entry,
            Reload::Moved, IntType);
        const FBPVariableDescription CurrentSlot = AddReloadLocal(Entry,
            Reload::CurrentSlot, IntType);
        const FBPVariableDescription Requested = AddReloadLocal(Entry,
            Reload::Requested, IntType);
        const FBPVariableDescription Validated = AddReloadLocal(Entry,
            Reload::Validated,
            ReloadPinType(UEdGraphSchema_K2::PC_Boolean));
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FKismetEditorUtilities::CompileBlueprint(BP);
        checkf(BP->Status != BS_Error,
            TEXT("Failed to establish Railgun reload function locals"));

        auto* RefreshLive = G.Call(BP->GeneratedClass, Reload::Refresh);
        G.Exec(RefreshLive);
        G.Branch(G.Read(Reload::Available));

        UEdGraphPin* Shell = ObserveCall(G, UActorComponent::StaticClass(),
            OP::ComponentOwner, G.Read(S::Anchor));
        G.Branch(G.Valid(Shell));
        auto* ModuleBlueprint = LoadObject<UBlueprint>(nullptr,
            RailgunInventoryShared::ModuleObjectPath);
        check(ModuleBlueprint && ModuleBlueprint->GeneratedClass &&
            ModuleBlueprint->GeneratedClass->ImplementsInterface(
                ValidatorInterface));
        auto* TypedShell = NewObject<UK2Node_DynamicCast>(G.Graph);
        TypedShell->TargetType = ModuleBlueprint->GeneratedClass;
        TypedShell->SetPurity(true);
        G.Node(TypedShell);
        G.Link(Shell, TypedShell->GetCastSourcePin());
        G.Branch(G.Valid(TypedShell->GetCastResultPin()));
        auto* ValidateCall = G.Call(ValidatorInterface,
            RailgunInventory::ValidateItem);
        G.Link(TypedShell->GetCastResultPin(),
            G.Pin(ValidateCall, P::FunctionTarget));
        G.Link(G.Read(Reload::TargetInventory),
            G.Pin(ValidateCall, Reload::InventoryParameter));
        G.Link(G.Read(Reload::AcceptedAmmo),
            G.Pin(ValidateCall, Reload::ItemParameter));
        G.Exec(ValidateCall);
        WriteReloadLocal(G, Validated,
            G.Pin(ValidateCall, Reload::IsValidParameter));
        G.Branch(ReadReloadLocal(G, Validated));

        auto* RemainingCapacity = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt));
        G.Default(RemainingCapacity, P::Binary::LeftOperand,
            Reload::Capacity);
        G.Link(G.Read(Reload::TargetCount),
            G.Pin(RemainingCapacity, P::Binary::RightOperand));
        WriteReloadLocal(G, Remaining,
            G.Pin(RemainingCapacity, P::ReturnValue));
        G.Branch(G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            ReadReloadLocal(G, Remaining), Reload::Zero));

        auto* FindSlotsCall = G.Call(InventoryClass,
            GET_FUNCTION_NAME_CHECKED(UVoyageBaseInventoryComponent,
                FindSlotsByItem));
        G.Link(G.Read(Reload::SourceInventory),
            G.Pin(FindSlotsCall, P::FunctionTarget));
        G.Link(G.Read(Reload::AcceptedAmmo),
            G.Pin(FindSlotsCall, Reload::ItemParameter));
        WriteReloadLocal(G, SlotSnapshot,
            G.Pin(FindSlotsCall, Reload::OutSlotsParameter));

        auto* SnapshotLength = G.ArrayCall(
            GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length));
        G.Link(ReadReloadLocal(G, SlotSnapshot),
            G.Pin(SnapshotLength, P::TargetArray));
        auto* HasSnapshot = G.Branch(G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            G.Pin(SnapshotLength, P::ReturnValue), Reload::Zero));
        auto* LastIndex = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt));
        G.Link(G.Pin(SnapshotLength, P::ReturnValue),
            G.Pin(LastIndex, P::Binary::LeftOperand));
        G.Default(LastIndex, P::Binary::RightOperand, Reload::One);
        UEdGraphPin* LastSnapshotIndex = G.Pin(LastIndex, P::ReturnValue);
        auto* Loop = RailgunWaterLoop(G, LastSnapshotIndex, nullptr,
            Reload::Zero);
        auto* GetSnapshotSlot = G.ArrayCall(
            GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get));
        G.Link(ReadReloadLocal(G, SlotSnapshot),
            G.Pin(GetSnapshotSlot, P::TargetArray));
        G.Link(G.Pin(Loop, P::Index), G.Pin(GetSnapshotSlot, P::Index));
        WriteReloadLocal(G, CurrentSlot,
            G.Pin(GetSnapshotSlot, Reload::ArrayItem));
        auto* HasRemaining = G.Branch(G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            ReadReloadLocal(G, Remaining), Reload::Zero));
        G.Link(G.Pin(HasRemaining, P::Else),
            G.Pin(Loop, Reload::BreakLoop));

        auto* GetSlotCall = G.Call(InventoryClass,
            GET_FUNCTION_NAME_CHECKED(UVoyageBaseInventoryComponent, GetSlot));
        G.Link(G.Read(Reload::SourceInventory),
            G.Pin(GetSlotCall, P::FunctionTarget));
        G.Link(ReadReloadLocal(G, CurrentSlot),
            G.Pin(GetSlotCall, Reload::SlotParameter));
        auto* SlotExists = G.Branch(G.Pin(GetSlotCall, P::ReturnValue));
        G.Link(G.Pin(SlotExists, P::Else),
            G.Pin(Loop, Reload::BreakLoop));
        auto* Record = NewObject<UK2Node_BreakStruct>(G.Graph);
        Record->StructType = FVoyageItemSerialize::StaticStruct();
        G.Node(Record);
        UEdGraphPin* RecordInput = nullptr;
        for (UEdGraphPin* Pin : Record->Pins)
            if (Pin->Direction == EGPD_Input)
            {
                check(!RecordInput);
                RecordInput = Pin;
            }
        check(RecordInput);
        G.Link(G.Pin(GetSlotCall, Reload::OutItemDataParameter), RecordInput);
        auto* ExactItem = G.Branch(G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                EqualEqual_ObjectObject),
            G.Pin(Record, RailgunInventory::SerializedItem),
            G.Read(Reload::AcceptedAmmo)));
        G.Link(G.Pin(ExactItem, P::Else),
            G.Pin(Loop, Reload::BreakLoop));
        auto* Data = NewObject<UK2Node_BreakStruct>(G.Graph);
        Data->StructType = FVoyageItemData::StaticStruct();
        G.Node(Data);
        UEdGraphPin* DataInput = nullptr;
        for (UEdGraphPin* Pin : Data->Pins)
            if (Pin->Direction == EGPD_Input)
            {
                check(!DataInput);
                DataInput = Pin;
            }
        check(DataInput);
        G.Link(G.Pin(Record, RailgunInventory::SerializedData), DataInput);
        UEdGraphPin* LiveCount = G.Pin(Data, RailgunInventory::ItemCount);
        auto* PositiveCount = G.Branch(G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            LiveCount, Reload::Zero));
        G.Link(G.Pin(PositiveCount, P::Else),
            G.Pin(Loop, Reload::BreakLoop));

        auto* Minimum = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Min));
        G.Link(LiveCount, G.Pin(Minimum, P::Binary::LeftOperand));
        G.Link(ReadReloadLocal(G, Remaining),
            G.Pin(Minimum, P::Binary::RightOperand));
        WriteReloadLocal(G, Requested, G.Pin(Minimum, P::ReturnValue));
        auto* PositiveRequest = G.Branch(G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            ReadReloadLocal(G, Requested), Reload::Zero));
        G.Link(G.Pin(PositiveRequest, P::Else),
            G.Pin(Loop, Reload::BreakLoop));

        auto* TransferCall = G.Call(InventoryClass,
            GET_FUNCTION_NAME_CHECKED(UVoyageBaseInventoryComponent,
                TransferSlot));
        G.Link(G.Read(Reload::TargetInventory),
            G.Pin(TransferCall, P::FunctionTarget));
        G.Link(G.Read(Reload::SourceInventory),
            G.Pin(TransferCall, Reload::SourceInventoryParameter));
        G.Link(ReadReloadLocal(G, CurrentSlot),
            G.Pin(TransferCall, Reload::SourceSlotParameter));
        G.Default(TransferCall, Reload::TargetSlotParameter,
            Reload::AutoPlacementSlot);
        G.Link(ReadReloadLocal(G, Requested),
            G.Pin(TransferCall, Reload::TransferAmountParameter));
        G.Exec(TransferCall);
        WriteReloadLocal(G, Moved, G.Pin(TransferCall, P::ReturnValue));
        UEdGraphPin* PositiveMoved = G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            ReadReloadLocal(G, Moved), Reload::Zero);
        UEdGraphPin* BoundedMoved = G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_IntInt),
            ReadReloadLocal(G, Moved), ReadReloadLocal(G, Requested));
        auto* ValidMoved = G.Branch(G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
            PositiveMoved, BoundedMoved));
        G.Link(G.Pin(ValidMoved, P::Else),
            G.Pin(Loop, Reload::BreakLoop));
        WriteReloadLocal(G, Remaining, G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt),
            ReadReloadLocal(G, Remaining), ReadReloadLocal(G, Moved)));

        G.Tail = G.Pin(Loop, CE::Completed);
        auto* RefreshAfter = G.Call(BP->GeneratedClass, Reload::Refresh);
        G.Exec(RefreshAfter);
    });

    Compile();
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Reload::Refresh) &&
        BP->GeneratedClass->FindFunctionByName(Reload::Bind) &&
        BP->GeneratedClass->FindFunctionByName(Reload::Unbind) &&
        BP->GeneratedClass->FindFunctionByName(Reload::Execute));
}

void BindRailgunReloadOnEntry(FGraph& G, UClass* StationClass)
{
    auto* Bind = G.Call(StationClass, Reload::Bind);
    G.Exec(Bind);
}

void AddRailgunReloadInput(FGraph& G, UClass* StationClass)
{
    auto* Action = LoadObject<UInputAction>(nullptr,
        RailgunInputNames::Reload);
    check(Action);
    auto* Event = NewObject<UK2Node_EnhancedInputAction>(G.Graph);
    Event->InputAction = Action;
    G.Node(Event);
    G.Tail = G.Pin(Event, DS::Started);
    auto* ReloadCall = G.Call(StationClass, Reload::Execute);
    G.Exec(ReloadCall);
}

void UnbindRailgunReload(FGraph& G, UClass* StationClass)
{
    auto* Unbind = G.Call(StationClass, Reload::Unbind);
    G.Exec(Unbind);
}
}
