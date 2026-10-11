// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "GameFramework/Actor.h"
#include "VoyageVirtualActorSocket.generated.h"

class UVoyageModuleSocketViewComponent;

UCLASS(BlueprintType)
class VOYAGE_API AVoyageVirtualActorSocket : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UVoyageModuleSocketViewComponent> VoyageModuleSocketView;
};
