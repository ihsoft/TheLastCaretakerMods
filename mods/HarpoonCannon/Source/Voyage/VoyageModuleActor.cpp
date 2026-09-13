// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../GAME_DERIVED_SOURCES.md for prefix limits and fingerprint renewal gate.
// EDITOR-ONLY native default-subobject contract for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#include "VoyageModuleActor.h"

#include "VoyageDestructibleObjectComponent.h"
#include "VoyageModuleComponent.h"
#include "VoyagePersistentActorComponent.h"

namespace VoyageModuleActorNames
{
const FName ModuleComponentName(TEXT("ModuleComponent"));
const FName PersistentComponentName(TEXT("PersistentComponent"));
const FName DestructibleObjectComponentName(TEXT("DestructibleObjectComponent"));
}

AVoyageModuleActor::AVoyageModuleActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bCollectable = false;
    ModuleComponent = CreateDefaultSubobject<UVoyageModuleComponent>(
        VoyageModuleActorNames::ModuleComponentName);
    PersistentComponent = CreateDefaultSubobject<UVoyagePersistentActorComponent>(
        VoyageModuleActorNames::PersistentComponentName);
    DestructibleObjectComponent = CreateDefaultSubobject<UVoyageDestructibleObjectComponent>(
        VoyageModuleActorNames::DestructibleObjectComponentName);
}
