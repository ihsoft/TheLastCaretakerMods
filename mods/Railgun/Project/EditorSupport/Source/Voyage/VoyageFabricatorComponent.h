// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: reviewed mappings plus the DieselGenerators v5.5 fabricator-component
// defaults. Railgun references only the three named tagged properties below.
// Revalidate on Steam build ID change.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageItem.h"
#include "VoyageFabricatorComponent.generated.h"

// Read-only queue shapes from the reviewed Steam 25191271 mapping. The game
// owns these structs; this mirror supplies editor compilation contracts only.
USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageFabricatorQueueItem
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="VoyageFabricatorQueueItem") TObjectPtr<UVoyageItem> Item;
    UPROPERTY(BlueprintReadOnly, Category="VoyageFabricatorQueueItem") int32 Count = 0;
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageItemData
{
    GENERATED_BODY()
    // Bytecode-only MakeStruct writes the named count field. Never serialize
    // this partial mirror as a struct default/CDO or assume its native size.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageItemData") int32 ItemCount = 0;
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageItemSerialize
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="VoyageItemSerialize") FPrimaryAssetType AssetType;
    UPROPERTY(BlueprintReadOnly, Category="VoyageItemSerialize") FName AssetName;
    UPROPERTY(BlueprintReadOnly, Category="VoyageItemSerialize") TWeakObjectPtr<UVoyageItem> Item;
    UPROPERTY(BlueprintReadOnly, Category="VoyageItemSerialize") FVoyageItemData Data;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageFabricatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category="VoyageFabricatorComponent") TMap<UVoyageItem*, UVoyageItem*> AvailablePatternsCurrent;
    UPROPERTY(BlueprintReadWrite, Category="VoyageFabricatorComponent") TSet<UVoyageItem*> AvailablePatternsInternal;
    UPROPERTY(BlueprintReadWrite, Category="VoyageFabricatorComponent") TSet<UVoyageItem*> AvailablePatterns;
    UPROPERTY(BlueprintReadOnly, Category="VoyageFabricatorComponent") TArray<FVoyageFabricatorQueueItem> ItemQueue;
    UPROPERTY(BlueprintReadOnly, Category="VoyageFabricatorComponent") TArray<FVoyageItemSerialize> OutputQueue;
};
