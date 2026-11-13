#pragma once
#include "Commandlets/Commandlet.h"
#include "ReadRailgunDataAssetsCommandlet.generated.h"

UCLASS()
class RAILGUNEDITOR_API UReadRailgunDataAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UReadRailgunDataAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
