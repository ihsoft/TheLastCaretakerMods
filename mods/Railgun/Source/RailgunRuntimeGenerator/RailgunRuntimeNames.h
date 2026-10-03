#pragma once
#include "CoreMinimal.h"

// HC13 Engine-only emission renewed for Steam25191271 / UE5.8 parser target,
// executable747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// See GAME_DERIVED_SOURCES.md.

// Mod-owned experiment tuning. Game identity revalidated using public stock
// JSON on Steam 25056839 / UE5.8.1, exe CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Renew the class and loader contract on fingerprint change. No Voyage mirror.
namespace RailgunRuntimeNames
{
inline constexpr TCHAR Package[] = TEXT("/Game/Mods/Railgun/Runtime/BP_RailgunCoordinator");
inline constexpr TCHAR Asset[] = TEXT("BP_RailgunCoordinator");
inline const FName HudInstance(TEXT("DiagnosticHUD"));
inline const FName HudTree(TEXT("WidgetTree"));
inline const FName HudCanvas(TEXT("DiagnosticCanvas"));
inline const FName Root(TEXT("ProbeRoot"));
inline const FName OriginalPawn(TEXT("OriginalPlayerPawn"));
inline const FName FreezeStatus(TEXT("FreezeStatusText"));
inline constexpr TCHAR UnitScale[] = TEXT("1,1,1");
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR False[] = TEXT("false");
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR EmptyText[] = TEXT("");
inline constexpr TCHAR VisibilityTrace[] = TEXT("TraceTypeQuery1");
inline constexpr TCHAR KeepWorld[] = TEXT("KeepWorld");
inline constexpr TCHAR AlwaysSpawn[] = TEXT("AlwaysSpawn");
}
