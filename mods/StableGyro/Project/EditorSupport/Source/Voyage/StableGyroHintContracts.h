#pragma once
// Editor-only identities reused from SmartOutlet's reviewed Steam 25982825 contracts.
// Stock classes/assets supply runtime behavior; none of these declarations ship.
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StableGyroHudContracts.h"
#include "PlayerInputInterfaceAction.h"
#include "StableGyroHintContracts.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputContextAsset : public UDataAsset { GENERATED_BODY() };

UCLASS(BlueprintType)
class VOYAGE_API UVoyageButtonInfoWidget : public UUserWidget { GENERATED_BODY() };

UCLASS(BlueprintType)
class VOYAGE_API UVoyageButtonInfoContainerWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Input")
    void SetActions(UVoyageInputContextAsset* InContextAsset, UInputAction* Action,
        const TArray<FPlayerInputInterfaceAction>& InOverrideActions, bool bRecreateWidget) {}
};
