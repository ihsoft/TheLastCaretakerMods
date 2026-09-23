#pragma once
namespace Settings
{
inline const FName FilePath(TEXT("InPath")); // Voyage declaring signature.
inline constexpr TCHAR RelativePath[] = TEXT("Paks/Railgun.ini");
inline constexpr TCHAR Separator[] = TEXT("=");
inline constexpr TCHAR OriginalPerPercent[] = TEXT("0.0128");
inline constexpr TCHAR PercentMultiplier[] = TEXT("0.01");
inline constexpr TCHAR Negate[] = TEXT("-1.0");
inline const FName FontObjectPin(TEXT("FontObject"));
inline const FName TypefaceFontNamePin(TEXT("TypefaceFontName"));
inline const FName FontSizePin(TEXT("Size"));
inline const FName FontInfoPin(TEXT("InFontInfo"));
inline const FName TranslationPin(TEXT("Translation"));
inline const FName OpacityPin(TEXT("InOpacity"));
inline const FName DisplayFontSizePin(TEXT("DisplayFontSize"));
inline const FName AssetPin(TEXT("Asset"));
inline const FName SoftObjectPathPin(TEXT("SoftObjectPath"));
inline const FName XPin(TEXT("X"));
inline const FName YPin(TEXT("Y"));
inline const FName MakeSlateFontInfoFunction(TEXT("MakeSlateFontInfo"));

struct FNumericSetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
    const TCHAR* Minimum;
    const TCHAR* Maximum;
};

struct FTextSetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
};

struct FFontSetting
{
    FName PathField;
    FName ObjectField;
};

#include "StationSettings.generated.h"
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

UEdGraphPin* TrimSetting(FGraph& G, UEdGraphPin* Text)
{
    auto* Leading = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Trim));
    G.Link(Text, G.Pin(Leading, ActorScanGraphNames::SourceString));
    auto* Trailing = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, TrimTrailing));
    G.Link(G.Pin(Leading, P::ReturnValue), G.Pin(Trailing, ActorScanGraphNames::SourceString));
    return G.Pin(Trailing, P::ReturnValue);
}

UEdGraphPin* ClampStationAim(FGraph& G, bool Horizontal, UEdGraphPin* Value)
{
    auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
    G.Link(Value, G.Pin(Clamp, OP::ClampValue));
    UEdGraphPin* Minimum = nullptr;
    if (Horizontal)
    {
        auto* Negate = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
        G.Link(G.Read(Settings::Yaw), G.Pin(Negate, P::Binary::LeftOperand)); G.Default(Negate, P::Binary::RightOperand, Settings::Negate);
        Minimum = G.Pin(Negate, P::ReturnValue);
    }
    else Minimum = G.Read(Settings::PitchMin);
    G.Link(Minimum, G.Pin(Clamp, OP::ClampMinimum));
    G.Link(G.Read(Horizontal ? Settings::Yaw : Settings::PitchMax), G.Pin(Clamp, OP::ClampMaximum));
    return G.Pin(Clamp, P::ReturnValue);
}

void ReadStationSettings(FGraph& G)
{
    for (const auto& Setting : Settings::NumericSettings) G.Write(Setting.Field, nullptr, Setting.Default);
    for (const auto& Setting : Settings::TextSettings) G.Write(Setting.Field, nullptr, Setting.Default);
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(G.Graph));
    G.Link(G.Tail, G.Pin(Work, P::Execute)); G.Tail = Work->GetThenPinGivenIndex(0);
    auto* Content = G.Call(UBlueprintPathsLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBlueprintPathsLibrary, ProjectContentDir));
    auto* Path = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr));
    G.Link(G.Pin(Content, P::ReturnValue), G.Pin(Path, P::Binary::LeftOperand)); G.Default(Path, P::Binary::RightOperand, Settings::RelativePath);
    auto* Read = G.Call(UVoyageEditorBlueprintFunctionLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVoyageEditorBlueprintFunctionLibrary, LoadFileToArray));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Read, Settings::FilePath)); G.Exec(Read);
    auto* Loop = ContextLoop(G, G.Pin(Read, P::ReturnValue));
    auto* Split = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Split));
    G.Link(G.Pin(Loop, CE::ArrayElement), G.Pin(Split, ActorScanGraphNames::SourceString));
    G.Default(Split, TextSettingsGraphNames::SplitDelimiter, Settings::Separator);
    G.Branch(G.Pin(Split, P::ReturnValue));
    auto* Key = TrimSetting(G, G.Pin(Split, TextSettingsGraphNames::SplitLeft));
    auto* Text = TrimSetting(G, G.Pin(Split, TextSettingsGraphNames::SplitRight));
    auto* SettingType = G.Node(NewObject<UK2Node_ExecutionSequence>(G.Graph));
    G.Link(G.Tail, G.Pin(SettingType, P::Execute));
    G.Tail = SettingType->GetThenPinGivenIndex(0);
    auto* Numeric = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, IsNumeric));
    G.Link(Text, G.Pin(Numeric, ActorScanGraphNames::SourceString)); G.Branch(G.Pin(Numeric, P::ReturnValue));
    auto* Value = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Conv_StringToDouble));
    G.Link(Text, G.Pin(Value, TextSettingsGraphNames::NumericString));
    auto* NextKey = G.Tail;
    auto ReadKey = [&](const TCHAR* Name, FName Field, const TCHAR* Minimum, const TCHAR* Maximum)
    {
        G.Tail = NextKey;
        auto* Match = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
        G.Link(Key, G.Pin(Match, P::Binary::LeftOperand)); G.Default(Match, P::Binary::RightOperand, Name);
        auto* Branch = G.Branch(G.Pin(Match, P::ReturnValue)); NextKey = G.Pin(Branch, P::Else);
        auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
        G.Link(G.Pin(Value, P::ReturnValue), G.Pin(Clamp, OP::ClampValue)); G.Default(Clamp, OP::ClampMinimum, Minimum); G.Default(Clamp, OP::ClampMaximum, Maximum);
        G.Write(Field, G.Pin(Clamp, P::ReturnValue));
    };
    for (const auto& Setting : Settings::NumericSettings)
        ReadKey(Setting.Key, Setting.Field, Setting.Minimum, Setting.Maximum);

    G.Tail = SettingType->GetThenPinGivenIndex(1);
    auto* NextTextKey = G.Tail;
    for (const auto& Setting : Settings::TextSettings)
    {
        G.Tail = NextTextKey;
        auto* Match = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
        G.Link(Key, G.Pin(Match, P::Binary::LeftOperand)); G.Default(Match, P::Binary::RightOperand, Setting.Key);
        auto* Branch = G.Branch(G.Pin(Match, P::ReturnValue)); NextTextKey = G.Pin(Branch, P::Else);
        G.Write(Setting.Field, Text);
    }
    // A missing file returns an empty array. Parsing must never block entry.
    G.Tail = Work->GetThenPinGivenIndex(1);
    G.Write(Aim::Yaw, ClampStationAim(G, true, G.Read(Aim::Yaw)));
    G.Write(Aim::Pitch, ClampStationAim(G, false, G.Read(Aim::Pitch)));
    auto LoadFont = [&](FName PathField, FName ObjectField)
    {
        auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
        G.Link(G.Read(PathField), G.Pin(Path, E::PathString));
        auto* Reference = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef));
        G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Reference, Settings::SoftObjectPathPin));
        auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
        G.Link(G.Pin(Reference, P::ReturnValue), G.Pin(Load, Settings::AssetPin)); G.Exec(Load);
        G.Write(ObjectField, G.Pin(Load, P::ReturnValue));
    };
    for (const auto& Setting : Settings::FontSettings) LoadFont(Setting.PathField, Setting.ObjectField);
}
