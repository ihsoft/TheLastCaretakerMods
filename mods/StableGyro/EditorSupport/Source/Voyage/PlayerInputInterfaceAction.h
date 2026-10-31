// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25929985, UE 5.8.2
// (parser target 5.8).
// The fields consumed by generated assets were revalidated from current native
// reflection metadata. Revalidate on Steam build ID change. Editor-only; never ship.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "PlayerInputInterfaceAction.generated.h"

UENUM(BlueprintType)
enum class EPlayerInputInterfaceActionType : uint8
{
    Central = 0,
    Context = 1,
    Hidden = 2
};

USTRUCT(BlueprintType)
struct VOYAGE_API FPlayerInputInterfaceAction
{
    GENERATED_BODY()

    UPROPERTY() TArray<TWeakObjectPtr<UObject>> ReferencedObjects;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) TObjectPtr<UInputAction> InputAction = nullptr;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) FString Category;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) FText Text;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) FText TextError;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) bool bAppendTargetName = false;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) bool bTriggerOnce = false;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) int32 Priority = 0;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) int32 CrosshairPriority = 0;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) float TriggerTime = 0.0f;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) float TriggerProgress = 0.0f;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) TObjectPtr<UObject> CrosshairWidgetData = nullptr;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) FVector2D CrosshairPosition = FVector2D::ZeroVector;
    UPROPERTY(Category="PlayerInputInterfaceAction", EditAnywhere, BlueprintReadWrite) EPlayerInputInterfaceActionType Type = EPlayerInputInterfaceActionType::Central;
};
