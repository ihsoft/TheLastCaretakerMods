// HAND-RECONSTRUCTED GAME API IDENTITY MIRROR: Steam build 25982825,
// UE 5.8.2 parser target. Current mappings establish the native class chain
// and the writable MaxResources map; stock Gyro JSON establishes Petrol=400.
// Revalidate on Steam build ID change. Editor-only; never ship this native
// mirror or author component defaults.

#pragma once

#include "VoyageModuleComponent.h"
#include "VoyageModuleSocketViewComponent.h"
#include "VoyageCustomModuleComponent.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API UVoyageCustomModuleComponent : public UVoyageModuleComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "StableGyroFuel")
    TMap<EModuleResourceType, double> MaxResources;
};
