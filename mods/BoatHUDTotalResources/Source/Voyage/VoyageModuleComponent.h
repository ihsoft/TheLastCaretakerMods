// EDITOR-ONLY IDENTITY MIRROR for Steam build 25056839 (UE 5.8.1).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "ModuleResourceType.h"
#include "VoyageModuleComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 CachedGridID = 0;

    UPROPERTY()
    bool bIsMasterModule = false;

    UPROPERTY()
    bool bDynamicTickInterval = false;

    UPROPERTY()
    bool bRequirementsMet = false;

    UPROPERTY()
    bool bIgnorePowerOffVolumes = false;

    UPROPERTY()
    bool bRegisterModule = false;

    UPROPERTY()
    TObjectPtr<UObject> ModuleConfigData;

    UPROPERTY()
    TObjectPtr<UObject> ItemAsset;

    UFUNCTION(BlueprintPure)
    double GetResourceAmount(EModuleResourceType Type) const;
};
