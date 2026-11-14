#include "GlbImportLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, UnrealGlbImportSupport)

bool UGlbImportLibrary::CopyMaterialDependencies(const TMap<FString, FString>& Packages)
{
    for (const auto& Pair : Packages)
    {
        if (!Pair.Key.StartsWith(TEXT("/InterchangeAssets/")) ||
            !Pair.Value.StartsWith(TEXT("/Game/")) ||
            !FPackageName::IsValidLongPackageName(Pair.Value) ||
            FPackageName::DoesPackageExist(Pair.Value))
        {
            return false;
        }
    }
    return FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools")
        .Get().AdvancedCopyPackages(Packages, true, false);
}
