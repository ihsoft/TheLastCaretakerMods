#include "BuildVoyageStaticMeshesCommandlet.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InterchangeManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Rendering/NaniteResources.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
#include "StaticMeshAttributes.h"
#include "UObject/SavePackage.h"

namespace
{
constexpr TCHAR ManifestArgument[] = TEXT("BuildManifest=");
constexpr TCHAR SummaryArgument[] = TEXT("OutputSummary=");

namespace JsonField
{
constexpr TCHAR SchemaVersion[] = TEXT("schemaVersion");
constexpr TCHAR Status[] = TEXT("status");
constexpr TCHAR ErrorCode[] = TEXT("errorCode");
constexpr TCHAR Message[] = TEXT("message");
constexpr TCHAR AssetId[] = TEXT("assetId");
constexpr TCHAR DiscoveredMeshes[] = TEXT("discoveredMeshes");
constexpr TCHAR Exact[] = TEXT("exact");
constexpr TCHAR Minimum[] = TEXT("min");
constexpr TCHAR Maximum[] = TEXT("max");
constexpr TCHAR Assets[] = TEXT("assets");
constexpr TCHAR Id[] = TEXT("id");
constexpr TCHAR SourcePath[] = TEXT("sourcePath");
constexpr TCHAR SourceMesh[] = TEXT("sourceMesh");
constexpr TCHAR PackagePath[] = TEXT("packagePath");
constexpr TCHAR ObjectName[] = TEXT("objectName");
constexpr TCHAR ObjectPath[] = TEXT("objectPath");
constexpr TCHAR ClassPath[] = TEXT("classPath");
constexpr TCHAR Slot[] = TEXT("slot");
constexpr TCHAR Value[] = TEXT("value");
constexpr TCHAR MaterialMode[] = TEXT("materialMode");
constexpr TCHAR MaterialBindings[] = TEXT("materialBindings");
constexpr TCHAR Import[] = TEXT("import");
constexpr TCHAR Nanite[] = TEXT("nanite");
constexpr TCHAR RayTracing[] = TEXT("rayTracing");
constexpr TCHAR Assertions[] = TEXT("assertions");
constexpr TCHAR GenerateLightmapUvs[] = TEXT("generateLightmapUVs");
constexpr TCHAR RemoveDegenerates[] = TEXT("removeDegenerates");
constexpr TCHAR RecomputeNormals[] = TEXT("recomputeNormals");
constexpr TCHAR RecomputeTangents[] = TEXT("recomputeTangents");
constexpr TCHAR UseFullPrecisionUvs[] = TEXT("useFullPrecisionUVs");
constexpr TCHAR UseHighPrecisionTangentBasis[] = TEXT("useHighPrecisionTangentBasis");
constexpr TCHAR Enabled[] = TEXT("enabled");
constexpr TCHAR ExplicitTangents[] = TEXT("explicitTangents");
constexpr TCHAR PositionPrecision[] = TEXT("positionPrecision");
constexpr TCHAR NormalPrecision[] = TEXT("normalPrecision");
constexpr TCHAR KeepPercentTriangles[] = TEXT("keepPercentTriangles");
constexpr TCHAR TrimRelativeError[] = TEXT("trimRelativeError");
constexpr TCHAR GenerateFallback[] = TEXT("generateFallback");
constexpr TCHAR FallbackTarget[] = TEXT("fallbackTarget");
constexpr TCHAR FallbackPercentTriangles[] = TEXT("fallbackPercentTriangles");
constexpr TCHAR FallbackRelativeError[] = TEXT("fallbackRelativeError");
constexpr TCHAR Support[] = TEXT("support");
constexpr TCHAR SourceTriangles[] = TEXT("sourceTriangles");
constexpr TCHAR NaniteInputTriangles[] = TEXT("naniteInputTriangles");
constexpr TCHAR FallbackTriangles[] = TEXT("fallbackTriangles");
constexpr TCHAR SelectedSourceMesh[] = TEXT("selectedSourceMesh");
constexpr TCHAR SourceVertices[] = TEXT("sourceVertices");
constexpr TCHAR SourceUvChannels[] = TEXT("sourceUvChannels");
constexpr TCHAR SourceHasNonWhiteVertexColors[] = TEXT("sourceHasNonWhiteVertexColors");
constexpr TCHAR FallbackVertices[] = TEXT("fallbackVertices");
constexpr TCHAR UvChannels[] = TEXT("uvChannels");
constexpr TCHAR NaniteInputVertices[] = TEXT("naniteInputVertices");
constexpr TCHAR NaniteClusters[] = TEXT("naniteClusters");
constexpr TCHAR NaniteRootPages[] = TEXT("naniteRootPages");
constexpr TCHAR SupportRayTracing[] = TEXT("supportRayTracing");
constexpr TCHAR RayTracingProxyLods[] = TEXT("rayTracingProxyLods");
constexpr TCHAR RayTracingUsesRenderingLods[] = TEXT("rayTracingUsesRenderingLods");
constexpr TCHAR EmbeddedMaterialsIgnored[] = TEXT("embeddedMaterialsIgnored");
constexpr TCHAR ImportedMaterialSlots[] = TEXT("importedMaterialSlots");
constexpr TCHAR ResolvedExternalMaterialReferences[] = TEXT("resolvedExternalMaterialReferences");
constexpr TCHAR IgnoredImportedAssets[] = TEXT("ignoredImportedAssets");
constexpr TCHAR AssetCount[] = TEXT("assetCount");
}

namespace JsonValue
{
constexpr TCHAR Failed[] = TEXT("failed");
constexpr TCHAR Passed[] = TEXT("passed");
constexpr TCHAR DefaultEngine[] = TEXT("DefaultEngine");
constexpr TCHAR ExternalReferences[] = TEXT("ExternalReferences");
constexpr TCHAR Enabled[] = TEXT("Enabled");
constexpr TCHAR PercentTriangles[] = TEXT("PercentTriangles");
constexpr TCHAR RelativeError[] = TEXT("RelativeError");
}

namespace FailureCode
{
constexpr TCHAR ManifestRead[] = TEXT("manifest-read");
constexpr TCHAR ManifestAssets[] = TEXT("manifest-assets");
constexpr TCHAR EditorWorld[] = TEXT("editor-world");
constexpr TCHAR AssetFields[] = TEXT("asset-fields");
constexpr TCHAR ImportFailed[] = TEXT("import-failed");
constexpr TCHAR MeshSelection[] = TEXT("mesh-selection");
constexpr TCHAR MeshDescription[] = TEXT("mesh-description");
constexpr TCHAR SourceTriangleAssertion[] = TEXT("source-triangle-assertion");
constexpr TCHAR DestinationRename[] = TEXT("destination-rename");
constexpr TCHAR MaterialBindings[] = TEXT("material-bindings");
constexpr TCHAR MaterialReferenceSave[] = TEXT("material-reference-save");
constexpr TCHAR NormalizedSettings[] = TEXT("normalized-settings");
constexpr TCHAR BuiltTriangleAssertion[] = TEXT("built-triangle-assertion");
constexpr TCHAR NaniteBuild[] = TEXT("nanite-build");
constexpr TCHAR RayTracingBuild[] = TEXT("ray-tracing-build");
constexpr TCHAR SavePackage[] = TEXT("save-package");
}

constexpr TCHAR ObjectPathSeparator[] = TEXT(".");
constexpr TCHAR ListSeparator[] = TEXT(", ");
constexpr TCHAR GamePackagePrefix[] = TEXT("/Game/");
constexpr TCHAR TemporaryPackagePattern[] = TEXT("/Game/__VoyageStaticMeshBuilder/%04d");

struct FCountAssertion
{
    TOptional<int32> Exact;
    TOptional<int32> Minimum;
    TOptional<int32> Maximum;

    bool Matches(const int32 Value) const
    {
        return (!Exact.IsSet() || Value == Exact.GetValue()) &&
            (!Minimum.IsSet() || Value >= Minimum.GetValue()) &&
            (!Maximum.IsSet() || Value <= Maximum.GetValue());
    }
};

bool ReadText(const FString& FileName, TSharedPtr<FJsonObject>& OutRoot)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text, *FileName) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), OutRoot) &&
        OutRoot.IsValid();
}

bool WriteJson(const FString& FileName, const TSharedRef<FJsonObject>& Root)
{
    FString Text;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
    return FJsonSerializer::Serialize(Root, Writer) &&
        FFileHelper::SaveStringToFile(Text, *FileName);
}

TSharedRef<FJsonObject> NewFailure(
    const FString& Code,
    const FString& Message,
    const FString& AssetId = FString(),
    const TArray<FString>& DiscoveredMeshes = {})
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(JsonField::SchemaVersion, 1);
    Root->SetStringField(JsonField::Status, JsonValue::Failed);
    Root->SetStringField(JsonField::ErrorCode, Code);
    Root->SetStringField(JsonField::Message, Message);
    if (!AssetId.IsEmpty()) Root->SetStringField(JsonField::AssetId, AssetId);
    TArray<TSharedPtr<FJsonValue>> Names;
    for (const FString& Name : DiscoveredMeshes)
    {
        Names.Add(MakeShared<FJsonValueString>(Name));
    }
    Root->SetArrayField(JsonField::DiscoveredMeshes, Names);
    return Root;
}

bool ReadRequiredString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, FString& Out)
{
    return Object.IsValid() && Object->TryGetStringField(Name, Out) && !Out.IsEmpty();
}

bool ReadCountAssertion(const TSharedPtr<FJsonObject>& Object, FCountAssertion& Out)
{
    if (!Object.IsValid()) return true;
    double Value = 0;
    if (Object->TryGetNumberField(JsonField::Exact, Value)) Out.Exact = static_cast<int32>(Value);
    if (Object->TryGetNumberField(JsonField::Minimum, Value)) Out.Minimum = static_cast<int32>(Value);
    if (Object->TryGetNumberField(JsonField::Maximum, Value)) Out.Maximum = static_cast<int32>(Value);
    return true;
}

ENaniteGenerateFallback ParseGenerateFallback(const FString& Value)
{
    if (Value == JsonValue::Enabled) return ENaniteGenerateFallback::Enabled;
    return ENaniteGenerateFallback::PlatformDefault;
}

ENaniteFallbackTarget ParseFallbackTarget(const FString& Value)
{
    if (Value == JsonValue::PercentTriangles) return ENaniteFallbackTarget::PercentTriangles;
    if (Value == JsonValue::RelativeError) return ENaniteFallbackTarget::RelativeError;
    return ENaniteFallbackTarget::Auto;
}

UMaterialInterface* ResolveMaterialReference(
    const FString& ObjectPath,
    const FString& ClassPath,
    FString& OutError)
{
    if (UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, *ObjectPath))
    {
        return Existing;
    }

    FString PackageName;
    FString ObjectName;
    if (!ObjectPath.Split(ObjectPathSeparator, &PackageName, &ObjectName,
            ESearchCase::CaseSensitive, ESearchDir::FromEnd) ||
        !FPackageName::IsValidLongPackageName(PackageName) || ObjectName.IsEmpty())
    {
        OutError = FString::Printf(TEXT("Invalid material objectPath: %s"), *ObjectPath);
        return nullptr;
    }
    UClass* MaterialClass = LoadObject<UClass>(nullptr, *ClassPath);
    if (!MaterialClass || !MaterialClass->IsChildOf(UMaterialInterface::StaticClass()) ||
        MaterialClass->HasAnyClassFlags(CLASS_Abstract))
    {
        OutError = FString::Printf(TEXT("Material class is missing, abstract, or not UMaterialInterface: %s"), *ClassPath);
        return nullptr;
    }
    UPackage* Package = CreatePackage(*PackageName);
    UObject* Object = StaticFindObject(nullptr, Package, *ObjectName);
    if (!Object)
    {
        Object = NewObject<UObject>(Package, MaterialClass, *ObjectName,
            RF_Public | RF_Standalone);
    }
    UMaterialInterface* Material = Cast<UMaterialInterface>(Object);
    if (!Material || Material->GetPathName() != ObjectPath)
    {
        OutError = FString::Printf(TEXT("Could not create exact material reference stub: %s"), *ObjectPath);
        return nullptr;
    }
    return Material;
}

bool ApplyMaterials(
    UStaticMesh* Mesh,
    const TSharedPtr<FJsonObject>& Asset,
    TArray<FString>& OutImportedSlots,
    TArray<UMaterialInterface*>& OutExternalMaterials,
    TArray<FString>& OutResolvedExternalReferences,
    FString& OutError)
{
    const TArray<FStaticMaterial> Imported = Mesh->GetStaticMaterials();
    for (const FStaticMaterial& Slot : Imported)
    {
        OutImportedSlots.Add(Slot.MaterialSlotName.ToString());
    }
    FString Mode;
    if (!ReadRequiredString(Asset, JsonField::MaterialMode, Mode)) return false;

    TMap<FName, UMaterialInterface*> Bindings;
    if (Mode == JsonValue::ExternalReferences)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Asset->TryGetArrayField(JsonField::MaterialBindings, Values))
        {
            OutError = TEXT("ExternalReferences requires materialBindings");
            return false;
        }
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            const TSharedPtr<FJsonObject> Binding = Value->AsObject();
            FString Slot;
            FString ObjectPath;
            FString ClassPath;
            if (!ReadRequiredString(Binding, JsonField::Slot, Slot) ||
                !ReadRequiredString(Binding, JsonField::ObjectPath, ObjectPath) ||
                !ReadRequiredString(Binding, JsonField::ClassPath, ClassPath) ||
                Bindings.Contains(FName(*Slot)))
            {
                OutError = TEXT("Invalid or duplicate material binding");
                return false;
            }
            UMaterialInterface* Material = ResolveMaterialReference(ObjectPath, ClassPath, OutError);
            if (!Material) return false;
            Bindings.Add(FName(*Slot), Material);
            OutExternalMaterials.AddUnique(Material);
            OutResolvedExternalReferences.AddUnique(Material->GetPathName());
        }
    }
    else if (Mode != JsonValue::DefaultEngine)
    {
        OutError = FString::Printf(TEXT("Unsupported materialMode: %s"), *Mode);
        return false;
    }

    Mesh->GetStaticMaterials().Reset();
    for (const FStaticMaterial& Slot : Imported)
    {
        UMaterialInterface* Material = nullptr;
        if (Mode == JsonValue::DefaultEngine)
        {
            Material = UMaterial::GetDefaultMaterial(MD_Surface);
        }
        else
        {
            UMaterialInterface** Found = Bindings.Find(Slot.MaterialSlotName);
            if (!Found)
            {
                OutError = FString::Printf(
                    TEXT("No material binding for imported slot '%s'; imported slots: %s"),
                    *Slot.MaterialSlotName.ToString(), *FString::Join(OutImportedSlots, ListSeparator));
                return false;
            }
            Material = *Found;
            Bindings.Remove(Slot.MaterialSlotName);
        }
        Mesh->GetStaticMaterials().Add(FStaticMaterial(
            Material, Slot.MaterialSlotName, Slot.ImportedMaterialSlotName));
    }
    if (!Bindings.IsEmpty())
    {
        OutError = FString::Printf(
            TEXT("materialBindings contains slots not present in the selected mesh; imported slots: %s"),
            *FString::Join(OutImportedSlots, ListSeparator));
        return false;
    }
    return true;
}

TSharedRef<FJsonObject> CountObject(const int32 Value)
{
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(JsonField::Value, Value);
    return Result;
}
}

UBuildVoyageStaticMeshesCommandlet::UBuildVoyageStaticMeshesCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UBuildVoyageStaticMeshesCommandlet::Main(const FString& Params)
{
    FString ManifestFile;
    FString SummaryFile;
    if (!FParse::Value(*Params, ManifestArgument, ManifestFile) ||
        !FParse::Value(*Params, SummaryArgument, SummaryFile) ||
        !FPaths::FileExists(ManifestFile) || ManifestFile == SummaryFile)
    {
        UE_LOG(LogTemp, Error, TEXT("Distinct existing BuildManifest and OutputSummary are required"));
        return 1;
    }

    TSharedPtr<FJsonObject> Manifest;
    if (!ReadText(ManifestFile, Manifest))
    {
        WriteJson(SummaryFile, NewFailure(FailureCode::ManifestRead, TEXT("Normalized build manifest cannot be read")));
        return 1;
    }
    const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
    if (!Manifest->TryGetArrayField(JsonField::Assets, Assets) || Assets->IsEmpty())
    {
        WriteJson(SummaryFile, NewFailure(FailureCode::ManifestAssets, TEXT("Normalized build manifest has no assets")));
        return 1;
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        WriteJson(SummaryFile, NewFailure(FailureCode::EditorWorld, TEXT("Editor world is unavailable")));
        return 1;
    }

    TArray<TSharedPtr<FJsonValue>> ResultAssets;
    for (int32 Index = 0; Index < Assets->Num(); ++Index)
    {
        const TSharedPtr<FJsonObject> Asset = (*Assets)[Index]->AsObject();
        FString Id;
        FString Source;
        FString SourceMesh;
        FString PackageName;
        FString ObjectName;
        if (!ReadRequiredString(Asset, JsonField::Id, Id) ||
            !ReadRequiredString(Asset, JsonField::SourcePath, Source) ||
            !ReadRequiredString(Asset, JsonField::PackagePath, PackageName) ||
            !ReadRequiredString(Asset, JsonField::ObjectName, ObjectName) ||
            !FPaths::FileExists(Source) || !PackageName.StartsWith(GamePackagePrefix) ||
            !FPackageName::IsValidLongPackageName(PackageName))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::AssetFields,
                TEXT("Asset has invalid normalized fields or non-/Game destination"), Id));
            return 1;
        }
        Asset->TryGetStringField(JsonField::SourceMesh, SourceMesh);

        TArray<UObject*> Imported;
        FImportAssetParameters Parameters;
        Parameters.bIsAutomated = true;
        Parameters.bReplaceExisting = false;
        Parameters.ImportLevel = World->PersistentLevel;
        Parameters.OnAssetDoneNative.BindLambda([&Imported](UObject* Object)
        {
            Imported.Add(Object);
        });
        const FString ImportDestination = FString::Printf(
            TemporaryPackagePattern, Index);
        UInterchangeManager& Manager = UInterchangeManager::GetInterchangeManager();
        if (!Manager.ImportScene(ImportDestination,
                UInterchangeManager::CreateSourceData(Source), Parameters))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::ImportFailed,
                TEXT("Interchange import failed; inspect editor log"), Id));
            return 1;
        }

        TArray<UStaticMesh*> Meshes;
        TArray<FString> DiscoveredMeshes;
        TArray<FString> IgnoredImportedAssets;
        for (UObject* Object : Imported)
        {
            if (UStaticMesh* Mesh = Cast<UStaticMesh>(Object))
            {
                Meshes.AddUnique(Mesh);
                DiscoveredMeshes.AddUnique(Mesh->GetName());
            }
            else if (Object)
            {
                IgnoredImportedAssets.AddUnique(Object->GetPathName());
            }
        }
        DiscoveredMeshes.Sort();

        UStaticMesh* Mesh = nullptr;
        if (SourceMesh.IsEmpty() && Meshes.Num() == 1)
        {
            Mesh = Meshes[0];
        }
        else if (!SourceMesh.IsEmpty())
        {
            for (UStaticMesh* Candidate : Meshes)
            {
                if (Candidate->GetName().Equals(SourceMesh, ESearchCase::CaseSensitive))
                {
                    Mesh = Candidate;
                    break;
                }
            }
        }
        if (!Mesh)
        {
            const FString Remedy = SourceMesh.IsEmpty()
                ? TEXT("GLB contains multiple or zero StaticMeshes; set sourceMesh to one discovered name")
                : TEXT("sourceMesh did not match; choose one discovered name exactly");
            WriteJson(SummaryFile, NewFailure(FailureCode::MeshSelection, Remedy, Id, DiscoveredMeshes));
            return 1;
        }

        const FMeshDescription* Description = Mesh->GetMeshDescription(0);
        if (!Description)
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::MeshDescription,
                TEXT("Selected mesh has no LOD0 MeshDescription"), Id, DiscoveredMeshes));
            return 1;
        }
        const int32 SourceTriangles = Description->Triangles().Num();
        const int32 SourceVertices = Description->Vertices().Num();
        const FString SelectedSourceMesh = Mesh->GetName();
        const FStaticMeshConstAttributes SourceAttributes(*Description);
        const auto SourceUVs = SourceAttributes.GetVertexInstanceUVs();
        const auto SourceColors = SourceAttributes.GetVertexInstanceColors();
        const int32 SourceUvChannels = SourceUVs.IsValid() ? SourceUVs.GetNumChannels() : 0;
        bool bSourceHasNonWhiteVertexColors = false;
        if (SourceColors.IsValid())
        {
            for (const FVertexInstanceID VertexInstanceId : Description->VertexInstances().GetElementIDs())
            {
                if (!SourceColors[VertexInstanceId].Equals(FVector4f(1.0f), UE_SMALL_NUMBER))
                {
                    bSourceHasNonWhiteVertexColors = true;
                    break;
                }
            }
        }

        FCountAssertion SourceTriangleAssertion;
        const TSharedPtr<FJsonObject>* Assertions = nullptr;
        if (Asset->TryGetObjectField(JsonField::Assertions, Assertions))
        {
            const TSharedPtr<FJsonObject>* SourceAssert = nullptr;
            if ((*Assertions)->TryGetObjectField(JsonField::SourceTriangles, SourceAssert))
            {
                ReadCountAssertion(*SourceAssert, SourceTriangleAssertion);
            }
        }
        if (!SourceTriangleAssertion.Matches(SourceTriangles))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::SourceTriangleAssertion,
                FString::Printf(TEXT("Source triangle assertion failed; observed %d"), SourceTriangles),
                Id, DiscoveredMeshes));
            return 1;
        }

        UPackage* DestinationPackage = CreatePackage(*PackageName);
        if (!DestinationPackage || !Mesh->Rename(*ObjectName, DestinationPackage,
                REN_DontCreateRedirectors | REN_NonTransactional))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::DestinationRename,
                TEXT("Could not move selected mesh to destination package/object"), Id));
            return 1;
        }
        Mesh->SetFlags(RF_Public | RF_Standalone);

        TArray<FString> ImportedSlots;
        TArray<UMaterialInterface*> ExternalMaterials;
        TArray<FString> ResolvedExternalReferences;
        FString MaterialError;
        if (!ApplyMaterials(Mesh, Asset, ImportedSlots, ExternalMaterials,
                ResolvedExternalReferences, MaterialError))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::MaterialBindings, MaterialError, Id));
            return 1;
        }

        // External game references need reloadable editor-only stubs for this
        // isolated host's later cook process. They remain private build inputs;
        // only the requested StaticMesh packages are staged for the caller.
        TSet<UPackage*> SavedReferencePackages;
        for (UMaterialInterface* ExternalMaterial : ExternalMaterials)
        {
            UPackage* ReferencePackage = ExternalMaterial
                ? ExternalMaterial->GetOutermost() : nullptr;
            if (!ReferencePackage || SavedReferencePackages.Contains(ReferencePackage) ||
                !ReferencePackage->GetName().StartsWith(GamePackagePrefix))
            {
                continue;
            }
            const FString ReferenceFilename = FPackageName::LongPackageNameToFilename(
                ReferencePackage->GetName(), FPackageName::GetAssetPackageExtension());
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReferenceFilename), true);
            FSavePackageArgs ReferenceSaveArgs;
            ReferenceSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            ReferenceSaveArgs.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(ReferencePackage, ExternalMaterial,
                    *ReferenceFilename, ReferenceSaveArgs))
            {
                WriteJson(SummaryFile, NewFailure(FailureCode::MaterialReferenceSave,
                    FString::Printf(TEXT("Could not persist external material reference stub: %s"),
                        *ExternalMaterial->GetPathName()), Id));
                return 1;
            }
            SavedReferencePackages.Add(ReferencePackage);
        }

        const TSharedPtr<FJsonObject>* Import = nullptr;
        const TSharedPtr<FJsonObject>* Nanite = nullptr;
        const TSharedPtr<FJsonObject>* RayTracing = nullptr;
        if (!Asset->TryGetObjectField(JsonField::Import, Import) ||
            !Asset->TryGetObjectField(JsonField::Nanite, Nanite) ||
            !Asset->TryGetObjectField(JsonField::RayTracing, RayTracing))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::NormalizedSettings,
                TEXT("Normalized import/nanite/rayTracing settings are missing"), Id));
            return 1;
        }

        FMeshBuildSettings& BuildSettings = Mesh->GetSourceModel(0).BuildSettings;
        BuildSettings.bGenerateLightmapUVs = (*Import)->GetBoolField(JsonField::GenerateLightmapUvs);
        BuildSettings.bRemoveDegenerates = (*Import)->GetBoolField(JsonField::RemoveDegenerates);
        BuildSettings.bRecomputeNormals = (*Import)->GetBoolField(JsonField::RecomputeNormals);
        BuildSettings.bRecomputeTangents = (*Import)->GetBoolField(JsonField::RecomputeTangents);
        BuildSettings.bUseFullPrecisionUVs = (*Import)->GetBoolField(JsonField::UseFullPrecisionUvs);
        BuildSettings.bUseHighPrecisionTangentBasis = (*Import)->GetBoolField(JsonField::UseHighPrecisionTangentBasis);

        FMeshNaniteSettings& NaniteSettings = Mesh->GetNaniteSettings();
        NaniteSettings.bEnabled = (*Nanite)->GetBoolField(JsonField::Enabled);
        NaniteSettings.bExplicitTangents = (*Nanite)->GetBoolField(JsonField::ExplicitTangents);
        NaniteSettings.PositionPrecision = (*Nanite)->GetIntegerField(JsonField::PositionPrecision);
        NaniteSettings.NormalPrecision = (*Nanite)->GetIntegerField(JsonField::NormalPrecision);
        NaniteSettings.KeepPercentTriangles = (*Nanite)->GetNumberField(JsonField::KeepPercentTriangles);
        NaniteSettings.TrimRelativeError = (*Nanite)->GetNumberField(JsonField::TrimRelativeError);
        NaniteSettings.GenerateFallback = ParseGenerateFallback(
            (*Nanite)->GetStringField(JsonField::GenerateFallback));
        NaniteSettings.FallbackTarget = ParseFallbackTarget(
            (*Nanite)->GetStringField(JsonField::FallbackTarget));
        NaniteSettings.FallbackPercentTriangles = (*Nanite)->GetNumberField(JsonField::FallbackPercentTriangles);
        NaniteSettings.FallbackRelativeError = (*Nanite)->GetNumberField(JsonField::FallbackRelativeError);
        Mesh->bSupportRayTracing = (*RayTracing)->GetBoolField(JsonField::Support);

        Mesh->Build(false);
        const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
        const Nanite::FResources* NaniteResources = RenderData &&
            RenderData->NaniteResourcesPtr.IsValid() ? RenderData->NaniteResourcesPtr.Get() : nullptr;
        const FStaticMeshRayTracingProxy* RayTracingProxy =
            RenderData ? RenderData->RayTracingProxy : nullptr;
        const int32 FallbackTriangles = Mesh->GetNumTriangles(0);
        const int32 FallbackVertices = Mesh->GetNumVertices(0);
        const int32 UvChannels = Mesh->GetNumUVChannels(0);

        FCountAssertion NaniteTriangleAssertion;
        FCountAssertion FallbackTriangleAssertion;
        if (Assertions)
        {
            const TSharedPtr<FJsonObject>* Count = nullptr;
            if ((*Assertions)->TryGetObjectField(JsonField::NaniteInputTriangles, Count))
                ReadCountAssertion(*Count, NaniteTriangleAssertion);
            if ((*Assertions)->TryGetObjectField(JsonField::FallbackTriangles, Count))
                ReadCountAssertion(*Count, FallbackTriangleAssertion);
        }
        const int32 NaniteInputTriangles = NaniteResources
            ? static_cast<int32>(NaniteResources->NumInputTriangles) : 0;
        if (!NaniteTriangleAssertion.Matches(NaniteInputTriangles) ||
            !FallbackTriangleAssertion.Matches(FallbackTriangles))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::BuiltTriangleAssertion,
                FString::Printf(TEXT("Built count assertion failed; Nanite=%d fallback=%d"),
                    NaniteInputTriangles, FallbackTriangles), Id));
            return 1;
        }
        if (NaniteSettings.bEnabled && (!NaniteResources || NaniteResources->NumRootPages == 0 ||
                NaniteResources->NumClusters == 0))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::NaniteBuild,
                TEXT("Nanite was requested but native resources are empty"), Id));
            return 1;
        }
        if (Mesh->bSupportRayTracing && (!RayTracingProxy || RayTracingProxy->LODs.IsEmpty()))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::RayTracingBuild,
                TEXT("Ray tracing support was requested but proxy LODs are empty"), Id));
            return 1;
        }

        DestinationPackage->MarkPackageDirty();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            PackageName, FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(DestinationPackage, Mesh, *Filename, SaveArgs))
        {
            WriteJson(SummaryFile, NewFailure(FailureCode::SavePackage,
                TEXT("Native StaticMesh package could not be saved"), Id));
            return 1;
        }

        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetStringField(JsonField::Id, Id);
        Result->SetStringField(JsonField::SourcePath, Source);
        Result->SetStringField(JsonField::SelectedSourceMesh, SelectedSourceMesh);
        Result->SetStringField(JsonField::PackagePath, PackageName);
        Result->SetStringField(JsonField::ObjectName, ObjectName);
        Result->SetStringField(JsonField::ObjectPath, PackageName + ObjectPathSeparator + ObjectName);
        Result->SetNumberField(JsonField::SourceTriangles, SourceTriangles);
        Result->SetNumberField(JsonField::SourceVertices, SourceVertices);
        Result->SetNumberField(JsonField::SourceUvChannels, SourceUvChannels);
        Result->SetBoolField(JsonField::SourceHasNonWhiteVertexColors, bSourceHasNonWhiteVertexColors);
        Result->SetNumberField(JsonField::FallbackTriangles, FallbackTriangles);
        Result->SetNumberField(JsonField::FallbackVertices, FallbackVertices);
        Result->SetNumberField(JsonField::UvChannels, UvChannels);
        Result->SetNumberField(JsonField::NaniteInputTriangles, NaniteInputTriangles);
        Result->SetNumberField(JsonField::NaniteInputVertices,
            NaniteResources ? NaniteResources->NumInputVertices : 0);
        Result->SetNumberField(JsonField::NaniteClusters,
            NaniteResources ? NaniteResources->NumClusters : 0);
        Result->SetNumberField(JsonField::NaniteRootPages,
            NaniteResources ? NaniteResources->NumRootPages : 0);
        Result->SetNumberField(JsonField::PositionPrecision,
            NaniteResources ? NaniteResources->PositionPrecision : NaniteSettings.PositionPrecision);
        Result->SetNumberField(JsonField::NormalPrecision,
            NaniteResources ? NaniteResources->NormalPrecision : NaniteSettings.NormalPrecision);
        Result->SetBoolField(JsonField::SupportRayTracing, Mesh->bSupportRayTracing);
        Result->SetNumberField(JsonField::RayTracingProxyLods,
            RayTracingProxy ? RayTracingProxy->LODs.Num() : 0);
        Result->SetBoolField(JsonField::RayTracingUsesRenderingLods,
            RayTracingProxy ? RayTracingProxy->bUsingRenderingLODs : false);
        Result->SetBoolField(JsonField::EmbeddedMaterialsIgnored, true);
        Result->SetStringField(JsonField::MaterialMode, Asset->GetStringField(JsonField::MaterialMode));
        TArray<TSharedPtr<FJsonValue>> MeshNames;
        for (const FString& Name : DiscoveredMeshes) MeshNames.Add(MakeShared<FJsonValueString>(Name));
        Result->SetArrayField(JsonField::DiscoveredMeshes, MeshNames);
        TArray<TSharedPtr<FJsonValue>> SlotNames;
        for (const FString& Name : ImportedSlots) SlotNames.Add(MakeShared<FJsonValueString>(Name));
        Result->SetArrayField(JsonField::ImportedMaterialSlots, SlotNames);
        TArray<TSharedPtr<FJsonValue>> ReferenceNames;
        for (const FString& Name : ResolvedExternalReferences)
            ReferenceNames.Add(MakeShared<FJsonValueString>(Name));
        Result->SetArrayField(JsonField::ResolvedExternalMaterialReferences, ReferenceNames);
        TArray<TSharedPtr<FJsonValue>> IgnoredNames;
        for (const FString& Name : IgnoredImportedAssets) IgnoredNames.Add(MakeShared<FJsonValueString>(Name));
        Result->SetArrayField(JsonField::IgnoredImportedAssets, IgnoredNames);
        ResultAssets.Add(MakeShared<FJsonValueObject>(Result));
    }

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(JsonField::SchemaVersion, 1);
    Root->SetStringField(JsonField::Status, JsonValue::Passed);
    Root->SetNumberField(JsonField::AssetCount, ResultAssets.Num());
    Root->SetArrayField(JsonField::Assets, ResultAssets);
    if (!WriteJson(SummaryFile, Root))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not write build summary: %s"), *SummaryFile);
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("VOYAGE STATIC MESH BUILD VERIFIED assets=%d"), ResultAssets.Num());
    return 0;
}
