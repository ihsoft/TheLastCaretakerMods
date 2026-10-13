#pragma once
// HAND-RECONSTRUCTED EDITOR-ONLY MIRROR: Steam 25880952, UE 5.8.1,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44.
// Current mappings and BP_Inventory_PlayerBox bytecode establish that Name is
// an FText owned by /Script/Voyage.VoyageBaseInventoryComponent. Revalidate on
// fingerprint change. No native mirror binary is shipped.

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
