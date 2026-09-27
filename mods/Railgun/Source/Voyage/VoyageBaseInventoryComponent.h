// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// executable SHA-256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: reviewed Items mapping plus native UHT AddItem/CanAddItem records
// (AddItem params at 0x14991ABA0) and the shipping AddItem implementation.
// Revalidate on fingerprint change. Editor-only: never package this definition.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VoyageFabricatorComponent.h"
#include "VoyageBaseInventoryComponent.generated.h"

// Read-only Items field revalidated against the Steam 25191271 mapping.
// The game owns this component; no mirror instance is created or serialized.
UCLASS(BlueprintType, ClassGroup = (Voyage))
class VOYAGE_API UVoyageBaseInventoryComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TMap<int32, FVoyageItemSerialize> Items;

    // Editor signatures only. Cooked calls dispatch to the game's functions;
    // neither this stub implementation nor a mirror instance is shipped.
    UFUNCTION(BlueprintCallable, Category="RailgunProbe")
    int32 AddItem(UVoyageItem* NewItem, const FVoyageItemData& InItemData,
        bool bAllowStacking = true, bool bNotifyChanged = true) { return 0; }

    UFUNCTION(BlueprintPure, Category="RailgunProbe")
    bool CanAddItem(UVoyageItem* NewItem) const { return false; }
};
