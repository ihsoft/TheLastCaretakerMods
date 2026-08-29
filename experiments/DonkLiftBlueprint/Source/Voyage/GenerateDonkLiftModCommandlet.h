#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateDonkLiftModCommandlet.generated.h"

#if WITH_EDITOR

UCLASS()
class VOYAGE_API UGenerateDonkLiftModCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateDonkLiftModCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#endif
