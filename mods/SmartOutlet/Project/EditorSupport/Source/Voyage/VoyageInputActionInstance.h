#pragma once
// EDITOR ONLY: adapted from Railgun's named-field/delegate mirror, based on
// Steam25191271 mappings and BP_CameraDrone callback metadata (UE5.8).
// Bytecode only, no native struct default/CDO serialization. Renew on update.
#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/Controller.h"
#include "VoyageInputActionInstance.generated.h"
USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageInputActionInstance : public FInputActionInstance
{
    GENERATED_BODY()
    UPROPERTY(Category="Interaction", BlueprintReadOnly) TObjectPtr<AController> Controller = nullptr;
};
DECLARE_DYNAMIC_DELEGATE_TwoParams(FPlayerInputInterfaceOnAction, const FVoyageInputActionInstance&, InputAction, FText&, Text);
