// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../ARCHITECTURE.md for prefix limits and Steam build change gate.
// EDITOR-ONLY IDENTITY MIRROR for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageModuleComponent.h"
#include "ModuleResourceType.h"
#include "VoyageCustomModuleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVoyageItemConsumedDelegate);

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageCustomModuleComponent : public UVoyageModuleComponent
{
    GENERATED_BODY()

public:
    // Historical prefix fields remain at empty defaults. Only ConsumptionsOn
    // and ConsumptionsStandby are authored here, with exact native enum keys.
    // This partial mirror requires tagged cooking.
    UPROPERTY()
    FVoyageItemConsumedDelegate OnItemConsumedDelegate;

    UPROPERTY()
    TArray<int32> InternalResourceConversion;

    UPROPERTY()
    TArray<int32> InternalItemConversion;

    UPROPERTY()
    TMap<uint8, double> ResourceGainsLastFrame;

    UPROPERTY()
    TMap<uint8, double> ResourceLossesLastFrame;

    UPROPERTY()
    TSet<uint8> ActiveFlowResources;

    UPROPERTY()
    float ActiveFlowTimerLength = 0.0f;

    UPROPERTY()
    TMap<uint8, double> MaxResources;

    UPROPERTY()
    TMap<EModuleResourceType, double> ConsumptionsOn;

    UPROPERTY()
    TMap<EModuleResourceType, double> ConsumptionsStandby;

    UPROPERTY()
    TMap<UObject*, double> ItemConsumptionsOn;

    UPROPERTY()
    TMap<uint8, double> ProductionsOn;

    UPROPERTY()
    TMap<UObject*, double> ItemProductionsOn;

    UPROPERTY()
    TMap<UObject*, double> ItemConsumptionBuffer;

    UPROPERTY()
    TMap<UObject*, double> ItemProductionBuffer;

    UPROPERTY()
    bool bUseItemConsumptionAsWeight = false;

    UPROPERTY()
    float AutomaticMaxResourceBuffer = 0.0f;

    UPROPERTY()
    float OperationalMultiplier = 0.0f;

    UPROPERTY()
    TMap<uint8, double> TemporaryConsumptions;

    UPROPERTY()
    TMap<uint8, double> TemporaryResourceMax;

};
