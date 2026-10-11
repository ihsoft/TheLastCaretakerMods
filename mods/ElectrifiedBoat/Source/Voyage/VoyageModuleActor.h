// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "GameFramework/Actor.h"
#include "VoyageModuleActor.generated.h"

class UVoyageModuleComponent;

UCLASS(BlueprintType)
class VOYAGE_API AVoyageModuleActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UVoyageModuleComponent> ModuleComponent;
};
