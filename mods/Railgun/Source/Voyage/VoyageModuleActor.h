// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../ARCHITECTURE.md for prefix limits and Steam build change gate.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractiveInterface.h"
#include "PersistentInterface.h"
#include "VoyageModuleActor.generated.h"

class UVoyageDestructibleObjectComponent;
class UVoyageModuleComponent;
class UVoyagePersistentActorComponent;

UCLASS(Blueprintable)
class VOYAGE_API AVoyageModuleActor : public AActor, public IInteractiveInterface,
    public IPersistentInterface
{
    GENERATED_BODY()

public:
    AVoyageModuleActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
    bool bCollectable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module")
    TObjectPtr<UVoyageModuleComponent> ModuleComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
    TObjectPtr<UVoyagePersistentActorComponent> PersistentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destruction")
    TObjectPtr<UVoyageDestructibleObjectComponent> DestructibleObjectComponent;
};
