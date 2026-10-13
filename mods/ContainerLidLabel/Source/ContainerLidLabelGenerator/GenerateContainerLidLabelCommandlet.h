#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateContainerLidLabelCommandlet.generated.h"

UCLASS()
class UGenerateContainerLidLabelCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateContainerLidLabelCommandlet();
    virtual int32 Main(const FString& Params) override;
};
