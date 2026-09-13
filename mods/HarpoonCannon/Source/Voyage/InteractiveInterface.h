// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 23962331
// (Unreal Engine 5.7.4; VoyageSteam-Win64-Shipping.exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D).
// Revalidate with ../../../../tools/Get-VoyageBuildFingerprint.ps1 and current
// mappings after a game update. Editor-only: never package this definition.

#pragma once

#include "CoreMinimal.h"
#include "PlayerInputInterfaceAction.h"
#include "UObject/Interface.h"
#include "VoyageBaseInventoryComponent.h"
#include "InteractiveInterface.generated.h"

UENUM(BlueprintType)
enum class EInteractWithGrabReturnValue : uint8
{
    Invalid = 0,
    No = 1,
    Yes = 2,
    YesNoDelay = 3
};

UINTERFACE(BlueprintType)
class VOYAGE_API UInteractiveInterface : public UInterface
{
    GENERATED_BODY()
};

class VOYAGE_API IInteractiveInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool GetInteractiveProvidedActions(
        APawn* MyCharacter,
        USceneComponent* Component,
        TArray<FPlayerInputInterfaceAction>& OutActions);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool GetInteractiveProvidedGrabActions(
        APawn* MyCharacter,
        USceneComponent* Component,
        TArray<FPlayerInputInterfaceAction>& OutActions);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void InteractAction(APawn* MyCharacter, int32 PartId, int32 InteractIndex);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool InteractActionPartial(APawn* MyCharacter, int32 PartId, int32 InteractIndex);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    UVoyageBaseInventoryComponent* InteractGetInventory(APawn* MyCharacter, int32 PartId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void InteractWithGrab(
        APawn* MyCharacter,
        int32 PartId,
        EInteractWithGrabReturnValue& OutValue);
};
