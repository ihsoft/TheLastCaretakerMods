// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 24990438
// (Unreal Engine 5.8.1; VoyageSteam-Win64-Shipping.exe SHA-256
// D9BF4C9624C60615198E62C87DA7792A9888AB02F7905AAAF1C9B02C7A9E524F).
// Exact class identity, parent, and six-property unversioned prefix are
// current-build inspection results. PetrolTB is a legacy UI property name; it
// does not mean that petrol is the boat's engine resource. Editor-only: never
// package this definition.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "VoyageInGameVehicleWidget.h"
#include "VoyageInGameBoatWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageInGameBoatWidget : public UVoyageInGameVehicleWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Voyage")
    TObjectPtr<UOverlay> AnchorOL = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Voyage")
    TObjectPtr<UWidgetAnimation> OnAnchorAnim = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Voyage")
    TObjectPtr<UTextBlock> BatteryTB = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Voyage")
    TObjectPtr<UTextBlock> BatteryDeltaTB = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Voyage")
    TObjectPtr<UTextBlock> PetrolTB = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Voyage")
    TObjectPtr<UTextBlock> PetrolDeltaTB = nullptr;
};
