// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25982825, UE 5.8.2
// (parser target 5.8).
// Current mapping metadata revalidated the exact native owner and property
// types used here. Access/SaveGame flags and defaults retain evidence from
// Steam build 25956314.
// The Tilt input mirrors intentionally remain writable for the generator and
// do not claim exact native access flags.
// Revalidate on Steam build ID change. Editor-only; never ship.

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
    float Collective = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float HoverPitchScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float CurrentThrottle = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    bool bEngineRunning = false;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float PropellerVelocity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "GyroCopter")
    float DropCargoTriggerTime = 1.0f;
};
