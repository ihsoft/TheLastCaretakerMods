// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 23962331
// (Unreal Engine 5.7.4; VoyageSteam-Win64-Shipping.exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D).
// Revalidate with ../../../../tools/Get-VoyageBuildFingerprint.ps1 and current
// mappings after a game update. Editor-only: never package this definition.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VoyageVehiclePawn.generated.h"

class AVoyagePlayerController;

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API AVoyageVehiclePawn : public APawn
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    void OnEnterVehicle(AVoyagePlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    void OnExitVehicle();
};
