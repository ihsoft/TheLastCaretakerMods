// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Method: public mappings plus stock Sniper Rod JSON. Railgun serializes only
// the named tagged fields declared below. Revalidate on Steam build ID change.

#pragma once

#include "VoyageBaseDataAsset.h"
#include "VoyageCombatSubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "VoyageItem.generated.h"

// Named tagged fields used by the maintained item; the game owns omitted state.
USTRUCT()
struct VOYAGE_API FVoyageDestructibleObjectProperties
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Voyage") float Health = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") float DestructionDamageThreshold = 0.0f;
};

namespace VoyageItemMirror
{
// Editor-only serialization sentinel. Authored Railgun ammo sets zero; the
// nonzero mirror default forces tagged cook to retain that explicit value.
inline constexpr float ScalePerItemSerializationSentinel = 1.0f;
}

UENUM()
enum class EVoyageItemCategory : uint8
{
    Undefined = 0,
    Ammo = 9,
    Module = 18
};

UENUM()
enum class EVoyageItemQuality : uint8
{
    Poor = 0,
    Common = 1,
    Uncommon = 2,
    Rare = 3,
    Artifact = 4
};

USTRUCT()
struct VOYAGE_API FVoyageAbilitySfxData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UObject> AbilityActivationSound;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UObject> ImpactSound;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UObject> ImpactMetaSound;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UObject> ActivationSystem;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UObject> ImpactSystem;
};

USTRUCT()
struct VOYAGE_API FVoyageWeaponDataStruct
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="Voyage") FVoyageAttack Attack;
    UPROPERTY(EditAnywhere, Category="Voyage") TSoftClassPtr<AActor> ProjectileClass;
    UPROPERTY(EditAnywhere, Category="Voyage") TSoftObjectPtr<UStaticMesh> ProjectilePreviewMesh;
    UPROPERTY(EditAnywhere, Category="Voyage") FVector LinearRecoilMin = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Voyage") FVector LinearRecoilMax = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Voyage") FVector AngularRecoilMin = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Voyage") FVector AngularRecoilMax = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<AActor> ProjectileTemplate;
    UPROPERTY(EditAnywhere, Category="Voyage") float ActivationTime = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") FVoyageAbilitySfxData SfxData;
};

USTRUCT()
struct VOYAGE_API FVoyageItemDropVariation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="Voyage") TSoftObjectPtr<UObject> RenderAsset;
    UPROPERTY(EditAnywhere, Category="Voyage") TSoftObjectPtr<UObject> PhysicalMaterialOverride;
    UPROPERTY(EditAnywhere, Category="Voyage") TMap<FName, TSoftObjectPtr<UObject>> MaterialOverridesBySlotName;
    UPROPERTY(EditAnywhere, Category="Voyage") TMap<int32, TSoftObjectPtr<UObject>> MaterialOverridesBySlotIndex;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemBase : public UVoyageBaseDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemCategoryAsset : public UVoyageBaseDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItem : public UVoyageItemBase
{
    GENERATED_BODY()

public:
    UVoyageItem();
    virtual void Serialize(FArchive& Ar) override;
    // Read by the pickup observer; native defaults/instances remain game-owned.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voyage") bool bIsInteractive = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voyage") bool bSupportInventory = false;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UTexture2D> SecondaryIcon;
    UPROPERTY(EditAnywhere, Category="Voyage") EVoyageItemCategory Category = EVoyageItemCategory::Undefined;
    UPROPERTY(EditAnywhere, Category="Voyage") TObjectPtr<UVoyageItemCategoryAsset> CategoryAsset;
    UPROPERTY(EditAnywhere, Category="Voyage") EVoyageItemQuality Quality = EVoyageItemQuality::Poor;
    UPROPERTY(EditAnywhere, Category="Voyage") int32 MaxStackCount = 0;
    UPROPERTY(EditAnywhere, Category="Voyage") float Weight = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") float CraftTime = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") float CraftElectricityCost = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") int32 CraftAmount = 0;
    UPROPERTY(EditAnywhere, Category="Voyage") uint8 CraftFilter = 0;
    UPROPERTY(EditAnywhere, Category="Voyage") TMap<UVoyageItem*, int32> Components;
    UPROPERTY(EditAnywhere, Category="Voyage") TArray<FVoyageItemDropVariation> DropVariations;
    UPROPERTY(EditAnywhere, Category="Voyage") TSoftClassPtr<AActor> DroppedActor;
    UPROPERTY(EditAnywhere, Category="Voyage") float ScalePerItem = VoyageItemMirror::ScalePerItemSerializationSentinel;
    UPROPERTY(EditAnywhere, Category="Voyage") int32 MaxDropCount = 0;
    UPROPERTY(EditAnywhere, Category="Voyage") bool bIsDismantlable = false;
    UPROPERTY(EditAnywhere, Category="Voyage") bool bIsRepairable = false;
    UPROPERTY(EditAnywhere, Category="Voyage") FName Tag;
    UPROPERTY(EditAnywhere, Category="Voyage") float DismantleDuration = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") float DismantleElectricityCost = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") float RepairDuration = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") float RepairElectricityCost = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") bool bIsDestructible = false;
    UPROPERTY(EditAnywhere, Category="Voyage") FVoyageDestructibleObjectProperties DestructibleObjectProperties;
    UPROPERTY(EditAnywhere, Category="Voyage") TMap<FName, TObjectPtr<UObject>> properties;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemMaterial : public UVoyageItem
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemPart : public UVoyageItem
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemAmmo : public UVoyageItem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="Voyage") float Caliber = 0.0f;
    UPROPERTY(EditAnywhere, Category="Voyage") FVoyageWeaponDataStruct WeaponData;
};
