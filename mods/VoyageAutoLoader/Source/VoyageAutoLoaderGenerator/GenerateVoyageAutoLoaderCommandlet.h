#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateVoyageAutoLoaderCommandlet.generated.h"

UCLASS()
class VOYAGEAUTOLOADERGENERATOR_API UGenerateVoyageAutoLoaderCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateVoyageAutoLoaderCommandlet();
    virtual int32 Main(const FString& Params) override;
};
