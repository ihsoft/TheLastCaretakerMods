#pragma once
// HC29 one-function identity mirror, current BP_CameraDrone / BP_Module_Turret
// SuperStruct+parameter metadata. Steam25191271, UE5.8, EXE747DC255...F58B.
// Inherited interface, not a complete reconstruction. No native CDO deltas.
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerInputInterfaceAction.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "InteractiveInterface.generated.h"
UINTERFACE(BlueprintType)
class VOYAGE_API UInteractiveInterface : public UInterface { GENERATED_BODY() };
class VOYAGE_API IInteractiveInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interaction")
    bool GetInteractiveProvidedActions(APawn* MyCharacter, USceneComponent* Component, TArray<FPlayerInputInterfaceAction>& OutActions);
};
