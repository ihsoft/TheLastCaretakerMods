// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../ARCHITECTURE.md for prefix limits and Steam build change gate.
// EDITOR-ONLY IDENTITY MIRROR for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageDynamicCollisionComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageDynamicCollisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Collision")
    bool bAutoWeld = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Collision")
    bool bAutoweldIgnoreNormal = false;
};
