#pragma once
#include "CoreMinimal.h"
namespace DedicatedStationNames
{
inline constexpr TCHAR OperatorPackage[] = TEXT("/Game/Mods/HarpoonCannon/Station/BP_HarpoonOperator");
inline constexpr TCHAR HudPackage[] = TEXT("/Game/Mods/HarpoonCannon/Station/WBP_HarpoonHUD");
inline constexpr TCHAR Title[] = TEXT("HC33 HARPOON / ALL HIT TARGETS / MOUSE 80%");
inline constexpr TCHAR Ready[] = TEXT("HC33: preparing built cannons; aim at base. Do not save.");
inline constexpr TCHAR Prepared[] = TEXT("HC28 PREPARED: F8 enters; check native E hint. Do not save.");
inline constexpr TCHAR Active[] = TEXT("HC28: check Exit Harpoon key card; E exits, F8 fallback (max20s).");
inline constexpr TCHAR HudLabel[] = TEXT("x5");
inline constexpr TCHAR HudObserved[] = TEXT("Own HUD override callback observed: YES");
inline constexpr TCHAR HudNotObserved[] = TEXT("Own HUD override callback observed: NO");
inline constexpr TCHAR ContextPath[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/DA_HarpoonInputContext.DA_HarpoonInputContext");
inline constexpr TCHAR EyeOffset[] = TEXT("170,0,60");
inline constexpr TCHAR MouseScale[] = TEXT("1.024");
inline constexpr float ExitLabelOffsetY = 130.0f;
inline constexpr float TargetNameOffsetY = 72.0f;
inline constexpr float TargetRangeOffsetY = 100.0f;
inline constexpr float HintStatusOffsetY = 175.0f;
inline const FVector2D HintHostOffset(24.0f, -24.0f);
inline constexpr TCHAR VerifySwitch[] = TEXT("VerifyTagged");
inline constexpr TCHAR DedicatedSwitch[] = TEXT("DedicatedStation");
inline constexpr TCHAR CookPrefix[] = TEXT("Saved/Cooked/Windows/Voyage/Content/");
inline constexpr TCHAR GamePrefix[] = TEXT("/Game/");
inline constexpr TCHAR ActorTemplateForbidden[] = TEXT("Native CDO/component override is not allowed in this tagged prototype");
inline const FName Camera(TEXT("HarpoonViewActor"));
inline const FName Controller(TEXT("HarpoonViewController"));
inline const FName ViewOwned(TEXT("HarpoonOwnsView"));
inline const FName Label(TEXT("HarpoonSessionLabel"));
inline const FName ActionValue(TEXT("ActionValue"));
inline const FName Triggered(TEXT("Triggered"));
inline const FName Started(TEXT("Started"));
}
