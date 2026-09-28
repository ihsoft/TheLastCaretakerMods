#pragma once
// Steam25191271 / UE5.8; exact fingerprint and method in GAME_DERIVED_SOURCES.md.
// Identity-only SceneComponent mirror for TAGGED SCS authoring. The game owns
// all 20 native properties and constructor defaults (InteractType=Direct).
// Never serialize guessed native fields or use this as a complete native ABI.
#include "Components/SceneComponent.h"
#include "VoyageOverlayWidgetData.h"
#include "InteractiveObjectComponent.generated.h"

UENUM()
enum class FVoyageInteractType : uint8
{
    Direct = 0,
    WidgetOverlay = 1
};

UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UInteractiveObjectComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY() FVoyageInteractType InteractType = FVoyageInteractType::Direct;
    UPROPERTY() int32 PartId = 0;
    UPROPERTY() TObjectPtr<UVoyageOverlayWidgetData> OverlayWidget;
};
