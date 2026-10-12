// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25827816, UE 5.8.2
// (parser target 5.8),
// VoyageSteam-Win64-Shipping.exe SHA-256
// C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
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
