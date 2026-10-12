// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25880952, UE 5.8.2
// (parser target 5.8), executable SHA-256
// 16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44.
// Signature shared with the game-validated Railgun settings reader. Revalidate
// owner and parameters after a fingerprint change. Editor-only; never ship.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Voyage Mirror")
    static TArray<FString> LoadFileToArray(FString InPath) { return {}; }
};
