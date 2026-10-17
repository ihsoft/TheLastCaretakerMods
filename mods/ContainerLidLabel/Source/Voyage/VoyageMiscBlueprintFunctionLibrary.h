// EDITOR-ONLY IDENTITY MIRROR for Steam build 25929985 (UE 5.8).
// Reconstructed from current native metadata; revalidate when the Steam build ID changes.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageMiscBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    static UObject* GetClassDefaultObject(TSubclassOf<UObject> Class)
    {
        return nullptr;
    }
};
