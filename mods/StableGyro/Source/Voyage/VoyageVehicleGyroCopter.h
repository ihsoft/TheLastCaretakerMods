// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25982825, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// 08228B72D4590F9B00E2171F252D618464C2E04B785237FE728DDDBF27D3B734.
// Current mapping metadata revalidated the exact native owner and property
// types used here. Access/SaveGame flags and defaults retain evidence from
// Steam build 25956314 / executable 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
// The Tilt input mirrors intentionally remain writable for the generator and
// do not claim exact native access flags.
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
    float HoverPitchScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float CurrentThrottle = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    bool bEngineRunning = false;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "GyroCopter")
    float PropellerVelocity = 0.0f;
};
