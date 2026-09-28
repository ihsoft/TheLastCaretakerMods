#pragma once
// SHELL-ONLY mirror: Steam25191271 / UE5.8, executable
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Reviewed mapping and Diesel Refinery defaults. The generated Blueprint
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
    void SetMaxWeightLimit(float NewMaxWeightLimit) {}
};
