#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
bool SaveDedicatedAsset(UObject* Asset);

// Shared stock-system contract for production impact VFX and manual canaries.
namespace RailgunVfx
{
inline constexpr TCHAR SeaMinePath[] =
    TEXT("/Game/VFX/Environment/Interactive/NS_Explosion_SeaMine.NS_Explosion_SeaMine");

inline const FName ShockwaveEmitter(TEXT("shockwave"));
inline const FName MainEmitter(TEXT("main"));
inline const FName SparkEmitter(TEXT("spark"));
inline const FName LargeSparkEmitter(TEXT("spark_l"));
inline const FName RefractionEmitter(TEXT("refr"));
inline const FName ProjectileEmitter(TEXT("project"));
inline const FName PuffEmitter(TEXT("puff"));
inline const FName DirtMainEmitter(TEXT("dirt_main"));

inline const FName EmitterName(TEXT("EmitterName"));
inline const FName EmitterEnabled(TEXT("bNewEnableState"));
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR False[] = TEXT("false");
}

void SetRailgunVfxEmitterEnabled(FGraph& G, UEdGraphPin* Component,
    FName EmitterName, bool Enabled);

void ConfigureRailgunSeaMineEmitters(FGraph& G, UEdGraphPin* Component,
    bool DirtMainOnly);

namespace RailgunImpactVfx
{
inline constexpr TCHAR Package[] =
    TEXT("/Game/Mods/Railgun/Station/BP_RailgunTransientVfx");
inline constexpr TCHAR MaximumLifetimeSeconds[] = TEXT("10.0");
inline const FName Component(TEXT("RailgunImpactVfxComponent"));
inline const FName LifeSpan(TEXT("InLifespan"));
inline const FName Asset(TEXT("InAsset"));
inline const FName ResetOverrides(TEXT("bResetExistingOverrideParameters"));
inline const FName AutoDestroyEnabled(TEXT("bInAutoDestroy"));
inline const FName Reset(TEXT("bReset"));
inline const FName DirtMainOnly(TEXT("DirtMainOnly"));
inline UClass* Class = nullptr;
}

UClass* CreateRailgunImpactVfx();

void SpawnRailgunVfx(FGraph& G, UEdGraphPin* Location, bool DirtMainOnly);

void SpawnRailgunImpactVfx(FGraph& G, UEdGraphPin* HitResult);
}
