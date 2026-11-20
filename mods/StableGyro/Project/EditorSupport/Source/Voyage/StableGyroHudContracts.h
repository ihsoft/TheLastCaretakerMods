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

// Map native identities inspected in Steam build 25982825.
// Partial mirrors for compilation only; inherited defaults require tagged cook.
UENUM(BlueprintType)
enum class EVoyageLocatorType : uint8
{
    Unknown = 0,
    Player = 1,
    Boat = 2,
    Waypoint = 3,
    Event = 4,
    Location = 5,
    Unit = 6,
    Portal = 7,
    Custom = 8,
    EVoyageLocatorType_MAX = 9 UMETA(Hidden)
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageLocatorWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    TArray<EVoyageLocatorType> TypeExcludeFilters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    bool bIsViewportWidget = false;
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageMinimapWidget : public UVoyageLocatorWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    float DefaultZoom = 0.1f;

    UFUNCTION(BlueprintCallable, Category="Map")
    void SetCenterLocation(FVector2D InCenter) {}

    UFUNCTION(BlueprintCallable, Category="Map")
    void SetZoom(float InZoom) {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    FLinearColor PathLineColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    float PathLineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    FLinearColor ForwardLineColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    float ForwardLineThickness = 1.0f;
};
