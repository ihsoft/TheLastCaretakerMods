// EDITOR-ONLY IDENTITY MIRROR for Steam build 24990438 (UE 5.8.1).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "VoyageModuleSubsystem.generated.h"

class UVoyageModuleComponent;

UCLASS(BlueprintType)
class VOYAGE_API UVoyageModuleSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void GetModulesInSameGrid(
        UVoyageModuleComponent* Module,
        TArray<UVoyageModuleComponent*>& OutModules);
};
