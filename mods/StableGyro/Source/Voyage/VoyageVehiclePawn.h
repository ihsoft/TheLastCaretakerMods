// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25827816, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
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
