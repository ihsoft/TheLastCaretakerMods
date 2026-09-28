// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// executable SHA-256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: reviewed mapping for the authored container configuration fields.
// Revalidate on fingerprint change. Editor-only: never package this definition.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VoyageFabricatorComponent.h"
#include "VoyageBaseInventoryComponent.generated.h"

UENUM()
enum class EVoyageInventoryType : uint8
{
    Undefined = 0,
    Container = 1
};

UENUM()
enum class EVoyageInventoryAccessType : uint8
{
    None = 0,
    ReadWrite = 1
};

// The game owns the runtime implementation; only authored configuration fields
// used by the generated Railgun component are mirrored here.
UCLASS(BlueprintType, ClassGroup = (Voyage))
class VOYAGE_API UVoyageBaseInventoryComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY() EVoyageInventoryType Type = EVoyageInventoryType::Undefined;
    UPROPERTY() TArray<EVoyageItemCategory> AcceptedItemCategories;
    UPROPERTY() EVoyageInventoryAccessType Access = EVoyageInventoryAccessType::None;
    // Sentinel defaults force explicit false tags for the authored component.
    UPROPERTY() bool bAllowFiltering = true;
    UPROPERTY() TSet<TObjectPtr<UVoyageItemCategoryAsset>> DepositAllCategoryFilter;
    UPROPERTY() bool bAllowNearbyQueries = true;
    UPROPERTY() bool bAutoCloseHudWhenEmpty = true;
};
