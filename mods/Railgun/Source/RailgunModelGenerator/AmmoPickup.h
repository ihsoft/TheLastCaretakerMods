#pragma once

#include "CoreMinimal.h"

class AActor;
class FJsonObject;

namespace RailgunAmmoPickup
{
inline constexpr TCHAR StockParentPackageName[] =
    TEXT("/Game/Blueprints/BP_DynamicMeshActor");
inline constexpr TCHAR StockParentAssetName[] = TEXT("BP_DynamicMeshActor");
inline constexpr TCHAR StockParentClassName[] = TEXT("BP_DynamicMeshActor_C");
inline constexpr TCHAR PackageName[] =
    TEXT("/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette");
inline constexpr TCHAR AssetName[] = TEXT("BP_RailgunAmmoCassette");
inline constexpr TCHAR ClassObjectPath[] =
    TEXT("/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette.BP_RailgunAmmoCassette_C");

bool Generate(const TMap<FString, AActor*>& Actors, const FString& RootName,
    const FString& CarrierName, TSharedPtr<FJsonObject>& OutEvidence);
}
