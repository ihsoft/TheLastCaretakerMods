#pragma once

// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// executable SHA-256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Exact owner, no-argument FText return and interface dispatch are confirmed by
// BP_ToolAbility_Maintenance_Base.AddDefaultTextFields cooked bytecode. This is
// editor-only and must be revalidated when the game fingerprint changes.

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
