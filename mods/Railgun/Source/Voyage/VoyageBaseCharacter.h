#pragma once
// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
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
