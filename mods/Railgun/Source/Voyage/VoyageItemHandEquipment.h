#pragma once
// Tagged-data mirror. Steam25956314 / UE5.8, EXE
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
// EquipmentClass and DesiredSlot are authored for the Railgun ADS service item.
#include "VoyageItem.h"
#include "VoyageEquipmentActor.h"
#include "VoyageWeaponHolsterTypes.h"
#include "VoyageItemHandEquipment.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemBaseEquipment : public UVoyageItem
{
    GENERATED_BODY()
public:
    UPROPERTY() TSoftClassPtr<AVoyageEquipmentActor> EquipmentClass;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemHandEquipment : public UVoyageItemBaseEquipment
{
    GENERATED_BODY()
public:
    UPROPERTY() EVoyageHolsterSlotType DesiredSlot =
        EVoyageHolsterSlotType::Any;
};
