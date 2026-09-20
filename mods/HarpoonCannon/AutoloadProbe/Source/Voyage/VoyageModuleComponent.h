#pragma once
// READ-ONLY editor mirror: Steam25191271 / UE5.8 parser target, executable
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Public mappings + three shark stock CDOs, GAME_DERIVED_SOURCES.md.
// Exact prefix through ItemAsset7; no instance, native code or data asset shipped.
// Revalidate after fingerprint change. Never use this stub to serialize a module.
// Module descriptor is Runtime so UHT omits PKG_EditorOnly; editor usage only,
// and the release inventory must still exclude every native binary.
#include "Components/ActorComponent.h"
#include "ModuleResourceType.h"
#include "VoyageModuleComponent.generated.h"
UCLASS(BlueprintType)
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 CachedGridID = 0;
    UPROPERTY() bool bIsMasterModule = false;
    UPROPERTY() bool bDynamicTickInterval = false;
    UPROPERTY() bool bRequirementsMet = false;
    UPROPERTY() bool bIgnorePowerOffVolumes = false;
    UPROPERTY() bool bRegisterModule = false;
    UPROPERTY() TObjectPtr<UObject> ModuleConfigData;
    UPROPERTY(BlueprintReadOnly, Category="HarpoonReadOnly") TObjectPtr<UObject> ItemAsset;

    // Steam25191271 native registration/thunk and stock FoodProcessor call.
    // Read-only methods; module lifecycle/consumption is not ActorComponent activation.
    UFUNCTION(BlueprintPure, Category="HarpoonReadOnly")
    bool HasPower() const { return false; }

    UFUNCTION(BlueprintPure, Category="HarpoonReadOnly")
    bool HasSocketConnection() const { return false; }

    UFUNCTION(BlueprintPure, Category="HarpoonReadOnly")
    double GetResourceAmount(EModuleResourceType Type) const { return 0; }

    UFUNCTION(BlueprintCallable, Category="HarpoonEnergy")
    bool RemoveResource(EModuleResourceType Type, double RemoveAmount, EModuleResourceRemovalType RemovalType) { return false; }

    UFUNCTION(BlueprintCallable, Category="HarpoonEnergy")
    void SetCustomConsumption(double InAcceptanceFilter, double InMaxResourceAmount, double InConsumptionON) {}
};
