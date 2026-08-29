#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateDonkLiftInheritanceCommandlet.generated.h"

UCLASS()
class VOYAGE_API UGenerateDonkLiftInheritanceCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateDonkLiftInheritanceCommandlet();
    virtual int32 Main(const FString& Params) override;
};
