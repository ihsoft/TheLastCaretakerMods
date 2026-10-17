// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 25056839
// (Unreal Engine 5.8.1).
// Recreate/revalidate with tools/Get-VoyageBuildFingerprint.ps1,
// tools/Inspect-VoyageAsset.ps1, and tools/VoyageExecutableInspector after a
// game update. Editor-only: never package this native definition.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerInputInterfaceAction.h"
#include "VoyageVehiclePawn.generated.h"

// The declaring class is part of the cooked Blueprint override identity, so
// this base must exist separately from VoyageVehicleForkliftPawn.
UCLASS(BlueprintType)
class VOYAGE_API AVoyageVehiclePawn : public APawn
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input")
    TArray<FPlayerInputInterfaceAction> GetProvidedActionsBP();
};
