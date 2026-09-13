#pragma once
// Steam25191271 / UE5.8; exact fingerprint and method in GAME_DERIVED_SOURCES.md.
// Identity-only SceneComponent mirror for TAGGED SCS authoring. The game owns
// all 20 native properties and constructor defaults (InteractType=Direct).
// Never serialize guessed native fields or use this as a complete native ABI.
#include "Components/SceneComponent.h"
#include "InteractiveObjectComponent.generated.h"
UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UInteractiveObjectComponent : public USceneComponent
{
    GENERATED_BODY()
};
