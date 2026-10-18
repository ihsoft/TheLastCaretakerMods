#pragma once

#if WITH_EDITOR

#include "VoyageModSettingsGraph.h"

#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace VoyageModSettings::Tests
{
namespace Fixture
{
using VoyageModSettings::ENumberStorage;
using VoyageModSettings::ESettingType;
using VoyageModSettings::FSetting;
#include "VoyageModSettingsTestSettings.generated.h"
}

inline void RunGeneratedLoaderTests()
{
    static const FName ReloadFunction(TEXT("ReloadTestSettings"));

    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString RelativePath = FString::Printf(
        TEXT("Paks/VoyageModSettingsRuntimeTest-%s.ini"), *Suffix);
    const FString AbsolutePath = FPaths::ProjectContentDir() / RelativePath;
    UPackage* Package = CreatePackage(
        *FString::Printf(TEXT("/Temp/VoyageModSettingsRuntimeTest_%s"), *Suffix));
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(), Package, FName(*FString::Printf(TEXT("Fixture_%s"), *Suffix)),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),
        FName(TEXT("VoyageModSettingsRuntimeTest")));
    check(Blueprint);
    check(Graph::AddSettingMembers(
        Blueprint, Fixture::Settings, UE_ARRAY_COUNT(Fixture::Settings),
        [](const FSetting&) { return true; }));
    Graph::AddLineParser(Blueprint);
    Graph::AddNumberParser(Blueprint);
    Graph::AddStringParser(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    check(Blueprint->Status != BS_Error);
    Graph::AddLoader(
        Blueprint, ReloadFunction, *RelativePath,
        Fixture::Settings, UE_ARRAY_COUNT(Fixture::Settings),
        [](const FSetting&) { return true; });
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    check(Blueprint->Status != BS_Error);

    UObject* Object = Blueprint->GeneratedClass->GetDefaultObject();
    UFunction* Reload = Blueprint->GeneratedClass->FindFunctionByName(ReloadFunction);
    check(Object && Reload);
    const FDoubleProperty* Number = FindFProperty<FDoubleProperty>(
        Blueprint->GeneratedClass, Fixture::Number);
    const FDoubleProperty* AnyDouble = FindFProperty<FDoubleProperty>(
        Blueprint->GeneratedClass, Fixture::AnyDouble);
    const FBoolProperty* Boolean = FindFProperty<FBoolProperty>(
        Blueprint->GeneratedClass, Fixture::Boolean);
    const FStrProperty* String = FindFProperty<FStrProperty>(
        Blueprint->GeneratedClass, Fixture::String);
    const FFloatProperty* PositiveFloat = FindFProperty<FFloatProperty>(
        Blueprint->GeneratedClass, Fixture::PositiveFloat);
    check(Number && AnyDouble && Boolean && String && PositiveFloat);

    auto Load = [&](const FString& Text)
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsolutePath), true);
        check(FFileHelper::SaveStringToFile(
            Text, *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
        Object->ProcessEvent(Reload, nullptr);
    };
    auto NumberValue = [&]() { return Number->GetPropertyValue_InContainer(Object); };
    auto AnyDoubleValue = [&]() { return AnyDouble->GetPropertyValue_InContainer(Object); };
    auto BooleanValue = [&]() { return Boolean->GetPropertyValue_InContainer(Object); };
    auto StringValue = [&]() { return String->GetPropertyValue_InContainer(Object); };
    auto PositiveFloatValue = [&]() { return PositiveFloat->GetPropertyValue_InContainer(Object); };

    Load(TEXT(
        " Number = -1.5e0 # number comment\n"
        "AnyDouble=1E+3\n"
        "Boolean=TrUe # boolean comment\n"
        "String=\"a=b#c\\\"q\\\\z\" # string comment\n"
        "PositiveFloat=1e-45\n"));
    check(NumberValue() == -1.5);
    check(AnyDoubleValue() == 1000.0);
    check(BooleanValue());
    check(StringValue() == TEXT("a=b#c\"q\\z"));
    check(PositiveFloatValue() == 1.401298464324817e-45f);

    Load(TEXT(
        "Unknown=\"unterminated\n"
        "Number=2\nNumber=not-a-number\n"
        "AnyDouble=5e-324\n"
        "Boolean=true\nBoolean=0\n"
        "String=\"replacement\"\nString=\"trailing\" junk\n"
        "PositiveFloat=1e-999\n"));
    check(NumberValue() == 7.0);
    check(AnyDoubleValue() > 0.0);
    check(!BooleanValue());
    check(StringValue() == TEXT("schema default"));
    check(PositiveFloatValue() == 200.0f);

    Load(TEXT(
        "Number=9\n"
        "AnyDouble=1e999\n"
        "Boolean=false;comment\n"
        "String=unquoted\n"
        "PositiveFloat=1e39\n"));
    check(NumberValue() == 7.0);
    check(AnyDoubleValue() == 9.0);
    check(!BooleanValue());
    check(StringValue() == TEXT("schema default"));
    check(PositiveFloatValue() == 200.0f);

    Load(TEXT(
        "AnyDouble=-1.7976931348623157e308\n"
        "PositiveFloat=0\n"));
    check(AnyDoubleValue() < -1.0e308);
    check(PositiveFloatValue() == 200.0f);

    check(IFileManager::Get().Delete(*AbsolutePath, false, true));
    Object->ProcessEvent(Reload, nullptr);
    check(NumberValue() == 7.0 && AnyDoubleValue() == 9.0 && !BooleanValue());
    check(StringValue() == TEXT("schema default") && PositiveFloatValue() == 200.0f);
}
}

#endif
