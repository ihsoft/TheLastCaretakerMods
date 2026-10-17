#pragma once
// READ-ONLY editor mirror: Steam25827816 / UE5.8 parser target.
// Current reviewed mappings and the stock WindTurbine Small component contract.
// Named fields consumed by the generator only; no instance, native code or data
// asset shipped.
// Revalidate after Steam build ID change. Never use this stub to serialize a module.
// Module descriptor is Runtime so UHT omits PKG_EditorOnly; editor usage only.
// and the release inventory must still exclude every native binary.
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ModuleConfigData.h"
#include "ModuleResourceType.h"
#include "VoyageModuleComponent.generated.h"
class UVoyageBaseInventoryComponent;
class UVoyageModuleComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoyageModuleCompDelegate,
    UVoyageModuleComponent*, Module);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FVoyageModuleCompPowerStateChanged, UVoyageModuleComponent*, SourceModule,
    bool, bHasPower);
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
    UPROPERTY(BlueprintReadOnly, Category="RailgunReadOnly") TObjectPtr<UObject> ItemAsset;
    UPROPERTY() FModuleConfigData ConfigData;
    UPROPERTY(BlueprintAssignable, Category="RailgunEvents")
    FVoyageModuleCompDelegate OnModuleValueChanged;
    UPROPERTY(BlueprintAssignable, Category="RailgunEvents")
    FVoyageModuleCompDelegate OnModuleSocketConnectionChanged;
    UPROPERTY(BlueprintAssignable, Category="RailgunEvents")
    FVoyageModuleCompPowerStateChanged OnModulePowerStateChanged;

    // Steam25827816 native registration/thunk contract.
    // Read-only methods; module lifecycle/consumption is not ActorComponent activation.
    UFUNCTION(BlueprintPure, Category="RailgunReadOnly")
    bool HasPower() const { return false; }

    UFUNCTION(BlueprintPure, Category="RailgunReadOnly")
    bool HasSocketConnection() const { return false; }

    UFUNCTION(BlueprintPure, Category="RailgunReadOnly")
    double GetResourceAmount(EModuleResourceType Type) const { return 0; }

    UFUNCTION(BlueprintPure, Category="RailgunInventory")
    UVoyageBaseInventoryComponent* GetInternalInventory() const { return nullptr; }

    UFUNCTION(BlueprintCallable, Category="RailgunEnergy")
    bool RemoveResource(EModuleResourceType Type, double RemoveAmount, EModuleResourceRemovalType RemovalType) { return false; }

    // Returns the amount accepted after native capacity correction.
    UFUNCTION(BlueprintCallable, Category="RailgunEnergy")
    double AddResource(EModuleResourceType Type, double AddAmount) { return 0; }

    UFUNCTION(BlueprintCallable, Category="RailgunEnergy")
    void SetCustomConsumption(double InAcceptanceFilter, double InMaxResourceAmount, double InConsumptionON) {}
};
