// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 25056839
// (Unreal Engine 5.8.1; VoyageSteam-Win64-Shipping.exe SHA-256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933).
// Recreate/revalidate with tools/Get-VoyageBuildFingerprint.ps1,
// tools/Inspect-VoyageAsset.ps1, and tools/VoyageExecutableInspector after a
// game update. Editor-only: never package this native definition.

#pragma once

#include "CoreMinimal.h"
#include "VoyageVehiclePawn.h"
#include "VoyageVehicleForkliftPawn.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageVehicleForkliftPawn : public AVoyageVehiclePawn
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forklift")
    float SteeringInput = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forklift")
    float ThrottleInput = 0.0f;
};
