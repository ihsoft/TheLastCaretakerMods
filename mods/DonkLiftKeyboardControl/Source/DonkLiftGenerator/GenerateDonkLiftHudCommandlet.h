// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that generates
// the standard-HUD replacement child. It is editor-only and never shipped.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateDonkLiftHudCommandlet.generated.h"

#if WITH_EDITOR

UCLASS()
class DONKLIFTGENERATOR_API UGenerateDonkLiftHudCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateDonkLiftHudCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#endif
