#pragma once

// READ-ONLY editor mirror: Steam 25191271 / UE5.8, executable SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Exact stock BP_DynamicMeshActor default-subobject carrier only. Revalidate on
// fingerprint change. Never spawn or ship this native mirror.
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
