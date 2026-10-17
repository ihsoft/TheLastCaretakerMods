#pragma once
// HAND-RECONSTRUCTED EDITOR-ONLY MIRROR: Steam 25929985, UE 5.8.2.
// Current mappings and BP_Inventory_PlayerBox bytecode establish that Name is
// an FText owned by /Script/Voyage.VoyageBaseInventoryComponent. Revalidate on
// Steam build ID change. No native mirror binary is shipped.

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VoyageBaseInventoryComponent.generated.h"

UCLASS(BlueprintType, ClassGroup=(Voyage))
class VOYAGE_API UVoyageBaseInventoryComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly)
    FText Name;
};
