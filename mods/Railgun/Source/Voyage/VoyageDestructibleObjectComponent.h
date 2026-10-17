// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../ARCHITECTURE.md for prefix limits and Steam build change gate.
// EDITOR-ONLY IDENTITY MIRROR for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageDestructibleObjectComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageDestructibleObjectComponent : public UActorComponent
{
    GENERATED_BODY()
};
