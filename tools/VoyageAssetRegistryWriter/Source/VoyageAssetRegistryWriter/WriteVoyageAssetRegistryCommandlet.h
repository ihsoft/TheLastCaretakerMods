#pragma once

#include "Commandlets/Commandlet.h"
#include "WriteVoyageAssetRegistryCommandlet.generated.h"

UCLASS()
class UWriteVoyageAssetRegistryCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UWriteVoyageAssetRegistryCommandlet();
    virtual int32 Main(const FString& Params) override;
};
