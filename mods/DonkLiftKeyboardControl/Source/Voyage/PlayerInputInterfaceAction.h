// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 25056839
// (Unreal Engine 5.8.1; VoyageSteam-Win64-Shipping.exe SHA-256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933).
// Recreate/revalidate with tools/Get-VoyageBuildFingerprint.ps1,
// tools/Inspect-VoyageAsset.ps1, and tools/VoyageExecutableInspector after a
// game update. Editor-only: never package this native definition.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "PlayerInputInterfaceAction.generated.h"

// Only the fields required by the generated Blueprint are mirrored below.
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

    UPROPERTY()
    TArray<TWeakObjectPtr<UObject>> ReferencedObjects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UInputAction> InputAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText TextError;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAppendTargetName = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTriggerOnce = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CrosshairPriority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TriggerTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TriggerProgress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UObject> CrosshairWidgetData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D CrosshairPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPlayerInputInterfaceActionType Type = EPlayerInputInterfaceActionType::Central;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TObjectPtr<UInputAction>> SecondaryInputActions;
};
