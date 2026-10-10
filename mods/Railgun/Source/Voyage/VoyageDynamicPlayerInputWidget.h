#pragma once
// HC28 runtime-field-only mirror. Steam25191271 / UE5.8 / EXE747DC255...F58B.
// Get-VoyageAssetJson stock horizontal widget + current mapping confirm parent
// and names. An exact-identity editor-only Blueprint stand-in may serialize
// tagged ContextAsset/filter INSTANCE values in an owned HUD. Never author or
// ship its CDO, native behavior, indicator class, or internal stock widget tree.
#include "VoyageBaseUserWidget.h"
#include "VoyageInputContextAsset.h"
#include "VoyageDynamicPlayerInputWidget.generated.h"

UCLASS(Abstract, BlueprintType)
class VOYAGE_API UVoyageDynamicPlayerInputWidget : public UVoyageBaseUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite) TObjectPtr<UVoyageInputContextAsset> ContextAsset;
    UPROPERTY(BlueprintReadWrite) bool bFilterByActionType;
};
