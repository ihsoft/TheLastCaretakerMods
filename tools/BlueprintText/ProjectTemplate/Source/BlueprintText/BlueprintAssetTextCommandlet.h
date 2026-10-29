#pragma once
#include "Commandlets/Commandlet.h"
#include "BlueprintAssetTextCommandlet.generated.h"

UCLASS()
class UBlueprintAssetTextCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UBlueprintAssetTextCommandlet();
    virtual int32 Main(const FString& Params) override;
};
