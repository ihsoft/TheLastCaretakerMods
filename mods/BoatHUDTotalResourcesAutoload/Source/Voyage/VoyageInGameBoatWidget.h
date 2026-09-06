// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 25056839
// (Unreal Engine 5.8.1; VoyageSteam-Win64-Shipping.exe SHA-256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933).
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
