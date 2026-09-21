#pragma once
#include "CoreMinimal.h"
namespace DedicatedStationNames
{
inline constexpr TCHAR OperatorPackage[] = TEXT("/Game/Mods/Railgun/Station/BP_RailgunOperator");
inline constexpr TCHAR HudPackage[] = TEXT("/Game/Mods/Railgun/Station/WBP_RailgunHUD");
inline constexpr TCHAR Title[] = TEXT("RAILGUN / BARREL SIGHT / FILE SETTINGS");
inline constexpr TCHAR Ready[] = TEXT("HC33: preparing built railguns; aim at base. Do not save.");
inline constexpr TCHAR Prepared[] = TEXT("HC28 PREPARED: F8 enters; check native E hint. Do not save.");
inline constexpr TCHAR Active[] = TEXT("HC28: check Exit Railgun key card; E exits, F8 fallback (max20s).");
inline constexpr TCHAR HudLabel[] = TEXT("x5");
inline constexpr TCHAR HudObserved[] = TEXT("Own HUD override callback observed: YES");
inline constexpr TCHAR HudNotObserved[] = TEXT("Own HUD override callback observed: NO");
inline constexpr TCHAR ContextPath[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunInputContext.DA_RailgunInputContext");
inline const FName Sight(TEXT("RailgunSightComponent"));
inline constexpr float TargetNameOffsetY = 72.0f;
inline constexpr float TargetRangeOffsetY = 100.0f;
inline constexpr float HintStatusOffsetY = 175.0f;
inline const FVector2D HintHostOffset(24.0f, -24.0f);
inline constexpr TCHAR VerifySwitch[] = TEXT("VerifyTagged");
inline constexpr TCHAR DedicatedSwitch[] = TEXT("DedicatedStation");
inline constexpr TCHAR CookPrefix[] = TEXT("Saved/Cooked/Windows/Voyage/Content/");
inline constexpr TCHAR GamePrefix[] = TEXT("/Game/");
inline constexpr TCHAR ActorTemplateForbidden[] = TEXT("Native CDO/component override is not allowed in this tagged prototype");
inline const FName Camera(TEXT("RailgunViewActor"));
inline const FName Controller(TEXT("RailgunViewController"));
inline const FName ViewOwned(TEXT("RailgunOwnsView"));
inline const FName Label(TEXT("RailgunSessionLabel"));
inline const FName ActionValue(TEXT("ActionValue"));
inline const FName Triggered(TEXT("Triggered"));
inline const FName Started(TEXT("Started"));
}
