// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25880952, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// 16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44.
// Class, property owner, type, and access flags were revalidated from current
// native reflection metadata. Revalidate on fingerprint change. Editor-only;
// never ship.

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GyroCopter")
    float TiltInput = 0.0f;

protected:
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float PropellerVelocity = 0.0f;
};
