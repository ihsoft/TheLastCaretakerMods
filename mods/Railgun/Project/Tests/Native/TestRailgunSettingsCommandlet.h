#pragma once

#include "Commandlets/Commandlet.h"
#include "TestRailgunSettingsCommandlet.generated.h"

UCLASS()
class RAILGUNEDITOR_API UTestRailgunSettingsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UTestRailgunSettingsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
