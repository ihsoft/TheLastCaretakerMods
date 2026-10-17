// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25929985, UE 5.8.2
// (parser target 5.8).
// Consumed action identities and Description ownership were revalidated from
// current native reflection metadata. Revalidate on Steam build ID change.
// Editor-only; never ship.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "VoyageInputAction.generated.h"

class UPlayerMappableKeySettings;

UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputAction : public UInputAction
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FText Description;

#if WITH_EDITOR
    void SetPlayerMappableKeySettings(UPlayerMappableKeySettings* Settings)
    {
        PlayerMappableKeySettings = Settings;
    }
#endif
};
