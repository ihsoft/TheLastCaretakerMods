#pragma once
#include "VoyageModuleComponent.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
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
UEdGraphPin* EnergyMath(FGraph& G, FName Function, UEdGraphPin* Left, const TCHAR* Right);
UEdGraphPin* RequiredEnergyAmount(FGraph& G, UEdGraphPin* EnergyKWh);
UEdGraphPin* RequiredEnergyAmount(FGraph& G);
UEdGraphPin* EnergyCapacityAmount(FGraph& G);
UEdGraphPin* ChargingInputW(FGraph& G);
UEdGraphPin* OfflineDrainAmount(FGraph& G, UEdGraphPin* DeltaSeconds,
    UEdGraphPin* StoredEnergy, UEdGraphPin* OfflineDischargeKW);
FMulticastDelegateProperty* RailgunModuleDelegateProperty(FName PropertyName);

FMulticastDelegateProperty* RailgunModulePowerDelegateProperty();

UEdGraphPin* ResolveEnergyModule(FGraph& G);

void IncrementRailgunInteger(FGraph& G, FName Field);

void GuardRailgunModuleCallback(FGraph& G, UEdGraphPin* Payload);

void SetEnergyDemand(FGraph& G, bool Charging);
UEdGraphPin* EnergyAmount(FGraph& G);

void RefreshRailgunEnergyPass(FGraph& G, UBlueprint* BP,
    UEdGraphPin* ForceDemand);

FEdGraphPinType RailgunEnergyBooleanPinType();

FEdGraphPinType RailgunEnergyRealPinType();

FEdGraphPinType RailgunEnergyIntPinType();

FEdGraphPinType RailgunEnergyModulePinType();

UEdGraphPin* DebitEnergy(FGraph& G, UEdGraphPin* Amount);

UK2Node_FunctionEntry* AddRailgunEnergyFunctionEntry(UBlueprint* BP,
    FName FunctionName, UEdGraph*& Graph);

void AddRailgunDrainRuntime(UBlueprint* BP);

void AddRailgunActivityFunctions(UBlueprint* BP,
    FName OfflineDischargeField);

void AddRailgunDeferredSupplyEvent(UBlueprint* BP);

void AddRailgunEnergyFunctions(UBlueprint* BP,
    FName OfflineDischargeField);

void AddRailgunEnergyTeardown(UBlueprint* BP);
UEdGraphPin* DebitEnergy(FGraph& G, UEdGraphPin* Amount);
}
