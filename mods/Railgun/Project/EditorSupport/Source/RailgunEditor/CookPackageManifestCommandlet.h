#pragma once

#include "Commandlets/Commandlet.h"
#include "CookPackageManifestCommandlet.generated.h"

// Editor-only adapter that keeps large explicit cook inventories out of the
// process command line. The stock CookCommandlet still owns the actual cook.
UCLASS()
class UCookPackageManifestCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCookPackageManifestCommandlet();
    virtual int32 Main(const FString& Params) override;
};
