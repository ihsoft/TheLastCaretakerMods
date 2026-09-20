#pragma once
#include "VoyageModuleComponent.h"
namespace Charge
{
inline const FName Sampled(TEXT("HarpoonEnergySampled")), Module(TEXT("HarpoonEnergyModule"));
inline const FName Energy(TEXT("HarpoonChargeKJ")), Previous(TEXT("HarpoonPreviousChargeKJ"));
inline const FName Rate(TEXT("HarpoonChargeKW"));
inline const FName Type(TEXT("Type")), RemoveAmount(TEXT("RemoveAmount")), RemovalType(TEXT("RemovalType"));
inline const FName Input(TEXT("InAcceptanceFilter")), Capacity(TEXT("InMaxResourceAmount")), Idle(TEXT("InConsumptionON"));
inline constexpr TCHAR Electricity[] = TEXT("Electricity"), ExactRemoval[] = TEXT("ConsumptionAfterModifiers");
inline constexpr TCHAR IdleW[] = TEXT("1000.0"), ChargeInputW[] = TEXT("251000.0");
inline constexpr TCHAR FullWh[] = TEXT("139.1666666666667");
inline constexpr TCHAR ShotWh[] = TEXT("138.8888888888889"), ShotKJ[] = TEXT("500.0");
inline constexpr TCHAR KJPerWh[] = TEXT("3.6");
}
UEdGraphPin* EnergyMath(FGraph& G, FName Function, UEdGraphPin* Left, const TCHAR* Right)
{
    auto* N = G.Call(UKismetMathLibrary::StaticClass(), Function);
    G.Link(Left, G.Pin(N, P::Binary::LeftOperand)); G.Default(N, P::Binary::RightOperand, Right);
    return G.Pin(N, P::ReturnValue);
}
UEdGraphPin* FindEnergyModule(FGraph& G)
{
    G.Branch(G.Valid(G.Read(S::Anchor)));
    auto* Cannon = ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor));
    G.Branch(G.Valid(Cannon));
    auto* Find = G.Call(AActor::StaticClass(), OpticalCameraGraphNames::FindComponent);
    G.Link(Cannon, G.Pin(Find, P::FunctionTarget)); G.Pin(Find, OP::ComponentClass)->DefaultObject = UVoyageModuleComponent::StaticClass();
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = UVoyageModuleComponent::StaticClass(); Cast->SetPurity(true); G.Node(Cast);
    G.Link(G.Pin(Find, P::ReturnValue), Cast->GetCastSourcePin());
    G.Write(Charge::Module, Cast->GetCastResultPin()); G.Branch(G.Valid(G.Read(Charge::Module)));
    return G.Read(Charge::Module);
}
void SetEnergyDemand(FGraph& G, bool Charging)
{
    auto* Set = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, SetCustomConsumption));
    G.Link(G.Read(Charge::Module), G.Pin(Set, P::FunctionTarget));
    G.Default(Set, Charge::Input, Charging ? Charge::ChargeInputW : Charge::IdleW);
    // A full gun keeps its stored charge; idle demand must not shrink capacity.
    G.Default(Set, Charge::Capacity, Charge::FullWh);
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
    if (Amount) G.Link(Amount, G.Pin(Debit, Charge::RemoveAmount)); else G.Default(Debit, Charge::RemoveAmount, Charge::ShotWh);
    G.Exec(Debit); return G.Pin(Debit, P::ReturnValue);
}
void UpdateAutomaticCharge(FGraph& G, UEdGraphPin* DeltaSeconds)
{
    G.Write(Charge::Rate, nullptr, N::Zero);
    FindEnergyModule(G);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), DeltaSeconds, N::Zero));
    auto* Positive = EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), EnergyAmount(G), N::Zero);
    auto* KJ = EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble), Positive, Charge::KJPerWh);
    G.Write(Charge::Energy, EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin), KJ, Charge::ShotKJ));
    auto* Sample = G.Branch(G.Read(Charge::Sampled));
    auto* Delta = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble), G.Read(Charge::Energy), G.Read(Charge::Previous));
    auto* Net = EnergyMath(G, GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), Delta, N::Zero);
    G.Write(Charge::Rate, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble), Net, DeltaSeconds));
    StationMerge(G, {G.Tail, G.Pin(Sample, P::Else)});
    G.Write(Charge::Previous, G.Read(Charge::Energy)); G.Write(Charge::Sampled, nullptr, N::True);
    auto* Full = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), EnergyAmount(G), Charge::ShotWh));
    SetEnergyDemand(G, false);
    G.Tail = G.Pin(Full, P::Else); SetEnergyDemand(G, true);
}
