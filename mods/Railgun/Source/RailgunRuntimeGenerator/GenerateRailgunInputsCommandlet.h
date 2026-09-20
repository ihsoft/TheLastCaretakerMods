#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateRailgunInputsCommandlet.generated.h"

// Editor-only authoring gate, never an autoload entry point.
UCLASS()
class UGenerateRailgunInputsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateRailgunInputsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
