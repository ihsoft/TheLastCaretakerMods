// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that creates
// the first Harpoon Cannon Blueprint and HUD skeleton.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateHarpoonCannonCommandlet.generated.h"

UCLASS()
class HARPOONCANNONGENERATOR_API UGenerateHarpoonCannonCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateHarpoonCannonCommandlet();
    virtual int32 Main(const FString& Params) override;
};
