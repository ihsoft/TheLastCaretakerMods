#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateHarpoonProbeCommandlet.generated.h"

// Editor tool only. Generated assets reference Engine types, never this module.
UCLASS()
class UGenerateHarpoonProbeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateHarpoonProbeCommandlet();
    virtual int32 Main(const FString& Params) override;
};
