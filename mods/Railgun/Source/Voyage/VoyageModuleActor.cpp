// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../ARCHITECTURE.md for prefix limits and Steam build change gate.
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
