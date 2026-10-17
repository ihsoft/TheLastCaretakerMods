#pragma once
// TYPE/FIELD REFERENCES ONLY. Steam25191271 / UE5.8.
// Public mapping confirms PawnComponent parent and InputContextAsset[0].
// Stock Forklift InputControls export identifies VoyageInputContextAsset type.
// Real Engine ModularGameplay parent; never instantiate/serialize this mirror.
// Revalidate owning schema and stock object types after Steam build ID change.
#include "Components/PawnComponent.h"
#include "VoyageInputContextAsset.h"
#include "VoyageInputControlsComponent.generated.h"
UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputControlsComponent : public UPawnComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UVoyageInputContextAsset> InputContextAsset;
};
