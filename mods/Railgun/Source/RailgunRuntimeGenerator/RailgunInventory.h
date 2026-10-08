#pragma once
#include "../../RailgunModelContract.h"
#include "RailgunInventoryNames.h"

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace RailgunInventory
{
using RailgunInventoryShared::ModuleObjectPath;
using RailgunInventoryShared::InventoryComponent;
using RailgunInventoryShared::AcceptedAmmo;
inline constexpr TCHAR ContainerOverlayPackage[] =
    TEXT("/Game/Data/UI/OverlayWidgets/DA_Widget_Container");
inline constexpr TCHAR ContainerOverlayAsset[] = TEXT("DA_Widget_Container");
inline constexpr TCHAR InventoryPartIdLiteral[] = TEXT("100");
inline const FName InteractionComponent(TEXT("RailgunAmmoInventoryInteraction"));
inline const FName InteractionQueryComponent(TEXT("RailgunAmmoInventoryQuery"));
inline const FName InteractionCollisionProfile(TEXT("Interactive"));
inline const FName ModuleComponent(TEXT("ModuleComponent"));
inline const FName ValidateItem(TEXT("ValidateItem"));
inline const FName InteractGetInventory(TEXT("InteractGetInventory"));
inline const FName ItemParameter(TEXT("Item"));
inline const FName IsValidParameter(TEXT("bIsValid"));
inline const FName PartIdParameter(TEXT("PartId"));
inline const FName NewMaxWeightLimitParameter(TEXT("NewMaxWeightLimit"));
using RailgunInventoryShared::SyncVisuals;
inline const FName InventoryChangedCallback(TEXT("OnRailgunAmmoInventoryChanged"));
inline const FName OnInventoryChanged(TEXT("OnInventoryChanged"));
inline const FName OnPersistentActorPostLoad(TEXT("OnPersistentActorPostLoad"));
inline constexpr TCHAR InventoryDelegateSignaturePath[] =
    TEXT("/Script/Voyage.InventoryDelegate__DelegateSignature");
inline const FName VisualCount(TEXT("RailgunAmmoVisualCount"));
using RailgunInventoryShared::LastVisualCount;
inline const FName Items(TEXT("Items"));
inline const FName TargetMap(TEXT("TargetMap"));
inline const FName Values(TEXT("Values"));
inline const FName SerializedItem(TEXT("Item"));
inline const FName SerializedData(TEXT("Data"));
inline const FName ItemCount(TEXT("ItemCount"));
inline const FName NewHidden(TEXT("NewHidden"));
inline const FName PropagateToChildren(TEXT("bPropagateToChildren"));
inline const FName SetHiddenInGame(TEXT("SetHiddenInGame"));
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR InvalidVisualCount[] = TEXT("-1");
inline constexpr TCHAR MaximumVisualCount[] = TEXT("6");
inline constexpr int32 InventoryPartId = 100;
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

UEdGraph* AddRailgunInterfaceFunction(UBlueprint* BP, UClass* Interface,
    const FName FunctionName, bool RequireInheritedInterface);

void FindFunctionTerminals(UEdGraph* Graph, UK2Node_FunctionEntry*& Entry,
    UK2Node_FunctionResult*& Result);

void AddRailgunInventoryLimitInitialization(UBlueprint* BP,
    FName DeferredInitialization);

void AddRailgunAmmoVisualSync(UBlueprint* BP);

void AddRailgunAmmoHudNotification(UBlueprint* BP, UClass* StationClass);

void AddRailgunAmmoVisualCallback(UBlueprint* BP);

void AddRailgunAmmoVisualPostLoad(UBlueprint* BP, FName DeferredInitialization);

void AddRailgunInventoryValidator(UBlueprint* BP);

void AddRailgunInventoryInteraction(UBlueprint* BP);

void ConfigureRailgunInventory(UVoyageItemAmmo* Ammo,
    UVoyageItemCategoryAsset* AmmoCategoryAsset, float AmmoWeightKg);
}
