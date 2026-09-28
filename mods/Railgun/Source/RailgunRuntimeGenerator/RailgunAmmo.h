#pragma once
#include "VoyageItem.h"

namespace RailgunAmmo
{
inline constexpr TCHAR AmmoIconPackage[] = TEXT("/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon");
inline constexpr TCHAR AmmoIconAsset[] = TEXT("T_RailgunAmmoIcon");
inline constexpr TCHAR AmmoIconSourceArgument[] = TEXT("AmmoIcon=");
inline constexpr TCHAR GunIconPackage[] = TEXT("/Game/Mods/Railgun/Fabricator/T_RailgunIcon");
inline constexpr TCHAR GunIconAsset[] = TEXT("T_RailgunIcon");
inline constexpr TCHAR GunIconSourceArgument[] = TEXT("GunIcon=");
inline constexpr TCHAR SkillIconPackage[] = TEXT("/Game/Mods/Railgun/Research/T_RailgunSkill");
inline constexpr TCHAR SkillIconAsset[] = TEXT("T_RailgunSkill");
inline constexpr TCHAR SkillIconSourceArgument[] = TEXT("SkillIcon=");
inline constexpr TCHAR StockAmmoPath[] = TEXT("/Game/Data/Assets/Ammo/DA_Ammo_Bolt_Sniper_Rod.DA_Ammo_Bolt_Sniper_Rod");
inline constexpr TCHAR FullClonePath[] = TEXT("/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod.DA_Ammo_Railgun_FullRod");
inline constexpr TCHAR FullClonePackagePath[] = TEXT("/Game/Data/Assets/Ammo");
inline constexpr TCHAR StockGunItemPath[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.DA_Item_Module_WindTurbineMedium");
inline constexpr TCHAR GunItemPackage[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01");
inline constexpr TCHAR GunItemPackagePath[] = TEXT("/Game/Data/Assets/Modules");
inline constexpr TCHAR GunItemAsset[] = TEXT("DA_Item_Module_RailgunCannonMk01");
inline constexpr TCHAR GunItemObjectPath[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01.DA_Item_Module_RailgunCannonMk01");
inline constexpr TCHAR StockSkillPath[] = TEXT("/Game/Data/Assets/Skill/Weapons/Ammo/DA_Skill_Ammo_Sniper_Rod.DA_Skill_Ammo_Sniper_Rod");
inline constexpr TCHAR SkillPackage[] = TEXT("/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun");
inline constexpr TCHAR SkillPackagePath[] = TEXT("/Game/Data/Assets/Skill/Railgun");
inline constexpr TCHAR SkillAsset[] = TEXT("DA_Skill_Railgun");
inline constexpr TCHAR SkillObjectPath[] = TEXT("/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun.DA_Skill_Railgun");
inline constexpr TCHAR FullClonePackage[] = TEXT("/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod");
inline constexpr TCHAR FullCloneAsset[] = TEXT("DA_Ammo_Railgun_FullRod");
inline constexpr TCHAR AmmoCategoryPackage[] = TEXT("/Game/Data/Assets/ItemCategories/DA_ItemCategory_Ammo");
inline constexpr TCHAR AmmoCategoryAsset[] = TEXT("DA_ItemCategory_Ammo");
inline constexpr TCHAR IronPackage[] = TEXT("/Game/Data/Assets/Materials/DA_Material_Iron");
inline constexpr TCHAR IronAsset[] = TEXT("DA_Material_Iron");
inline constexpr TCHAR CopperPackage[] = TEXT("/Game/Data/Assets/Materials/DA_Material_Copper");
inline constexpr TCHAR CopperAsset[] = TEXT("DA_Material_Copper");
inline constexpr TCHAR PlasticPackage[] = TEXT("/Game/Data/Assets/Materials/DA_Material_Plastic");
inline constexpr TCHAR PlasticAsset[] = TEXT("DA_Material_Plastic");
inline constexpr TCHAR AmmoBoxObjectPath[] = TEXT("/Game/AssetSets/Items/Ammobox/SM_Ammobox_03.SM_Ammobox_03");
inline constexpr TCHAR DroppedActorClassPath[] = TEXT("/Game/Blueprints/BP_DynamicMeshActor.BP_DynamicMeshActor_C");
inline constexpr TCHAR AmmoDisplayName[] = TEXT("Railgun Kinetic Rounds");
inline constexpr TCHAR AmmoDescription[] = TEXT("Armor-piercing kinetic rounds. No explosives, just mass and velocity.");
inline constexpr TCHAR SkillDisplayName[] = TEXT("Railgun");
inline constexpr TCHAR SkillDescription[] = TEXT("Unlocks the Railgun and its long-range kinetic ammunition.");
inline constexpr float AmmoWeight = 3.9f;
inline constexpr float AmmoCraftTime = 6.0f;
inline constexpr float AmmoCraftElectricityCost = 5.0f;
inline constexpr int32 AmmoCraftAmount = 6;
inline constexpr int32 InventoryRoundCapacity = 6;
inline constexpr float InventoryWeightLimit = AmmoWeight * InventoryRoundCapacity;
inline constexpr uint8 AmmoCraftFilter = 3;
inline constexpr int32 IronAmount = 2;
inline constexpr int32 CopperAmount = 2;
inline constexpr int32 PlasticAmount = 1;
inline constexpr int32 AmmoMaxDropCount = 50;
inline constexpr float AmmoCaliber = 45.0f;
inline constexpr int32 SkillTierRequirement = 19;
inline constexpr int32 SkillResearchCost = 0;
inline const FName PrimaryAssetTypeName(TEXT("Item"));
}

template<typename TObjectType>
TObjectType* CreateRailgunReference(const TCHAR* PackageName, const TCHAR* AssetName)
{
    UPackage* Package = CreatePackage(PackageName);
    TObjectType* Existing = FindObject<TObjectType>(Package, AssetName);
    if (Existing)
    {
        return Existing;
    }
    TObjectType* Reference = NewObject<TObjectType>(Package, FName(AssetName), RF_Public | RF_Standalone);
    check(Reference);
    check(SaveDedicatedAsset(Reference));
    return Reference;
}

UVoyageItemAmmo* CreateRailgunAmmo(UTexture2D* AmmoIcon)
{
    using namespace RailgunAmmo;
    check(AmmoIcon);
    UPackage* AmmoPackage = CreatePackage(FullClonePackage);
    UVoyageItemAmmo* Ammo = NewObject<UVoyageItemAmmo>(AmmoPackage,
        FName(FullCloneAsset), RF_Public | RF_Standalone);
    check(Ammo);
    Ammo->Name = FText::FromString(AmmoDisplayName);
    Ammo->Description = FText::FromString(AmmoDescription);
    Ammo->Icon = AmmoIcon;
    Ammo->Category = EVoyageItemCategory::Ammo;
    Ammo->CategoryAsset = CreateRailgunReference<UVoyageItemCategoryAsset>(AmmoCategoryPackage, AmmoCategoryAsset);
    Ammo->Quality = EVoyageItemQuality::Common;
    Ammo->Weight = AmmoWeight;
    Ammo->CraftTime = AmmoCraftTime;
    Ammo->CraftElectricityCost = AmmoCraftElectricityCost;
    Ammo->CraftAmount = AmmoCraftAmount;
    Ammo->CraftFilter = AmmoCraftFilter;
    Ammo->Components.Add(CreateRailgunReference<UVoyageItemMaterial>(IronPackage, IronAsset), IronAmount);
    Ammo->Components.Add(CreateRailgunReference<UVoyageItemMaterial>(CopperPackage, CopperAsset), CopperAmount);
    Ammo->Components.Add(CreateRailgunReference<UVoyageItemMaterial>(PlasticPackage, PlasticAsset), PlasticAmount);
    FVoyageItemDropVariation DropVariation;
    DropVariation.RenderAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(AmmoBoxObjectPath));
    Ammo->DropVariations.Add(DropVariation);
    Ammo->DroppedActor = TSoftClassPtr<AActor>(FSoftObjectPath(DroppedActorClassPath));
    Ammo->Caliber = AmmoCaliber;
    Ammo->MaxDropCount = AmmoMaxDropCount;
    check(SaveDedicatedAsset(Ammo));
    return Ammo;
}

UVoyageSkill* CreateRailgunResearchSkill(UTexture2D* SkillIcon, UTexture2D* AmmoIcon,
    const FPrimaryAssetType& StockSkillType)
{
    using namespace RailgunAmmo;
    check(SkillIcon);
    check(AmmoIcon);
    check(StockSkillType.IsValid());

    UVoyageItemAmmo* Ammo = CreateRailgunAmmo(AmmoIcon);

    UVoyageItem* Gun = LoadObject<UVoyageItem>(nullptr, GunItemObjectPath);
    check(Gun);

    UPackage* Package = CreatePackage(SkillPackage);
    UVoyageSkill* Skill = NewObject<UVoyageSkill>(Package, FName(SkillAsset), RF_Public | RF_Standalone);
    check(Skill);
    Skill->Type = StockSkillType;
    Skill->Name = FText::FromString(SkillDisplayName);
    Skill->Description = FText::FromString(SkillDescription);
    Skill->Unlock.UnlockMethod = EVoyageSkillUnlockMethod::Tier;
    Skill->Unlock.Cost = SkillResearchCost;
    Skill->Unlock.Requirement = SkillTierRequirement;
    Skill->Items.Add(Gun);
    Skill->Items.Add(Ammo);
    Skill->Icon = SkillIcon;
    check(SaveDedicatedAsset(Skill));
    return Skill;
}
