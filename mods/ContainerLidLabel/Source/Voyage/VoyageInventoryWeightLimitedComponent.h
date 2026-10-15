#pragma once
// HAND-RECONSTRUCTED EDITOR-ONLY MIRROR: Steam 25929985, UE 5.8.2,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F.
// Reviewed mappings and current stock Blueprint calls establish the exact native
// superclass and methods below. Revalidate on fingerprint change. No native
// mirror binary is shipped.

#include "VoyageBaseInventoryComponent.h"
#include "VoyageInventoryWeightLimitedComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageInventoryWeightLimitedComponent : public UVoyageBaseInventoryComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly)
    float MaxWeightLimit = 0.0f;

    UFUNCTION(BlueprintCallable, Category="ContainerLidLabel")
    void SetMaxWeightLimit(float NewDefaultMax) {}

    UFUNCTION(BlueprintPure, Category="ContainerLidLabel")
    float GetPercentWeightUsed() const { return 0.0f; }
};
