#pragma once
#include "VoyageItem.h"

namespace RailgunAmmo
{
inline constexpr TCHAR AmmoIconPackage[] = TEXT("/Game/Mods/Railgun/Fabricator/T_RailgunAmmoIcon");
inline constexpr TCHAR AmmoIconAsset[] = TEXT("T_RailgunAmmoIcon");
inline constexpr TCHAR AmmoIconSourceArgument[] = TEXT("AmmoIcon=");
inline constexpr TCHAR AmmoWeightSourceArgument[] = TEXT("AmmoWeightKg=");
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
inline constexpr int32 InventoryRoundCapacity = 6;
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
    TObjectType* Reference = NewObject<TObjectType>(Package, FName(AssetName),
        RF_Public | RF_Standalone);
    check(Reference);
    check(SaveDedicatedAsset(Reference));
    return Reference;
}

UVoyageItemAmmo* CreateRailgunAmmoReference()
{
    using namespace RailgunAmmo;
    UPackage* AmmoPackage = CreatePackage(FullClonePackage);
    UVoyageItemAmmo* Ammo = NewObject<UVoyageItemAmmo>(AmmoPackage,
        FName(FullCloneAsset), RF_Public | RF_Standalone);
    check(Ammo);
    check(SaveDedicatedAsset(Ammo));
    return Ammo;
}

UVoyageSkill* CreateRailgunSkillReference()
{
    using namespace RailgunAmmo;
    UPackage* Package = CreatePackage(SkillPackage);
    UVoyageSkill* Skill = NewObject<UVoyageSkill>(Package, FName(SkillAsset), RF_Public | RF_Standalone);
    check(Skill);
    check(SaveDedicatedAsset(Skill));
    return Skill;
}
