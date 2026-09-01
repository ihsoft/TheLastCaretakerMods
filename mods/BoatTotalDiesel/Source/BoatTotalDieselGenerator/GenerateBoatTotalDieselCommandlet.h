// HAND-WRITTEN BUILD TOOL SOURCE: generates the BoatTotalDiesel assets.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateBoatTotalDieselCommandlet.generated.h"

UCLASS()
class BOATTOTALDIESELGENERATOR_API UGenerateBoatTotalDieselCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateBoatTotalDieselCommandlet();
    virtual int32 Main(const FString& Params) override;
};
