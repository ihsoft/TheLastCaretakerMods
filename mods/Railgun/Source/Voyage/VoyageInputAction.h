#pragma once
// TYPE-ONLY mirror: Steam25191271, UE5.8,
// Inspect-VoyageAsset mappings:VoyageInputAction confirms InputAction parent.
// Used only for runtime cast/reference to stock action; NEVER author its CDO/assets.
// Revalidate mapping, asset identity and native field type after Steam build ID change.
#include "InputAction.h"
#include "VoyageInputAction.generated.h"
UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputAction : public UInputAction
{
    GENERATED_BODY()
};
