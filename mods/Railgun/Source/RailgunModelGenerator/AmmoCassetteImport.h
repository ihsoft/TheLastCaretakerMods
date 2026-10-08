#pragma once

#include "CoreMinimal.h"

// Imports the user-authored physical round as one owned static mesh. This is
// intentionally separate from GlbShell: the cassette has no station roles,
// moving assemblies, sockets, or six-slot visual contract.
namespace RailgunAmmoCassette
{
inline constexpr TCHAR SourceArgument[] = TEXT("AmmoCassette=");
inline constexpr TCHAR ImportRoot[] = TEXT("/Game/Mods/Railgun/Fabricator/AmmoCassette");
inline constexpr TCHAR AssetName[] = TEXT("SM_RailgunAmmoCassette");
inline constexpr TCHAR PackageName[] = TEXT("/Game/Mods/Railgun/Fabricator/AmmoCassette/SM_RailgunAmmoCassette");
inline constexpr TCHAR ObjectPath[] = TEXT("/Game/Mods/Railgun/Fabricator/AmmoCassette/SM_RailgunAmmoCassette.SM_RailgunAmmoCassette");
inline constexpr TCHAR DefaultGltfAssetsPipeline[] = TEXT("/Interchange/Pipelines/DefaultGLTFAssetsPipeline.DefaultGLTFAssetsPipeline");
inline constexpr TCHAR DefaultGltfPipeline[] = TEXT("/Interchange/Pipelines/DefaultGLTFPipeline.DefaultGLTFPipeline");
inline constexpr TCHAR InventoryKey[] = TEXT("ammoCassette");
inline constexpr TCHAR MeshPackageKey[] = TEXT("meshPackage");
inline constexpr TCHAR ObjectPathKey[] = TEXT("objectPath");
inline constexpr TCHAR SourceFileKey[] = TEXT("sourceFile");
inline constexpr TCHAR TrianglesKey[] = TEXT("triangles");
inline constexpr TCHAR MaterialSlotsKey[] = TEXT("materialSlots");
inline constexpr TCHAR CollisionPrimitivesKey[] = TEXT("collisionPrimitives");
inline constexpr TCHAR BoundsCmKey[] = TEXT("boundsCm");
inline constexpr TCHAR MaterialPackagesKey[] = TEXT("materialPackages");
inline constexpr TCHAR TexturePackagesKey[] = TEXT("texturePackages");
inline constexpr double MinimumBoundsExtentCm = 0.0;

int32 Generate(const FString& SourceFile);
}
