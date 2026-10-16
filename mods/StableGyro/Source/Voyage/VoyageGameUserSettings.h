// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25929985, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// 608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F.
// Consumed enum values and CustomSettings/LanguageType identities were
// revalidated from current native reflection metadata. Revalidate on fingerprint
// change. Editor-only; never ship.

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
