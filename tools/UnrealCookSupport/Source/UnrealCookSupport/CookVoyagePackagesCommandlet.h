#pragma once

#include "Commandlets/Commandlet.h"
#include "CookVoyagePackagesCommandlet.generated.h"

// Keeps explicit inventories off the command line; Unreal owns the cook.
UCLASS()
class UCookVoyagePackagesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UCookVoyagePackagesCommandlet();
    virtual int32 Main(const FString& Params) override;
};
