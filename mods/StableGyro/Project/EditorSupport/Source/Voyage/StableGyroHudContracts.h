// HAND-RECONSTRUCTED EDITOR-ONLY IDENTITIES: Steam build 25982825, UE 5.8.2.
// Parent chains and compass fields verified through current game mappings. These mirrors
// compile external HUD references only; their binaries and CDOs never ship.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StableGyroHudContracts.generated.h"

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageBaseUserWidget : public UUserWidget
{
    GENERATED_BODY()
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageInGameWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageInGameVehicleWidget : public UVoyageInGameWidget
{
    GENERATED_BODY()
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageIngameGenericVehicleWidget : public UVoyageInGameVehicleWidget
{
    GENERATED_BODY()
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageCompassWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="Compass")
    TObjectPtr<class UImage> CompassImage;

    UPROPERTY(BlueprintReadOnly, Category="Compass")
    TObjectPtr<class UTextBlock> CompassAngleText;
};
