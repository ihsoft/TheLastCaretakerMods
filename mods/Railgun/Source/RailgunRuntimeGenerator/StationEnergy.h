#pragma once
#include "VoyageModuleComponent.h"
namespace Charge
{
inline const FName Module(TEXT("RailgunEnergyModule"));
inline const FName Energy(TEXT("RailgunChargeAmount"));
inline const FName DemandInitialized(TEXT("RailgunEnergyDemandInitialized"));
inline const FName DemandCharging(TEXT("RailgunEnergyDemandCharging"));
inline const FName UpdateActive(TEXT("RailgunEnergyUpdateActive"));
inline const FName UpdatePending(TEXT("RailgunEnergyUpdatePending"));
inline const FName RefreshFunction(TEXT("RefreshRailgunEnergy"));
inline const FName BindFunction(TEXT("BindRailgunEnergy"));
inline const FName CallbackFunction(TEXT("OnRailgunEnergyModuleValueChanged"));
inline const FName OnModuleValueChanged(TEXT("OnModuleValueChanged"));
inline const FName ModuleParameter(TEXT("Module"));
inline const FName ForceDemandParameter(TEXT("ForceDemand"));
inline const FName ConfiguredEnergyKWh(TEXT("RailgunFullChargeEnergyKWh"));
inline const FName ConfiguredTimeSeconds(TEXT("RailgunFullChargeTimeSeconds"));
inline const FName Type(TEXT("Type")), RemoveAmount(TEXT("RemoveAmount")),
    AddAmount(TEXT("AddAmount")), RemovalType(TEXT("RemovalType"));
inline const FName Input(TEXT("InAcceptanceFilter")), Capacity(TEXT("InMaxResourceAmount")), Idle(TEXT("InConsumptionON"));
inline constexpr TCHAR Electricity[] = TEXT("Electricity"), ExactRemoval[] = TEXT("ConsumptionAfterModifiers");
inline constexpr TCHAR IdleW[] = TEXT("1000.0");
inline constexpr TCHAR IdleCapacityAmount[] = TEXT("1.0");
inline constexpr TCHAR GameResourceUnitsPerKWh[] = TEXT("1000.0");
inline constexpr TCHAR WattsPerResourceUnit[] = TEXT("1000.0");
inline constexpr TCHAR WattsPerKilowatt[] = TEXT("1000.0");
inline constexpr TCHAR SecondsPerHour[] = TEXT("3600.0");
inline constexpr TCHAR DelegateSignaturePath[] =
    TEXT("/Script/Voyage.VoyageModuleCompDelegate__DelegateSignature");
}

// Public tuning and HUD values use the game's displayed KWh scale. The module stores
// 1000 native electricity amount units per displayed KWh, while custom demand
// is expressed in W. Keep that game contract at this boundary.
UEdGraphPin* EnergyMath(FGraph& G, FName Function, UEdGraphPin* Left, const TCHAR* Right)
{
    auto* N = G.Call(UKismetMathLibrary::StaticClass(), Function);
    G.Link(Left, G.Pin(N, P::Binary::LeftOperand)); G.Default(N, P::Binary::RightOperand, Right);
    return G.Pin(N, P::ReturnValue);
}
UEdGraphPin* RequiredEnergyAmount(FGraph& G, UEdGraphPin* EnergyKWh)
{
    return EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble),
        EnergyKWh, Charge::GameResourceUnitsPerKWh);
}
UEdGraphPin* RequiredEnergyAmount(FGraph& G)
{
    return RequiredEnergyAmount(G, G.Read(Charge::ConfiguredEnergyKWh));
}
UEdGraphPin* EnergyCapacityAmount(FGraph& G)
{
    return EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble),
        RequiredEnergyAmount(G), Charge::IdleCapacityAmount);
}
UEdGraphPin* ChargingInputW(FGraph& G)
{
    auto* ChargeInput = EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble),
        RequiredEnergyAmount(G), Charge::WattsPerResourceUnit);
    auto* NetChargeW = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble),
        ChargeInput, G.Read(Charge::ConfiguredTimeSeconds));
    return EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), NetChargeW, Charge::IdleW);
}
UEdGraphPin* OfflineDrainAmount(FGraph& G, UEdGraphPin* DeltaSeconds,
    UEdGraphPin* StoredEnergy, UEdGraphPin* OfflineDischargeKW)
{
    UEdGraphPin* OfflineDischargeW = EnergyMath(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble),
        OfflineDischargeKW, Charge::WattsPerKilowatt);
    auto* OfflineEnergySeconds = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble),
        DeltaSeconds, OfflineDischargeW);
    UEdGraphPin* OfflineEnergyAmount = EnergyMath(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble),
        OfflineEnergySeconds, Charge::SecondsPerHour);
    return G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin),
        StoredEnergy, OfflineEnergyAmount);
}
FMulticastDelegateProperty* RailgunModuleValueDelegateProperty()
{
    auto* Property = FindFProperty<FMulticastDelegateProperty>(
        UVoyageModuleComponent::StaticClass(), Charge::OnModuleValueChanged);
    check(Property && Property->SignatureFunction &&
        Property->SignatureFunction->GetPathName() ==
            Charge::DelegateSignaturePath &&
        Property->SignatureFunction->NumParms == 1);
    auto* Module = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, Charge::ModuleParameter);
    check(Module && Module->HasAnyPropertyFlags(CPF_Parm) &&
        Module->PropertyClass == UVoyageModuleComponent::StaticClass());
    return Property;
}

UEdGraphPin* ResolveEnergyModule(FGraph& G)
{
    G.Branch(G.Valid(G.Read(S::Anchor)));
    auto* Railgun = ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor));
    G.Branch(G.Valid(Railgun));
    auto* Find = G.Call(AActor::StaticClass(), OpticalCameraGraphNames::FindComponent);
    G.Link(Railgun, G.Pin(Find, P::FunctionTarget)); G.Pin(Find, OP::ComponentClass)->DefaultObject = UVoyageModuleComponent::StaticClass();
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = UVoyageModuleComponent::StaticClass(); Cast->SetPurity(true); G.Node(Cast);
    G.Link(G.Pin(Find, P::ReturnValue), Cast->GetCastSourcePin());
    G.Write(Charge::Module, Cast->GetCastResultPin()); G.Branch(G.Valid(G.Read(Charge::Module)));
    return G.Read(Charge::Module);
}
void SetEnergyDemand(FGraph& G, bool Charging)
{
    auto* Set = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, SetCustomConsumption));
    G.Link(G.Read(Charge::Module), G.Pin(Set, P::FunctionTarget));
    if (Charging) G.Link(ChargingInputW(G), G.Pin(Set, Charge::Input));
    else G.Default(Set, Charge::Input, Charge::IdleW);
    // A full gun keeps its stored charge; idle demand must not shrink capacity.
    G.Link(EnergyCapacityAmount(G), G.Pin(Set, Charge::Capacity));
    G.Default(Set, Charge::Idle, Charge::IdleW); G.Exec(Set);
}
UEdGraphPin* EnergyAmount(FGraph& G)
{
    auto* Get = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetResourceAmount));
    G.Link(G.Read(Charge::Module), G.Pin(Get, P::FunctionTarget)); G.Default(Get, Charge::Type, Charge::Electricity);
    return G.Pin(Get, P::ReturnValue);
}

void RefreshRailgunEnergyPass(FGraph& G, UEdGraphPin* ForceDemand)
{
    UEdGraphPin* StoredEnergy = EnergyAmount(G);
    UEdGraphPin* Positive = EnergyMath(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax),
        StoredEnergy, N::Zero);
    G.Write(Charge::Energy, G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin), Positive,
        RequiredEnergyAmount(G)));

    UEdGraphPin* Charging = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble),
        StoredEnergy, RequiredEnergyAmount(G));
    UEdGraphPin* NotInitialized = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        G.Read(Charge::DemandInitialized), N::False);
    UEdGraphPin* ModeChanged = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_BoolBool),
        Charging, G.Read(Charge::DemandCharging));
    UEdGraphPin* StateNeedsDemand = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        NotInitialized, ModeChanged);
    auto* NeedsDemand = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        ForceDemand, StateNeedsDemand));
    G.Write(Charge::DemandCharging, Charging);
    G.Write(Charge::DemandInitialized, nullptr, N::True);
    auto* DesiredMode = G.Branch(Charging);
    SetEnergyDemand(G, true);
    UEdGraphPin* ChargingTail = G.Tail;
    G.Tail = G.Pin(DesiredMode, P::Else);
    SetEnergyDemand(G, false);
    StationMerge(G, {ChargingTail, G.Tail});
    StationMerge(G, {G.Tail, G.Pin(NeedsDemand, P::Else)});
}

FEdGraphPinType RailgunEnergyBooleanPinType()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    return Type;
}

void AddRailgunEnergyFunctions(UBlueprint* BP)
{
    check(BP && BP->GeneratedClass);
    FMulticastDelegateProperty* DelegateProperty =
        RailgunModuleValueDelegateProperty();

    UEdGraph* RefreshGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        Charge::RefreshFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, RefreshGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* RefreshEntry = nullptr;
    for (UEdGraphNode* Node : RefreshGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            RefreshEntry = Candidate;
    check(RefreshEntry);
    UEdGraphPin* ForceDemand = RefreshEntry->CreateUserDefinedPin(
        Charge::ForceDemandParameter, RailgunEnergyBooleanPinType(),
        EGPD_Output);
    check(ForceDemand);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::RefreshFunction));

    RefreshEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Refresh(RefreshGraph, nullptr);
    Refresh.Tail = Refresh.Pin(RefreshEntry, P::Then);
    Refresh.Branch(Refresh.Valid(Refresh.Read(Charge::Module)));
    auto* Busy = Refresh.Branch(Refresh.Read(Charge::UpdateActive));
    Refresh.Write(Charge::UpdatePending, nullptr, N::True);
    Refresh.Tail = Refresh.Pin(Busy, P::Else);
    Refresh.Write(Charge::UpdateActive, nullptr, N::True);
    Refresh.Write(Charge::UpdatePending, nullptr, N::False);
    RefreshRailgunEnergyPass(Refresh, ForceDemand);
    Refresh.Write(Charge::UpdateActive, nullptr, N::False);
    Refresh.Branch(Refresh.Read(Charge::UpdatePending));
    // The demand mode is cached before SetCustomConsumption. A synchronous
    // module notification therefore requests one final read without issuing
    // the same native demand change again.
    auto* FinalRefresh = Refresh.Call(BP->GeneratedClass,
        Charge::RefreshFunction);
    Refresh.Default(FinalRefresh, Charge::ForceDemandParameter, N::False);
    Refresh.Exec(FinalRefresh);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);

    UEdGraph* CallbackGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        Charge::CallbackFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, CallbackGraph, true,
        DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* CallbackEntry = nullptr;
    for (UEdGraphNode* Node : CallbackGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            CallbackEntry = Candidate;
    check(CallbackEntry);
    CallbackEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Callback(CallbackGraph, nullptr);
    Callback.Tail = Callback.Pin(CallbackEntry, P::Then);
    UEdGraphPin* ChangedModule = Callback.Pin(CallbackEntry,
        Charge::ModuleParameter);
    Callback.Branch(Callback.Valid(ChangedModule));
    Callback.Branch(Callback.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject), ChangedModule,
        Callback.Read(Charge::Module)));
    auto* RefreshCall = Callback.Call(BP->GeneratedClass,
        Charge::RefreshFunction);
    Callback.Default(RefreshCall, Charge::ForceDemandParameter, N::False);
    Callback.Exec(RefreshCall);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::CallbackFunction));

    UEdGraph* BindGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        Charge::BindFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, BindGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* BindEntry = nullptr;
    for (UEdGraphNode* Node : BindGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            BindEntry = Candidate;
    check(BindEntry);
    BindEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Bind(BindGraph, nullptr);
    Bind.Tail = Bind.Pin(BindEntry, P::Then);

    auto* CallbackDelegate = Bind.Node(
        NewObject<UK2Node_CreateDelegate>(BindGraph));
    CallbackDelegate->SetFunction(Charge::CallbackFunction);
    auto NewRemove = [&]()
    {
        auto* Remove = NewObject<UK2Node_RemoveDelegate>(BindGraph);
        Remove->SetFromProperty(DelegateProperty, false,
            UVoyageModuleComponent::StaticClass());
        Bind.Node(Remove);
        Bind.Link(CallbackDelegate->GetDelegateOutPin(),
            Remove->GetDelegatePin());
        return Remove;
    };
    auto* RemovePrevious = NewRemove();
    auto* RemoveCurrent = NewRemove();
    auto* Add = NewObject<UK2Node_AddDelegate>(BindGraph);
    Add->SetFromProperty(DelegateProperty, false,
        UVoyageModuleComponent::StaticClass());
    Bind.Node(Add);
    Bind.Link(CallbackDelegate->GetDelegateOutPin(), Add->GetDelegatePin());

    auto* PreviousValid = Bind.Branch(Bind.Valid(Bind.Read(Charge::Module)));
    Bind.Link(Bind.Read(Charge::Module),
        Bind.Pin(RemovePrevious, P::FunctionTarget));
    Bind.Exec(RemovePrevious);
    UEdGraphPin* RemovedTail = Bind.Tail;
    Bind.Tail = Bind.Pin(PreviousValid, P::Else);
    StationMerge(Bind, {RemovedTail, Bind.Tail});
    Bind.Write(Charge::Module, nullptr);

    UEdGraphPin* Module = ResolveEnergyModule(Bind);
    Bind.Link(Module, Bind.Pin(RemoveCurrent, P::FunctionTarget));
    Bind.Exec(RemoveCurrent);
    Bind.Link(Module, Bind.Pin(Add, P::FunctionTarget));
    Bind.Exec(Add);
    Bind.Write(Charge::DemandInitialized, nullptr, N::False);
    Bind.Write(Charge::UpdateActive, nullptr, N::False);
    Bind.Write(Charge::UpdatePending, nullptr, N::False);
    auto* InitialRefresh = Bind.Call(BP->GeneratedClass,
        Charge::RefreshFunction);
    Bind.Default(InitialRefresh, Charge::ForceDemandParameter, N::True);
    Bind.Exec(InitialRefresh);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::BindFunction));
}

void AddRailgunEnergyTeardown(UBlueprint* BP)
{
    check(BP && BP->UbergraphPages.Num() == 1);
    UEdGraph* Graph = BP->UbergraphPages[0];
    UK2Node_Event* EndPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        auto* Event = Cast<UK2Node_Event>(Node);
        if (!Event || Event->EventReference.GetMemberName() !=
            ActorLifecycleGraphNames::EndPlayEvent) continue;
        check(!EndPlay);
        EndPlay = Event;
    }
    check(EndPlay);
    UEdGraphPin* EndPlayTail = EndPlay->FindPinChecked(P::Then);
    check(EndPlayTail->LinkedTo.Num() == 1);
    UEdGraphPin* ExistingWork = EndPlayTail->LinkedTo[0];
    EndPlayTail->BreakAllPinLinks();
    FGraph G(Graph, nullptr);
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(EndPlayTail, G.Pin(Work, P::Execute));
    G.Link(Work->GetThenPinGivenIndex(0), ExistingWork);
    G.Tail = Work->GetThenPinGivenIndex(1);

    auto* DelegateProperty = RailgunModuleValueDelegateProperty();
    auto* Callback = G.Node(NewObject<UK2Node_CreateDelegate>(Graph));
    Callback->SetFunction(Charge::CallbackFunction);
    auto* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false,
        UVoyageModuleComponent::StaticClass());
    G.Node(Remove);
    G.Link(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    auto* ModuleValid = G.Branch(G.Valid(G.Read(Charge::Module)));
    G.Link(G.Read(Charge::Module), G.Pin(Remove, P::FunctionTarget));
    G.Exec(Remove);
    UEdGraphPin* RemovedTail = G.Tail;
    G.Tail = G.Pin(ModuleValid, P::Else);
    StationMerge(G, {RemovedTail, G.Tail});
    G.Write(Charge::Module, nullptr);
    G.Write(Charge::Energy, nullptr, N::Zero);
    G.Write(Charge::DemandInitialized, nullptr, N::False);
    G.Write(Charge::UpdateActive, nullptr, N::False);
    G.Write(Charge::UpdatePending, nullptr, N::False);
}
UEdGraphPin* DebitEnergy(FGraph& G, UEdGraphPin* Amount = nullptr)
{
    auto* Debit = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, RemoveResource));
    G.Link(G.Read(Charge::Module), G.Pin(Debit, P::FunctionTarget)); G.Default(Debit, Charge::Type, Charge::Electricity);
    G.Default(Debit, Charge::RemovalType, Charge::ExactRemoval);
    G.Link(Amount ? Amount : RequiredEnergyAmount(G), G.Pin(Debit, Charge::RemoveAmount));
    G.Exec(Debit); return G.Pin(Debit, P::ReturnValue);
}
UEdGraphPin* CreditEnergy(FGraph& G, UEdGraphPin* Amount)
{
    auto* Credit = G.Call(UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, AddResource));
    G.Link(G.Read(Charge::Module), G.Pin(Credit, P::FunctionTarget));
    G.Default(Credit, Charge::Type, Charge::Electricity);
    G.Link(Amount, G.Pin(Credit, Charge::AddAmount));
    G.Exec(Credit);
    return G.Pin(Credit, P::ReturnValue);
}
void UpdateAutomaticCharge(FGraph& G, UEdGraphPin* DeltaSeconds,
    UEdGraphPin* OfflineDischargeKW)
{
    G.Branch(G.Valid(G.Read(Charge::Module)));
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), DeltaSeconds, N::Zero));
    UEdGraphPin* Connected = ObserveCall(G,
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent,
            HasSocketConnection), G.Read(Charge::Module));
    UEdGraphPin* Powered = ObserveCall(G,
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasPower),
        G.Read(Charge::Module));
    auto* HasSupply = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND));
    G.Link(Connected, G.Pin(HasSupply, P::Binary::LeftOperand));
    G.Link(Powered, G.Pin(HasSupply, P::Binary::RightOperand));
    auto* Supply = G.Branch(G.Pin(HasSupply, P::ReturnValue));
    UEdGraphPin* SuppliedTail = G.Tail;
    G.Tail = G.Pin(Supply, P::Else);
    UEdGraphPin* StoredEnergy = EnergyMath(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax),
        EnergyAmount(G), N::Zero);
    auto* HasStoredEnergy = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble),
        StoredEnergy, N::Zero));
    DebitEnergy(G, OfflineDrainAmount(G, DeltaSeconds, StoredEnergy,
        OfflineDischargeKW));
    StationMerge(G, {G.Tail, G.Pin(HasStoredEnergy, P::Else)});
    StationMerge(G, {SuppliedTail, G.Tail});
}
