#pragma once
// Editor-only mirror of /Script/Voyage.VoyageSkill for Steam 25191271,
// Only tagged properties used by the Railgun skill are declared here.
// Revalidate against the reviewed mapping and stock skill on a game update.

#include "VoyageBaseDataAsset.h"
#include "Engine/Texture2D.h"
#include "VoyageSkill.generated.h"

class UVoyageItem;

UENUM()
enum class EVoyageSkillUnlockMethod : uint8
{
    Unknown = 0,
    Level = 1,
    Tier = 2,
    Never = 3
};

USTRUCT()
struct VOYAGE_API FVoyageSkillUnlock
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="Voyage") EVoyageSkillUnlockMethod UnlockMethod = EVoyageSkillUnlockMethod::Unknown;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<class UVoyageSkill> ParentSkill;
    UPROPERTY(EditAnywhere, Category="Voyage") int32 Cost = 0;
    UPROPERTY(EditAnywhere, Category="Voyage") int32 Requirement = 0;
    UPROPERTY(EditAnywhere, Category="Voyage") TArray<TObjectPtr<UObject>> CustomRequirements;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UObject> QuestOnUnlock;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageSkill : public UVoyageBaseDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="Voyage") FVoyageSkillUnlock Unlock;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voyage") TArray<TObjectPtr<UVoyageItem>> Items;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UTexture2D> Icon;
};
