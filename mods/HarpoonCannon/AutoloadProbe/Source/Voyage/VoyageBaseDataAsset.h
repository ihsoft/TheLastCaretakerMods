#pragma once
// READ-ONLY editor prefix: Steam25191271 / UE5.8, executable
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Public mappings: Type0, Name1; never instantiate/serialize this stub.
// GAME_DERIVED_SOURCES.md owns revalidation on fingerprint change.
// Runtime module identity is required during cook; the game supplies this class.
#include "Engine/DataAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "VoyageBaseDataAsset.generated.h"
UCLASS(BlueprintType)
class VOYAGE_API UVoyageBaseDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY() FPrimaryAssetType Type;
    UPROPERTY(BlueprintReadOnly, Category="HarpoonReadOnly") FText Name;
};
