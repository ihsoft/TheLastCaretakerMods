#include "WriteVoyageAssetRegistryCommandlet.h"

#include "AssetRegistry/AssetRegistryState.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr TCHAR MetadataArgument[] = TEXT("RegistryMetadata=");
constexpr TCHAR OutputArgument[] = TEXT("OutputRegistry=");
constexpr TCHAR SummaryArgument[] = TEXT("OutputSummary=");
constexpr int32 SupportedSchemaVersion = 1;
constexpr int32 SupportedRegistryVersion = 24;
constexpr int32 RegistryVersionOffset = 16;
constexpr int32 RegistryFilterOffset = 20;
constexpr TCHAR ObjectSeparator[] = TEXT(".");

namespace Fields
{
constexpr TCHAR SchemaVersion[] = TEXT("schemaVersion");
constexpr TCHAR FormatVersion[] = TEXT("formatVersion");
constexpr TCHAR FilterEditorOnly[] = TEXT("filterEditorOnly");
constexpr TCHAR Assets[] = TEXT("assets");
constexpr TCHAR PackageName[] = TEXT("packageName");
constexpr TCHAR PackagePath[] = TEXT("packagePath");
constexpr TCHAR AssetName[] = TEXT("assetName");
constexpr TCHAR ObjectPath[] = TEXT("objectPath");
constexpr TCHAR OptionalOuterPath[] = TEXT("optionalOuterPath");
constexpr TCHAR AssetClassPath[] = TEXT("assetClassPath");
constexpr TCHAR Tags[] = TEXT("tags");
constexpr TCHAR ChunkIds[] = TEXT("chunkIds");
constexpr TCHAR PackageFlags[] = TEXT("packageFlags");
constexpr TCHAR AssetBundles[] = TEXT("assetBundles");
constexpr TCHAR AssetCount[] = TEXT("assetCount");
constexpr TCHAR PackageCount[] = TEXT("packageCount");
constexpr TCHAR ReopenVerified[] = TEXT("reopenVerified");
constexpr TCHAR PrimaryAssetId[] = TEXT("primaryAssetId");
}

struct FRegistryAssetDefinition
{
    FName PackageName;
    FName PackagePath;
    FName AssetName;
    FName ObjectPath;
    FTopLevelAssetPath AssetClassPath;
    FName OptionalOuterPath;
    FAssetDataTagMap Tags;
    TArray<int32> ChunkIds;
    uint32 PackageFlags = 0;
    FPrimaryAssetId PrimaryAssetId;
};

bool ReadMetadata(const FString& FileName, TArray<FRegistryAssetDefinition>& Definitions,
    bool& bFilterEditorOnly)
{
    FString Text;
    TSharedPtr<FJsonObject> Root;
    double SchemaVersion = 0.0;
    double FormatVersion = 0.0;
    if (!FFileHelper::LoadFileToString(Text, *FileName) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) ||
        !Root.IsValid() ||
        !Root->TryGetNumberField(Fields::SchemaVersion, SchemaVersion) ||
        SchemaVersion != SupportedSchemaVersion ||
        !Root->TryGetNumberField(Fields::FormatVersion, FormatVersion) ||
        FormatVersion != SupportedRegistryVersion ||
        !Root->TryGetBoolField(Fields::FilterEditorOnly, bFilterEditorOnly) ||
        !bFilterEditorOnly)
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
    if (!Root->TryGetArrayField(Fields::Assets, Assets) || !Assets || Assets->IsEmpty())
    {
        return false;
    }

    TSet<FName> ObjectPaths;
    TSet<FPrimaryAssetId> PrimaryAssetIds;
    for (const TSharedPtr<FJsonValue>& Value : *Assets)
    {
        const TSharedPtr<FJsonObject> Asset = Value->AsObject();
        if (!Asset.IsValid())
        {
            return false;
        }

        FRegistryAssetDefinition Definition;
        FString PackageName;
        FString PackagePath;
        FString AssetName;
        FString ObjectPath;
        FString OptionalOuterPath;
        FString ClassPath;
        if (!Asset->TryGetStringField(Fields::PackageName, PackageName) ||
            !Asset->TryGetStringField(Fields::PackagePath, PackagePath) ||
            !Asset->TryGetStringField(Fields::AssetName, AssetName) ||
            !Asset->TryGetStringField(Fields::ObjectPath, ObjectPath) ||
            !Asset->TryGetStringField(Fields::OptionalOuterPath, OptionalOuterPath) ||
            !Asset->TryGetStringField(Fields::AssetClassPath, ClassPath))
        {
            return false;
        }
        Definition.PackageName = FName(PackageName);
        Definition.PackagePath = FName(PackagePath);
        Definition.AssetName = FName(AssetName);
        Definition.ObjectPath = FName(ObjectPath);
        Definition.OptionalOuterPath = FName(OptionalOuterPath);
        FString ClassPackage;
        FString ClassName;
        if (Definition.PackageName.IsNone() || Definition.PackagePath.IsNone() ||
            Definition.AssetName.IsNone() || Definition.ObjectPath.IsNone() ||
            !Definition.OptionalOuterPath.IsNone() ||
            !ClassPath.Split(ObjectSeparator, &ClassPackage, &ClassName,
                ESearchCase::CaseSensitive, ESearchDir::FromEnd) ||
            ClassPackage.IsEmpty() || ClassName.IsEmpty() ||
            Definition.ObjectPath != FName(Definition.PackageName.ToString() +
                ObjectSeparator + Definition.AssetName.ToString()) ||
            Definition.PackagePath != FName(FPackageName::GetLongPackagePath(
                Definition.PackageName.ToString())) || ObjectPaths.Contains(Definition.ObjectPath))
        {
            return false;
        }
        ObjectPaths.Add(Definition.ObjectPath);
        Definition.AssetClassPath = FTopLevelAssetPath(FName(ClassPackage), FName(ClassName));

        const TSharedPtr<FJsonObject>* Tags = nullptr;
        if (!Asset->TryGetObjectField(Fields::Tags, Tags) || !Tags || !Tags->IsValid())
        {
            return false;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Tag : (*Tags)->Values)
        {
            FString TagValue;
            if (!Tag.Value.IsValid() || !Tag.Value->TryGetString(TagValue))
            {
                return false;
            }
            Definition.Tags.Add(FName(Tag.Key), MoveTemp(TagValue));
        }

        const TArray<TSharedPtr<FJsonValue>>* ChunkIds = nullptr;
        if (!Asset->TryGetArrayField(Fields::ChunkIds, ChunkIds) || !ChunkIds)
        {
            return false;
        }
        for (const TSharedPtr<FJsonValue>& Chunk : *ChunkIds)
        {
            double ChunkValue = 0.0;
            if (!Chunk.IsValid() || !Chunk->TryGetNumber(ChunkValue) ||
                ChunkValue < MIN_int32 || ChunkValue > MAX_int32 ||
                ChunkValue != FMath::FloorToDouble(ChunkValue))
            {
                return false;
            }
            Definition.ChunkIds.Add(static_cast<int32>(ChunkValue));
        }

        double PackageFlags = 0.0;
        const TArray<TSharedPtr<FJsonValue>>* AssetBundles = nullptr;
        if (!Asset->TryGetNumberField(Fields::PackageFlags, PackageFlags) ||
            !Asset->TryGetArrayField(Fields::AssetBundles, AssetBundles) || !AssetBundles ||
            PackageFlags < 0.0 || PackageFlags > MAX_uint32 ||
            PackageFlags != FMath::FloorToDouble(PackageFlags) || !AssetBundles->IsEmpty())
        {
            return false;
        }
        Definition.PackageFlags = static_cast<uint32>(PackageFlags);
        Definition.PrimaryAssetId = FPrimaryAssetId(
            FName(Definition.Tags.FindRef(FPrimaryAssetId::PrimaryAssetTypeTag)),
            FName(Definition.Tags.FindRef(FPrimaryAssetId::PrimaryAssetNameTag)));
        if (!Definition.PrimaryAssetId.IsValid() ||
            PrimaryAssetIds.Contains(Definition.PrimaryAssetId))
        {
            return false;
        }
        PrimaryAssetIds.Add(Definition.PrimaryAssetId);
        Definitions.Add(MoveTemp(Definition));
    }
    return Definitions.Num() == Assets->Num();
}

bool MatchesDefinition(const FAssetData& Asset, const FRegistryAssetDefinition& Definition)
{
    const FAssetData::FChunkArrayView ActualChunkIds = Asset.GetChunkIDs();
    if (Asset.PackageName != Definition.PackageName ||
        Asset.PackagePath != Definition.PackagePath || Asset.AssetName != Definition.AssetName ||
        Asset.GetObjectPathString() != Definition.ObjectPath.ToString() ||
        Asset.AssetClassPath != Definition.AssetClassPath ||
        Asset.GetOptionalOuterPathName() != Definition.OptionalOuterPath ||
        Asset.PackageFlags != Definition.PackageFlags ||
        ActualChunkIds.Num() != Definition.ChunkIds.Num() ||
        Asset.TaggedAssetBundles.IsValid() || Asset.GetPrimaryAssetId() != Definition.PrimaryAssetId)
    {
        return false;
    }
    for (int32 Index = 0; Index < ActualChunkIds.Num(); ++Index)
    {
        if (ActualChunkIds[Index] != Definition.ChunkIds[Index])
        {
            return false;
        }
    }
    const FAssetDataTagMap ActualTags = Asset.TagsAndValues.CopyMap();
    if (ActualTags.Num() != Definition.Tags.Num())
    {
        return false;
    }
    for (const TPair<FName, FString>& Expected : Definition.Tags)
    {
        if (ActualTags.FindRef(Expected.Key) != Expected.Value)
        {
            return false;
        }
    }
    return true;
}

bool WriteSummary(const FString& FileName, const FAssetRegistryState& Registry,
    const TArray<FRegistryAssetDefinition>& Definitions, int32 RegistryVersion,
    bool bFilterEditorOnly)
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(Fields::SchemaVersion, 1);
    Root->SetNumberField(Fields::FormatVersion, RegistryVersion);
    Root->SetBoolField(Fields::FilterEditorOnly, bFilterEditorOnly);
    Root->SetNumberField(Fields::AssetCount, Registry.GetNumAssets());
    Root->SetNumberField(Fields::PackageCount, Registry.GetNumPackages());
    Root->SetBoolField(Fields::ReopenVerified, true);
    TArray<TSharedPtr<FJsonValue>> Assets;
    for (const FRegistryAssetDefinition& Definition : Definitions)
    {
        TSharedRef<FJsonObject> Asset = MakeShared<FJsonObject>();
        Asset->SetStringField(Fields::ObjectPath, Definition.ObjectPath.ToString());
        Asset->SetStringField(Fields::AssetClassPath, Definition.AssetClassPath.ToString());
        Asset->SetStringField(Fields::PrimaryAssetId, Definition.PrimaryAssetId.ToString());
        Assets.Add(MakeShared<FJsonValueObject>(Asset));
    }
    Root->SetArrayField(Fields::Assets, MoveTemp(Assets));
    FString Text;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
    return FJsonSerializer::Serialize(Root, Writer) && FFileHelper::SaveStringToFile(Text, *FileName);
}
}

UWriteVoyageAssetRegistryCommandlet::UWriteVoyageAssetRegistryCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UWriteVoyageAssetRegistryCommandlet::Main(const FString& Params)
{
    FString MetadataFile;
    FString OutputFile;
    FString SummaryFile;
    if (!FParse::Value(*Params, MetadataArgument, MetadataFile) ||
        !FParse::Value(*Params, OutputArgument, OutputFile) ||
        !FParse::Value(*Params, SummaryArgument, SummaryFile) ||
        !FPaths::FileExists(MetadataFile) || MetadataFile == OutputFile ||
        OutputFile == SummaryFile || MetadataFile == SummaryFile)
    {
        UE_LOG(LogTemp, Error, TEXT("Distinct RegistryMetadata, OutputRegistry and OutputSummary paths are required"));
        return 1;
    }

    TArray<FRegistryAssetDefinition> Definitions;
    bool bFilterEditorOnly = false;
    if (!ReadMetadata(MetadataFile, Definitions, bFilterEditorOnly))
    {
        UE_LOG(LogTemp, Error, TEXT("Registry metadata is invalid or unsupported"));
        return 1;
    }

    FAssetRegistryState Registry;
    TSet<FName> PackageNames;
    for (const FRegistryAssetDefinition& Definition : Definitions)
    {
        FAssetData* Record = new FAssetData(Definition.PackageName, Definition.PackagePath,
            Definition.AssetName, Definition.AssetClassPath, Definition.Tags,
            Definition.ChunkIds, Definition.PackageFlags);
        if (!MatchesDefinition(*Record, Definition))
        {
            delete Record;
            UE_LOG(LogTemp, Error, TEXT("Generated record differs from metadata: %s"),
                *Definition.ObjectPath.ToString());
            return 1;
        }
        Registry.AddAssetData(Record);
        PackageNames.Add(Definition.PackageName);
    }
    if (Registry.GetNumAssets() != Definitions.Num() ||
        Registry.GetNumPackages() != PackageNames.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Generated registry counts are invalid"));
        return 1;
    }

    FAssetRegistrySerializationOptions Options(UE::AssetRegistry::ESerializationTarget::ForDevelopment);
    {
        TUniquePtr<FArchive> Output(IFileManager::Get().CreateFileWriter(*OutputFile));
        if (!Output)
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot create output AssetRegistry.bin"));
            return 1;
        }
        Output->SetFilterEditorOnly(bFilterEditorOnly);
        if (!Registry.Save(*Output, Options) || Output->IsError())
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot serialize output AssetRegistry.bin"));
            return 1;
        }
    }

    TArray<uint8> OutputBytes;
    int32 OutputVersion = -1;
    int32 OutputFilter = -1;
    if (!FFileHelper::LoadFileToArray(OutputBytes, *OutputFile) ||
        OutputBytes.Num() < RegistryFilterOffset + static_cast<int32>(sizeof(int32)))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot read output AssetRegistry.bin header"));
        return 1;
    }
    FMemory::Memcpy(&OutputVersion, OutputBytes.GetData() + RegistryVersionOffset, sizeof(int32));
    FMemory::Memcpy(&OutputFilter, OutputBytes.GetData() + RegistryFilterOffset, sizeof(int32));
    if (OutputVersion != SupportedRegistryVersion || OutputFilter != 1)
    {
        UE_LOG(LogTemp, Error, TEXT("Registry writer profile differs: version=%d filter=%d"),
            OutputVersion, OutputFilter);
        return 1;
    }

    FAssetRegistryState Reopened;
    if (!FAssetRegistryState::LoadFromDisk(*OutputFile, FAssetRegistryLoadOptions(), Reopened) ||
        Reopened.GetNumAssets() != Definitions.Num() ||
        Reopened.GetNumPackages() != PackageNames.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Output registry cannot be reopened with exact counts"));
        return 1;
    }
    for (const FRegistryAssetDefinition& Definition : Definitions)
    {
        const FAssetData* Asset = Reopened.GetAssetByObjectPath(
            FSoftObjectPath(Definition.ObjectPath.ToString()));
        if (!Asset || !MatchesDefinition(*Asset, Definition))
        {
            UE_LOG(LogTemp, Error, TEXT("Record changed on reopening: %s"),
                *Definition.ObjectPath.ToString());
            return 1;
        }
    }
    if (!WriteSummary(SummaryFile, Reopened, Definitions, OutputVersion, bFilterEditorOnly))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot write verification summary"));
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("ASSET REGISTRY VERIFIED assets=%d packages=%d"),
        Reopened.GetNumAssets(), Reopened.GetNumPackages());
    return 0;
}
