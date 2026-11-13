#pragma once

#include "Commandlets/Commandlet.h"
#include "InspectRailgunProjectCommandlet.generated.h"

UCLASS()
class RAILGUNEDITOR_API UInspectRailgunProjectCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UInspectRailgunProjectCommandlet();
    virtual int32 Main(const FString& Params) override;
};
