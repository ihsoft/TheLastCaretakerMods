// READ-ONLY editor mirror: Steam 25191271 / UE5.8, executable SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: reviewed mappings + stock BP_DynamicMeshActor + native Loot provider.
// Revalidate on fingerprint change. Never spawn or ship this native mirror.
#pragma once

#include "GameFramework/Actor.h"
#include "VoyageItem.h"
#include "VoyageBaseInventoryComponent.h"
#include "VoyageDynamicMeshActor.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageDynamicMeshActor : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UVoyageBaseInventoryComponent> InternalInventory;
    UPROPERTY(BlueprintReadOnly) TMap<TSoftObjectPtr<UVoyageItem>, int32> Items;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UVoyageItem> CurrentVisualizeItem;
    UPROPERTY(BlueprintReadOnly) bool bInventorySupported = false;
    UPROPERTY(BlueprintReadOnly) bool bSimulatePhysics = false;
};
