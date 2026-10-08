#pragma once
#include "RailgunInventoryNames.h"

namespace EnergyHud
{
inline const FName ChargeRadial(TEXT("RailgunChargeRadial"));
inline const FName ChargeText(TEXT("RailgunChargeText"));
inline const FName ChargeBlock(TEXT("RailgunChargeBlock"));
inline const FName AmmoIndicatorRow(TEXT("RailgunAmmoIndicatorRow"));
inline const FName ScopeAmmoIndicatorRow(TEXT("RailgunScopeAmmoIndicatorRow"));
inline const FName AmmoInitialized(TEXT("RailgunAmmoHudInitialized"));
inline const FName Station(TEXT("RailgunHudStation"));
inline const FName ActiveHud(StationLifecycle::ActiveHud);
inline const FName HudParameter(TEXT("Hud"));
inline const FName OwningPlayerPawnGetter(TEXT("GetOwningPlayerPawn"));
inline const FName RegisterFunction(TEXT("RegisterRailgunHud"));
inline const FName UnregisterFunction(TEXT("UnregisterRailgunHud"));
inline const FName RefreshEnergyFunction(Charge::RefreshHudFunction);
inline const FName RefreshAmmoFunction(TEXT("RefreshRailgunHudAmmo"));
inline const FName RefreshStyleFunction(TEXT("RefreshRailgunHudStyle"));
inline const FName RefreshModeFunction(TEXT("RefreshRailgunHudMode"));
inline const FName ApplyStatusFunction(TEXT("ApplyRailgunHudStatus"));
inline const FName ResetStatusFunction(TEXT("ResetRailgunHudStatus"));
inline const FName StatusState(TEXT("RailgunHudStatusState"));
inline const FName StatusInitialized(TEXT("RailgunHudStatusInitialized"));
inline const FName NewStatusParameter(TEXT("NewStatus"));
inline const FName StatusBlinkAnimation(TEXT("RailgunStatusChargingBlink"));
inline const TArray<FName> AmmoIndicators {
    TEXT("RailgunAmmoIndicator01"), TEXT("RailgunAmmoIndicator02"),
    TEXT("RailgunAmmoIndicator03"), TEXT("RailgunAmmoIndicator04"),
    TEXT("RailgunAmmoIndicator05"), TEXT("RailgunAmmoIndicator06")};
inline const TArray<FName> ScopeAmmoIndicators {
    TEXT("RailgunScopeAmmoIndicator01"), TEXT("RailgunScopeAmmoIndicator02"),
    TEXT("RailgunScopeAmmoIndicator03"), TEXT("RailgunScopeAmmoIndicator04"),
    TEXT("RailgunScopeAmmoIndicator05"), TEXT("RailgunScopeAmmoIndicator06")};
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
inline constexpr TCHAR AmmoIndicatorPackage[] =
    TEXT("/Game/Mods/Railgun/Station/T_RailgunAmmoIndicator");
inline constexpr TCHAR AmmoIndicatorAsset[] = TEXT("T_RailgunAmmoIndicator");
inline constexpr TCHAR AmmoIndicatorSourceArgument[] = TEXT("AmmoIndicator=");
inline UTexture2D* ChargingTexture = nullptr;
inline UTexture2D* OfflineTexture = nullptr;
inline UTexture2D* ReadyTexture = nullptr;
inline UTexture2D* AmmoIndicatorTexture = nullptr;
inline constexpr float StatusOffsetY = -190.0f;
inline constexpr float ChargeGaugeSize = 150.0f;
inline constexpr float ChargeGaugeMargin = 20.0f;
inline constexpr float ChargeGaugeCenterOffset = -(ChargeGaugeMargin + ChargeGaugeSize * 0.5f);
inline constexpr float ChargeGaugeBarThickness = 20.0f;
inline constexpr float ChargeGaugeFontSize = 14.0f;
inline const FLinearColor ChargeGaugeBarColor(1.0f, 1.0f, 1.0f, 0.2f);
inline const FLinearColor ChargeGaugeProgressColor = FLinearColor::White;
inline const FLinearColor EmptyAmmoTint(1.0f, 0.25f, 0.25f, 0.3f);
inline constexpr TCHAR ReadyChargeTextColor[] =
    TEXT("(SpecifiedColor=(R=1.000000,G=1.000000,B=1.000000,A=1.000000),ColorUseRule=UseColor_Specified)");
inline constexpr TCHAR InsufficientChargeTextColor[] =
    TEXT("(SpecifiedColor=(R=1.000000,G=0.250000,B=0.250000,A=0.300000),ColorUseRule=UseColor_Specified)");
inline constexpr float AmmoIndicatorHeight = 30.0f;
inline constexpr float AmmoIndicatorGap = 6.0f;
inline constexpr float AmmoIndicatorTextGap = 5.0f;
// Fixed authored HUD metric: rounded 70% of the current 54px scope energy
// indicator.
// Do not derive this from either texture so future source-art changes cannot
// resize the scope ammunition row implicitly.
inline constexpr float ScopeAmmoIndicatorHeight = 38.0f;
inline constexpr float ScopeAmmoIndicatorGap = 14.0f;
inline constexpr float ScopeAmmoIndicatorOffsetY = -252.0f;
inline const FLinearColor ScopeAmmoActiveTint(0.65f, 0.95f, 1.0f, 0.65f);
inline const FLinearColor ScopeAmmoInactiveTint(0.65f, 0.95f, 1.0f, 0.22f);
inline constexpr float AmmoCropLeft = 548.0f;
inline constexpr float AmmoCropTop = 372.0f;
inline constexpr float AmmoCropRight = 706.0f;
inline constexpr float AmmoCropBottom = 895.0f;
inline constexpr TCHAR EmptyChargeDisplay[] = TEXT("0.0 KWh");
inline constexpr TCHAR ChargeUnitSuffix[] = TEXT(" KWh");
inline constexpr TCHAR Hidden[] = TEXT("Collapsed");
inline constexpr TCHAR Shown[] = TEXT("HitTestInvisible");
inline constexpr TCHAR PercentMultiplier[] = TEXT("0.01");
inline constexpr TCHAR NormalizedMaximum[] = TEXT("1.0");
inline constexpr TCHAR MinimumChargeFractionDigits[] = TEXT("1");
inline constexpr TCHAR MaximumChargeFractionDigits[] = TEXT("1");
inline constexpr TCHAR StatusHiddenState[] = TEXT("0");
inline constexpr TCHAR StatusOfflineState[] = TEXT("1");
inline constexpr TCHAR StatusReadyState[] = TEXT("2");
inline constexpr TCHAR StatusChargingState[] = TEXT("3");
inline constexpr int32 BlinkDisplayFramesPerSecond = 4;
inline constexpr double BlinkDurationSeconds = 0.5;
inline constexpr double BlinkHiddenTimeSeconds = 0.25;
inline constexpr float StatusVisibleOpacity = 1.0f;
inline constexpr float StatusHiddenOpacity = 0.0f;
inline constexpr TCHAR AnimationStartTime[] = TEXT("0.0");
inline constexpr TCHAR AnimationLoopForever[] = TEXT("0");
inline constexpr TCHAR AnimationPlaybackSpeed[] = TEXT("1.0");
inline constexpr TCHAR AnimationForward[] = TEXT("Forward");
inline const FName AnimationPin(TEXT("InAnimation"));
inline const FName AnimationStartTimePin(TEXT("StartAtTime"));
inline const FName AnimationLoopCountPin(TEXT("NumLoopsToPlay"));
inline const FName AnimationPlayModePin(TEXT("PlayMode"));
inline const FName AnimationPlaybackSpeedPin(TEXT("PlaybackSpeed"));
inline const FName AnimationRestoreStatePin(TEXT("bRestoreState"));
inline const FName RenderOpacityProperty(TEXT("RenderOpacity"));
inline const FName DoubleInputPin(TEXT("InDouble"));
inline const FName RadialValuePin(TEXT("InValue"));
inline const FName SliderProgressColorPin(TEXT("InValue"));
inline const FName NumericValuePin(TEXT("Value"));
inline const FName UseGroupingPin(TEXT("bUseGrouping"));
inline const FName MinimumFractionalDigitsPin(TEXT("MinimumFractionalDigits"));
inline const FName MaximumFractionalDigitsPin(TEXT("MaximumFractionalDigits"));
inline const FName FormattedTextPin(TEXT("InText"));
inline const FName ColorAndOpacityPin(TEXT("InColorAndOpacity"));
}

void UpdateStationAmmoHud(FGraph& G, UEdGraphPin* Station,
    UClass* StationClass, UClass* RailgunModuleClass)
{
    using namespace EnergyHud;
    UEdGraphPin* Anchor = ReadNativeInputField(
        G, Station, StationClass, S::Anchor);
    G.Branch(G.Valid(Anchor));
    auto* Railgun = ObserveCall(G, UActorComponent::StaticClass(),
        ActorScanGraphNames::GetActorOwner, Anchor);
    auto* Module = NewObject<UK2Node_DynamicCast>(G.Graph);
    Module->TargetType = RailgunModuleClass;
    Module->SetPurity(true);
    G.Node(Module);
    G.Link(Railgun, Module->GetCastSourcePin());
    G.Branch(G.Valid(Module->GetCastResultPin()));

    auto* Initialized = G.Branch(G.Read(AmmoInitialized));
    UEdGraphPin* AlreadyInitialized = G.Tail;
    G.Tail = G.Pin(Initialized, P::Else);
    auto* InitialSync = G.Call(
        RailgunModuleClass, RailgunInventoryShared::SyncVisuals);
    G.Link(Module->GetCastResultPin(),
        G.Pin(InitialSync, P::FunctionTarget));
    G.Exec(InitialSync);
    G.Write(AmmoInitialized, nullptr, N::True);
    StationMerge(G, {AlreadyInitialized, G.Tail});

    UEdGraphPin* Count = ReadNativeInputField(G,
        Module->GetCastResultPin(), RailgunModuleClass,
        RailgunInventoryShared::LastVisualCount);
    UEdGraphPin* FaintColor = ReadNativeInputField(G,
        G.Read(ChargeRadial), URadialSlider::StaticClass(),
        GET_MEMBER_NAME_CHECKED(URadialSlider, SliderBarColor));
    UEdGraphPin* Empty = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Count, N::Zero);
    auto* InactiveTint = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectColor));
    G.Default(InactiveTint, P::Select::WhenTrue, *EmptyAmmoTint.ToString());
    G.Link(FaintColor, G.Pin(InactiveTint, P::Select::WhenFalse));
    G.Link(Empty, G.Pin(InactiveTint, P::Select::Condition));
    UEdGraphPin* InactiveColor = G.Pin(InactiveTint, P::ReturnValue);
    auto* ScopeInactiveTint = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectColor));
    G.Default(ScopeInactiveTint, P::Select::WhenTrue,
        *EmptyAmmoTint.ToString());
    G.Default(ScopeInactiveTint, P::Select::WhenFalse,
        *ScopeAmmoInactiveTint.ToString());
    G.Link(Empty, G.Pin(ScopeInactiveTint, P::Select::Condition));
    UEdGraphPin* ScopeInactiveColor =
        G.Pin(ScopeInactiveTint, P::ReturnValue);
    for (int32 Index = 0; Index < AmmoIndicators.Num(); ++Index)
    {
        const int32 ActivationThreshold = AmmoIndicators.Num() - Index - 1;
        UEdGraphPin* Active = G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            Count, *FString::FromInt(ActivationThreshold));
        auto* Tint = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectColor));
        G.Default(Tint, P::Select::WhenTrue,
            *ChargeGaugeProgressColor.ToString());
        G.Link(InactiveColor, G.Pin(Tint, P::Select::WhenFalse));
        G.Link(Active, G.Pin(Tint, P::Select::Condition));
        auto* SetColor = G.Call(UImage::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UImage, SetColorAndOpacity));
        G.Link(G.Read(AmmoIndicators[Index]),
            G.Pin(SetColor, P::FunctionTarget));
        G.Link(G.Pin(Tint, P::ReturnValue),
            G.Pin(SetColor, ColorAndOpacityPin));
        G.Exec(SetColor);
    }
    for (int32 Index = 0; Index < ScopeAmmoIndicators.Num(); ++Index)
    {
        const int32 ActivationThreshold =
            ScopeAmmoIndicators.Num() - Index - 1;
        UEdGraphPin* Active = G.Compare(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
            Count, *FString::FromInt(ActivationThreshold));
        auto* Tint = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectColor));
        G.Default(Tint, P::Select::WhenTrue,
            *ScopeAmmoActiveTint.ToString());
        G.Link(ScopeInactiveColor, G.Pin(Tint, P::Select::WhenFalse));
        G.Link(Active, G.Pin(Tint, P::Select::Condition));
        auto* SetColor = G.Call(UImage::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UImage, SetColorAndOpacity));
        G.Link(G.Read(ScopeAmmoIndicators[Index]),
            G.Pin(SetColor, P::FunctionTarget));
        G.Link(G.Pin(Tint, P::ReturnValue),
            G.Pin(SetColor, ColorAndOpacityPin));
        G.Exec(SetColor);
    }
}

// The charge display reads production state from the possessed station.
void UpdateStationEnergyHud(FGraph& G, UEdGraphPin* Station,
    UClass* StationClass)
{
    // This graph belongs to the widget itself, not an external observer actor.
    auto SetText = [&](FName Field, const TCHAR* Literal, UEdGraphPin* String = nullptr)
    {
        auto* Text = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
        if (String) G.Link(String, G.Pin(Text, E::StringValue));
        else G.Default(Text, E::StringValue, Literal);
        auto* Set = G.Call(UTextBlock::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
        G.Link(G.Read(Field), G.Pin(Set, P::FunctionTarget));
        G.Link(G.Pin(Text, P::ReturnValue), G.Pin(Set, E::WidgetText)); G.Exec(Set);
    };
    auto* CurrentCharge = ReadNativeInputField(G, Station, StationClass, Charge::Energy);
    auto* FullCharge = ReadNativeInputField(G, Station, StationClass, Charge::ConfiguredEnergyKWh);
    auto* Fraction = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble),
        CurrentCharge, RequiredEnergyAmount(G, FullCharge));
    auto* ClampedFraction = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
    G.Link(Fraction, G.Pin(ClampedFraction, OP::ClampValue));
    G.Default(ClampedFraction, OP::ClampMinimum, N::Zero);
    G.Default(ClampedFraction, OP::ClampMaximum, EnergyHud::NormalizedMaximum);
    auto* FractionAsFloat = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_DoubleToFloat));
    G.Link(G.Pin(ClampedFraction, P::ReturnValue),
        G.Pin(FractionAsFloat, EnergyHud::DoubleInputPin));
    auto* SetRadialValue = G.Call(URadialSlider::StaticClass(), GET_FUNCTION_NAME_CHECKED(URadialSlider, SetValue));
    G.Link(G.Read(EnergyHud::ChargeRadial), G.Pin(SetRadialValue, P::FunctionTarget));
    G.Link(G.Pin(FractionAsFloat, P::ReturnValue), G.Pin(SetRadialValue, EnergyHud::RadialValuePin)); G.Exec(SetRadialValue);

    auto SetChargeColors = [&](const TCHAR* TextColor,
        const FLinearColor& ProgressColor)
    {
        auto* SetTextColor = G.Call(UTextBlock::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UTextBlock, SetColorAndOpacity));
        G.Link(G.Read(EnergyHud::ChargeText),
            G.Pin(SetTextColor, P::FunctionTarget));
        G.Default(SetTextColor, EnergyHud::ColorAndOpacityPin, TextColor);
        G.Exec(SetTextColor);
        auto* SetProgressColor = G.Call(URadialSlider::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(URadialSlider,
                SetSliderProgressColor));
        G.Link(G.Read(EnergyHud::ChargeRadial),
            G.Pin(SetProgressColor, P::FunctionTarget));
        G.Default(SetProgressColor, EnergyHud::SliderProgressColorPin,
            *ProgressColor.ToString());
        G.Exec(SetProgressColor);
    };
    UEdGraphPin* InsufficientCharge = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble),
        CurrentCharge, RequiredEnergyAmount(G, FullCharge));
    UEdGraphPin* ChargeModule = ReadNativeInputField(
        G, Station, StationClass, Charge::Module);
    auto* ChargeModuleValid = G.Branch(G.Valid(ChargeModule));
    UEdGraphPin* ValidChargeModuleTail = G.Tail;
    G.Tail = G.Pin(ChargeModuleValid, P::Else);
    SetChargeColors(EnergyHud::ReadyChargeTextColor,
        EnergyHud::ChargeGaugeProgressColor);
    UEdGraphPin* InvalidChargeModuleTail = G.Tail;
    G.Tail = ValidChargeModuleTail;
    auto* TintCharge = G.Branch(InsufficientCharge);
    SetChargeColors(EnergyHud::InsufficientChargeTextColor,
        EnergyHud::EmptyAmmoTint);
    UEdGraphPin* InsufficientChargeTail = G.Tail;
    G.Tail = G.Pin(TintCharge, P::Else);
    SetChargeColors(EnergyHud::ReadyChargeTextColor,
        EnergyHud::ChargeGaugeProgressColor);
    StationMerge(G, {InvalidChargeModuleTail, InsufficientChargeTail, G.Tail});

    UEdGraphPin* ChargeKWh = EnergyMath(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble),
        CurrentCharge, Charge::GameResourceUnitsPerKWh);
    auto* ChargeAsText = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_DoubleToText));
    G.Link(ChargeKWh, G.Pin(ChargeAsText, EnergyHud::NumericValuePin));
    G.Default(ChargeAsText, EnergyHud::UseGroupingPin, N::False);
    G.Default(ChargeAsText, EnergyHud::MinimumFractionalDigitsPin, EnergyHud::MinimumChargeFractionDigits);
    G.Default(ChargeAsText, EnergyHud::MaximumFractionalDigitsPin, EnergyHud::MaximumChargeFractionDigits);
    auto* ChargeAsString = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_TextToString));
    G.Link(G.Pin(ChargeAsText, P::ReturnValue), G.Pin(ChargeAsString, EnergyHud::FormattedTextPin));
    auto* WithUnit = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr));
    G.Link(G.Pin(ChargeAsString, P::ReturnValue), G.Pin(WithUnit, P::Binary::LeftOperand));
    G.Default(WithUnit, P::Binary::RightOperand, EnergyHud::ChargeUnitSuffix);
    SetText(EnergyHud::ChargeText, nullptr, G.Pin(WithUnit, P::ReturnValue));

}

void ApplyStationStatusHud(FGraph& G, UClass* HudClass, const TCHAR* State)
{
    auto* Apply = G.Call(HudClass, EnergyHud::ApplyStatusFunction);
    G.Default(Apply, EnergyHud::NewStatusParameter, State);
    G.Exec(Apply);
}

void UpdateStationStatusHud(FGraph& G, UEdGraphPin* Station,
    UClass* StationClass, UClass* HudClass, UEdGraphPin* IsWide)
{
    auto* Wide = G.Branch(IsWide);
    ApplyStationStatusHud(G, HudClass, EnergyHud::StatusHiddenState);
    auto* WideTail = G.Tail;
    G.Tail = G.Pin(Wide, P::Else);
    auto* Module = ReadNativeInputField(G, Station, StationClass, Charge::Module);
    auto* ModuleValid = G.Branch(G.Valid(Module));
    auto* ValidModuleTail = G.Tail;
    G.Tail = G.Pin(ModuleValid, P::Else);
    ApplyStationStatusHud(G, HudClass, EnergyHud::StatusOfflineState);
    auto* MissingModuleTail = G.Tail;
    G.Tail = ValidModuleTail;

    auto* Full = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
        ReadNativeInputField(G, Station, StationClass, Charge::Energy),
        RequiredEnergyAmount(G, ReadNativeInputField(G, Station, StationClass, Charge::ConfiguredEnergyKWh))));
    ApplyStationStatusHud(G, HudClass, EnergyHud::StatusReadyState);
    auto* ReadyTail = G.Tail;

    G.Tail = G.Pin(Full, P::Else);
    auto* CanCharge = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND));
    G.Link(ReadNativeInputField(G, Station, StationClass,
        Charge::SocketConnected), G.Pin(CanCharge, P::Binary::LeftOperand));
    G.Link(ReadNativeInputField(G, Station, StationClass,
        Charge::PowerAvailable), G.Pin(CanCharge, P::Binary::RightOperand));
    auto* Charging = G.Branch(G.Pin(CanCharge, P::ReturnValue));
    ApplyStationStatusHud(G, HudClass, EnergyHud::StatusChargingState);
    auto* ChargingTail = G.Tail;

    G.Tail = G.Pin(Charging, P::Else);
    ApplyStationStatusHud(G, HudClass, EnergyHud::StatusOfflineState);
    auto* OfflineTail = G.Tail;
    StationMerge(G, {WideTail, MissingModuleTail, ReadyTail, ChargingTail,
        OfflineTail});
}
