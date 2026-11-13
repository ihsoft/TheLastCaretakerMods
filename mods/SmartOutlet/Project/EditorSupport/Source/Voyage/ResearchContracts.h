// Editor-only member access mirrors, Steam 25982825 / UE5.8.
// Mutate only named fields of the live Unlock reference; never replace the struct.
#pragma once
#include "Engine/DataAsset.h"
#include "ResearchContracts.generated.h"

UENUM(BlueprintType)
enum class EVoyageSkillUnlockMethod : uint8 { Unknown = 0, Level = 1, Tier = 2, Never = 3 };

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageSkillUnlock
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") EVoyageSkillUnlockMethod UnlockMethod = EVoyageSkillUnlockMethod::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") TObjectPtr<class UVoyageSkill> ParentSkill;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") int32 Cost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") int32 Requirement = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") TArray<TObjectPtr<UObject>> CustomRequirements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") TObjectPtr<UObject> QuestOnUnlock;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageSkill : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Research") FVoyageSkillUnlock Unlock;
};
