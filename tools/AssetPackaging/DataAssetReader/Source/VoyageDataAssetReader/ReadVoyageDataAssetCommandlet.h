#pragma once
#include "Commandlets/Commandlet.h"
#include "ReadVoyageDataAssetCommandlet.generated.h"

UCLASS()
class VOYAGEDATAASSETREADER_API UReadVoyageDataAssetCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UReadVoyageDataAssetCommandlet();
    virtual int32 Main(const FString& Params) override;
};
