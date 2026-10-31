// Editor-only serialization mirrors from the reviewed Steam 25982825 schema.
// All declared slots are required by unversioned cooking. Native code is not shipped.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "VoyageItemResourceType.h"
#include "VoyageItemMirrors.generated.h"
UENUM()
enum class EVoyageItemTargetInventory : uint8
{
    Inventory = 0,
    Vault = 1,
    EVoyageItemTargetInventory_MAX = 2
};

UENUM()
enum class EVoyageInventoryType : uint8
{
    Undefined = 0,
    PlayerInventory = 1,
    PlayerVault = 2,
    Merchant_DEPRECATEAD = 3,
    Container = 4,
    Hidden = 5,
    EVoyageInventoryType_MAX = 6
};

UENUM()
enum class EVoyageItemCategory : uint8
{
    Undefined = 0,
    Seed = 1,
    Weapon = 2,
    FabricatorPattern_DEPRECATED = 3,
    GardenPattern_DEPRECATED = 4,
    Organic = 5,
    FoodMaterial = 6,
    HumanObject_DEPRECATED = 7,
    MultiToolBuilderPattern_DEPRECATED = 8,
    Ammo = 9,
    EmptyContainer_DEPRECATED = 10,
    Grenade = 11,
    Armor = 12,
    Cable = 13,
    Consumable = 14,
    Backpack = 15,
    Item = 16,
    Magazine = 17,
    Module = 18,
    Part = 19,
    Equipment = 20,
    Attachment = 21,
    Material = 22,
    InventoryContainer = 23,
    Food = 24,
    Memory = 25,
    Paint = 26,
    Decoration = 27,
    Trash = 28,
    Cartridge = 29,
    Quest = 30,
    EVoyageItemCategory_MAX = 31
};

UENUM()
enum class EVoyageItemQuality : uint8
{
    Poor = 0,
    Common = 1,
    Uncommon = 2,
    Rare = 3,
    Artifact = 4,
    Count = 5,
    EVoyageItemQuality_MAX = 6
};

UENUM()
enum class EVoyageItemOverlapEventsOverride : uint8
{
    Default = 0,
    Enabled = 1,
    Disabled = 2,
    EVoyageItemOverlapEventsOverride_MAX = 3
};

UENUM()
enum class EVoyageItemAction : uint8
{
    Default = 0,
    NoAction = 1,
    EVoyageItemAction_MAX = 2
};


UENUM()
enum class FVoyageLootTableSelector : uint8
{
    Single = 0,
    All = 1,
    FVoyageLootTableSelector_MAX = 2
};

UENUM()
enum class EVoyageItemDropVariationType : uint8
{
    Default = 0,
    Random = 1,
    ItemCount = 2,
    EVoyageItemDropVariationType_MAX = 3
};

UENUM()
enum class EVoyageHumanPropertyModifierType : uint8
{
    Multiplicative = 0,
    Additive = 1,
    EVoyageHumanPropertyModifierType_MAX = 2
};

USTRUCT()
struct VOYAGE_API FVoyageGameVersion
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") int32 MajorVersion{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 MinorVersion{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 PatchVersion{};
};

USTRUCT()
struct VOYAGE_API FVoyageGameVersionRange
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageGameVersion InitialVersion{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bHasSunsetVersion{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageGameVersion SunsetVersion{};
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class VOYAGE_API UVoyageBaseDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") FPrimaryAssetType Type{};
    UPROPERTY(EditAnywhere, Category="Mirror") FText Name{};
    UPROPERTY(EditAnywhere, Category="Mirror") FText Description{};
    UPROPERTY(EditAnywhere, Category="Mirror") FText DescriptionFooter{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageGameVersionRange VersionRange{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bExcludeFromDistribution{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bExcludeFromDemo{};
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class VOYAGE_API UVoyageItemBase : public UVoyageBaseDataAsset
{
    GENERATED_BODY()
public:

};

USTRUCT()
struct VOYAGE_API FVoyageLootItemTables
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, float> ItemTables{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageLootTableSelector Selector{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 SpawnCountMinimum{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 SpawnCountMaximum{};
};

USTRUCT()
struct VOYAGE_API FVoyageItemObjectArray
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") TArray<TSoftObjectPtr<UObject>> Data{};
};

USTRUCT()
struct VOYAGE_API FVoyageItemDropVariation
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> RenderAsset{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> PhysicalMaterialOverride{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<FName, TSoftObjectPtr<UObject>> MaterialOverridesBySlotName{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<int32, TSoftObjectPtr<UObject>> MaterialOverridesBySlotIndex{};
};

USTRUCT()
struct VOYAGE_API FVoyageItemSpawnSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") FVector LinearVelocity{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVector AngularVelocity{};
    UPROPERTY(EditAnywhere, Category="Mirror") float AutoCollectRadius{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bForceAutoCollect{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bResolveOverlaps{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bAwake{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bSimulatePhysics{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVector BoxHalfExtents{};
};

USTRUCT()
struct VOYAGE_API FVoyageContainerItemTables
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> ContainerItem{};
    UPROPERTY(EditAnywhere, Category="Mirror") float SpawnChance{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageLootItemTables ItemTables{};
};

USTRUCT()
struct VOYAGE_API FVoyageDestructibleObjectProperties
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") float Health{};
    UPROPERTY(EditAnywhere, Category="Mirror") float DestroyedLootMultiplier{};
    UPROPERTY(EditAnywhere, Category="Mirror") float DirectDamageMultiplier{};
    UPROPERTY(EditAnywhere, Category="Mirror") float CollisionDamageMultiplier{};
    UPROPERTY(EditAnywhere, Category="Mirror") float CollisionDamageThreshold{};
    UPROPERTY(EditAnywhere, Category="Mirror") float DestructionDamageThreshold{};
    UPROPERTY(EditAnywhere, Category="Mirror") float DestructionExtraDamageTotal{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> ItemToSpawn{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 ItemToSpawnCount{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageItemSpawnSettings ItemToSpawnSettings{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageContainerItemTables DestructionLoot{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageItemSpawnSettings DestructionLootSpawnSettings{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DestructionLootEffect{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> ActorToSpawn{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DestroyedActor{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVector DestroyedActorScale{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DestroyedGeometryCollection{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DestructionAudio{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DestructionEffect{};
    UPROPERTY(EditAnywhere, Category="Mirror") FTransform DestructionEffectTransform{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool SpawnGenericRecycleEffect{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> DestructionWeaponData{};
    UPROPERTY(EditAnywhere, Category="Mirror") FTransform DestructionWeaponTransform{};
};

USTRUCT()
struct VOYAGE_API FVoyageHumanPropertyModifier
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") float ValueMax{};
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageHumanPropertyModifierType Type{};
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class VOYAGE_API UVoyageItem : public UVoyageItemBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageItemTargetInventory DesiredInventory{};
    UPROPERTY(EditAnywhere, Category="Mirror") TArray<EVoyageInventoryType> BlockedInventoryTypes{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bLootAsPattern{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bOnlyAllowDesiredInventory{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> Icon{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> SecondaryIcon{};
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageItemCategory Category{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> CategoryAsset{};
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageItemQuality Quality{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsPhysicsObject{};
    UPROPERTY(EditAnywhere, Category="Mirror") float AutoCombineRange{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsAwake{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bAutoWeld{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bAutoweldIgnoreNormal{};
    UPROPERTY(EditAnywhere, Category="Mirror") float LinearDamping{};
    UPROPERTY(EditAnywhere, Category="Mirror") float AngularDamping{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<ECollisionResponse>> MeshCollisionReponses{};
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageItemOverlapEventsOverride GenerateOverlapEvents{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsInteractive{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bLootAsItemTables{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bSupportInventory{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bAllowFabricationOnTop{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsDismantlable{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsRecyclable = true; // Sentinel retains the stock explicit false during cook.
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsRepairable{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bCanGainLootExp{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 MaxStackCount{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 MaxCount{};
    UPROPERTY(EditAnywhere, Category="Mirror") float Weight{};
    UPROPERTY(EditAnywhere, Category="Mirror") float Buoyancy{};
    UPROPERTY(EditAnywhere, Category="Mirror") float Lifetime{};
    UPROPERTY(EditAnywhere, Category="Mirror") float LifeTimeBuoyancy{};
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageItemAction DefaultDropBehaviour{};
    UPROPERTY(EditAnywhere, Category="Mirror") FName Tag{};
    UPROPERTY(EditAnywhere, Category="Mirror") TArray<TObjectPtr<UObject>> Logic{};
    UPROPERTY(EditAnywhere, Category="Mirror") float CraftTime{};
    UPROPERTY(EditAnywhere, Category="Mirror") float CraftElectricityCost{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 CraftAmount{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bAllowPlacement{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bCustomCraftInitialRotation{};
    UPROPERTY(EditAnywhere, Category="Mirror") FRotator CraftInitialRotation{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bCraftSocketVisualization{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<EModuleResourceType, float> CraftResourceRequirements{};
    UPROPERTY(EditAnywhere, Category="Mirror") uint8 CraftFilter{};
    UPROPERTY(EditAnywhere, Category="Mirror", Instanced) TObjectPtr<UObject> CraftingLocationGenerator{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool IsScannable{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 ScanLevelRequirement{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bUnlockAsBuildable{};
    UPROPERTY(EditAnywhere, Category="Mirror") TArray<TObjectPtr<UObject>> ScanLogics{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, int32> Components{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> DamagedItem{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool DurabilityAffectsDismantleMultiplier{};
    UPROPERTY(EditAnywhere, Category="Mirror") float DismantleDuration{};
    UPROPERTY(EditAnywhere, Category="Mirror") float DismantleElectricityCost{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, int32> RepairComponents{};
    UPROPERTY(EditAnywhere, Category="Mirror") float RepairDuration{};
    UPROPERTY(EditAnywhere, Category="Mirror") float RepairElectricityCost{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageLootItemTables LootItemTables{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<FString, FVoyageLootItemTables> ItemTables{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DroppedMesh{};
    UPROPERTY(EditAnywhere, Category="Mirror") TArray<TSoftObjectPtr<UObject>> DroppedMeshMaterialOverrides{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<FString, FVoyageItemObjectArray> DroppedActorData{};
    UPROPERTY(EditAnywhere, Category="Mirror") TArray<FVoyageItemDropVariation> DropVariations{};
    UPROPERTY(EditAnywhere, Category="Mirror") EVoyageItemDropVariationType DropVariationType{};
    UPROPERTY(EditAnywhere, Category="Mirror") TSoftObjectPtr<UObject> DroppedActor{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bUseCCD{};
    UPROPERTY(EditAnywhere, Category="Mirror") float MinimumDropScale{};
    UPROPERTY(EditAnywhere, Category="Mirror") float MaximumDropScale{};
    UPROPERTY(EditAnywhere, Category="Mirror") float ScalePerItem{};
    UPROPERTY(EditAnywhere, Category="Mirror") int32 MaxDropCount{};
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> LootSound{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool bIsDestructible{};
    UPROPERTY(EditAnywhere, Category="Mirror") FVoyageDestructibleObjectProperties DestructibleObjectProperties{};
    UPROPERTY(EditAnywhere, Category="Mirror") float ProcessTime{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, FVoyageHumanPropertyModifier> PropertyModifiers{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, float> MemoryTraits{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, float> TraitGroups{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<TObjectPtr<UObject>, float> HumanModifiers{};
    UPROPERTY(EditAnywhere, Category="Mirror") bool ConsumedInHumanGrowingProcess{};
    UPROPERTY(EditAnywhere, Category="Mirror") TMap<FName, TObjectPtr<UObject>> properties{};
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class VOYAGE_API UVoyageCraftingLocationGenerator : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Mirror") TObjectPtr<UObject> MyInstigator{};
};

UCLASS()
class VOYAGE_API UVoyageItemPart : public UVoyageItem { GENERATED_BODY() };

UCLASS()
class VOYAGE_API UVoyageItemMaterial : public UVoyageItem { GENERATED_BODY() };

UCLASS()
class VOYAGE_API UVoyageItemCategoryAsset : public UVoyageBaseDataAsset { GENERATED_BODY() };
