// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25827816, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
// Consumed action identities and Description ownership were revalidated from
// current native reflection metadata. Revalidate on fingerprint change.
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
