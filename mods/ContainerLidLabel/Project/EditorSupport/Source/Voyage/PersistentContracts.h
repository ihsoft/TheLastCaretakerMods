#pragma once
// Editor-only reflection mirror, Steam 25982825 / UE5.8. No native DLL ships.
// Public mappings + native FMulticastDelegatePropertyParams establish assignable
// properties and shared signature constructor. Actor parameter follows the
// established OnActorRestoredPersistentSubsystemDelegate signature.
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "Engine/LatentActionManager.h"
#include "PersistentContracts.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorRestoredPersistentSubsystemDelegate, AActor*, Actor);
UCLASS(BlueprintType)
class VOYAGE_API UVoyagePersistentSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="Persistence", BlueprintAssignable) FOnActorRestoredPersistentSubsystemDelegate OnActorSpawned;
    UPROPERTY(Category="Persistence", BlueprintAssignable) FOnActorRestoredPersistentSubsystemDelegate OnActorRestored;

    // Synthetic editor event emitters; never referenced by shipped Blueprints.
    UFUNCTION(BlueprintCallable, Category="Editor Tests") void TestFlushIconLoads()
    {
        FlushAsyncLoading();
        GetWorld()->GetLatentActionManager().BeginFrame();
        GetWorld()->GetLatentActionManager().ProcessLatentActions(nullptr, 0.0f);
    }
    UFUNCTION(BlueprintCallable, Category="Editor Tests") void TestSpawned(AActor* Actor) { OnActorSpawned.Broadcast(Actor); }
    UFUNCTION(BlueprintCallable, Category="Editor Tests") void TestRestored(AActor* Actor) { OnActorRestored.Broadcast(Actor); }
    UFUNCTION(BlueprintCallable, Category="Editor Tests") void TestBeginPlay(AActor* Actor)
    {
        if (!Actor->IsActorInitialized())
        {
            Actor->PreInitializeComponents();
            Actor->InitializeComponents();
            Actor->PostInitializeComponents();
        }
        Actor->DispatchBeginPlay();
    }
    UFUNCTION(BlueprintCallable, Category="Editor Tests") void TestTargetEndPlay(AActor* Actor)
    { Actor->OnEndPlay.Broadcast(Actor, EEndPlayReason::RemovedFromWorld); }
    UFUNCTION(BlueprintPure, Category="Editor Tests") bool TestTargetHasEndPlayBindings(AActor* Actor) const
    { return Actor->OnEndPlay.IsBound(); }
};
