#pragma once
// HC28 runtime-field-only mirror. Steam25191271 / UE5.8 / EXE747DC255...F58B.
// Get-VoyageAssetJson stock horizontal widget + current mapping confirm parent
// and names. Never author native widget/CDO or serialize these partial fields.
// Load the unchanged STOCK Blueprint and configure before adding to own HUD.
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
