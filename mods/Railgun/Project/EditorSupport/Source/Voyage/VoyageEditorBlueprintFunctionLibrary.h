#pragma once
// Editor-only mirror; never shipped. Steam25191271, UE5.8.
// Revalidated from native FFunctionParams at0x14b174110: owner /Script/Voyage,
// flags0x04022401, InPath FString at0, returned TArray<FString> at16, size32.
// Method: public executable inspector plus bounded registration decode.
// Revalidate owner/signature on Steam build ID change; stub is authoring only.
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "VoyageEditorBlueprintFunctionLibrary.generated.h"
UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    static TArray<FString> LoadFileToArray(FString InPath)
    {
        FString Text;
        TArray<FString> Lines;
        if (FFileHelper::LoadFileToString(Text, *InPath))
            Text.ParseIntoArrayLines(Lines, false);
        return Lines;
    }
};
