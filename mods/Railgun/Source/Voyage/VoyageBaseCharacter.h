#pragma once
// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// executable SHA-256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Reviewed mapping confirms WeightInventory is an own ObjectProperty of
// VoyageBaseCharacter. Editor-only: never package this partial definition.

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VoyageBaseInventoryComponent.h"
#include "VoyageBaseCharacter.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageBaseCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UVoyageBaseInventoryComponent> WeightInventory;
};
