#pragma once
#include "VoyageItem.h"

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
bool SaveDedicatedAsset(UObject* Asset);

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
inline constexpr TCHAR GunItemPackage[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01");
inline constexpr TCHAR SkillPackage[] = TEXT("/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun");
inline constexpr TCHAR SkillAsset[] = TEXT("DA_Skill_Railgun");
inline constexpr TCHAR FullClonePackage[] = TEXT("/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod");
inline constexpr TCHAR FullCloneAsset[] = TEXT("DA_Ammo_Railgun_FullRod");
inline constexpr TCHAR AmmoCategoryPackage[] = TEXT("/Game/Data/Assets/ItemCategories/DA_ItemCategory_Ammo");
inline constexpr TCHAR AmmoCategoryAsset[] = TEXT("DA_ItemCategory_Ammo");
inline constexpr int32 InventoryRoundCapacity = 6;
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

UVoyageItemAmmo* CreateRailgunAmmoReference();

UVoyageSkill* CreateRailgunSkillReference();
}
