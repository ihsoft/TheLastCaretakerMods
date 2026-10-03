#include "CookPackageManifestCommandlet.h"

#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

namespace
{
constexpr TCHAR PackageManifestParameter[] = TEXT("PackageManifest=");
constexpr TCHAR PackageManifestCountParameter[] = TEXT("PackageManifestCount=");
constexpr TCHAR PackageManifestSha1Parameter[] = TEXT("PackageManifestSha1=");
constexpr TCHAR ValidateOnlySwitch[] = TEXT("ValidatePackageManifestOnly");
constexpr TCHAR StockCookCommandletPath[] = TEXT("/Script/UnrealEd.CookCommandlet");
constexpr TCHAR DirectPackageParameter[] = TEXT("Package=");
constexpr TCHAR PackageListPrefix[] = TEXT(" -Package=");
constexpr TCHAR PackageSeparator[] = TEXT("+");

bool IsSha1Text(const FString& Value)
{
    if (Value.Len() != FSHAHash::GetStringLen())
    {
        return false;
    }
    for (const TCHAR Character : Value)
    {
        if (!FChar::IsHexDigit(Character))
        {
            return false;
        }
    }
    return true;
}
}

UCookPackageManifestCommandlet::UCookPackageManifestCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCookPackageManifestCommandlet::Main(const FString& Params)
{
    FString ManifestPath;
    FString ExpectedSha1;
    int32 ExpectedCount = 0;
    if (!FParse::Value(*Params, PackageManifestParameter, ManifestPath) ||
        !FParse::Value(*Params, PackageManifestCountParameter, ExpectedCount) ||
        !FParse::Value(*Params, PackageManifestSha1Parameter, ExpectedSha1))
    {
        UE_LOG(LogTemp, Error, TEXT("Package manifest path, count, and SHA-1 are required."));
        return 1;
    }
    if (ExpectedCount <= 0 || !IsSha1Text(ExpectedSha1))
    {
        UE_LOG(LogTemp, Error, TEXT("Package manifest count or SHA-1 is invalid."));
        return 1;
    }

    TArray<uint8> ManifestBytes;
    if (!FFileHelper::LoadFileToArray(ManifestBytes, *ManifestPath) || ManifestBytes.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Package manifest is missing or empty: %s"), *ManifestPath);
        return 1;
    }
    const FString ActualSha1 = FSHA1::HashBuffer(
        ManifestBytes.GetData(), ManifestBytes.Num()).ToString();
    if (!ActualSha1.Equals(ExpectedSha1, ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Error, TEXT("Package manifest SHA-1 mismatch: %s"), *ManifestPath);
        return 1;
    }

    FString ManifestText;
    FFileHelper::BufferToString(ManifestText, ManifestBytes.GetData(), ManifestBytes.Num());
    if (ManifestText.TrimStartAndEnd().IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Package manifest contains no package names: %s"), *ManifestPath);
        return 1;
    }
    TArray<FString> Packages;
    ManifestText.ParseIntoArrayLines(Packages, true);
    if (Packages.Num() != ExpectedCount)
    {
        UE_LOG(LogTemp, Error, TEXT("Package manifest count mismatch: expected %d, found %d."),
            ExpectedCount, Packages.Num());
        return 1;
    }

    TSet<FName> SeenPackages;
    for (FString& Package : Packages)
    {
        Package.TrimStartAndEndInline();
        if (!FPackageName::IsValidLongPackageName(Package, true))
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid long package name in manifest: %s"), *Package);
            return 1;
        }
        const FName PackageName(*Package);
        if (SeenPackages.Contains(PackageName))
        {
            UE_LOG(LogTemp, Error, TEXT("Duplicate package name in manifest: %s"), *Package);
            return 1;
        }
        SeenPackages.Add(PackageName);
    }

    UE_LOG(LogTemp, Display, TEXT("Validated package manifest: %d packages, SHA-1 %s."),
        Packages.Num(), *ActualSha1);
    if (FParse::Param(*Params, ValidateOnlySwitch))
    {
        return 0;
    }
    FString DirectPackage;
    if (FParse::Value(*Params, DirectPackageParameter, DirectPackage))
    {
        UE_LOG(LogTemp, Error, TEXT("Direct Package argument cannot be combined with PackageManifest."));
        return 1;
    }

    UClass* CookClass = LoadObject<UClass>(nullptr, StockCookCommandletPath);
    if (!CookClass || !CookClass->IsChildOf(UCommandlet::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("Stock CookCommandlet class could not be loaded."));
        return 1;
    }
    TStrongObjectPtr<UCommandlet> CookCommandlet(
        NewObject<UCommandlet>(GetTransientPackage(), CookClass));
    if (!CookCommandlet)
    {
        UE_LOG(LogTemp, Error, TEXT("Stock CookCommandlet could not be created."));
        return 1;
    }

    const FString StockParams = Params + PackageListPrefix + FString::Join(Packages, PackageSeparator);
    return CookCommandlet->Main(StockParams);
}
