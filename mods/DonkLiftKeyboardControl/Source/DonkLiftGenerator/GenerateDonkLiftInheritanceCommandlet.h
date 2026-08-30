// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that generates
// the forklift replacement child. It is compiled for the editor, never shipped.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateDonkLiftInheritanceCommandlet.generated.h"

UCLASS()
class DONKLIFTGENERATOR_API UGenerateDonkLiftInheritanceCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateDonkLiftInheritanceCommandlet();
    virtual int32 Main(const FString& Params) override;
};
