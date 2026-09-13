// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings + stock CDO/default-subobject JSON; see
// ../../GAME_DERIVED_SOURCES.md for prefix limits and fingerprint renewal gate.
// EDITOR-ONLY IDENTITY MIRROR for Steam build 23962331.
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageModuleComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Exact unversioned-property prefix through ItemAsset. Properties after
    // ItemAsset remain intentionally absent because this probe serializes only
    // the item reference at property index 7.
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
};
