// HAND-WRITTEN BUILD TOOL SOURCE: declares the editor commandlet that generates
// the Gyro replacement child. It is compiled for the editor, never shipped.

#pragma once

#include "Commandlets/Commandlet.h"
#include "GenerateGyroKeyboardInheritanceCommandlet.generated.h"

UCLASS()
class GYROKEYBOARDGENERATOR_API UGenerateGyroKeyboardInheritanceCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGenerateGyroKeyboardInheritanceCommandlet();
    virtual int32 Main(const FString& Params) override;
};
