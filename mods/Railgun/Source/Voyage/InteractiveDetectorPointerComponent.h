// READ-ONLY editor mirror: Steam 25191271 / UE5.8.
// Method: reviewed InteractiveDetectorPointerComponent mapping and Loot guard.
// Revalidate on Steam build ID change. Never instantiate or ship this mirror.
#pragma once

#include "Components/SceneComponent.h"
#include "InteractiveDetectorPointerComponent.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API UInteractiveDetectorPointerComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) bool bAllowInteract = false;
    UPROPERTY(BlueprintReadOnly) bool bAllowLoot = false;
};
