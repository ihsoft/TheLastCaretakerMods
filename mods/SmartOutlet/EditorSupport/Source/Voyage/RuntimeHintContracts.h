#pragma once
// EDITOR ONLY. Steam25982825 / UE5.8, native reflected registration inspected in
// Tmp/SmartOutlet/research-port-action/native-contracts.json. No native code ships.
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DataAsset.h"
#include "Components/PanelWidget.h"
#include "PlayerInputInterfaceAction.h"
#include "RuntimeHintContracts.generated.h"

UENUM(BlueprintType)
enum class EHudWidgetType : uint8 { Unknown=0, Hud=1, Overlay=2 };
UCLASS(BlueprintType)
class VOYAGE_API UVoyageBaseUserWidget : public UUserWidget { GENERATED_BODY() };
UCLASS(BlueprintType)
class VOYAGE_API UVoyageInGameWidget : public UVoyageBaseUserWidget { GENERATED_BODY() };
UCLASS(BlueprintType)
class VOYAGE_API UVoyageInGameCharacterWidget : public UVoyageInGameWidget
{
    GENERATED_BODY()
public:
    // Native readonly member; used only through runtime bytecode, never serialized.
    UPROPERTY(BlueprintReadOnly, Category="HUD")
    TObjectPtr<UPanelWidget> CentralInputActionContainer;
};
UCLASS(BlueprintType)
class VOYAGE_API AVoyagePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, Category="Editor validation")
    TObjectPtr<UVoyageBaseUserWidget> EditorFocusWidget;
    UPROPERTY(VisibleAnywhere, Category="Editor validation")
    bool EditorHudActive = true;
    UFUNCTION(BlueprintPure, Category="HUD")
    UVoyageBaseUserWidget* GetPlayerFocusWidget() const { return EditorFocusWidget; }
    UFUNCTION(BlueprintPure, Category="HUD")
    bool IsHudTopInputStack(EHudWidgetType HudType) const { return EditorHudActive; }
    UFUNCTION(BlueprintPure, Category="HUD")
    bool IsPlayerHudWidgetVisible(EHudWidgetType Type) const { return EditorHudActive; }
};
UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputContextAsset : public UDataAsset { GENERATED_BODY() };
UCLASS(BlueprintType)
class VOYAGE_API UVoyageButtonInfoWidget : public UUserWidget { GENERATED_BODY() };
UCLASS(BlueprintType)
class VOYAGE_API UVoyageButtonInfoContainerWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()
public:
    // Read-only bytecode access. Never serialize a partial native widget mirror.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    TSubclassOf<UVoyageButtonInfoWidget> ButtonInfoWidgetClass;
    UPROPERTY(BlueprintReadOnly, Category="Input")
    TObjectPtr<UPanelWidget> RootPanel;
    // Editor fixture captures configuration at the actual SetActions call.
    UPROPERTY(VisibleAnywhere, Category="Editor validation")
    TSubclassOf<UVoyageButtonInfoWidget> EditorConfiguredClass;
    UPROPERTY(VisibleAnywhere, Category="Editor validation")
    FString EditorHintText;
    UPROPERTY(VisibleAnywhere, Category="Editor validation")
    int32 EditorHintUpdates = 0;
    UFUNCTION(BlueprintCallable, Category="Input")
    void SetActions(UVoyageInputContextAsset* InContextAsset, UInputAction* Action,
        const TArray<FPlayerInputInterfaceAction>& InOverrideActions, bool bRecreateWidget)
    {
        EditorConfiguredClass = ButtonInfoWidgetClass;
        EditorHintText = InOverrideActions.Num() ? InOverrideActions[0].Text.ToString() : FString();
        ++EditorHintUpdates;
    }
};
