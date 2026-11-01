// TEST-ONLY mirror of the existing Voyage file-reader signature used by
// VoyageModSettings. Runtime owner: /Script/Voyage. Do not ship this plugin.
// Signature follows the maintained mirror for Steam build 25929985 / UE 5.8.2.
#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    inline static int32 ReadCount = 0;
public:
    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    static TArray<FString> LoadFileToArray(FString InPath)
    {
        ++ReadCount;
        FString Text;
        TArray<FString> Lines;
        if(FFileHelper::LoadFileToString(Text,*InPath))Text.ParseIntoArrayLines(Lines,false);
        return Lines;
    }
    UFUNCTION(BlueprintPure, Category="BPT Tests")
    static int32 GetTestReadCount() { return ReadCount; }
};
