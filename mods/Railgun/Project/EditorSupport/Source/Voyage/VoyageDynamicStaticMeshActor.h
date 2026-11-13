#pragma once

// READ-ONLY editor mirror: Steam 25191271 / UE5.8.
// Exact stock BP_DynamicMeshActor default-subobject carrier only. Revalidate on
// Steam build ID change. Never spawn or ship this native mirror.
#include "VoyageDynamicMeshActor.h"
#include "VoyageFastSceneComponent.h"
#include "VoyageDynamicStaticMeshActor.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageDynamicStaticMeshActor : public AVoyageDynamicMeshActor
{
    GENERATED_BODY()
public:
    AVoyageDynamicStaticMeshActor();
};
