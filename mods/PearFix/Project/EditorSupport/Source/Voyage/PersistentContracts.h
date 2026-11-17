#pragma once
// Editor-only reflection contracts, Steam 25982825. Runtime implementation is
// supplied by Voyage. Restored/Attached identities follow the repository's
// inspected persistence contracts; no native module enters the release.
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "PersistentContracts.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorRestoredPersistentSubsystemDelegate, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoyagePersistentSubsystemActorAttached, AActor*, Child, USceneComponent*, ParentComponent);
UCLASS(BlueprintType)
class VOYAGE_API UVoyagePersistentSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="Persistence") FOnActorRestoredPersistentSubsystemDelegate OnActorRestored;
    UPROPERTY(BlueprintAssignable, Category="Persistence") FVoyagePersistentSubsystemActorAttached OnActorAttached;
};
