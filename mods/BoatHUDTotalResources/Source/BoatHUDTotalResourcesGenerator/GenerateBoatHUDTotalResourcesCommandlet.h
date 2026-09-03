// HAND-WRITTEN BUILD TOOL SOURCE: generates the BoatHUDTotalResources assets.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateBoatHUDTotalResourcesCommandlet.generated.h"

UCLASS()
class BOATHUDTOTALRESOURCESGENERATOR_API UGenerateBoatHUDTotalResourcesCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateBoatHUDTotalResourcesCommandlet();
    virtual int32 Main(const FString& Params) override;
};
