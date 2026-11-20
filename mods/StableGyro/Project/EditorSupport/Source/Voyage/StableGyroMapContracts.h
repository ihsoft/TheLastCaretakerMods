#pragma once
// Editor-only identities, Steam 25982825 / UE 5.8.2. Never shipped.
// ShowOverlayWidget signature: current native registration 0x14b3d5180;
// overlay-data fields and controller parent chain: reviewed current mappings.
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/PlayerController.h"
#include "Components/CanvasPanelSlot.h"
#include "StableGyroHudContracts.h"
#include "StableGyroMapContracts.generated.h"

UENUM(BlueprintType)
enum class EInputModeType : uint8
{
    Unknown = 0, Character = 1, Overlay = 2
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageOverlayWidgetData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Map") TSubclassOf<UVoyageBaseUserWidget> WidgetClass;
    UPROPERTY(EditAnywhere, Category="Map") FAnchorData Anchor;
    UPROPERTY(EditAnywhere, Category="Map") FVector2D Size = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Map") bool bApplyAspect = false;
    UPROPERTY(EditAnywhere, Category="Map") bool bHideInGameHud = false;
    UPROPERTY(EditAnywhere, Category="Map") bool bHideGameStateWidget = false;
    UPROPERTY(EditAnywhere, Category="Map") float BlurStrength = 0;
    UPROPERTY(EditAnywhere, Category="Map") bool bBackgroundBorder = false;
    UPROPERTY(EditAnywhere, Category="Map") bool bAllowOverrideMargin = false;
    UPROPERTY(EditAnywhere, Category="Map") TObjectPtr<UObject> OverlayValidator;
};

UCLASS(BlueprintType)
class VOYAGE_API AVoyagePlayerControllerBase : public APlayerController
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API AVoyagePlayerController : public AVoyagePlayerControllerBase
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Map")
    UVoyageBaseUserWidget* ShowOverlayWidget(UVoyageOverlayWidgetData* InWidgetData, EInputModeType InputModeOverride) { return nullptr; }
    // Existing SmartOutlet contract: the active in-game HUD is the focus widget.
    UFUNCTION(BlueprintPure, Category="HUD")
    UVoyageBaseUserWidget* GetPlayerFocusWidget() const { return nullptr; }
};

// Stock map chrome uses these exact native parent/field identities.
UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageBaseMenuUserWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()
};

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API UVoyageScreenOverlayWidget : public UVoyageBaseMenuUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category="Map") TObjectPtr<UVoyageBaseUserWidget> ScreenWidget;
};
