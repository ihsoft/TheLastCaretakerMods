#pragma once
// Component/call mirror. Steam25956314 / UE5.8, EXE
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
// Only authored slot configuration and proved Blueprint APIs are exposed.
#include "VoyageBaseBarComponent.h"
#include "VoyageItemHandEquipment.h"
#include "VoyageWeaponHolsterTypes.h"
#include "VoyageWeaponHolsterComponent.generated.h"

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageHolsterSlotConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EVoyageHolsterSlotType Type = EVoyageHolsterSlotType::Any;
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageHolsterSlotOverride
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<UVoyageItemBaseEquipment> Item;
};

UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageWeaponHolsterComponent : public UVoyageBaseBarComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVoyageHolsterSlotConfig> HolsterSlots;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SkeletonSocketTag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EquipmentComponentTag;

    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    bool AddOverlayState(const FName& InKey,
        const FVoyageHolsterSlotOverride& InOverride, bool bForceAdd)
    {
        return false;
    }

    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    bool RemoveOverlayState(const FName& InKey) { return false; }

    UFUNCTION(BlueprintPure, Category="Voyage Mirror")
    AVoyageEquipmentActor* GetActiveSlotActor(bool bDucked) const
    {
        return nullptr;
    }
};
