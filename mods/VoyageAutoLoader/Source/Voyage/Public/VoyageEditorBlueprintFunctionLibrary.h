#pragma once

// Editor-only identity mirror, never shipped as native code.
// Voyage Steam 25056839 / UE 5.8.1, executable SHA256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Reconstructed from canonical jmap live reflection (2026-09-05).
// Revalidate owner, parameter types/flags and return type after fingerprint change.
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Voyage Probe Mirror")
    static TArray<FString> GetAllFilesInDirectory(FString InPath, FString FileTypes) { return {}; }

    UFUNCTION(BlueprintCallable, Category="Voyage Probe Mirror")
    static TArray<FString> LoadFileToArray(FString InPath) { return {}; }
};
