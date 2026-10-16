#pragma once
// Editor-only mirror; never shipped. Steam25956314, UE5.8, executable SHA256
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
// Reconstructed from native Voyage registration/thunks and cooked Blueprint
// bytecode. BP_CraftingLocationGenerator_Default calls GetActorWorld(AActor*)
// and then GetWaterHeightAtLocation(UWorld*, FVector), whose native return is
// float. BP_ToolAbility_Maintenance_Dismantle calls
// GetDestructibleInterface(UObject*) and consumes its UObject return.
// Revalidate owner, parameter order/types and return type on fingerprint change.
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyagePlayerController.h"
#include "VoyageMiscBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    static UObject* DuplicateObjectVoyage(UObject* Obj, UObject* Outer)
    {
        return nullptr;
    }

    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    static FVector2D CalculateCameraMovementValue(
        AVoyagePlayerController* Controller, const FVector2D& InputValue)
    {
        return FVector2D::ZeroVector;
    }

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
