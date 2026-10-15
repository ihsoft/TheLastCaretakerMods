#pragma once
// HAND-RECONSTRUCTED EDITOR-ONLY MIRROR: Steam 25929985, UE 5.8.2,
// VoyageSteam-Win64-Shipping.exe SHA-256
// 608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F.
// Current native registration metadata establishes this exact owner, FString
// input, and TArray<FString> return. Revalidate on fingerprint change. No native
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
