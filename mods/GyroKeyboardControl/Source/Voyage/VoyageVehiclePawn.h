// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25191271, UE 5.8,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Parent identity revalidated from BP_GyroCopter_Possessable on 2026-09-20.
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
