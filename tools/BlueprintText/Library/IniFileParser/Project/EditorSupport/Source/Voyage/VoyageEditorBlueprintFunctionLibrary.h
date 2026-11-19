#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"

// Editor declaration of the existing Voyage file reader. The game supplies its
// implementation; this source support module is never a runtime parser payload.
UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    static TArray<FString> LoadFileToArray(FString InPath) { return {}; }
};
