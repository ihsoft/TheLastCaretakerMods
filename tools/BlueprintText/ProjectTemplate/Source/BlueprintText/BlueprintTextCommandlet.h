#pragma once
#include "Commandlets/Commandlet.h"
#include "BlueprintTextCommandlet.generated.h"
UCLASS()
class UBlueprintTextCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UBlueprintTextCommandlet();
    virtual int32 Main(const FString& Params) override;
};
