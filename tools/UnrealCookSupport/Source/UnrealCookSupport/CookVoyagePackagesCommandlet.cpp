#include "CookVoyagePackagesCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/PackageFileSummary.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

namespace
{
constexpr TCHAR ManifestArgument[] = TEXT("PackageManifest=");
constexpr TCHAR DirectPackageArgument[] = TEXT("Package=");
constexpr TCHAR UnversionedSwitch[] = TEXT("unversioned");
constexpr TCHAR StockCookClass[] = TEXT("/Script/UnrealEd.CookCommandlet");
constexpr TCHAR PackageListPrefix[] = TEXT(" -Package=");
constexpr TCHAR PackageSeparator[] = TEXT("+");
constexpr TCHAR GameRoot[] = TEXT("/Game/");
constexpr TCHAR CookedPlatform[] = TEXT("Cooked/Windows");
constexpr TCHAR ContentDirectory[] = TEXT("Content");
}

UCookVoyagePackagesCommandlet::UCookVoyagePackagesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCookVoyagePackagesCommandlet::Main(const FString& Params)
{
    FString ManifestPath;
    FString DirectPackage;
    TArray<FString> Packages;
    if (!FParse::Value(*Params, ManifestArgument, ManifestPath) ||
        FParse::Value(*Params, DirectPackageArgument, DirectPackage) ||
        !FFileHelper::LoadFileToStringArray(Packages, *ManifestPath) || Packages.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Supply one nonempty package manifest, without a direct Package argument."));
        return 1;
    }
    TSet<FName> Seen;
    for (FString& Package : Packages)
    {
        Package.TrimStartAndEndInline();
        if (!Package.StartsWith(GameRoot) || !FPackageName::IsValidLongPackageName(Package) || Seen.Contains(FName(*Package)))
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid or duplicated package: %s"), *Package);
            return 1;
        }
        Seen.Add(FName(*Package));
    }
    UClass* CookClass = LoadObject<UClass>(nullptr, StockCookClass);
    if (!CookClass || !CookClass->IsChildOf(UCommandlet::StaticClass())) return 1;
    TStrongObjectPtr<UCommandlet> Cook(NewObject<UCommandlet>(GetTransientPackage(), CookClass));
    if (!Cook) return 1;
    const int32 Result = Cook->Main(Params + PackageListPrefix + FString::Join(Packages, PackageSeparator));
    if (Result != 0) return Result;

    const bool bUnversioned = FParse::Param(*Params, UnversionedSwitch);
    // These headers were just written by this engine. No cooked objects are loaded.
    TGuardValue<int32> AllowFreshUnversionedHeaders(GAllowUnversionedContentInEditor, true);
    for (const FString& Package : Packages)
    {
        FString Relative = Package;
        Relative.RemoveFromStart(GameRoot);
        const FString File = FPaths::Combine(FPaths::ProjectSavedDir(), CookedPlatform,
            FApp::GetProjectName(), ContentDirectory, Relative) + FPackageName::GetAssetPackageExtension();
        TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*File));
        if (!Reader)
        {
            UE_LOG(LogTemp, Error, TEXT("Cook did not produce requested package: %s"), *File);
            return 1;
        }
        FPackageFileSummary Summary;
        *Reader << Summary;
        if (Reader->IsError() || bool(Summary.GetPackageFlags() & PKG_UnversionedProperties) != bUnversioned)
        {
            UE_LOG(LogTemp, Error, TEXT("Cooked package does not use requested property serialization: %s"), *File);
            return 1;
        }
    }
    return 0;
}
