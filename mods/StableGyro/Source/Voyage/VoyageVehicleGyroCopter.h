// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25956314, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
// Native owner/type and the readonly assistance flags were revalidated from
// current reflection metadata. The Tilt input mirrors intentionally remain
// writable for the generator and do not claim exact native access flags.
// Revalidate on fingerprint change. Editor-only; never ship.

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GyroCopter")
    bool bAutoHoverCollective = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GyroCopter")
    bool bAltitudeAssist = true;

protected:
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float CurrentThrottle = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    bool bEngineRunning = false;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float PropellerVelocity = 0.0f;
};
