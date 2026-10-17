#pragma once
// Field-only mirror. Steam25956314 / UE5.8.
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
