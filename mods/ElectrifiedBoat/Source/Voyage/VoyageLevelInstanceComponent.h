// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Runtime implementation and the weak-object array layout are supplied by Voyage.

#pragma once

#include "Components/SceneComponent.h"
#include "VoyageLevelInstanceComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageLevelInstanceComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> OwnedActors;
};

VOYAGE_API void ExposeVoyageLevelInstanceOwnedActorsToBlueprint();
