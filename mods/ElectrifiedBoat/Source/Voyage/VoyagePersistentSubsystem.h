// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Delegate metadata was recovered from the current shipping executable.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/SceneComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoyagePersistentSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FVoyagePersistentSubsystemActorAttached,
    AActor*, Child,
    USceneComponent*, ParentComponent);

UCLASS(BlueprintType)
class VOYAGE_API UVoyagePersistentSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FVoyagePersistentSubsystemActorAttached OnActorAttached;
};
