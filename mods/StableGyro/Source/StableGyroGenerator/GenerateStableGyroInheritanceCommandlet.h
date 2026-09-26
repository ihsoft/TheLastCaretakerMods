// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that generates
// the Gyro replacement child. It is compiled for the editor, never shipped.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateStableGyroInheritanceCommandlet.generated.h"

UCLASS()
class STABLEGYROGENERATOR_API UGenerateStableGyroInheritanceCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateStableGyroInheritanceCommandlet();
    virtual int32 Main(const FString& Params) override;
};
