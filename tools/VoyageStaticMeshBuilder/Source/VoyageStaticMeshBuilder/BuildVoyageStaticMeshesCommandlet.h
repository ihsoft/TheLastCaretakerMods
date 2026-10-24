#pragma once

#include "Commandlets/Commandlet.h"
#include "BuildVoyageStaticMeshesCommandlet.generated.h"

UCLASS()
class UBuildVoyageStaticMeshesCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UBuildVoyageStaticMeshesCommandlet();
    virtual int32 Main(const FString& Params) override;
};
