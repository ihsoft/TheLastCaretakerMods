#include "GenerateRailgunRuntimeCommandlet.h"
#include "DedicatedStationGenerator.h"
#include "RailgunAmmo.h"
#include "RailgunInventory.h"
#include "RailgunShot.h"
#include "RailgunStationInitialization.h"
#include "RailgunVfx.h"
#include "RailgunWaterWake.h"
#include "StationEnergyHud.h"
#include "RailgunRuntimeGeneratorPrivate.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, RailgunRuntimeGenerator)

using namespace Railgun::Runtime;

UGenerateRailgunRuntimeCommandlet::UGenerateRailgunRuntimeCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGenerateRailgunRuntimeCommandlet::Main(const FString& Params)
{
    if (FParse::Param(*Params, DedicatedStationNames::VerifySwitch))
    {
        TArray<const TCHAR*> VerifyPackages {
            DedicatedStationNames::OperatorPackage,
            DedicatedStationNames::HudPackage,
            RailgunInputNames::LookYaw,
            RailgunInputNames::LookPitch,
            RailgunInputNames::Exit,
            RailgunInputNames::Zoom,
            RailgunInputNames::Fire,
            Shot::Package,
            RailgunWaterWake::ControllerPackage,
            RailgunImpactVfx::Package,
            ShotAudio::Package,
            ZoomTest::MaskPackage,
            EnergyHud::ChargingPackage,
            EnergyHud::OfflinePackage,
            EnergyHud::ReadyPackage,
            EnergyHud::AmmoIndicatorPackage,
            RailgunInputNames::Keyboard,
            RailgunInputNames::Context,
            RailgunAmmo::AmmoIconPackage,
            RailgunAmmo::GunIconPackage,
            RailgunAmmo::SkillIconPackage,
            RailgunAmmo::FullClonePackage,
            RailgunAmmo::SkillPackage};
        for (const TCHAR* Package : VerifyPackages)
        {
            FString Relative(Package);
            check(Relative.RemoveFromStart(DedicatedStationNames::GamePrefix));
            FString File = FPaths::Combine(FPaths::ProjectDir(),
                DedicatedStationNames::CookPrefix, Relative)
                + FPackageName::GetAssetPackageExtension();
            TUniquePtr<FArchive> Reader(
                IFileManager::Get().CreateFileReader(*File));
            check(Reader);
            FPackageFileSummary Summary;
            *Reader << Summary;
            checkf(!Reader->IsError()
                && !(Summary.GetPackageFlags() & PKG_UnversionedProperties),
                TEXT("Tagged property gate failed: %s"), *File);
            UE_LOG(LogTemp, Display, TEXT("TAGGED VERIFIED %s flags=%u"),
                Package, Summary.GetPackageFlags());
        }
        return 0;
    }

    const bool Dedicated = FParse::Param(
        *Params, DedicatedStationNames::DedicatedSwitch);
    checkf(Dedicated,
        TEXT("HC24 runtime emission is rejected; use DedicatedStation only"));

    float AmmoWeightKg = 0.0f;
    checkf(FParse::Value(*Params, RailgunAmmo::AmmoWeightSourceArgument,
        AmmoWeightKg) && AmmoWeightKg > 0.0f,
        TEXT("DedicatedStation requires a positive ammo weight from the owned JSON"));

    FString ShotSoundFile;
    checkf(FParse::Value(*Params, ShotAudio::SourceArgument, ShotSoundFile)
        && FPaths::FileExists(ShotSoundFile),
        TEXT("Missing shot sound source: %s"), *ShotSoundFile);
    ShotAudio::Wave = ImportShotSound(ShotSoundFile);

    auto ImportRequiredTexture = [&](const TCHAR* Argument,
        const TCHAR* PackageName, const TCHAR* AssetName, bool RequireSquare)
    {
        FString SourceFile;
        checkf(FParse::Value(*Params, Argument, SourceFile)
            && FPaths::FileExists(SourceFile),
            TEXT("Missing UI texture source for %s: %s"), AssetName,
            *SourceFile);
        return ImportUiTexture(SourceFile, PackageName, AssetName,
            RequireSquare);
    };

    ZoomTest::OverlayTexture = ImportRequiredTexture(
        ZoomTest::OverlaySourceArgument, ZoomTest::MaskPackage,
        ZoomTest::MaskAsset, true);
    EnergyHud::ChargingTexture = ImportRequiredTexture(
        EnergyHud::ChargingSourceArgument, EnergyHud::ChargingPackage,
        EnergyHud::ChargingAsset, false);
    EnergyHud::OfflineTexture = ImportRequiredTexture(
        EnergyHud::OfflineSourceArgument, EnergyHud::OfflinePackage,
        EnergyHud::OfflineAsset, false);
    EnergyHud::ReadyTexture = ImportRequiredTexture(
        EnergyHud::ReadySourceArgument, EnergyHud::ReadyPackage,
        EnergyHud::ReadyAsset, false);
    EnergyHud::AmmoIndicatorTexture = ImportRequiredTexture(
        EnergyHud::AmmoIndicatorSourceArgument,
        EnergyHud::AmmoIndicatorPackage, EnergyHud::AmmoIndicatorAsset, true);
    ImportRequiredTexture(RailgunAmmo::AmmoIconSourceArgument,
        RailgunAmmo::AmmoIconPackage, RailgunAmmo::AmmoIconAsset, true);
    ImportRequiredTexture(RailgunAmmo::GunIconSourceArgument,
        RailgunAmmo::GunIconPackage, RailgunAmmo::GunIconAsset, true);
    ImportRequiredTexture(RailgunAmmo::SkillIconSourceArgument,
        RailgunAmmo::SkillIconPackage, RailgunAmmo::SkillIconAsset, true);

    UVoyageItemAmmo* Ammo = CreateRailgunAmmoReference();
    CreateRailgunSkillReference();
    UVoyageItemCategoryAsset* AmmoCategory =
        CreateRailgunReference<UVoyageItemCategoryAsset>(
            RailgunAmmo::AmmoCategoryPackage, RailgunAmmo::AmmoCategoryAsset);
    ConfigureRailgunInventory(Ammo, AmmoCategory, AmmoWeightKg);
    RailgunImpactVfx::Class = CreateRailgunImpactVfx();
    RailgunWaterWake::ControllerClass = CreateRailgunWaterWakeController();
    Shot::Class = CreateRailgunShot();
    UClass* StationClass = CreateDedicatedStation();
    return ConfigureRailgunStationInitialization(StationClass) ? 0 : 1;
}
