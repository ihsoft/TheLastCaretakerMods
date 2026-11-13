#pragma once
// HC28 bytecode-only mirror. Steam25827816, UE5.8,
// Current native metadata confirms these 16 fields, followed by six delegates.
// Never serialize this partial struct as a CDO/default; MakeStruct writes only
// named fields. Current native reflection owns the schema; the prior DonkLift
// graph is historical precedent only. See ../../../../ARCHITECTURE.md. Editor-only.
#include "CoreMinimal.h"
#include "InputAction.h"
#include "VoyageInputActionInstance.h"
#include "PlayerInputInterfaceAction.generated.h"

UENUM(BlueprintType)
enum class EPlayerInputInterfaceActionType : uint8 { Central=0, Context=1, Hidden=2 };

USTRUCT(BlueprintType)
struct VOYAGE_API FPlayerInputInterfaceAction
{
    GENERATED_BODY()
    UPROPERTY() TArray<TWeakObjectPtr<UObject>> ReferencedObjects;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") TObjectPtr<UInputAction> InputAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FString Category;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FText TextError;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") bool bAppendTargetName = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") bool bTriggerOnce = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") int32 CrosshairPriority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") float TriggerTime = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") float TriggerProgress = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") TObjectPtr<UObject> CrosshairWidgetData = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FVector2D CrosshairPosition = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") EPlayerInputInterfaceActionType Type = EPlayerInputInterfaceActionType::Central;
    // HC29 current named delegate identities. Still bytecode-only: native
    // non-reflected storage means sizeof(editor mirror) is NOT shipping ABI.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FPlayerInputInterfaceOnAction OnValue;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FPlayerInputInterfaceOnAction OnTriggered;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FPlayerInputInterfaceOnAction OnStarted;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FPlayerInputInterfaceOnAction OnOngoing;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FPlayerInputInterfaceOnAction OnCanceled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerInputInterfaceAction") FPlayerInputInterfaceOnAction OnCompleted;
};
