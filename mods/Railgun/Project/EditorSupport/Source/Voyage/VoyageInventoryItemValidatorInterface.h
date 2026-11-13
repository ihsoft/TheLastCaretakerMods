#pragma once
// Exact one-function interface mirror from Steam25191271 / UE5.8 mappings and
// the Diesel Refinery Blueprint. Editor-only; the game owns runtime dispatch.

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoyageInventoryItemValidatorInterface.generated.h"

class UVoyageBaseInventoryComponent;
class UVoyageItem;

UINTERFACE(BlueprintType)
class VOYAGE_API UVoyageInventoryItemValidatorInterface : public UInterface
{
    GENERATED_BODY()
};

class VOYAGE_API IVoyageInventoryItemValidatorInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Inventory")
    void ValidateItem(UVoyageBaseInventoryComponent* Inventory, UVoyageItem* Item,
        bool& bIsValid);
};
