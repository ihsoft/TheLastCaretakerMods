#pragma once
// TYPE-ONLY mirror: Steam25191271, UE5.8,
// EXE747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Inspect-VoyageAsset mappings:VoyageInputAction confirms InputAction parent.
// Used only for runtime cast/reference to stock action; NEVER author its CDO/assets.
// Revalidate mapping, asset identity and native field type after fingerprint change.
#include "InputAction.h"
#include "VoyageInputAction.generated.h"
UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputAction : public UInputAction
{
    GENERATED_BODY()
};
