// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25929985, UE 5.8.2
// (parser target 5.8).
// Declaring owner and function signature were revalidated from current native
// reflection metadata.
// Revalidate after a Steam build ID change. Editor-only; never ship.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerInputInterfaceAction.h"
#include "VoyageVehiclePawn.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageVehiclePawn : public APawn
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input")
    TArray<FPlayerInputInterfaceAction> GetProvidedActionsBP();
};
