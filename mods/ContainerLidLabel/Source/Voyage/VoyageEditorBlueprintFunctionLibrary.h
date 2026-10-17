#pragma once
// HAND-RECONSTRUCTED EDITOR-ONLY MIRROR: Steam 25929985, UE 5.8.2.
// Current native registration metadata establishes this exact owner, FString
// input, and TArray<FString> return. Revalidate on Steam build ID change. No native
// mirror binary is shipped.

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="ContainerLidLabel")
    static TArray<FString> LoadFileToArray(FString InPath) { return {}; }
};
