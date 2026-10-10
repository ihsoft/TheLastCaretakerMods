#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace Reload
{
inline const FName Available(TEXT("RailgunReloadAvailable"));
inline const FName SourceInventory(TEXT("RailgunReloadSourceInventory"));
inline const FName TargetInventory(TEXT("RailgunReloadTargetInventory"));
inline const FName AcceptedAmmo(TEXT("RailgunReloadAcceptedAmmo"));
inline const FName SourceCount(TEXT("RailgunReloadSourceCount"));
inline const FName TargetCount(TEXT("RailgunReloadTargetCount"));
inline const FName Refresh(TEXT("RefreshRailgunReloadAvailability"));
inline const FName Bind(TEXT("BindRailgunReloadInventories"));
inline const FName Unbind(TEXT("UnbindRailgunReloadInventories"));
inline const FName InventoryChanged(TEXT("OnRailgunReloadInventoryChanged"));
inline const FName Execute(TEXT("ReloadRailgun"));
inline const FName WeightInventory(TEXT("WeightInventory"));
inline const FName InventoryParameter(TEXT("Inventory"));
inline const FName ItemParameter(TEXT("Item"));
inline const FName IsValidParameter(TEXT("bIsValid"));
inline const FName OutSlotsParameter(TEXT("OutSlots"));
inline const FName SourceInventoryParameter(TEXT("Source"));
inline const FName SourceSlotParameter(TEXT("SourceSlot"));
inline const FName TargetSlotParameter(TEXT("TargetSlot"));
inline const FName TransferAmountParameter(TEXT("TransferAmount"));
inline const FName SlotParameter(TEXT("Slot"));
inline const FName OutItemDataParameter(TEXT("OutItemData"));
inline const FName SlotSnapshot(TEXT("RailgunReloadSlotSnapshot"));
inline const FName Remaining(TEXT("RailgunReloadRemaining"));
inline const FName Moved(TEXT("RailgunReloadMoved"));
inline const FName CurrentSlot(TEXT("RailgunReloadCurrentSlot"));
inline const FName Requested(TEXT("RailgunReloadRequested"));
inline const FName Validated(TEXT("RailgunReloadValidated"));
inline const FName BreakLoop(TEXT("Break"));
inline const FName ArrayItem(TEXT("Item"));
inline constexpr TCHAR InventoryDelegateSignaturePath[] =
    TEXT("/Script/Voyage.InventoryDelegate__DelegateSignature");
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR One[] = TEXT("1");
inline constexpr TCHAR Capacity[] = TEXT("6");
inline constexpr TCHAR AutoPlacementSlot[] = TEXT("-1");
}

void AddRailgunReloadVariables(UBlueprint* BP);
void AddRailgunReloadFunctions(UBlueprint* BP);
void BindRailgunReloadOnEntry(FGraph& G, UClass* StationClass);
void AddRailgunReloadInput(FGraph& G, UClass* StationClass);
void UnbindRailgunReload(FGraph& G, UClass* StationClass);
}
