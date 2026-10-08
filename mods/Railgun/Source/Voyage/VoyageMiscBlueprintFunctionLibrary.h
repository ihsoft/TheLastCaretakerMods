#pragma once
// Editor-only mirror; never shipped. Steam25191271, UE5.8, executable SHA256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Reconstructed from native Voyage registration/thunks and cooked Blueprint
// bytecode. BP_CraftingLocationGenerator_Default calls GetActorWorld(AActor*)
// and then GetWaterHeightAtLocation(UWorld*, FVector), whose native return is
// float. BP_ToolAbility_Maintenance_Dismantle calls
// GetDestructibleInterface(UObject*) and consumes its UObject return.
// Revalidate owner, parameter order/types and return type on fingerprint change.
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageMiscBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Voyage Mirror")
    static UWorld* GetActorWorld(AActor* Actor) { return nullptr; }

    UFUNCTION(BlueprintPure, Category="Voyage Mirror")
    static float GetWaterHeightAtLocation(UWorld* World, FVector Loc)
    {
        return -100000.0f;
    }

    UFUNCTION(BlueprintPure, Category="Voyage Mirror")
    static UObject* GetDestructibleInterface(UObject* Object)
    {
        return nullptr;
    }
};
