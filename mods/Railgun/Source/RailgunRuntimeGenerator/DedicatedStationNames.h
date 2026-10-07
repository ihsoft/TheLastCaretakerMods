#pragma once
#include "CoreMinimal.h"
namespace DedicatedStationNames
{
inline constexpr TCHAR OperatorPackage[] = TEXT("/Game/Mods/Railgun/Station/BP_RailgunOperator");
inline constexpr TCHAR HudPackage[] = TEXT("/Game/Mods/Railgun/Station/WBP_RailgunHUD");
inline constexpr TCHAR ContextPath[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunInputContext.DA_RailgunInputContext");
inline const FName Sight(TEXT("RailgunSightComponent"));
inline const FVector2D HintHostOffset(24.0f, -24.0f);
inline constexpr TCHAR VerifySwitch[] = TEXT("VerifyTagged");
inline constexpr TCHAR DedicatedSwitch[] = TEXT("DedicatedStation");
inline constexpr TCHAR CookPrefix[] = TEXT("Saved/Cooked/Windows/Voyage/Content/");
inline constexpr TCHAR GamePrefix[] = TEXT("/Game/");
inline const FName Camera(TEXT("RailgunViewActor"));
inline const FName Controller(TEXT("RailgunViewController"));
inline const FName ViewOwned(TEXT("RailgunOwnsView"));
inline const FName RefreshActivity(TEXT("RefreshRailgunRuntimeActivity"));
inline constexpr float ContinuousTickInterval = 0.0f;
inline const FName ActionValue(TEXT("ActionValue"));
inline const FName Triggered(TEXT("Triggered"));
inline const FName Started(TEXT("Started"));
}
