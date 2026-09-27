// SHELL-ONLY mirror revalidated for Steam25191271 / UE5.8 parser target.
// Executable 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: public mappings plus stock Sniper Rod JSON. Railgun serializes only
// the named tagged fields declared below. Revalidate on fingerprint change.

#pragma once

#include "VoyageBaseDataAsset.h"
#include "VoyageCombatSubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "VoyageItem.generated.h"

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
    Ammo = 9
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

    UPROPERTY() TObjectPtr<UObject> AbilityActivationSound;
    UPROPERTY() TObjectPtr<UObject> ImpactSound;
    UPROPERTY() TObjectPtr<UObject> ImpactMetaSound;
    UPROPERTY() TObjectPtr<UObject> ActivationSystem;
    UPROPERTY() TObjectPtr<UObject> ImpactSystem;
};

USTRUCT()
struct VOYAGE_API FVoyageWeaponDataStruct
{
    GENERATED_BODY()

    UPROPERTY() FVoyageAttack Attack;
    UPROPERTY() TSoftClassPtr<AActor> ProjectileClass;
    UPROPERTY() TSoftObjectPtr<UStaticMesh> ProjectilePreviewMesh;
    UPROPERTY() FVector LinearRecoilMin = FVector::ZeroVector;
    UPROPERTY() FVector LinearRecoilMax = FVector::ZeroVector;
    UPROPERTY() FVector AngularRecoilMin = FVector::ZeroVector;
    UPROPERTY() FVector AngularRecoilMax = FVector::ZeroVector;
    UPROPERTY() TObjectPtr<AActor> ProjectileTemplate;
    UPROPERTY() float ActivationTime = 0.0f;
    UPROPERTY() FVoyageAbilitySfxData SfxData;
};

USTRUCT()
struct VOYAGE_API FVoyageItemDropVariation
{
    GENERATED_BODY()

    UPROPERTY() TSoftObjectPtr<UObject> RenderAsset;
    UPROPERTY() TSoftObjectPtr<UObject> PhysicalMaterialOverride;
    UPROPERTY() TMap<FName, TSoftObjectPtr<UObject>> MaterialOverridesBySlotName;
    UPROPERTY() TMap<int32, TSoftObjectPtr<UObject>> MaterialOverridesBySlotIndex;
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
    // Read by the pickup observer; native defaults/instances remain game-owned.
    UPROPERTY(BlueprintReadOnly) bool bIsInteractive = false;
    UPROPERTY(BlueprintReadOnly) bool bSupportInventory = false;
    UPROPERTY() TObjectPtr<UTexture2D> Icon;
    UPROPERTY() TObjectPtr<UTexture2D> SecondaryIcon;
    UPROPERTY() EVoyageItemCategory Category = EVoyageItemCategory::Undefined;
    UPROPERTY() TObjectPtr<UVoyageItemCategoryAsset> CategoryAsset;
    UPROPERTY() EVoyageItemQuality Quality = EVoyageItemQuality::Poor;
    UPROPERTY() int32 MaxStackCount = 0;
    UPROPERTY() float Weight = 0.0f;
    UPROPERTY() float CraftTime = 0.0f;
    UPROPERTY() float CraftElectricityCost = 0.0f;
    UPROPERTY() int32 CraftAmount = 0;
    UPROPERTY() uint8 CraftFilter = 0;
    UPROPERTY() TMap<UVoyageItem*, int32> Components;
    UPROPERTY() TArray<FVoyageItemDropVariation> DropVariations;
    UPROPERTY() TSoftClassPtr<AActor> DroppedActor;
    UPROPERTY() float ScalePerItem = VoyageItemMirror::ScalePerItemSerializationSentinel;
    UPROPERTY() int32 MaxDropCount = 0;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemMaterial : public UVoyageItem
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemAmmo : public UVoyageItem
{
    GENERATED_BODY()

public:
    UPROPERTY() float Caliber = 0.0f;
    UPROPERTY() FVoyageWeaponDataStruct WeaponData;
};
