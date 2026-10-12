// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25827816, UE 5.8.2
// (parser target 5.8), executable SHA-256
// C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
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
