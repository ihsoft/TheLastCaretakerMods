#pragma once
// HC28 bytecode-only mirror. Steam25191271, UE5.8, EXE747DC255...F58B.
// Current mappings confirm these 17 fields, followed by six native delegates.
// Never serialize this partial struct as a CDO/default; MakeStruct writes only
// named fields. Exact owner/precedent: DonkLift GetProvidedActionsBP, renewed in
// GAME_DERIVED_SOURCES.md. Revalidate on fingerprint change. Editor-only.
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TriggerTime = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TriggerProgress = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UObject> CrosshairWidgetData = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D CrosshairPosition = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerInputInterfaceActionType Type = EPlayerInputInterfaceActionType::Central;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UInputAction>> SecondaryInputActions;
    // HC29 current named delegate identities. Still bytecode-only: native
    // non-reflected storage means sizeof(editor mirror) is NOT shipping ABI.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnValue;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnTriggered;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnStarted;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnOngoing;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnCanceled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerInputInterfaceOnAction OnCompleted;
};
