#pragma once
#include "CoreMinimal.h"

// Stable gameplay roles. Model object names and pivot offsets live in the
// model-owned GLB registry, not in station input code.
namespace RailgunModelContract
{
inline const FName RootTag(TEXT("Railgun.Model.Root"));
inline const FName YawTag(TEXT("Railgun.Model.Yaw"));
inline const FName PitchTag(TEXT("Railgun.Model.Pitch"));
inline const FName MuzzleTag(TEXT("Railgun.Model.Muzzle"));
inline const FName SightTag(TEXT("Railgun.Model.Sight"));
inline const FName ChargeIndicatorTag(TEXT("Railgun.Model.ChargeIndicator"));
inline const FName ChargeIndicatorProgressParameter(TEXT("ProgressLevel"));
inline constexpr TCHAR ChargeIndicatorMaterialObjectPath[] =
    TEXT("/Game/Materials/Modules/MI_PogressBar_Basic_LED.MI_PogressBar_Basic_LED");
inline const FName EntryTag(TEXT("Railgun.Model.Entry"));
inline const FName EntryComponent(TEXT("RailgunEntryReference"));
inline const FName InventoryTag(TEXT("Railgun.Model.Inventory"));
inline const FName InventoryComponent(TEXT("RailgunInventoryReference"));
inline const TArray<FName> AmmoCassetteRoots {
    TEXT("Slot_01_AmmoCassette"),
    TEXT("Slot_02_AmmoCassette"),
    TEXT("Slot_03_AmmoCassette"),
    TEXT("Slot_04_AmmoCassette"),
    TEXT("Slot_05_AmmoCassette"),
    TEXT("Slot_06_AmmoCassette")
};
}
