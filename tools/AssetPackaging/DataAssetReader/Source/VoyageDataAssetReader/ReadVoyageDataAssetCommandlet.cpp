#include "ReadVoyageDataAssetCommandlet.h"
#include "Engine/DataAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace
{
namespace Names
{
constexpr TCHAR Enum[] = TEXT("enum");
constexpr TCHAR Value[] = TEXT("value");
constexpr TCHAR PackageName[] = TEXT("PackageName");
constexpr TCHAR None[] = TEXT("None");
constexpr TCHAR AssetName[] = TEXT("AssetName");
constexpr TCHAR AssetPath[] = TEXT("AssetPath");
constexpr TCHAR SubPathString[] = TEXT("SubPathString");
constexpr TCHAR Type[] = TEXT("type");
constexpr TCHAR SoftObjectType[] = TEXT("SoftObjectPropertyData");
constexpr TCHAR Object[] = TEXT("object");
constexpr TCHAR HistoryType[] = TEXT("HistoryType");
constexpr TCHAR StringTableEntry[] = TEXT("StringTableEntry");
constexpr TCHAR Flags[] = TEXT("Flags");
constexpr TCHAR TableId[] = TEXT("TableId");
constexpr TCHAR TextValue[] = TEXT("Value");
constexpr TCHAR Text[] = TEXT("text");
constexpr TCHAR NativeHistory[] = TEXT("nativeHistory");
constexpr TCHAR Base[] = TEXT("Base");
constexpr TCHAR Namespace[] = TEXT("Namespace");
constexpr TCHAR CultureInvariantString[] = TEXT("CultureInvariantString");
constexpr TCHAR Array[] = TEXT("array");
constexpr TCHAR Map[] = TEXT("map");
constexpr TCHAR Struct[] = TEXT("struct");
constexpr TCHAR Fields[] = TEXT("fields");
constexpr TCHAR BoolType[] = TEXT("BoolPropertyData");
constexpr TCHAR NameType[] = TEXT("NamePropertyData");
constexpr TCHAR DataSuffix[] = TEXT("Data");
constexpr TCHAR Class[] = TEXT("class");
constexpr TCHAR Outer[] = TEXT("outer");
constexpr TCHAR Objects[] = TEXT("objects");
constexpr TCHAR StringTablePrefix[] = TEXT("LOCTABLE(");
constexpr TCHAR BaseTextPrefix[] = TEXT("NSLOCTEXT(");
constexpr TCHAR OutputArgument[] = TEXT("Output=");
constexpr TCHAR PackageArgument[] = TEXT("Package=");
constexpr TCHAR ClassArgument[] = TEXT("ExpectedClass=");
}
using FNode = TSharedPtr<FJsonObject>;
FNode ReadValue(FProperty* Property, const void* Value, FString& Error, const void* DefaultValue = nullptr);

FNode ReadFields(UStruct* Type, const void* Container, const void* Defaults, FString& Error)
{
    auto Fields = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> It(Type); It; ++It)
    {
        FProperty* Property = *It;
        if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_SkipSerialization)) continue;
        if (Defaults && Property->Identical_InContainer(Container, Defaults)) continue;
        if (Property->ArrayDim != 1) { Error = TEXT("Static arrays unsupported"); return nullptr; }
        auto Node = ReadValue(Property, Property->ContainerPtrToValuePtr<void>(Container), Error,
            Defaults ? Property->ContainerPtrToValuePtr<void>(Defaults) : nullptr);
        if (!Node) { Error = Property->GetName() + TEXT(": ") + Error; return nullptr; }
        Fields->SetObjectField(Property->GetName(), Node);
    }
    return Fields;
}

FNode ReadValue(FProperty* Property, const void* Value, FString& Error, const void* DefaultValue)
{
    auto Result = MakeShared<FJsonObject>();
    if (auto* Enum = CastField<FEnumProperty>(Property))
    {
        Result->SetStringField(Names::Enum, Enum->GetEnum()->GetName());
        Result->SetStringField(Names::Value, Enum->GetEnum()->GetNameStringByValue(
            Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value)));
    }
    else if (auto* Soft = CastField<FSoftObjectProperty>(Property))
    {
        const FSoftObjectPath Path = Soft->GetPropertyValue(Value).ToSoftObjectPath();
        auto Top = MakeShared<FJsonObject>();
        Top->SetStringField(Names::PackageName, Path.GetLongPackageName().IsEmpty() ? Names::None : Path.GetLongPackageName());
        Top->SetStringField(Names::AssetName, Path.GetAssetName().IsEmpty() ? Names::None : Path.GetAssetName());
        auto Object = MakeShared<FJsonObject>();
        Object->SetObjectField(Names::AssetPath, Top);
        if (Path.GetSubPathString().IsEmpty()) Object->SetField(Names::SubPathString, MakeShared<FJsonValueNull>());
        else Object->SetStringField(Names::SubPathString, Path.GetSubPathString());
        Result->SetStringField(Names::Type, Names::SoftObjectType);
        Result->SetObjectField(Names::Value, Object);
    }
    else if (auto* Object = CastField<FObjectPropertyBase>(Property))
    {
        UObject* Target = Object->GetObjectPropertyValue(Value);
        if (Target) Result->SetStringField(Names::Object, Target->GetPathName());
        else Result->SetField(Names::Object, MakeShared<FJsonValueNull>());
    }
    else if (auto* TextProperty = CastField<FTextProperty>(Property))
    {
        const FText& Text = TextProperty->GetPropertyValue(Value);
        FString History;
        FTextStringHelper::WriteToBuffer(History, Text);
        FName TableId; FString TableKey;
        if (FTextInspector::GetTableIdAndKey(Text, TableId, TableKey))
        {
            if (!History.StartsWith(Names::StringTablePrefix)) { Error = TEXT("String-table history mismatch"); return nullptr; }
            auto State = MakeShared<FJsonObject>();
            State->SetStringField(Names::HistoryType, Names::StringTableEntry);
            State->SetNumberField(Names::Flags, FTextInspector::GetFlags(Text));
            State->SetStringField(Names::TableId, TableId.ToString());
            State->SetStringField(Names::TextValue, TableKey);
            Result->SetObjectField(Names::Text, State);
            Result->SetStringField(Names::NativeHistory, History);
            return Result;
        }
        // String-table text is handled above; the remaining supported form is
        // Base text. Preserve its native identity rather than display text alone.
        if (!History.StartsWith(Names::BaseTextPrefix)) { Error = TEXT("Unsupported text history: ") + History; return nullptr; }
        const auto Namespace = FTextInspector::GetNamespace(Text);
        const auto Key = FTextInspector::GetKey(Text);
        const FString* Source = FTextInspector::GetSourceString(Text);
        if (!Namespace.IsSet() || !Key.IsSet() || !Source) { Error = TEXT("Missing Base text identity"); return nullptr; }
        auto State = MakeShared<FJsonObject>();
        State->SetStringField(Names::HistoryType, Names::Base);
        State->SetNumberField(Names::Flags, FTextInspector::GetFlags(Text));
        State->SetStringField(Names::Namespace, Namespace.GetValue());
        State->SetStringField(Names::TextValue, Key.GetValue());
        State->SetStringField(Names::CultureInvariantString, *Source);
        Result->SetObjectField(Names::Text, State);
        Result->SetStringField(Names::NativeHistory, History);
    }
    else if (auto* Array = CastField<FArrayProperty>(Property))
    {
        FScriptArrayHelper Helper(Array, Value);
        TArray<TSharedPtr<FJsonValue>> Values;
        for (int32 I = 0; I < Helper.Num(); ++I)
        {
            auto Node = ReadValue(Array->Inner, Helper.GetRawPtr(I), Error);
            if (!Node) return nullptr;
            Values.Add(MakeShared<FJsonValueObject>(Node));
        }
        Result->SetArrayField(Names::Array, Values);
    }
    else if (auto* Map = CastField<FMapProperty>(Property))
    {
        FScriptMapHelper Helper(Map, Value);
        TArray<TSharedPtr<FJsonValue>> Entries;
        for (int32 I = 0; I < Helper.GetMaxIndex(); ++I)
        {
            if (!Helper.IsValidIndex(I)) continue;
            auto Key = ReadValue(Map->KeyProp, Helper.GetKeyPtr(I), Error);
            auto Val = ReadValue(Map->ValueProp, Helper.GetValuePtr(I), Error);
            if (!Key || !Val) return nullptr;
            TArray<TSharedPtr<FJsonValue>> Pair{MakeShared<FJsonValueObject>(Key), MakeShared<FJsonValueObject>(Val)};
            Entries.Add(MakeShared<FJsonValueArray>(Pair));
        }
        Result->SetArrayField(Names::Map, Entries);
    }
    else if (auto* Struct = CastField<FStructProperty>(Property))
    {
        auto Fields = ReadFields(Struct->Struct, Value, DefaultValue, Error);
        if (!Fields) return nullptr;
        Result->SetStringField(Names::Struct, Struct->Struct->GetName());
        Result->SetObjectField(Names::Fields, Fields);
    }
    else if (auto* Boolean = CastField<FBoolProperty>(Property))
    {
        Result->SetStringField(Names::Type, Names::BoolType);
        Result->SetBoolField(Names::Value, Boolean->GetPropertyValue(Value));
    }
    else if (auto* Name = CastField<FNameProperty>(Property))
    {
        Result->SetStringField(Names::Type, Names::NameType);
        Result->SetStringField(Names::Value, Name->GetPropertyValue(Value).ToString());
    }
    else if (auto* Number = CastField<FNumericProperty>(Property))
    {
        Result->SetStringField(Names::Type, Property->GetClass()->GetName() + Names::DataSuffix);
        Result->SetNumberField(Names::Value, Number->IsFloatingPoint()
            ? Number->GetFloatingPointPropertyValue(Value) : Number->GetSignedIntPropertyValue(Value));
    }
    else { Error = TEXT("Unsupported reflected property type: ") + Property->GetClass()->GetName(); return nullptr; }
    return Result;
}
}

UReadVoyageDataAssetCommandlet::UReadVoyageDataAssetCommandlet()
{
    IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true;
}

int32 UReadVoyageDataAssetCommandlet::Main(const FString& Params)
{
    FString Output, Package, ExpectedClass, Error, Text;
    if (!FParse::Value(*Params, Names::OutputArgument, Output) ||
        !FParse::Value(*Params, Names::PackageArgument, Package) ||
        !FParse::Value(*Params, Names::ClassArgument, ExpectedClass))
    {
        UE_LOG(LogTemp, Error, TEXT("Package, ExpectedClass and Output are required"));
        return 1;
    }
    FString Name = FPackageName::GetShortName(Package);
    auto* Asset = LoadObject<UDataAsset>(nullptr, *(Package + TEXT(".") + Name));
    if (!Asset || Asset->GetClass()->GetPathName() != ExpectedClass ||
        !Asset->GetClass()->HasAnyClassFlags(CLASS_Native) ||
        Asset->GetOutermost()->GetName() != Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Expected native DataAsset %s of exact class %s"), *Package, *ExpectedClass);
        return 1;
    }
    auto Graph = MakeShared<FJsonObject>();
    TArray<UObject*> Objects;
    GetObjectsWithOuter(Asset, Objects, EGetObjectsFlags::IncludeNestedObjects);
    Objects.Add(Asset);
    for (UObject* Object : Objects)
    {
        if (Object->HasAnyFlags(RF_Transient)) continue;
        auto Data = ReadFields(Object->GetClass(), Object, Object->GetClass()->GetDefaultObject(), Error);
        if (!Data)
        {
            UE_LOG(LogTemp, Error, TEXT("DataAsset readback %s: %s"), *Object->GetPathName(), *Error);
            return 1;
        }
        auto Entry = MakeShared<FJsonObject>();
        Entry->SetObjectField(Names::Fields, Data);
        Entry->SetStringField(Names::Class, Object->GetClass()->GetPathName());
        if (Object == Asset) Entry->SetField(Names::Outer, MakeShared<FJsonValueNull>());
        else Entry->SetStringField(Names::Outer, Object->GetOuter()->GetPathName());
        if (Graph->HasField(Object->GetName()))
        {
            UE_LOG(LogTemp, Error, TEXT("Duplicate owned-object name: %s"), *Object->GetName());
            return 1;
        }
        Graph->SetObjectField(Object->GetName(), Entry);
    }
    auto Report = MakeShared<FJsonObject>();
    Report->SetObjectField(Names::Objects, Graph);
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text)) ||
        !FFileHelper::SaveStringToFile(Text, *Output)) return 1;
    return 0;
}
