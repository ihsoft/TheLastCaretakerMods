// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../GAME_DERIVED_SOURCES.md for prefix limits and fingerprint renewal gate.
// EDITOR-ONLY class identity mirrors for Steam build 23962331.
// The generated reference stub is never cooked or shipped; Voyage supplies the
// real data asset and class hierarchy at runtime.

#pragma once

#include "Engine/DataAsset.h"
#include "VoyageItem.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API UVoyageBaseDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemBase : public UVoyageBaseDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItem : public UVoyageItemBase
{
    GENERATED_BODY()
};
