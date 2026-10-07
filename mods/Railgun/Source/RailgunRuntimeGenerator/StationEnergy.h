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
inline const FName SocketConnected(TEXT("RailgunSocketConnected"));
inline const FName PowerAvailable(TEXT("RailgunPowerAvailable"));
inline const FName OfflineDrainActive(TEXT("RailgunOfflineDrainActive"));
inline const FName OfflineDrainDebitActive(
    TEXT("RailgunOfflineDrainDebitActive"));
inline const FName SupplyReconcilePending(
    TEXT("RailgunSupplyReconcilePending"));
inline const FName SupplyReconcileGeneration(
    TEXT("RailgunSupplyReconcileGeneration"));
inline const FName SupplyReconcileTime(
    TEXT("RailgunSupplyReconcileTime"));
inline const FName OfflineDrainTimestamp(
    TEXT("RailgunOfflineDrainTimestamp"));
inline const FName OfflineDrainRate(TEXT("RailgunOfflineDrainRateKW"));
inline const FName OfflineDrainElapsed(TEXT("RailgunOfflineDrainElapsed"));
inline const FName OfflineDrainTimerHandle(
    TEXT("RailgunOfflineDrainTimerHandle"));
inline const FName RefreshFunction(TEXT("RefreshRailgunEnergy"));
inline const FName RefreshHudFunction(TEXT("RefreshRailgunHudEnergy"));
inline const FName RefreshSupplyFunction(TEXT("RefreshRailgunSupplyState"));
inline const FName SettleDrainFunction(TEXT("SettleRailgunOfflineDrain"));
inline const FName StopDrainFunction(TEXT("StopRailgunOfflineDrain"));
inline const FName DrainTimerEvent(TEXT("OnRailgunOfflineDrainTimer"));
inline const FName DeferredSupplyEvent(
    TEXT("DeferredRailgunSupplyReconcile"));
inline const FName BindFunction(TEXT("BindRailgunEnergy"));
inline const FName CallbackFunction(TEXT("OnRailgunEnergyModuleValueChanged"));
inline const FName SocketCallbackFunction(
    TEXT("OnRailgunSocketConnectionChanged"));
inline const FName PowerCallbackFunction(
    TEXT("OnRailgunPowerStateChanged"));
inline const FName OnModuleValueChanged(TEXT("OnModuleValueChanged"));
inline const FName OnModuleSocketConnectionChanged(
    TEXT("OnModuleSocketConnectionChanged"));
inline const FName OnModulePowerStateChanged(
    TEXT("OnModulePowerStateChanged"));
inline const FName ModuleParameter(TEXT("Module"));
inline const FName SourceModuleParameter(TEXT("SourceModule"));
inline const FName HasPowerParameter(TEXT("bHasPower"));
inline const FName ForceDemandParameter(TEXT("ForceDemand"));
inline const FName CutoffTimeParameter(TEXT("CutoffTime"));
inline const FName SettleParameter(TEXT("SettleBeforeStop"));
inline const FName ReconcileModuleParameter(TEXT("ReconcileModule"));
inline const FName ReconcileGenerationParameter(
    TEXT("ReconcileGeneration"));
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
inline constexpr TCHAR OfflineDrainIntervalSeconds[] = TEXT("0.3333333333");
inline constexpr TCHAR DrainTimerFunctionName[] =
    TEXT("OnRailgunOfflineDrainTimer");
inline constexpr TCHAR TimerHandlePin[] = TEXT("Handle");
inline constexpr TCHAR TimerMaxOncePerFramePin[] = TEXT("bMaxOncePerFrame");
inline constexpr TCHAR TimerInitialDelayPin[] = TEXT("InitialStartDelay");
inline constexpr TCHAR WorldContextObjectPin[] = TEXT("WorldContextObject");
inline constexpr TCHAR IntegerIncrement[] = TEXT("1");
inline constexpr TCHAR DelegateSignaturePath[] =
    TEXT("/Script/Voyage.VoyageModuleCompDelegate__DelegateSignature");
inline constexpr TCHAR PowerDelegateSignaturePath[] = TEXT(
    "/Script/Voyage.VoyageModuleCompPowerStateChanged__DelegateSignature");
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
FMulticastDelegateProperty* RailgunModuleDelegateProperty(FName PropertyName)
{
    auto* Property = FindFProperty<FMulticastDelegateProperty>(
        UVoyageModuleComponent::StaticClass(), PropertyName);
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

FMulticastDelegateProperty* RailgunModulePowerDelegateProperty()
{
    auto* Property = FindFProperty<FMulticastDelegateProperty>(
        UVoyageModuleComponent::StaticClass(),
        Charge::OnModulePowerStateChanged);
    check(Property && Property->SignatureFunction &&
        Property->SignatureFunction->GetPathName() ==
            Charge::PowerDelegateSignaturePath &&
        Property->SignatureFunction->NumParms == 2);
    auto* Module = FindFProperty<FObjectProperty>(
        Property->SignatureFunction, Charge::SourceModuleParameter);
    auto* HasPower = FindFProperty<FBoolProperty>(
        Property->SignatureFunction, Charge::HasPowerParameter);
    check(Module && Module->HasAnyPropertyFlags(CPF_Parm) &&
        Module->PropertyClass == UVoyageModuleComponent::StaticClass() &&
        HasPower && HasPower->HasAnyPropertyFlags(CPF_Parm));
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

void IncrementRailgunInteger(FGraph& G, FName Field)
{
    auto* Increment = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt));
    G.Link(G.Read(Field), G.Pin(Increment, P::Binary::LeftOperand));
    G.Default(Increment, P::Binary::RightOperand,
        Charge::IntegerIncrement);
    G.Write(Field, G.Pin(Increment, P::ReturnValue));
}

void GuardRailgunModuleCallback(FGraph& G, UEdGraphPin* Payload)
{
    G.Branch(G.Valid(Payload));
    G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject), Payload, G.Read(Charge::Module)));
}

void SetEnergyDemand(FGraph& G, bool Charging)
{
    UEdGraphPin* RequestedAcceptance = Charging ? ChargingInputW(G) : nullptr;
    UEdGraphPin* RequestedCapacity = EnergyCapacityAmount(G);
    auto* Set = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, SetCustomConsumption));
    G.Link(G.Read(Charge::Module), G.Pin(Set, P::FunctionTarget));
    if (RequestedAcceptance) G.Link(RequestedAcceptance, G.Pin(Set, Charge::Input));
    else G.Default(Set, Charge::Input, Charge::IdleW);
    // A full gun keeps its stored charge; idle demand must not shrink capacity.
    G.Link(RequestedCapacity, G.Pin(Set, Charge::Capacity));
    G.Default(Set, Charge::Idle, Charge::IdleW); G.Exec(Set);
}
UEdGraphPin* EnergyAmount(FGraph& G)
{
    auto* Get = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetResourceAmount));
    G.Link(G.Read(Charge::Module), G.Pin(Get, P::FunctionTarget)); G.Default(Get, Charge::Type, Charge::Electricity);
    return G.Pin(Get, P::ReturnValue);
}

void RefreshRailgunEnergyPass(FGraph& G, UBlueprint* BP,
    UEdGraphPin* ForceDemand)
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
    auto* RefreshActivity = G.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    G.Exec(RefreshActivity);
}

FEdGraphPinType RailgunEnergyBooleanPinType()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    return Type;
}

FEdGraphPinType RailgunEnergyRealPinType()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Real;
    Type.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    return Type;
}

FEdGraphPinType RailgunEnergyIntPinType()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Int;
    return Type;
}

FEdGraphPinType RailgunEnergyModulePinType()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Object;
    Type.PinSubCategoryObject = UVoyageModuleComponent::StaticClass();
    return Type;
}

UEdGraphPin* DebitEnergy(FGraph& G, UEdGraphPin* Amount);

UK2Node_FunctionEntry* AddRailgunEnergyFunctionEntry(UBlueprint* BP,
    FName FunctionName, UEdGraph*& Graph)
{
    Graph = FBlueprintEditorUtils::CreateNewGraph(BP, FunctionName,
        UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
        static_cast<UClass*>(nullptr));
    for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Entry = Cast<UK2Node_FunctionEntry>(Node))
            return Entry;
    checkNoEntry();
    return nullptr;
}

void AddRailgunDrainRuntime(UBlueprint* BP)
{
    check(BP && BP->GeneratedClass && BP->UbergraphPages.Num() == 1);

    UEdGraph* SettleGraph = nullptr;
    UK2Node_FunctionEntry* SettleEntry = AddRailgunEnergyFunctionEntry(
        BP, Charge::SettleDrainFunction, SettleGraph);
    UEdGraphPin* CutoffTime = SettleEntry->CreateUserDefinedPin(
        Charge::CutoffTimeParameter, RailgunEnergyRealPinType(), EGPD_Output);
    check(CutoffTime);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::SettleDrainFunction));

    SettleEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Settle(SettleGraph, nullptr);
    Settle.Tail = Settle.Pin(SettleEntry, P::Then);
    Settle.Branch(Settle.Read(Charge::OfflineDrainActive));
    Settle.Branch(Settle.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        Settle.Read(Charge::OfflineDrainDebitActive), N::False));
    Settle.Branch(Settle.Valid(Settle.Read(Charge::Module)));
    UEdGraphPin* Elapsed = EnergyMath(Settle,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax),
        Settle.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_DoubleDouble), CutoffTime,
            Settle.Read(Charge::OfflineDrainTimestamp)), N::Zero);
    Settle.Write(Charge::OfflineDrainElapsed, Elapsed);
    // Advance the integration boundary before RemoveResource. A synchronous
    // OnModuleValueChanged callback therefore cannot debit the same interval.
    Settle.Write(Charge::OfflineDrainTimestamp, CutoffTime);
    Settle.Branch(Settle.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble),
        Settle.Read(Charge::OfflineDrainElapsed), N::Zero));
    UEdGraphPin* StoredEnergy = EnergyMath(Settle,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax),
        EnergyAmount(Settle), N::Zero);
    Settle.Branch(Settle.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), StoredEnergy, N::Zero));
    UEdGraphPin* Amount = OfflineDrainAmount(Settle,
        Settle.Read(Charge::OfflineDrainElapsed),
        StoredEnergy, Settle.Read(Charge::OfflineDrainRate));
    Settle.Branch(Settle.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), Amount, N::Zero));
    Settle.Write(Charge::OfflineDrainDebitActive, nullptr, N::True);
    DebitEnergy(Settle, Amount);
    Settle.Write(Charge::OfflineDrainDebitActive, nullptr, N::False);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);

    UEdGraph* StopGraph = nullptr;
    UK2Node_FunctionEntry* StopEntry = AddRailgunEnergyFunctionEntry(
        BP, Charge::StopDrainFunction, StopGraph);
    UEdGraphPin* StopCutoff = StopEntry->CreateUserDefinedPin(
        Charge::CutoffTimeParameter, RailgunEnergyRealPinType(), EGPD_Output);
    UEdGraphPin* ShouldSettle = StopEntry->CreateUserDefinedPin(
        Charge::SettleParameter, RailgunEnergyBooleanPinType(), EGPD_Output);
    check(StopCutoff && ShouldSettle);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::StopDrainFunction));

    StopEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Stop(StopGraph, nullptr);
    Stop.Tail = Stop.Pin(StopEntry, P::Then);
    auto* SettleRequested = Stop.Branch(ShouldSettle);
    auto* SettleCall = Stop.Call(BP->GeneratedClass,
        Charge::SettleDrainFunction);
    Stop.Link(StopCutoff, Stop.Pin(SettleCall, Charge::CutoffTimeParameter));
    Stop.Exec(SettleCall);
    UEdGraphPin* SettledTail = Stop.Tail;
    Stop.Tail = Stop.Pin(SettleRequested, P::Else);
    StationMerge(Stop, {SettledTail, Stop.Tail});
    auto* WasActive = Stop.Branch(Stop.Read(Charge::OfflineDrainActive));
    auto* ClearTimer = Stop.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            K2_ClearAndInvalidateTimerHandle));
    Stop.Link(OpticalSelf(Stop), Stop.Pin(ClearTimer,
        Charge::WorldContextObjectPin));
    Stop.Link(Stop.Read(Charge::OfflineDrainTimerHandle),
        Stop.Pin(ClearTimer, Charge::TimerHandlePin));
    Stop.Exec(ClearTimer);
    UEdGraphPin* ClearedTail = Stop.Tail;
    Stop.Tail = Stop.Pin(WasActive, P::Else);
    StationMerge(Stop, {ClearedTail, Stop.Tail});
    Stop.Write(Charge::OfflineDrainActive, nullptr, N::False);
    Stop.Write(Charge::OfflineDrainDebitActive, nullptr, N::False);
    Stop.Write(Charge::OfflineDrainTimestamp, nullptr, N::Zero);
    Stop.Write(Charge::OfflineDrainRate, nullptr, N::Zero);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);

    UEdGraph* Ubergraph = BP->UbergraphPages[0];
    FGraph Timer(Ubergraph, nullptr);
    auto* TimerEvent = NewObject<UK2Node_CustomEvent>(Ubergraph);
    TimerEvent->CustomFunctionName = Charge::DrainTimerEvent;
    Timer.Node(TimerEvent);
    Timer.Tail = Timer.Pin(TimerEvent, P::Then);
    auto* TimerNowCall = Timer.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    UEdGraphPin* TimerNow = Timer.Pin(TimerNowCall, P::ReturnValue);
    auto* TimerSettle = Timer.Call(BP->GeneratedClass,
        Charge::SettleDrainFunction);
    Timer.Link(TimerNow,
        Timer.Pin(TimerSettle, Charge::CutoffTimeParameter));
    Timer.Exec(TimerSettle);
    Timer.Branch(Timer.Read(Charge::OfflineDrainActive));
    UEdGraphPin* RuntimeValid = Timer.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Timer.Valid(Timer.Read(Charge::Module)),
        Timer.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_BoolBool),
            Timer.Read(StationLifecycle::TeardownPending), N::False));
    auto* KeepRuntime = Timer.Branch(RuntimeValid);
    UEdGraphPin* LiveEnergy = EnergyMath(Timer,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax),
        EnergyAmount(Timer), N::Zero);
    auto* HasLiveEnergy = Timer.Branch(Timer.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), LiveEnergy, N::Zero));
    UEdGraphPin* ValidTail = Timer.Tail;
    Timer.Tail = Timer.Pin(KeepRuntime, P::Else);
    UEdGraphPin* InvalidRuntimeTail = Timer.Tail;
    Timer.Tail = Timer.Pin(HasLiveEnergy, P::Else);
    StationMerge(Timer, {InvalidRuntimeTail, Timer.Tail});
    auto* StopInvalid = Timer.Call(BP->GeneratedClass,
        Charge::StopDrainFunction);
    Timer.Link(TimerNow,
        Timer.Pin(StopInvalid, Charge::CutoffTimeParameter));
    Timer.Default(StopInvalid, Charge::SettleParameter, N::False);
    Timer.Exec(StopInvalid);
    Timer.Tail = ValidTail;

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::DrainTimerEvent));
}

void AddRailgunActivityFunctions(UBlueprint* BP,
    FName OfflineDischargeField)
{
    check(BP && BP->GeneratedClass);
    AddRailgunDrainRuntime(BP);
    UEdGraph* ActivityGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        DS::RefreshActivity, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, ActivityGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* ActivityEntry = nullptr;
    for (UEdGraphNode* Node : ActivityGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            ActivityEntry = Candidate;
    check(ActivityEntry);
    ActivityEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Activity(ActivityGraph, nullptr);
    Activity.Tail = Activity.Pin(ActivityEntry, P::Then);

    UEdGraphPin* HasSupply = Activity.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Activity.Read(Charge::SocketConnected),
        Activity.Read(Charge::PowerAvailable));
    UEdGraphPin* MissingSupply = Activity.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        HasSupply, N::False);
    UEdGraphPin* HasEnergy = Activity.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), Activity.Read(Charge::Energy), N::Zero);
    UEdGraphPin* PositiveRate = Activity.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), Activity.Read(OfflineDischargeField),
        N::Zero);
    UEdGraphPin* CanDrain = Activity.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Activity.Binary(GET_FUNCTION_NAME_CHECKED(
            UKismetMathLibrary, BooleanAND), MissingSupply, HasEnergy),
        PositiveRate);
    UEdGraphPin* ModuleValid = Activity.Valid(
        Activity.Read(Charge::Module));
    UEdGraphPin* NotTeardown = Activity.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        Activity.Read(StationLifecycle::TeardownPending), N::False);
    UEdGraphPin* DrainActive = Activity.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Activity.Binary(GET_FUNCTION_NAME_CHECKED(
            UKismetMathLibrary, BooleanAND), CanDrain, ModuleValid),
        NotTeardown);
    UEdGraphPin* RateMatches = Activity.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_DoubleDouble),
        Activity.Read(Charge::OfflineDrainRate),
        Activity.Read(OfflineDischargeField));
    UEdGraphPin* KeepTimer = Activity.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        DrainActive, RateMatches);
    auto* TimerWasActive = Activity.Branch(
        Activity.Read(Charge::OfflineDrainActive));
    auto* KeepExisting = Activity.Branch(KeepTimer);
    UEdGraphPin* ExistingTimerTail = Activity.Tail;
    Activity.Tail = Activity.Pin(KeepExisting, P::Else);
    auto* StopNowCall = Activity.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    UEdGraphPin* StopNow = Activity.Pin(StopNowCall, P::ReturnValue);
    auto* StopDrain = Activity.Call(BP->GeneratedClass,
        Charge::StopDrainFunction);
    Activity.Link(StopNow,
        Activity.Pin(StopDrain, Charge::CutoffTimeParameter));
    Activity.Default(StopDrain, Charge::SettleParameter, N::True);
    Activity.Exec(StopDrain);
    UEdGraphPin* StoppedTimerTail = Activity.Tail;
    Activity.Tail = Activity.Pin(TimerWasActive, P::Else);
    StationMerge(Activity, {StoppedTimerTail, Activity.Tail});

    auto* ShouldStart = Activity.Branch(DrainActive);
    auto* StartNowCall = Activity.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    UEdGraphPin* StartNow = Activity.Pin(StartNowCall, P::ReturnValue);
    Activity.Write(Charge::OfflineDrainRate,
        Activity.Read(OfflineDischargeField));
    Activity.Write(Charge::OfflineDrainTimestamp, StartNow);
    Activity.Write(Charge::OfflineDrainActive, nullptr, N::True);
    auto* SetTimer = Activity.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, K2_SetTimer));
    Activity.Link(OpticalSelf(Activity),
        Activity.Pin(SetTimer, TimerGraphNames::Object));
    Activity.Default(SetTimer, TimerGraphNames::Function,
        Charge::DrainTimerFunctionName);
    Activity.Default(SetTimer, TimerGraphNames::Interval,
        Charge::OfflineDrainIntervalSeconds);
    Activity.Default(SetTimer, TimerGraphNames::Looping, N::True);
    Activity.Default(SetTimer, Charge::TimerMaxOncePerFramePin, N::True);
    Activity.Default(SetTimer, Charge::TimerInitialDelayPin,
        Charge::OfflineDrainIntervalSeconds);
    Activity.Exec(SetTimer);
    Activity.Write(Charge::OfflineDrainTimerHandle,
        Activity.Pin(SetTimer, P::ReturnValue));
    UEdGraphPin* StartedTimerTail = Activity.Tail;
    Activity.Tail = Activity.Pin(ShouldStart, P::Else);
    StationMerge(Activity, {StartedTimerTail, Activity.Tail,
        ExistingTimerTail});

    UEdGraphPin* NeedsTick = Activity.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Activity.Read(DS::ViewOwned), NotTeardown);
    auto* SetTick = Activity.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, SetActorTickEnabled));
    Activity.Link(OpticalSelf(Activity),
        Activity.Pin(SetTick, P::FunctionTarget));
    Activity.Link(NeedsTick,
        Activity.Pin(SetTick, ActorLifecycleGraphNames::TickEnabled));
    Activity.Exec(SetTick);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(DS::RefreshActivity));

    UEdGraph* SupplyGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        Charge::RefreshSupplyFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, SupplyGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* SupplyEntry = nullptr;
    for (UEdGraphNode* Node : SupplyGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            SupplyEntry = Candidate;
    check(SupplyEntry);
    SupplyEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Supply(SupplyGraph, nullptr);
    Supply.Tail = Supply.Pin(SupplyEntry, P::Then);
    Supply.Branch(Supply.Valid(Supply.Read(Charge::Module)));
    Supply.Write(Charge::SocketConnected, ObserveCall(Supply,
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent,
            HasSocketConnection), Supply.Read(Charge::Module)));
    Supply.Write(Charge::PowerAvailable, ObserveCall(Supply,
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasPower),
        Supply.Read(Charge::Module)));
    auto* RefreshActivity = Supply.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    Supply.Exec(RefreshActivity);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(
            Charge::RefreshSupplyFunction));
}

void AddRailgunDeferredSupplyEvent(UBlueprint* BP)
{
    check(BP && BP->GeneratedClass && BP->UbergraphPages.Num() == 1);
    UEdGraph* Graph = BP->UbergraphPages[0];
    FGraph G(Graph, nullptr);
    auto* Event = NewObject<UK2Node_CustomEvent>(Graph);
    Event->CustomFunctionName = Charge::DeferredSupplyEvent;
    G.Node(Event);
    UEdGraphPin* ReconcileModule = Event->CreateUserDefinedPin(
        Charge::ReconcileModuleParameter, RailgunEnergyModulePinType(),
        EGPD_Output);
    UEdGraphPin* ReconcileGeneration = Event->CreateUserDefinedPin(
        Charge::ReconcileGenerationParameter, RailgunEnergyIntPinType(),
        EGPD_Output);
    check(ReconcileModule && ReconcileGeneration);
    G.Tail = G.Pin(Event, P::Then);
    auto* Delay = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            DelayUntilNextTick));
    G.Exec(Delay);
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_IntInt), ReconcileGeneration,
        G.Read(Charge::SupplyReconcileGeneration)));
    G.Write(Charge::SupplyReconcilePending, nullptr, N::False);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_BoolBool), G.Read(StationLifecycle::TeardownPending),
        N::False));
    G.Branch(G.Valid(ReconcileModule));
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_ObjectObject), ReconcileModule,
        G.Read(Charge::Module)));

    UEdGraphPin* DeferredSocket = ObserveCall(G,
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent,
            HasSocketConnection), ReconcileModule);
    UEdGraphPin* DeferredPower = ObserveCall(G,
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasPower),
        ReconcileModule);
    UEdGraphPin* RestoredSupply = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        DeferredSocket, DeferredPower);
    UEdGraphPin* MustSettle = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Read(Charge::OfflineDrainActive), RestoredSupply);
    auto* SettleBeforeRestore = G.Branch(MustSettle);
    auto* StopDrain = G.Call(BP->GeneratedClass,
        Charge::StopDrainFunction);
    G.Link(G.Read(Charge::SupplyReconcileTime),
        G.Pin(StopDrain, Charge::CutoffTimeParameter));
    G.Default(StopDrain, Charge::SettleParameter, N::True);
    G.Exec(StopDrain);
    UEdGraphPin* SettledTail = G.Tail;
    G.Tail = G.Pin(SettleBeforeRestore, P::Else);
    StationMerge(G, {SettledTail, G.Tail});
    G.Write(Charge::SocketConnected, DeferredSocket);
    G.Write(Charge::PowerAvailable, DeferredPower);
    auto* RefreshActivity = G.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    G.Exec(RefreshActivity);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(
            Charge::DeferredSupplyEvent));
}

void AddRailgunEnergyFunctions(UBlueprint* BP,
    FName OfflineDischargeField)
{
    check(BP && BP->GeneratedClass);
    AddRailgunActivityFunctions(BP, OfflineDischargeField);
    AddRailgunDeferredSupplyEvent(BP);
    FMulticastDelegateProperty* DelegateProperty =
        RailgunModuleDelegateProperty(Charge::OnModuleValueChanged);

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
    RefreshRailgunEnergyPass(Refresh, BP, ForceDemand);
    auto* RefreshHud = Refresh.Call(BP->GeneratedClass,
        Charge::RefreshHudFunction);
    Refresh.Exec(RefreshHud);
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
    GuardRailgunModuleCallback(Callback, ChangedModule);
    auto* RefreshCall = Callback.Call(BP->GeneratedClass,
        Charge::RefreshFunction);
    Callback.Default(RefreshCall, Charge::ForceDemandParameter, N::False);
    Callback.Exec(RefreshCall);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Charge::CallbackFunction));

    FMulticastDelegateProperty* SocketDelegateProperty =
        RailgunModuleDelegateProperty(
            Charge::OnModuleSocketConnectionChanged);
    FMulticastDelegateProperty* PowerDelegateProperty =
        RailgunModulePowerDelegateProperty();
    auto AddSupplyCallback = [&](FName FunctionName,
        FMulticastDelegateProperty* Property, FName ModuleParameter,
        bool UsePowerPayload)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
            FunctionName, UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, true,
            Property->SignatureFunction.Get());
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        FGraph Callback(Graph, nullptr);
        Callback.Tail = Callback.Pin(Entry, P::Then);
        UEdGraphPin* ChangedModule = Callback.Pin(Entry, ModuleParameter);
        GuardRailgunModuleCallback(Callback, ChangedModule);
        if (UsePowerPayload)
        {
            Callback.Write(Charge::PowerAvailable,
                Callback.Pin(Entry, Charge::HasPowerParameter));
        }
        else
        {
            auto* EventTimeCall = Callback.Call(
                UKismetSystemLibrary::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
                    GetGameTimeInSeconds));
            Callback.Write(Charge::SupplyReconcileTime,
                Callback.Pin(EventTimeCall, P::ReturnValue));
            auto* AlreadyPending = Callback.Branch(
                Callback.Read(Charge::SupplyReconcilePending));
            UEdGraphPin* PendingTail = Callback.Tail;
            Callback.Tail = Callback.Pin(AlreadyPending, P::Else);
            Callback.Write(Charge::SupplyReconcilePending, nullptr,
                N::True);
            IncrementRailgunInteger(Callback,
                Charge::SupplyReconcileGeneration);
            auto* Reconcile = NewObject<UK2Node_CallFunction>(Graph);
            Reconcile->FunctionReference.SetSelfMember(
                Charge::DeferredSupplyEvent);
            Callback.Node(Reconcile);
            Callback.Link(ChangedModule,
                Callback.Pin(Reconcile,
                    Charge::ReconcileModuleParameter));
            Callback.Link(Callback.Read(
                    Charge::SupplyReconcileGeneration),
                Callback.Pin(Reconcile,
                    Charge::ReconcileGenerationParameter));
            Callback.Exec(Reconcile);
            StationMerge(Callback, {PendingTail, Callback.Tail});
        }
        if (UsePowerPayload)
        {
            auto* RefreshActivity = Callback.Call(BP->GeneratedClass,
                DS::RefreshActivity);
            Callback.Exec(RefreshActivity);
        }

        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FKismetEditorUtilities::CompileBlueprint(BP);
        check(BP->Status != BS_Error && BP->GeneratedClass &&
            BP->GeneratedClass->FindFunctionByName(FunctionName));
    };
    AddSupplyCallback(Charge::SocketCallbackFunction,
        SocketDelegateProperty, Charge::ModuleParameter, false);
    AddSupplyCallback(Charge::PowerCallbackFunction,
        PowerDelegateProperty, Charge::SourceModuleParameter, true);

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

    auto* BindNowCall = Bind.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    auto* StopPreviousDrain = Bind.Call(BP->GeneratedClass,
        Charge::StopDrainFunction);
    Bind.Link(Bind.Pin(BindNowCall, P::ReturnValue),
        Bind.Pin(StopPreviousDrain, Charge::CutoffTimeParameter));
    Bind.Default(StopPreviousDrain, Charge::SettleParameter, N::True);
    Bind.Exec(StopPreviousDrain);
    IncrementRailgunInteger(Bind,
        Charge::SupplyReconcileGeneration);
    Bind.Write(Charge::SupplyReconcilePending, nullptr, N::False);

    const TArray<FMulticastDelegateProperty*> DelegateProperties {
        DelegateProperty, SocketDelegateProperty, PowerDelegateProperty};
    const TArray<FName> CallbackFunctions {Charge::CallbackFunction,
        Charge::SocketCallbackFunction, Charge::PowerCallbackFunction};
    TArray<UK2Node_RemoveDelegate*> RemovePrevious;
    TArray<UK2Node_RemoveDelegate*> RemoveCurrent;
    TArray<UK2Node_AddDelegate*> AddCurrent;
    for (int32 Index = 0; Index < DelegateProperties.Num(); ++Index)
    {
        auto* CallbackDelegate = Bind.Node(
            NewObject<UK2Node_CreateDelegate>(BindGraph));
        CallbackDelegate->SetFunction(CallbackFunctions[Index]);
        auto NewRemove = [&]()
        {
            auto* Remove = NewObject<UK2Node_RemoveDelegate>(BindGraph);
            Remove->SetFromProperty(DelegateProperties[Index], false,
                UVoyageModuleComponent::StaticClass());
            Bind.Node(Remove);
            Bind.Link(CallbackDelegate->GetDelegateOutPin(),
                Remove->GetDelegatePin());
            return Remove;
        };
        RemovePrevious.Add(NewRemove());
        RemoveCurrent.Add(NewRemove());
        auto* Add = NewObject<UK2Node_AddDelegate>(BindGraph);
        Add->SetFromProperty(DelegateProperties[Index], false,
            UVoyageModuleComponent::StaticClass());
        Bind.Node(Add);
        Bind.Link(CallbackDelegate->GetDelegateOutPin(),
            Add->GetDelegatePin());
        AddCurrent.Add(Add);
    }

    auto* PreviousValid = Bind.Branch(Bind.Valid(Bind.Read(Charge::Module)));
    for (UK2Node_RemoveDelegate* Remove : RemovePrevious)
    {
        Bind.Link(Bind.Read(Charge::Module),
            Bind.Pin(Remove, P::FunctionTarget));
        Bind.Exec(Remove);
    }
    UEdGraphPin* RemovedTail = Bind.Tail;
    Bind.Tail = Bind.Pin(PreviousValid, P::Else);
    StationMerge(Bind, {RemovedTail, Bind.Tail});
    Bind.Write(Charge::Module, nullptr);
    Bind.Write(Charge::Energy, nullptr, N::Zero);
    Bind.Write(Charge::DemandInitialized, nullptr, N::False);
    Bind.Write(Charge::UpdateActive, nullptr, N::False);
    Bind.Write(Charge::UpdatePending, nullptr, N::False);
    Bind.Write(Charge::SocketConnected, nullptr, N::False);
    Bind.Write(Charge::PowerAvailable, nullptr, N::False);
    auto* ResetActivity = Bind.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    Bind.Exec(ResetActivity);

    UEdGraphPin* Module = ResolveEnergyModule(Bind);
    for (UK2Node_RemoveDelegate* Remove : RemoveCurrent)
    {
        Bind.Link(Module, Bind.Pin(Remove, P::FunctionTarget));
        Bind.Exec(Remove);
    }
    for (UK2Node_AddDelegate* Add : AddCurrent)
    {
        Bind.Link(Module, Bind.Pin(Add, P::FunctionTarget));
        Bind.Exec(Add);
    }
    auto* InitialSupply = Bind.Call(BP->GeneratedClass,
        Charge::RefreshSupplyFunction);
    Bind.Exec(InitialSupply);
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

    auto* EndPlayNowCall = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    auto* StopDrain = G.Call(BP->GeneratedClass,
        Charge::StopDrainFunction);
    G.Link(G.Pin(EndPlayNowCall, P::ReturnValue),
        G.Pin(StopDrain, Charge::CutoffTimeParameter));
    G.Default(StopDrain, Charge::SettleParameter, N::True);
    G.Exec(StopDrain);
    IncrementRailgunInteger(G,
        Charge::SupplyReconcileGeneration);
    G.Write(Charge::SupplyReconcilePending, nullptr, N::False);

    const TArray<FMulticastDelegateProperty*> DelegateProperties {
        RailgunModuleDelegateProperty(Charge::OnModuleValueChanged),
        RailgunModuleDelegateProperty(
            Charge::OnModuleSocketConnectionChanged),
        RailgunModulePowerDelegateProperty()};
    const TArray<FName> CallbackFunctions {Charge::CallbackFunction,
        Charge::SocketCallbackFunction, Charge::PowerCallbackFunction};
    TArray<UK2Node_RemoveDelegate*> Removes;
    for (int32 Index = 0; Index < DelegateProperties.Num(); ++Index)
    {
        auto* Callback = G.Node(NewObject<UK2Node_CreateDelegate>(Graph));
        Callback->SetFunction(CallbackFunctions[Index]);
        auto* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
        Remove->SetFromProperty(DelegateProperties[Index], false,
            UVoyageModuleComponent::StaticClass());
        G.Node(Remove);
        G.Link(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
        Removes.Add(Remove);
    }
    auto* ModuleValid = G.Branch(G.Valid(G.Read(Charge::Module)));
    for (UK2Node_RemoveDelegate* Remove : Removes)
    {
        G.Link(G.Read(Charge::Module), G.Pin(Remove, P::FunctionTarget));
        G.Exec(Remove);
    }
    UEdGraphPin* RemovedTail = G.Tail;
    G.Tail = G.Pin(ModuleValid, P::Else);
    StationMerge(G, {RemovedTail, G.Tail});
    G.Write(Charge::Module, nullptr);
    G.Write(Charge::Energy, nullptr, N::Zero);
    G.Write(Charge::DemandInitialized, nullptr, N::False);
    G.Write(Charge::UpdateActive, nullptr, N::False);
    G.Write(Charge::UpdatePending, nullptr, N::False);
    G.Write(Charge::SocketConnected, nullptr, N::False);
    G.Write(Charge::PowerAvailable, nullptr, N::False);
}
UEdGraphPin* DebitEnergy(FGraph& G, UEdGraphPin* Amount)
{
    UEdGraphPin* RequestedAmount = Amount ? Amount : RequiredEnergyAmount(G);
    auto* Debit = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, RemoveResource));
    G.Link(G.Read(Charge::Module), G.Pin(Debit, P::FunctionTarget)); G.Default(Debit, Charge::Type, Charge::Electricity);
    G.Default(Debit, Charge::RemovalType, Charge::ExactRemoval);
    G.Link(RequestedAmount, G.Pin(Debit, Charge::RemoveAmount));
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
