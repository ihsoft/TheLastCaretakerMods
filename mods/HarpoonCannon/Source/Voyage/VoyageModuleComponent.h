// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../GAME_DERIVED_SOURCES.md for prefix limits and fingerprint renewal gate.
// EDITOR-ONLY IDENTITY MIRROR for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ModuleConfigData.h"
#include "VoyageModuleComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Partial native schema. The shell writes named tags; never cook this
    // mirror with unversioned property indices.
    UPROPERTY()
    int32 CachedGridID = 0;

    UPROPERTY()
    bool bIsMasterModule = false;

    UPROPERTY()
    bool bDynamicTickInterval = false;

    UPROPERTY()
    bool bRequirementsMet = false;

    UPROPERTY()
    bool bIgnorePowerOffVolumes = false;

    UPROPERTY()
    bool bRegisterModule = false;

    UPROPERTY()
    TObjectPtr<UObject> ModuleConfigData;

    UPROPERTY()
    TObjectPtr<UObject> ItemAsset;

    UPROPERTY() FModuleConfigData ConfigData;

    // Exact native owner; partial schema is only valid with tagged cooking.
    UPROPERTY()
    bool bUseSocketCustomTarget = false;

    UPROPERTY()
    FComponentReference SocketCustomTarget;

    // Exact native function identities used by stock active modules. The
    // editor mirror only exposes the signatures needed by the generated
    // Blueprint; Voyage supplies the runtime implementations.
    UFUNCTION(BlueprintCallable)
    bool HasPower() const { return false; }

};
