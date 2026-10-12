#pragma once
// SHELL-ONLY mirror: Steam25827816 / UE5.8, executable
// C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
// Current native metadata and retained authored defaults. The generated Blueprint
// serializes only the named tagged fields; no mirror binary is shipped.

#include "VoyageBaseInventoryComponent.h"
#include "VoyageInventoryWeightLimitedComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageInventoryWeightLimitedComponent : public UVoyageBaseInventoryComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) float MaxWeightLimit = 0.0f;
    // Sentinel default forces the authored false value into tagged output.
    UPROPERTY() bool bAllowBeyondWeightLimit = true;

    UFUNCTION(BlueprintCallable, Category="RailgunInventory")
    void SetMaxWeightLimit(float NewDefaultMax) {}
};
