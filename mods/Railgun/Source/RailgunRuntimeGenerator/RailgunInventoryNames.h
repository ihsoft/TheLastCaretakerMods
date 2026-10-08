#pragma once
#include "CoreMinimal.h"

namespace Railgun::Runtime
{
namespace RailgunInventoryShared
{
inline constexpr TCHAR ModuleObjectPath[] =
    TEXT("/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun");
inline const FName InventoryComponent(TEXT("RailgunAmmoInventory"));
inline const FName AcceptedAmmo(TEXT("AcceptedRailgunAmmo"));
inline const FName SyncVisuals(TEXT("SyncRailgunAmmoVisuals"));
inline const FName LastVisualCount(TEXT("RailgunAmmoLastVisualCount"));
}
}
