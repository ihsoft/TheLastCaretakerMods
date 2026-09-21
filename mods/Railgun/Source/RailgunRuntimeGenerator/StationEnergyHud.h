#pragma once

namespace EnergyHud
{
inline const FName Connection(TEXT("RailgunEnergyConnection"));
inline const FName Power(TEXT("RailgunEnergyPower"));
inline const FName Progress(TEXT("RailgunEnergyProgress")), Rate(TEXT("RailgunEnergyRate"));
inline const FName StatusCharging(TEXT("RailgunStatusCharging"));
inline const FName StatusOffline(TEXT("RailgunStatusOffline"));
inline const FName StatusReady(TEXT("RailgunStatusReady"));
inline constexpr TCHAR ChargingPackage[] = TEXT("/Game/Mods/Railgun/Station/T_RailgunStatusCharging");
inline constexpr TCHAR OfflinePackage[] = TEXT("/Game/Mods/Railgun/Station/T_RailgunStatusOffline");
inline constexpr TCHAR ReadyPackage[] = TEXT("/Game/Mods/Railgun/Station/T_RailgunStatusReady");
inline constexpr TCHAR ChargingAsset[] = TEXT("T_RailgunStatusCharging");
inline constexpr TCHAR OfflineAsset[] = TEXT("T_RailgunStatusOffline");
inline constexpr TCHAR ReadyAsset[] = TEXT("T_RailgunStatusReady");
inline constexpr TCHAR ChargingSourceArgument[] = TEXT("ChargingStatusIcon=");
inline constexpr TCHAR OfflineSourceArgument[] = TEXT("OfflineStatusIcon=");
inline constexpr TCHAR ReadySourceArgument[] = TEXT("ReadyStatusIcon=");
inline UTexture2D* ChargingTexture = nullptr;
inline UTexture2D* OfflineTexture = nullptr;
inline UTexture2D* ReadyTexture = nullptr;
inline constexpr float DiagnosticLeft = 24.0f;
inline constexpr float StatusOffsetY = -190.0f;
inline constexpr float ConnectionOffset = 400.0f;
inline constexpr float PowerOffset = 428.0f;
inline constexpr float ProgressOffset = 456.0f;
inline constexpr float RateOffset = 484.0f;
inline constexpr TCHAR EmptyCharge[] = TEXT("Charge kJ: 0");
inline constexpr TCHAR EmptyRate[] = TEXT("CHARGING");
inline constexpr TCHAR ChargePrefix[] = TEXT("Charge kJ: ");
inline constexpr TCHAR Ready[] = TEXT("READY");
inline constexpr TCHAR UnknownConnection[] = TEXT("Grid: module unavailable");
inline constexpr TCHAR UnknownPower[] = TEXT("Power: unknown");
inline constexpr TCHAR Connected[] = TEXT("Grid: connected");
inline constexpr TCHAR Disconnected[] = TEXT("Grid: disconnected");
inline constexpr TCHAR Powered[] = TEXT("Power: available");
inline constexpr TCHAR Unpowered[] = TEXT("Power: unavailable");
inline constexpr TCHAR Hidden[] = TEXT("Collapsed");
inline constexpr TCHAR Shown[] = TEXT("HitTestInvisible");
inline constexpr TCHAR PercentMultiplier[] = TEXT("0.01");
inline constexpr TCHAR BlinkPeriodSeconds[] = TEXT("0.5");
inline constexpr TCHAR BlinkVisibleSeconds[] = TEXT("0.25");
inline const FName DividendPin(TEXT("Dividend"));
inline const FName DivisorPin(TEXT("Divisor"));
inline const FName RemainderPin(TEXT("Remainder"));
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
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Pin(Amount, P::ReturnValue),
            RequiredEnergyAmount(G, ReadNativeInputField(G, Station, StationClass, Charge::ConfiguredEnergyKJ))),
        EnergyHud::Ready, EnergyHud::EmptyRate);
    SetBoolean(EnergyHud::Connection,
        ObserveCall(G, UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasSocketConnection), Module->GetCastResultPin()),
        EnergyHud::Connected, EnergyHud::Disconnected);
    SetBoolean(EnergyHud::Power,
        ObserveCall(G, UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasPower), Module->GetCastResultPin()),
        EnergyHud::Powered, EnergyHud::Unpowered);
}

void UpdateStationStatusHud(FGraph& G, UEdGraphPin* Station, UClass* StationClass,
    UEdGraphPin* IsWide, UEdGraphPin* OpacityPercent)
{
    auto SetVisibility = [&](FName Field, const TCHAR* Visibility)
    {
        auto* Set = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
        G.Link(G.Read(Field), G.Pin(Set, P::FunctionTarget));
        G.Default(Set, OP::Visibility, Visibility); G.Exec(Set);
    };
    auto SetVisible = [&](FName Field) { SetVisibility(Field, EnergyHud::Shown); };
    const FName StatusFields[] = {EnergyHud::StatusCharging, EnergyHud::StatusOffline, EnergyHud::StatusReady};
    for (FName Field : StatusFields) SetVisibility(Field, EnergyHud::Hidden);

    auto* NormalizedOpacity = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(OpacityPercent, G.Pin(NormalizedOpacity, P::Binary::LeftOperand));
    G.Default(NormalizedOpacity, P::Binary::RightOperand, EnergyHud::PercentMultiplier);
    for (FName Field : StatusFields)
    {
        auto* Set = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetRenderOpacity));
        G.Link(G.Read(Field), G.Pin(Set, P::FunctionTarget));
        G.Link(G.Pin(NormalizedOpacity, P::ReturnValue), G.Pin(Set, Settings::OpacityPin)); G.Exec(Set);
    }

    auto* Wide = G.Branch(IsWide);
    auto* WideTail = G.Tail;
    G.Tail = G.Pin(Wide, P::Else);
    auto* Module = ReadNativeInputField(G, Station, StationClass, Charge::Module);
    auto* ModuleValid = G.Branch(G.Valid(Module));
    auto* ValidModuleTail = G.Tail;
    G.Tail = G.Pin(ModuleValid, P::Else); SetVisible(EnergyHud::StatusOffline); auto* MissingModuleTail = G.Tail;
    G.Tail = ValidModuleTail;

    auto* Amount = G.Call(UVoyageModuleComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, GetResourceAmount));
    G.Link(Module, G.Pin(Amount, P::FunctionTarget)); G.Default(Amount, Charge::Type, Charge::Electricity);
    auto* Full = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
        G.Pin(Amount, P::ReturnValue),
        RequiredEnergyAmount(G, ReadNativeInputField(G, Station, StationClass, Charge::ConfiguredEnergyKJ))));
    SetVisible(EnergyHud::StatusReady); auto* ReadyTail = G.Tail;

    G.Tail = G.Pin(Full, P::Else);
    auto* Connected = ObserveCall(G, UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasSocketConnection), Module);
    auto* Powered = ObserveCall(G, UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent, HasPower), Module);
    auto* CanCharge = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND));
    G.Link(Connected, G.Pin(CanCharge, P::Binary::LeftOperand));
    G.Link(Powered, G.Pin(CanCharge, P::Binary::RightOperand));
    auto* Charging = G.Branch(G.Pin(CanCharge, P::ReturnValue));

    auto* Time = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetGameTimeInSeconds));
    auto* Phase = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMod));
    G.Link(G.Pin(Time, P::ReturnValue), G.Pin(Phase, EnergyHud::DividendPin));
    G.Default(Phase, EnergyHud::DivisorPin, EnergyHud::BlinkPeriodSeconds);
    auto* BlinkOn = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble),
        G.Pin(Phase, EnergyHud::RemainderPin), EnergyHud::BlinkVisibleSeconds));
    SetVisible(EnergyHud::StatusCharging); auto* ChargingVisibleTail = G.Tail;
    auto* ChargingHiddenTail = G.Pin(BlinkOn, P::Else);

    G.Tail = G.Pin(Charging, P::Else); SetVisible(EnergyHud::StatusOffline); auto* OfflineTail = G.Tail;
    StationMerge(G, {WideTail, MissingModuleTail, ReadyTail, ChargingVisibleTail, ChargingHiddenTail, OfflineTail});
}
