// EDITOR-ONLY IDENTITY MIRROR for Steam build 25929985 (UE 5.8).
// Executable SHA-256: 608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F
// Reconstructed from current native metadata; revalidate when the executable fingerprint changes.
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
