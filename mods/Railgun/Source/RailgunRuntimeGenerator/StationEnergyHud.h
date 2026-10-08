#pragma once
#include "RailgunInventoryNames.h"
#include "StationEnergy.h"
#include "StationEntryGraph.h"

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
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
    UClass* StationClass, UClass* RailgunModuleClass);

// The charge display reads production state from the possessed station.
void UpdateStationEnergyHud(FGraph& G, UEdGraphPin* Station,
    UClass* StationClass);

void ApplyStationStatusHud(FGraph& G, UClass* HudClass, const TCHAR* State);

void UpdateStationStatusHud(FGraph& G, UEdGraphPin* Station,
    UClass* StationClass, UClass* HudClass, UEdGraphPin* IsWide);
}
