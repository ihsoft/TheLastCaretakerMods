#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateElectrifiedBoatCommandlet.generated.h"

UCLASS()
class UGenerateElectrifiedBoatCommandlet final : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateElectrifiedBoatCommandlet();
    virtual int32 Main(const FString& Params) override;
};
