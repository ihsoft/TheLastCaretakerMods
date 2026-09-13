#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateHarpoonInputsCommandlet.generated.h"

// Editor-only authoring gate, never an autoload entry point.
UCLASS()
class UGenerateHarpoonInputsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateHarpoonInputsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
