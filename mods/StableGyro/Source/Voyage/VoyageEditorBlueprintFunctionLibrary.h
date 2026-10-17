// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25929985, UE 5.8.2
// (parser target 5.8).
// Signature shared with the game-validated Railgun settings reader. Revalidate
// owner and parameters after a Steam build ID change. Editor-only; never ship.

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
