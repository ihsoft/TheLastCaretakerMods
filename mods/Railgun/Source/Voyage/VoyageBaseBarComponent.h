#pragma once
// Field-only mirror. Steam25956314 / UE5.8, EXE
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
#include "VoyageInventoryWeightLimitedComponent.h"
#include "VoyageBaseBarComponent.generated.h"

UENUM(BlueprintType)
enum class EBarSlotType : uint8
{
    Active = 0,
    Activation = 1
};

USTRUCT(BlueprintType)
struct VOYAGE_API FBarSlotConfig
{
    GENERATED_BODY()
    UPROPERTY() EBarSlotType Type = EBarSlotType::Active;
    UPROPERTY() bool bReadOnly = false;
    UPROPERTY() bool bHidden = false;
    UPROPERTY() TSoftObjectPtr<UObject> SlotIcon;
    UPROPERTY() int32 OverrideSlotIndex = -1;
    UPROPERTY() TArray<FString> WhitelistedPrimaryAssetTypes;
    UPROPERTY() TArray<FString> BlacklistedPrimaryAssetTypes;
};

UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageBaseBarComponent
    : public UVoyageInventoryWeightLimitedComponent
{
    GENERATED_BODY()
public:
    UPROPERTY() TArray<FBarSlotConfig> Slots;
};
