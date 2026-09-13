#pragma once
// HC29 named-field/delegate mirror; Steam25191271, UE5.8,
// EXE747DC255...F58B. Current mapping and BP_CameraDrone callback metadata.
// Bytecode only, no native struct default/CDO serialization. Renew on update.
#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/Controller.h"
#include "VoyageInputActionInstance.generated.h"
USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageInputActionInstance : public FInputActionInstance
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AController> Controller = nullptr;
};
DECLARE_DYNAMIC_DELEGATE_TwoParams(FPlayerInputInterfaceOnAction, const FVoyageInputActionInstance&, InputAction, FText&, Text);
