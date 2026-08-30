// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 23962331
// (Unreal Engine 5.7.4; VoyageSteam-Win64-Shipping.exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D).
// Recreate/revalidate with tools/Get-VoyageBuildFingerprint.ps1,
// tools/Inspect-VoyageAsset.ps1, and tools/VoyageExecutableInspector after a
// game update. Editor-only: never package this native definition.

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
