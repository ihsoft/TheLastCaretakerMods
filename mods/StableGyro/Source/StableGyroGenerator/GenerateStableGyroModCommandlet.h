// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that generates
// helper/input assets. It depends on version-bound game contracts; never ship.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateStableGyroModCommandlet.generated.h"

#if WITH_EDITOR

UCLASS()
class STABLEGYROGENERATOR_API UGenerateStableGyroModCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateStableGyroModCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#endif
