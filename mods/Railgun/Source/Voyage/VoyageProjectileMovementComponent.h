#pragma once
// Editor-only identity mirror. Steam25191271, UE5.8.
// Parent/properties from reviewed mappings; revalidate on Steam build ID change.
// Own fields are written by named Blueprint bytecode, never serialized defaults.
#include "GameFramework/ProjectileMovementComponent.h"
#include "VoyageProjectileMovementComponent.generated.h"
UCLASS(meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageProjectileMovementComponent : public UProjectileMovementComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite) bool bAllowPenetration;
    UPROPERTY(BlueprintReadWrite) bool bAllowRicochet;
};
