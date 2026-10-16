// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25929985, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// 608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F.
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
