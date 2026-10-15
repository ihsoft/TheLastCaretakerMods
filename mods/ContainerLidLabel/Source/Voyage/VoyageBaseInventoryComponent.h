#pragma once
// HAND-RECONSTRUCTED EDITOR-ONLY MIRROR: Steam 25929985, UE 5.8.2,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F.
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
