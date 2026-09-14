#pragma once
// Editor-only mirror; never shipped. Steam25191271, UE5.8, executable SHA256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Revalidated from native FFunctionParams at0x14b174110: owner /Script/Voyage,
// flags0x04022401, InPath FString at0, returned TArray<FString> at16, size32.
// Method: public executable inspector plus bounded registration decode.
// Revalidate owner/signature on fingerprint change; stub is authoring only.
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"
UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    static TArray<FString> LoadFileToArray(FString InPath) { return {}; }
};
