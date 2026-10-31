#pragma once
#include "Commandlets/Commandlet.h"
#include "VoyageItemPatchEditorCommandlet.generated.h"
UCLASS()
class UVoyageItemPatchEditorCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UVoyageItemPatchEditorCommandlet();
    virtual int32 Main(const FString& Params) override;
};
