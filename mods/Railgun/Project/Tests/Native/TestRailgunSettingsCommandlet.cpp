#include "TestRailgunSettingsCommandlet.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Script.h"
#include "UObject/UnrealType.h"

// Consumer integration test: execute the saved Blueprint loader, not a native
// parser substitute. Only the editor project's INI and in-memory CDO are used.
namespace
{
struct FSettingCase
{
    FString Key;
    FDoubleProperty* Property;
    double Default;
    double Minimum;
    double Maximum;
};
}

UTestRailgunSettingsCommandlet::UTestRailgunSettingsCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UTestRailgunSettingsCommandlet::Main(const FString& Params)
{
    FString Output;
    if (!FParse::Value(*Params, TEXT("Output="), Output)) return 1;
    FString SchemaPath;
    if (!FParse::Value(*Params, TEXT("Schema="), SchemaPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Railgun settings test requires -Schema"));
        return 1;
    }
    UClass* Class = LoadObject<UClass>(nullptr,
        TEXT("/Game/Mods/Railgun/Station/BP_RailgunOperator.BP_RailgunOperator_C"));
    UObject* Owner = Class ? Class->GetDefaultObject() : nullptr;
    UFunction* Reload = Owner ? Owner->FindFunction(TEXT("ReloadRailgunSettings")) : nullptr;
    auto* ParserProperty = Class ? FindFProperty<FObjectPropertyBase>(Class, TEXT("RailgunIniParser")) : nullptr;
    if (!Reload || !ParserProperty || !ParserProperty->HasAnyPropertyFlags(CPF_Transient)) return 1;

    FString SchemaText;
    TSharedPtr<FJsonObject> Schema;
    if (!FFileHelper::LoadFileToString(SchemaText, *SchemaPath) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(SchemaText), Schema)) return 1;
    const TMap<FString, FString> FieldBindings = {
        {TEXT("ShotAudio::VolumePercent"), TEXT("RailgunShotVolumePercent")},
        {TEXT("ShotAttack::ConfiguredDamage"), TEXT("RailgunHitDamage")},
        {TEXT("Charge::ConfiguredEnergyKWh"), TEXT("RailgunFullChargeEnergyKWh")},
        {TEXT("Charge::ConfiguredTimeSeconds"), TEXT("RailgunFullChargeTimeSeconds")},
        {TEXT("Charge::ConfiguredIdleConsumptionKW"), TEXT("RailgunIdleConsumptionKW")}
    };
    TArray<FSettingCase> Settings;
    for (const auto& Group : Schema->GetArrayField(TEXT("groups")))
    for (const auto& Value : Group->AsObject()->GetArrayField(TEXT("settings")))
    {
        const auto Setting = Value->AsObject();
        FString Name;
        if (!Setting->TryGetStringField(TEXT("runtimeName"), Name))
            Name = FieldBindings.FindRef(Setting->GetStringField(TEXT("field")));
        auto* Property = FindFProperty<FDoubleProperty>(Class, *Name);
        if (!Property || Setting->GetStringField(TEXT("type")) != TEXT("number")) return 1;
        Settings.Add({Setting->GetStringField(TEXT("key")), Property,
            Setting->GetNumberField(TEXT("default")),
            Setting->GetNumberField(TEXT("minimum")), Setting->GetNumberField(TEXT("maximum"))});
    }
    if (Settings.IsEmpty()) return 1;

    const FString Ini = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Paks/Railgun.ini"));
    IFileManager& Files = IFileManager::Get();
    const bool Existed = Files.FileExists(*Ini);
    TArray<uint8> Before;
    if (Existed && (!FFileHelper::LoadFileToArray(Before, *Ini) ||
        !FFileHelper::SaveArrayToFile(Before, *(Output + TEXT(".ini-before"))))) return 1;
    Files.MakeDirectory(*FPaths::GetPath(Ini), true);
    TArray<TSharedPtr<FJsonValue>> Results;
    auto Run = [&](const TCHAR* Label, const FString* Text, const TArray<double>& Expected)
    {
        if (Text ? !FFileHelper::SaveStringToFile(*Text, *Ini, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
                 : !Files.Delete(*Ini, false, false, true)) return false;
        FEditorScriptExecutionGuard ScriptGuard;
        Owner->ProcessEvent(Reload, nullptr);
        UObject* Parser = ParserProperty->GetObjectPropertyValue_InContainer(Owner);
        if (!Parser || Parser->GetOuter() != Owner || Parser->GetClass()->GetPathName() !=
            TEXT("/Game/Mods/Railgun/Settings/BP_IniFileParser.BP_IniFileParser_C")) return false;
        for (int32 Index = 0; Index < Settings.Num(); ++Index)
        {
            const double Actual = Settings[Index].Property->GetPropertyValue_InContainer(Owner);
            if (Actual != Expected[Index])
            {
                UE_LOG(LogTemp, Error, TEXT("Settings %s: %s expected %.17g, got %.17g"),
                    Label, *Settings[Index].Key, Expected[Index], Actual);
                return false;
            }
        }
        Results.Add(MakeShared<FJsonValueString>(Label));
        return true;
    };
    auto Test = [&]()
    {
        TArray<double> Defaults, Minimums, Maximums, Custom;
        FString MinText, MaxText, CustomText, InvalidText;
        // The schema owns the setting set and range. Different per-field values
        // expose swapped bindings without freezing a positional ten-key fixture.
        for (int32 Index = 0; Index < Settings.Num(); ++Index)
        {
            const auto& S = Settings[Index];
            const double Value = S.Minimum + (S.Maximum - S.Minimum) *
                (static_cast<double>(Index) + 0.375) / Settings.Num();
            Defaults.Add(S.Default); Minimums.Add(S.Minimum); Maximums.Add(S.Maximum); Custom.Add(Value);
            MinText += FString::Printf(TEXT("%s=%.17g\n"), *S.Key, S.Minimum);
            MaxText += FString::Printf(TEXT("%s=%.17g\n"), *S.Key, S.Maximum);
            CustomText += FString::Printf(TEXT("%s=%.17g\n"), *S.Key, Value);
            InvalidText += FString::Printf(TEXT("%s=%.17g\n"), *S.Key, S.Maximum + 1.0);
        }
        const FString Empty;
        const FString OneSetting = FString::Printf(TEXT("%s=%.17g\n"), *Settings[0].Key, Custom[0]);
        TArray<double> Partial = Defaults;
        Partial[0] = Custom[0];
        return Run(TEXT("missing-defaults"), nullptr, Defaults) &&
            Run(TEXT("schema-field-bindings"), &CustomText, Custom) &&
            Run(TEXT("minimum-inclusive"), &MinText, Minimums) &&
            Run(TEXT("maximum-inclusive"), &MaxText, Maximums) &&
            Run(TEXT("out-of-range-defaults"), &InvalidText, Defaults) &&
            Run(TEXT("reload-custom"), &CustomText, Custom) &&
            Run(TEXT("removed-keys-reset"), &OneSetting, Partial) &&
            Run(TEXT("empty-file-reset"), &Empty, Defaults) &&
            Run(TEXT("reload-before-delete"), &CustomText, Custom) &&
            Run(TEXT("deleted-file-reset"), nullptr, Defaults);
    };
    const bool Passed = Test();
    bool Restored = Existed ? FFileHelper::SaveArrayToFile(Before, *Ini) : Files.Delete(*Ini, false, false, true);
    if (Existed && Restored)
    {
        TArray<uint8> After;
        Restored = FFileHelper::LoadFileToArray(After, *Ini) && After == Before;
    }
    auto Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("status"), Passed && Restored ? TEXT("passed") : TEXT("failed"));
    Report->SetBoolField(TEXT("editorIniRestored"), Restored);
    Report->SetArrayField(TEXT("cases"), Results);
    Report->SetStringField(TEXT("boundary"), TEXT("Saved editor Blueprint execution; game lifecycle remains a separate check."));
    FString Json;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
    return FFileHelper::SaveStringToFile(Json, *Output) && Passed && Restored ? 0 : 1;
}
