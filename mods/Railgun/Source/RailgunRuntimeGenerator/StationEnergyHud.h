#pragma once

namespace EnergyHud
{
inline const FName Connection(TEXT("RailgunEnergyConnection"));
inline const FName Power(TEXT("RailgunEnergyPower"));
inline const FName Progress(TEXT("RailgunEnergyProgress")), Rate(TEXT("RailgunEnergyRate"));
inline constexpr float DiagnosticLeft = 24.0f;
inline constexpr float ConnectionOffset = 400.0f;
inline constexpr float PowerOffset = 428.0f;
inline constexpr float ProgressOffset = 456.0f;
inline constexpr float RateOffset = 484.0f;
inline constexpr TCHAR EmptyCharge[] = TEXT("Charge kJ (required 500): 0");
inline constexpr TCHAR EmptyRate[] = TEXT("CHARGING");
inline constexpr TCHAR ChargePrefix[] = TEXT("Charge kJ (required 500): ");
inline constexpr TCHAR Ready[] = TEXT("READY");
inline constexpr TCHAR UnknownConnection[] = TEXT("Grid: module unavailable");
inline constexpr TCHAR UnknownPower[] = TEXT("Power: unknown");
inline constexpr TCHAR Connected[] = TEXT("Grid: connected");
inline constexpr TCHAR Disconnected[] = TEXT("Grid: disconnected");
inline constexpr TCHAR Powered[] = TEXT("Power: available");
inline constexpr TCHAR Unpowered[] = TEXT("Power: unavailable");
}

// Diagnostic values come from the railgun module, not the possessed station.
// Reset before guards so a destroyed/unavailable module cannot leave stale YES.
void UpdateStationEnergyHud(FGraph& G, UEdGraphPin* Station, UClass* StationClass)
{
    // This graph belongs to the widget itself, not an external observer actor.
    // FGraph::Text requires HudClass/HudInstance and is a no-op in this graph.
    auto SetText = [&](FName Field, const TCHAR* Literal, UEdGraphPin* String = nullptr)
    {
        auto* Text = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
        if (String) G.Link(String, G.Pin(Text, E::StringValue));
        else G.Default(Text, E::StringValue, Literal);
        auto* Set = G.Call(UTextBlock::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
        G.Link(G.Read(Field), G.Pin(Set, P::FunctionTarget));
        G.Link(G.Pin(Text, P::ReturnValue), G.Pin(Set, E::WidgetText)); G.Exec(Set);
    };
    auto SetNumber = [&](FName Field, const TCHAR* Prefix, UEdGraphPin* Value)
    {
        auto* String = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Double));
        G.Default(String, E::StringPrefix, Prefix); G.Link(Value, G.Pin(String, E::DoubleValue));
        SetText(Field, nullptr, G.Pin(String, P::ReturnValue));
    };
    auto SetBoolean = [&](FName Field, UEdGraphPin* Condition, const TCHAR* WhenTrue, const TCHAR* WhenFalse)
    {
        auto* Select = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectString));
        G.Link(Condition, G.Pin(Select, P::Select::Condition));
        G.Default(Select, P::Select::WhenTrue, WhenTrue); G.Default(Select, P::Select::WhenFalse, WhenFalse);
        SetText(Field, nullptr, G.Pin(Select, P::ReturnValue));
    };
    SetText(EnergyHud::Connection, EnergyHud::UnknownConnection);
    SetText(EnergyHud::Power, EnergyHud::UnknownPower);
    SetNumber(EnergyHud::Progress, EnergyHud::ChargePrefix, ReadNativeInputField(G, Station, StationClass, Charge::Energy));
    SetText(EnergyHud::Rate, EnergyHud::EmptyRate);
    auto* Anchor = ReadNativeInputField(G, Station, StationClass, S::Anchor);
    G.Branch(G.Valid(Anchor));
    auto* Railgun = ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, Anchor);
    G.Branch(G.Valid(Railgun));
    auto* Find = G.Call(AActor::StaticClass(), OpticalCameraGraphNames::FindComponent);
    G.Link(Railgun, G.Pin(Find, P::FunctionTarget));
    G.Pin(Find, OP::ComponentClass)->DefaultObject = UVoyageModuleComponent::StaticClass();
    auto* Module = NewObject<UK2Node_DynamicCast>(G.Graph);
    Module->TargetType = UVoyageModuleComponent::StaticClass(); Module->SetPurity(true); G.Node(Module);
    G.Link(G.Pin(Find, P::ReturnValue), Module->GetCastSourcePin());
    G.Branch(G.Valid(Module->GetCastResultPin()));
    auto* Amount = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetResourceAmount));
    G.Link(Module->GetCastResultPin(), G.Pin(Amount, P::FunctionTarget));
    G.Default(Amount, Charge::Type, Charge::Electricity);
    SetBoolean(EnergyHud::Rate,
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Pin(Amount, P::ReturnValue), Charge::ShotWh),
        EnergyHud::Ready, EnergyHud::EmptyRate);
    SetBoolean(EnergyHud::Connection,
        ObserveCall(G, UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasSocketConnection), Module->GetCastResultPin()),
        EnergyHud::Connected, EnergyHud::Disconnected);
    SetBoolean(EnergyHud::Power,
        ObserveCall(G, UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasPower), Module->GetCastResultPin()),
        EnergyHud::Powered, EnergyHud::Unpowered);
}
