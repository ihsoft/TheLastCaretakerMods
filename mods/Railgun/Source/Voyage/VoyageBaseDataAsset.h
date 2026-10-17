#pragma once
// READ-ONLY editor prefix: Steam25191271 / UE5.8.
// Public mappings: Type0, Name1, Description2. Railgun serializes only named
// tagged properties from this reviewed prefix.
// ../../ARCHITECTURE.md owns revalidation on Steam build ID change.
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
    UPROPERTY(BlueprintReadOnly, Category="RailgunReadOnly") FText Name;
    UPROPERTY(BlueprintReadOnly, Category="RailgunReadOnly") FText Description;
};
