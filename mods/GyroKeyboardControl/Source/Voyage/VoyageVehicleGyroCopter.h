// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25191271, UE 5.8,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Class and float field identity revalidated from current cooked Gyro bytecode
// on 2026-09-20. Revalidate on fingerprint change. Editor-only; never ship.

#pragma once

#include "CoreMinimal.h"
#include "VoyageVehiclePawn.h"
#include "VoyageVehicleGyroCopter.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageVehicleGyroCopter : public AVoyageVehiclePawn
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GyroCopter")
    float TiltForwardInput = 0.0f;

};
