// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that creates
// the first Railgun Railgun Blueprint and HUD skeleton.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateRailgunCommandlet.generated.h"

UCLASS()
class RAILGUNMODELGENERATOR_API UGenerateRailgunCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateRailgunCommandlet();
    virtual int32 Main(const FString& Params) override;
};
