// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 25056839
// (Unreal Engine 5.8.1).
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
