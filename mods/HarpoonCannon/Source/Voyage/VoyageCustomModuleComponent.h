// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../GAME_DERIVED_SOURCES.md for prefix limits and fingerprint renewal gate.
// EDITOR-ONLY IDENTITY MIRROR for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "VoyageModuleComponent.h"
#include "VoyageCustomModuleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVoyageItemConsumedDelegate);

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageCustomModuleComponent : public UVoyageModuleComponent
{
    GENERATED_BODY()

public:
    // Exact 20-property derived-class prefix. Container inner types are
    // alignment-only editor placeholders: this probe does not serialize any of
    // these values, but their presence keeps inherited ItemAsset at the same
    // unversioned field index as the runtime class.
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
    TMap<uint8, double> ConsumptionsOn;

    UPROPERTY()
    TMap<uint8, double> ConsumptionsStandby;

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
