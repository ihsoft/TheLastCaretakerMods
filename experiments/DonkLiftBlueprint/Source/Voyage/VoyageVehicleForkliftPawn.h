#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VoyageVehicleForkliftPawn.generated.h"

// Editor-only stand-in for /Script/Voyage.VoyageVehicleForkliftPawn.
// Never package this class with the mod: the game supplies the real class.
UCLASS(BlueprintType)
class VOYAGE_API AVoyageVehicleForkliftPawn : public APawn
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forklift")
    float SteeringInput = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forklift")
    float ThrottleInput = 0.0f;
};
