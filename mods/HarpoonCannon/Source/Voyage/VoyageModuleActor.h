// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../GAME_DERIVED_SOURCES.md for prefix limits and fingerprint renewal gate.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoyageModuleActor.generated.h"

class UVoyageDestructibleObjectComponent;
class UVoyageModuleComponent;
class UVoyagePersistentActorComponent;

UCLASS(Blueprintable)
class VOYAGE_API AVoyageModuleActor : public AActor
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
