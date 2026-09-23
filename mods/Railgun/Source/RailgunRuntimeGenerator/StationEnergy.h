#pragma once
#include "VoyageModuleComponent.h"
namespace Charge
{
inline const FName Sampled(TEXT("RailgunEnergySampled")), Module(TEXT("RailgunEnergyModule"));
inline const FName Energy(TEXT("RailgunChargeAmount")), Previous(TEXT("RailgunPreviousChargeAmount"));
inline const FName Rate(TEXT("RailgunChargeKW"));
inline const FName ConfiguredEnergyKWh(TEXT("RailgunFullChargeEnergyKWh"));
inline const FName ConfiguredTimeSeconds(TEXT("RailgunFullChargeTimeSeconds"));
inline const FName Type(TEXT("Type")), RemoveAmount(TEXT("RemoveAmount")), RemovalType(TEXT("RemovalType"));
inline const FName Input(TEXT("InAcceptanceFilter")), Capacity(TEXT("InMaxResourceAmount")), Idle(TEXT("InConsumptionON"));
inline constexpr TCHAR Electricity[] = TEXT("Electricity"), ExactRemoval[] = TEXT("ConsumptionAfterModifiers");
inline constexpr TCHAR IdleW[] = TEXT("1000.0");
inline constexpr TCHAR IdleCapacityAmount[] = TEXT("1.0");
inline constexpr TCHAR GameResourceUnitsPerKWh[] = TEXT("1000.0");
inline constexpr TCHAR WattsPerResourceUnit[] = TEXT("1000.0");
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
UEdGraphPin* FindEnergyModule(FGraph& G)
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
UEdGraphPin* DebitEnergy(FGraph& G, UEdGraphPin* Amount = nullptr)
{
    auto* Debit = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, RemoveResource));
    G.Link(G.Read(Charge::Module), G.Pin(Debit, P::FunctionTarget)); G.Default(Debit, Charge::Type, Charge::Electricity);
    G.Default(Debit, Charge::RemovalType, Charge::ExactRemoval);
    G.Link(Amount ? Amount : RequiredEnergyAmount(G), G.Pin(Debit, Charge::RemoveAmount));
    G.Exec(Debit); return G.Pin(Debit, P::ReturnValue);
}
void UpdateAutomaticCharge(FGraph& G, UEdGraphPin* DeltaSeconds)
{
    G.Write(Charge::Rate, nullptr, N::Zero);
    FindEnergyModule(G);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), DeltaSeconds, N::Zero));
    auto* Positive = EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), EnergyAmount(G), N::Zero);
    G.Write(Charge::Energy, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin),
        Positive, RequiredEnergyAmount(G)));
    auto* Sample = G.Branch(G.Read(Charge::Sampled));
    auto* Delta = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble), G.Read(Charge::Energy), G.Read(Charge::Previous));
    auto* Net = EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), Delta, N::Zero);
    G.Write(Charge::Rate, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble), Net, DeltaSeconds));
    StationMerge(G, {G.Tail, G.Pin(Sample, P::Else)});
    G.Write(Charge::Previous, G.Read(Charge::Energy)); G.Write(Charge::Sampled, nullptr, N::True);
    auto* Full = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
        EnergyAmount(G), RequiredEnergyAmount(G)));
    SetEnergyDemand(G, false);
    G.Tail = G.Pin(Full, P::Else); SetEnergyDemand(G, true);
}
