#pragma once
// EDITOR ONLY: adapted from Railgun's bytecode-only interaction mirror.
// Steam25982825 metadata confirms 16 fields followed by six delegates.
// Never serialize this partial struct as a CDO/default; MakeStruct writes only
// named fields. Native reflection owns the runtime schema.
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
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) TObjectPtr<UInputAction> InputAction = nullptr;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FString Category;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FText Text;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FText TextError;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) bool bAppendTargetName = false;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) bool bTriggerOnce = false;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) int32 Priority = 0;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) int32 CrosshairPriority = 0;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) float TriggerTime = 0;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) float TriggerProgress = 0;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) TObjectPtr<UObject> CrosshairWidgetData = nullptr;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FVector2D CrosshairPosition = FVector2D::ZeroVector;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) EPlayerInputInterfaceActionType Type = EPlayerInputInterfaceActionType::Central;
    // Named delegate identities. Still bytecode-only: native
    // non-reflected storage means sizeof(editor mirror) is NOT shipping ABI.
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnValue;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnTriggered;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnStarted;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnOngoing;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnCanceled;
    UPROPERTY(Category="Interaction", EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnCompleted;
};
