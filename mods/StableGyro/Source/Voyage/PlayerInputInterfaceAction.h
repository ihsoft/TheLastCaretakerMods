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
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UInputAction> InputAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Category;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText TextError;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAppendTargetName = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTriggerOnce = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CrosshairPriority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TriggerTime = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TriggerProgress = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UObject> CrosshairWidgetData = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D CrosshairPosition = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerInputInterfaceActionType Type = EPlayerInputInterfaceActionType::Central;
};
