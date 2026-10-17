// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 25056839
// (Unreal Engine 5.8.1).
// Recreate/revalidate with tools/Get-VoyageBuildFingerprint.ps1,
// tools/Inspect-VoyageAsset.ps1, and tools/VoyageExecutableInspector after a
// game update. Editor-only: never package this native definition.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "VoyageGameUserSettings.generated.h"

// Only the language fields required by the generated Blueprint are mirrored.
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

// Never package this class: the shipping game supplies the real native class.
UCLASS(BlueprintType)
class VOYAGE_API UVoyageGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    FVoyageCustomGameUserSettings CustomSettings;
};
