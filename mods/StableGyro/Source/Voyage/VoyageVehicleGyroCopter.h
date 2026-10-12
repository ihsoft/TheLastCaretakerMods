// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25827816, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
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
    float CurrentThrottle = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float PropellerVelocity = 0.0f;
};
