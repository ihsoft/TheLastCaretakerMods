#pragma once
// Editor-only mirror of /Script/Voyage.VoyageSkill for Steam 25191271,
// executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
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

    UPROPERTY() EVoyageSkillUnlockMethod UnlockMethod = EVoyageSkillUnlockMethod::Unknown;
    UPROPERTY() TObjectPtr<class UVoyageSkill> ParentSkill;
    UPROPERTY() int32 Cost = 0;
    UPROPERTY() int32 Requirement = 0;
    UPROPERTY() TArray<TObjectPtr<UObject>> CustomRequirements;
    UPROPERTY() TObjectPtr<UObject> QuestOnUnlock;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageSkill : public UVoyageBaseDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY() FVoyageSkillUnlock Unlock;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<UVoyageItem>> Items;
    UPROPERTY() TObjectPtr<UTexture2D> Icon;
};
