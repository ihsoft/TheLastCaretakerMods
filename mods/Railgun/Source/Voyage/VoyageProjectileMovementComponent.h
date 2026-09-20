#pragma once
// Editor-only identity mirror. Steam25191271, EXE747DC255...F58B, UE5.8.
// Parent/properties from reviewed mappings; revalidate on fingerprint change.
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
