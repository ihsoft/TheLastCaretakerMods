#pragma once

// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// Exact owner, no-argument FText return and interface dispatch are confirmed by
// BP_ToolAbility_Maintenance_Base.AddDefaultTextFields cooked bytecode. This is
// editor-only and must be revalidated when the Steam build ID changes.

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoyageItemInterface.generated.h"

UINTERFACE(BlueprintType)
class VOYAGE_API UVoyageItemInterface : public UInterface
{
    GENERATED_BODY()
};

class VOYAGE_API IVoyageItemInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Item")
    FText GetItemName() const;
};
