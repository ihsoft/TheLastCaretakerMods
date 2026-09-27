// READ-ONLY editor mirror: Steam 25191271 / UE5.8, executable SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: reviewed InteractiveDetectorPointerComponent mapping and Loot guard.
// Revalidate on fingerprint change. Never instantiate or ship this mirror.
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
