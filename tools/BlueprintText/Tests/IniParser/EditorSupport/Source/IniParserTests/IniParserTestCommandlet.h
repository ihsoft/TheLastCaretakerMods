#pragma once
#include "Commandlets/Commandlet.h"
#include "IniParserTestCommandlet.generated.h"
UCLASS()
class UIniParserTestCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UIniParserTestCommandlet();
    virtual int32 Main(const FString& Params) override;
};
