#pragma once
#include "CoreMinimal.h"

// HC13 engine-only emission uses the UE5.8 parser target.
// See ../../ARCHITECTURE.md for its engine contract.

// Mod-owned experiment tuning. Game identity revalidated using public stock
// JSON on Steam 25056839 / UE5.8.1.
// Renew the generated runtime contract on Steam build ID change. No Voyage mirror.
namespace RailgunRuntimeNames
{
inline const FName HudTree(TEXT("WidgetTree"));
inline const FName HudCanvas(TEXT("DiagnosticCanvas"));
inline const FName OriginalPawn(TEXT("OriginalPlayerPawn"));
inline constexpr TCHAR UnitScale[] = TEXT("1,1,1");
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR False[] = TEXT("false");
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR EmptyText[] = TEXT("");
inline constexpr TCHAR VisibilityTrace[] = TEXT("TraceTypeQuery1");
inline constexpr TCHAR KeepWorld[] = TEXT("KeepWorld");
inline constexpr TCHAR AlwaysSpawn[] = TEXT("AlwaysSpawn");
}
