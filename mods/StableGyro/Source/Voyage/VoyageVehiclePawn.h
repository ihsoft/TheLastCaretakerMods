// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25880952, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// 16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44.
// Declaring owner and function signature were revalidated from current native
// reflection metadata.
// Revalidate after any executable fingerprint change. Editor-only; never ship.

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
