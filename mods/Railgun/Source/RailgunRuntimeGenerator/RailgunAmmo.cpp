#include "RailgunAmmo.h"

#include "DedicatedStationGenerator.h"
#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
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
}
