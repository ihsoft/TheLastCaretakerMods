// Editor-only GLB shell generator. Never shipped; native identities are gated
// by GAME_DERIVED_SOURCES.md. Runtime generation lives in RailgunRuntimeGenerator.

#include "GenerateRailgunCommandlet.h"
#include "AmmoPickup.h"
#include "GlbShell.h"
#include "RailgunAssetNames.h"
#include "RailgunModelGeneratorPrivate.h"

#if WITH_EDITOR
UGenerateRailgunCommandlet::UGenerateRailgunCommandlet()
{
    // Interchange material parameter discovery needs client-side material data,
    // matching the stock Python import commandlet, even with the NullRHI.
    IsClient = true;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateRailgunCommandlet::Main(const FString& Params)
{
    if (FParse::Param(*Params, RailgunAssetNames::VerifyTaggedParameter))
    {
        FString Json;
        TSharedPtr<FJsonObject> Inventory;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(FPaths::ProjectDir(), RailgunGlb::InventoryFile)) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Inventory)) return 1;
        for (const auto& Value : Inventory->GetArrayField(RailgunGlb::PackagesKey))
        {
            FString Relative = Value->AsString();
            if (!Relative.RemoveFromStart(RailgunAssetNames::GamePackagePrefix)) return 1;
            const FString File = FPaths::Combine(FPaths::ProjectDir(), RailgunAssetNames::CookedContentDirectory, Relative) + FPackageName::GetAssetPackageExtension();
            TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*File));
            if (!Reader) return 1;
            FPackageFileSummary Summary; *Reader << Summary;
            if (Reader->IsError() || (Summary.GetPackageFlags() & PKG_UnversionedProperties))
            {
                UE_LOG(LogTemp, Error, TEXT("Tagged property gate failed: %s"), *File);
                return 1;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("All inventory packages use tagged properties"));
        return 0;
    }
    if (!FParse::Param(*Params, RailgunAssetNames::ShellOnlyParameter) ||
        RailgunAssetNames::IncludeBaseGameLoadedConnectorReference)
    {
        UE_LOG(LogTemp, Error, TEXT("Only explicit -ShellOnly with the mod-authored connector proxy is currently revalidated; historical operator generation is blocked"));
        return 1;
    }
    if (FPackageName::DoesPackageExist(RailgunAssetNames::PawnPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::HudPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::LeafProbePackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::BaseMeshPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::YawMeshPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::PitchMeshPackageName) ||
        FPackageName::DoesPackageExist(RailgunAmmoPickup::PackageName) ||
        (RailgunAssetNames::IncludeBaseGameLoadedConnectorReference &&
            FPackageName::DoesPackageExist(RailgunAssetNames::LoadedConnectorPackageName)) ||
        (RailgunAssetNames::UsesStockCameraDroneClass &&
            FPackageName::DoesPackageExist(RailgunAssetNames::CameraDronePackageName)) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::LeafItemPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Railgun Railgun generated assets already exist"));
        return 1;
    }

    return RailgunGlb::Generate();
}

#endif
