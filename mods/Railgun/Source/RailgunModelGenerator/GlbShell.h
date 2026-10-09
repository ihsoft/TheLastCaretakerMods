#pragma once
#include "CoreMinimal.h"

class FJsonValue;

// Native Interchange owns geometry/material conversion. This adapter owns only
// Voyage shell contracts and stable role tags; source hierarchy stays intact.
namespace RailgunGlb
{
inline constexpr TCHAR RegistryPath[] = TEXT("Assets/Model/model-source.json");
inline constexpr TCHAR ModelPath[] = TEXT("Assets/Model/Railgun.glb");
inline constexpr TCHAR ImportRoot[] = TEXT("/Game/Mods/Railgun/Visual");
inline constexpr TCHAR InventoryFile[] = TEXT("Saved/RailgunGlbInventory.json");
inline constexpr TCHAR SchemaVersionKey[] = TEXT("schemaVersion");
inline constexpr TCHAR NodesKey[] = TEXT("nodes");
inline constexpr TCHAR RootKey[] = TEXT("root");
inline constexpr TCHAR BaseKey[] = TEXT("base");
inline constexpr TCHAR YawKey[] = TEXT("yaw");
inline constexpr TCHAR PitchKey[] = TEXT("pitch");
inline constexpr TCHAR SightKey[] = TEXT("sight");
inline constexpr TCHAR MuzzleKey[] = TEXT("muzzle");
inline constexpr TCHAR ChargeIndicatorMeshKey[] = TEXT("chargeIndicatorMesh");
inline constexpr TCHAR PowerSocketAnchorKey[] = TEXT("powerSocketAnchor");
inline constexpr TCHAR FabricatorKey[] = TEXT("fabricatorCollision");
inline constexpr TCHAR EntryKey[] = TEXT("entryInteraction");
inline constexpr TCHAR InventoryInteractionKey[] = TEXT("inventoryInteraction");
inline constexpr TCHAR CollisionNodeKey[] = TEXT("node");
inline constexpr TCHAR SizeKey[] = TEXT("sizeCm");
inline constexpr TCHAR CenterKey[] = TEXT("centerCm");
inline constexpr TCHAR AmmoKey[] = TEXT("ammoInstances");
inline constexpr TCHAR AmmoPickupRootKey[] = TEXT("ammoPickupRoot");
inline constexpr TCHAR AmmoPickupCarrierKey[] = TEXT("ammoPickupCarrier");
inline constexpr TCHAR AmmoPickupKey[] = TEXT("ammoPickup");
inline constexpr TCHAR AmmoCassetteSuffix[] = TEXT("_AmmoCassette");
inline constexpr TCHAR AmmoBinSuffix[] = TEXT("_AmmoBin");
inline constexpr TCHAR NameKey[] = TEXT("name");
inline constexpr TCHAR ParentKey[] = TEXT("parent");
inline constexpr TCHAR MeshKey[] = TEXT("mesh");
inline constexpr TCHAR LocationKey[] = TEXT("location");
inline constexpr TCHAR RotationKey[] = TEXT("rotation");
inline constexpr TCHAR ScaleKey[] = TEXT("scale");
inline constexpr TCHAR ComponentsKey[] = TEXT("components");
inline constexpr TCHAR PackagesKey[] = TEXT("packages");
inline constexpr TCHAR CollisionKey[] = TEXT("collisionMesh");
inline constexpr TCHAR RolesKey[] = TEXT("roles");
inline constexpr TCHAR HiddenInGameKey[] = TEXT("hiddenInGame");

TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& V);

int32 Generate();
}
