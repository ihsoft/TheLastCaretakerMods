// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25191271, UE 5.8,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Editor-only; never ship.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "VoyageGameUserSettings.generated.h"

UENUM(BlueprintType)
enum class EVoyageLanguageType : uint8
{
    System = 0,
    English = 1,
    Russian = 11
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageCustomGameUserSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EVoyageLanguageType LanguageType = EVoyageLanguageType::System;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    FVoyageCustomGameUserSettings CustomSettings;
};
