#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateRailgunRuntimeCommandlet.generated.h"

// Editor tool only. Generated assets reference Engine types, never this module.
UCLASS()
class UGenerateRailgunRuntimeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateRailgunRuntimeCommandlet();
    virtual int32 Main(const FString& Params) override;
};
