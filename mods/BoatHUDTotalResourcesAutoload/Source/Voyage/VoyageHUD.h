// HAND-WRITTEN EDITOR SCAFFOLD: exact /Script/Voyage identity used only to compile assets.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/HUD.h"
#include "VoyageHUD.generated.h"

UCLASS()
class VOYAGE_API AVoyageHUD : public AHUD
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    TMap<TSubclassOf<AActor>, TSubclassOf<UUserWidget>> CharacterWidgetClasses;
};
