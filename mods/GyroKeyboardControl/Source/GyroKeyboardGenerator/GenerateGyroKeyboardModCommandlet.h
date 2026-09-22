// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that generates
// helper/input assets. It depends on version-bound game contracts; never ship.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateGyroKeyboardModCommandlet.generated.h"

#if WITH_EDITOR

UCLASS()
class GYROKEYBOARDGENERATOR_API UGenerateGyroKeyboardModCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateGyroKeyboardModCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#endif
