#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateMooringCommandlet.generated.h"
UCLASS()
class UGenerateMooringCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateMooringCommandlet();
    virtual int32 Main(const FString& Params) override;
};
