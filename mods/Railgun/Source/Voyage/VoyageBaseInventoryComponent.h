// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25827816 / UE5.8 parser target;
// executable SHA-256 C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
// Method: current native metadata for the authored container configuration
// fields, read-only Items map, enums and consumed function signatures.
// Revalidate on fingerprint change. Editor-only: never package this definition.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VoyageFabricatorComponent.h"
#include "VoyageBaseInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryDelegate);

UENUM()
enum class EVoyageInventoryType : uint8
{
    Undefined = 0,
    PlayerInventory = 1,
    PlayerVault = 2,
    Merchant_DEPRECATEAD = 3,
    Container = 4,
    Hidden = 5
};

UENUM()
enum class EVoyageInventoryAccessType : uint8
{
    Read = 0,
    Write = 1,
    ReadWrite = 2
};

// The game owns the runtime implementation; only authored configuration fields
// used by the generated Railgun component are mirrored here.
UCLASS(BlueprintType, ClassGroup = (Voyage))
class VOYAGE_API UVoyageBaseInventoryComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    // Read-only Steam 25827816 runtime contents. The generated graph reads the
    // map to drive presentation; it never mutates or serializes this field.
    UPROPERTY(BlueprintReadOnly) TMap<int32, FVoyageItemSerialize> Items;
    UPROPERTY(BlueprintAssignable) FInventoryDelegate OnInventoryChanged;
    UPROPERTY() EVoyageInventoryType Type = EVoyageInventoryType::Undefined;
    UPROPERTY() TArray<EVoyageItemCategory> AcceptedItemCategories;
    UPROPERTY() EVoyageInventoryAccessType Access = EVoyageInventoryAccessType::Read;
    // Sentinel defaults force explicit false tags for the authored component.
    UPROPERTY() bool bAllowFiltering = true;
    UPROPERTY() TSet<TObjectPtr<UVoyageItemCategoryAsset>> DepositAllCategoryFilter;
    UPROPERTY() bool bAllowNearbyQueries = true;
    UPROPERTY() bool bAutoCloseHudWhenEmpty = true;

    // Steam25827816 native inventory access used by the generated fire graph.
    // RemoveItem returns the number of items actually removed.
    UFUNCTION(BlueprintPure, Category="RailgunInventory")
    int32 GetLastOccupiedSlot() const { return -1; }

    UFUNCTION(BlueprintPure, Category="RailgunInventory")
    bool GetSlot(int32 SlotIndex, FVoyageItemSerialize& OutItemData) const
    {
        return false;
    }

    UFUNCTION(BlueprintCallable, Category="RailgunInventory")
    int32 RemoveItem(UVoyageItem* RemovedItem, int32 RemoveCount,
        int32 DesiredSlot, bool bNotifyChanged)
    {
        return 0;
    }

    // Steam25827816 native slot transaction used by manual reload. The
    // destination is the receiver; SourceInventory and SourceSlot identify the
    // live source entry, TargetSlot=-1 requests native autoplacement, and the
    // return value is the amount actually moved.
    UFUNCTION(BlueprintCallable, Category="RailgunInventory")
    int32 TransferSlot(UVoyageBaseInventoryComponent* Source,
        int32 SourceSlot, int32 TargetSlot, int32 TransferAmount)
    {
        return 0;
    }

    UFUNCTION(BlueprintCallable, Category="RailgunInventory")
    void FindSlotsByItem(UVoyageItem* Item, TArray<int32>& OutSlotIndices) const
    {
    }
};
