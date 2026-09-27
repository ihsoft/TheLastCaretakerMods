// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: reviewed mappings plus the DieselGenerators v5.5 fabricator-component
// defaults. Railgun references only the three named tagged properties below.
// Revalidate on fingerprint change.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageItem.h"
#include "VoyageFabricatorComponent.generated.h"

// Read-only queue shapes from the reviewed Steam 25191271 mapping. The game
// owns these structs; this mirror exists only while compiling the probe graph.
USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageFabricatorQueueItem
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UVoyageItem> Item;
    UPROPERTY(BlueprintReadOnly) int32 Count = 0;
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageItemData
{
    GENERATED_BODY()
    // Bytecode-only MakeStruct writes the named count field. Never serialize
    // this partial mirror as a struct default/CDO or assume its native size.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ItemCount = 0;
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageItemSerialize
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FPrimaryAssetType AssetType;
    UPROPERTY(BlueprintReadOnly) FName AssetName;
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<UVoyageItem> Item;
    UPROPERTY(BlueprintReadOnly) FVoyageItemData Data;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageFabricatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite) TMap<UVoyageItem*, UVoyageItem*> AvailablePatternsCurrent;
    UPROPERTY(BlueprintReadWrite) TSet<UVoyageItem*> AvailablePatternsInternal;
    UPROPERTY(BlueprintReadWrite) TSet<UVoyageItem*> AvailablePatterns;
    UPROPERTY(BlueprintReadOnly) TArray<FVoyageFabricatorQueueItem> ItemQueue;
    UPROPERTY(BlueprintReadOnly) TArray<FVoyageItemSerialize> OutputQueue;
};
