#pragma once

#include "InterchangeGenericAssetsPipeline.h"
#include "InterchangeGenericAnimationPipeline.h"
#include "InterchangeGenericMaterialPipeline.h"
#include "InterchangeGenericMeshPipeline.h"
#include "InterchangeGenericTexturePipeline.h"

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

inline int32 Generate(const FString& SourceFile)
{
    if (!FPaths::FileExists(SourceFile))
    {
        UE_LOG(LogTemp, Error, TEXT("Railgun ammo cassette source is missing: %s"), *SourceFile);
        return 1;
    }

    auto* DefaultPipeline = LoadObject<UInterchangeGenericAssetsPipeline>(
        nullptr, DefaultGltfAssetsPipeline);
    if (!DefaultPipeline)
    {
        UE_LOG(LogTemp, Error, TEXT("Default GLTF assets pipeline is unavailable"));
        return 1;
    }
    auto* Pipeline = DuplicateObject<UInterchangeGenericAssetsPipeline>(
        DefaultPipeline, GetTransientPackage());
    if (!Pipeline || !Pipeline->MeshPipeline || !Pipeline->MaterialPipeline ||
        !Pipeline->MaterialPipeline->TexturePipeline ||
        !Pipeline->AnimationPipeline)
    {
        UE_LOG(LogTemp, Error, TEXT("Default GLTF assets pipeline is incomplete"));
        return 1;
    }
    Pipeline->bUseSourceNameForAsset = false;
    Pipeline->bSceneNameSubFolder = false;
    Pipeline->bAssetTypeSubFolders = false;
    Pipeline->AssetName = AssetName;
    Pipeline->MeshPipeline->bImportStaticMeshes = true;
    Pipeline->MeshPipeline->CombineStaticMeshesBehavior =
        EInterchangeCombineStaticMeshesBehavior::All;
    Pipeline->MeshPipeline->bImportSkeletalMeshes = false;
    Pipeline->MeshPipeline->bCollision = true;
    Pipeline->MeshPipeline->bImportCollisionAccordingToMeshName = false;
    Pipeline->MeshPipeline->Collision = EInterchangeMeshCollision::Box;
    Pipeline->MeshPipeline->bForceCollisionPrimitiveGeneration = true;
    Pipeline->MeshPipeline->bBuildNanite = false;
    Pipeline->MaterialPipeline->bImportMaterials = true;
    Pipeline->MaterialPipeline->TexturePipeline->bImportTextures = true;
    Pipeline->AnimationPipeline->bImportAnimations = false;

    FImportAssetParameters Parameters;
    Parameters.bIsAutomated = true;
    Parameters.bReplaceExisting = false;
    Parameters.DestinationName = AssetName;
    Parameters.OverridePipelines.Add(Pipeline);
    Parameters.OverridePipelines.Add(FSoftObjectPath(DefaultGltfPipeline));

    TArray<UObject*> Imported;
    auto& Manager = UInterchangeManager::GetInterchangeManager();
    if (!Manager.ImportAsset(ImportRoot,
        UInterchangeManager::CreateSourceData(SourceFile), Parameters, Imported))
    {
        UE_LOG(LogTemp, Error, TEXT("Railgun ammo cassette import failed"));
        return 1;
    }

    TArray<UStaticMesh*> Meshes;
    TSet<FString> PackageNames;
    TSet<FString> MaterialPackageNames;
    TSet<FString> TexturePackageNames;
    for (UObject* Asset : Imported)
    {
        if (!Asset || !Asset->GetOutermost()->GetName().StartsWith(ImportRoot))
        {
            UE_LOG(LogTemp, Error, TEXT("Ammo cassette import escaped its owned root"));
            return 1;
        }
        if (auto* Mesh = Cast<UStaticMesh>(Asset))
        {
            Meshes.Add(Mesh);
        }
        else if (Asset->IsA<UMaterialInterface>())
        {
            MaterialPackageNames.Add(Asset->GetOutermost()->GetName());
        }
        else if (Asset->IsA<UTexture>())
        {
            TexturePackageNames.Add(Asset->GetOutermost()->GetName());
        }
        else
        {
            continue;
        }
        if (!SaveGeneratedAsset(Asset->GetOutermost(), Asset))
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot save imported ammo cassette asset: %s"),
                *Asset->GetPathName());
            return 1;
        }
        PackageNames.Add(Asset->GetOutermost()->GetName());
    }
    if (Meshes.Num() != 1 || Meshes[0]->GetPathName() != ObjectPath)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Ammo cassette must import as exactly one stable static mesh; count=%d"),
            Meshes.Num());
        return 1;
    }
    UStaticMesh* Mesh = Meshes[0];
    UBodySetup* BodySetup = Mesh->GetBodySetup();
    const int32 CollisionPrimitives = BodySetup
        ? BodySetup->AggGeom.GetElementCount() : 0;
    const int32 TriangleCount = Mesh->GetNumTriangles(0);
    const FVector BoundsSize = Mesh->GetBoundingBox().GetSize();
    if (TriangleCount <= 0 || Mesh->GetStaticMaterials().IsEmpty() ||
        CollisionPrimitives <= 0 || BoundsSize.GetMin() <= 0.0 ||
        BoundsSize.ContainsNaN())
    {
        UE_LOG(LogTemp, Error,
            TEXT("Ammo cassette readback is incomplete: triangles=%d materials=%d collision=%d bounds=%s"),
            TriangleCount, Mesh->GetStaticMaterials().Num(), CollisionPrimitives,
            *BoundsSize.ToString());
        return 1;
    }

    const FString InventoryFile = FPaths::Combine(
        FPaths::ProjectDir(), RailgunGlb::InventoryFile);
    FString InventoryText;
    TSharedPtr<FJsonObject> Inventory;
    if (!FFileHelper::LoadFileToString(InventoryText, *InventoryFile) ||
        !FJsonSerializer::Deserialize(
            TJsonReaderFactory<>::Create(InventoryText), Inventory))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot reopen Railgun model inventory"));
        return 1;
    }
    TSet<FString> AllPackageNames;
    for (const auto& Value : Inventory->GetArrayField(RailgunGlb::PackagesKey))
    {
        AllPackageNames.Add(Value->AsString());
    }
    for (const FString& ImportedPackageName : PackageNames)
    {
        AllPackageNames.Add(ImportedPackageName);
    }
    TArray<FString> SortedPackageNames = AllPackageNames.Array();
    SortedPackageNames.Sort();
    TArray<TSharedPtr<FJsonValue>> Packages;
    for (const FString& ImportedPackageName : SortedPackageNames)
    {
        Packages.Add(MakeShared<FJsonValueString>(ImportedPackageName));
    }
    Inventory->SetArrayField(RailgunGlb::PackagesKey, Packages);

    auto Evidence = MakeShared<FJsonObject>();
    Evidence->SetStringField(SourceFileKey, SourceFile);
    Evidence->SetStringField(MeshPackageKey, Mesh->GetOutermost()->GetName());
    Evidence->SetStringField(ObjectPathKey, Mesh->GetPathName());
    Evidence->SetNumberField(TrianglesKey, TriangleCount);
    Evidence->SetNumberField(MaterialSlotsKey, Mesh->GetStaticMaterials().Num());
    Evidence->SetNumberField(CollisionPrimitivesKey, CollisionPrimitives);
    Evidence->SetArrayField(BoundsCmKey, RailgunGlb::VectorJson(BoundsSize));
    const auto PackageArray = [](const TSet<FString>& Names)
    {
        TArray<FString> Sorted = Names.Array();
        Sorted.Sort();
        TArray<TSharedPtr<FJsonValue>> Values;
        for (const FString& Name : Sorted)
        {
            Values.Add(MakeShared<FJsonValueString>(Name));
        }
        return Values;
    };
    Evidence->SetArrayField(MaterialPackagesKey,
        PackageArray(MaterialPackageNames));
    Evidence->SetArrayField(TexturePackagesKey,
        PackageArray(TexturePackageNames));
    Inventory->SetObjectField(InventoryKey, Evidence);

    FString Json;
    FJsonSerializer::Serialize(Inventory, TJsonWriterFactory<>::Create(&Json));
    if (!FFileHelper::SaveStringToFile(Json, *InventoryFile))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot update Railgun model inventory"));
        return 1;
    }
    UE_LOG(LogTemp, Display,
        TEXT("Ammo cassette imported: %s triangles=%d materials=%d collision=%d packages=%d"),
        *Mesh->GetPathName(), TriangleCount, Mesh->GetStaticMaterials().Num(),
        CollisionPrimitives, PackageNames.Num());
    return 0;
}
}
