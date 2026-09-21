#pragma once
namespace Settings
{
inline const FName Mouse(TEXT("RailgunMousePercent"));
inline const FName Yaw(TEXT("RailgunYawLimitDegrees"));
inline const FName PitchMin(TEXT("RailgunMinimumPitchDegrees"));
inline const FName PitchMax(TEXT("RailgunMaximumPitchDegrees"));
inline const FName StatusIconOpacity(TEXT("RailgunStatusIconOpacityPercent"));
inline const FName TargetNameOffsetX(TEXT("RailgunTargetNameOffsetX"));
inline const FName TargetNameOffsetY(TEXT("RailgunTargetNameOffsetY"));
inline const FName TargetNameOpacity(TEXT("RailgunTargetNameOpacityPercent"));
inline const FName TargetNameFontSize(TEXT("RailgunTargetNameFontSize"));
inline const FName TargetDistanceOffsetX(TEXT("RailgunTargetDistanceOffsetX"));
inline const FName TargetDistanceOffsetY(TEXT("RailgunTargetDistanceOffsetY"));
inline const FName TargetDistanceOpacity(TEXT("RailgunTargetDistanceOpacityPercent"));
inline const FName TargetDistanceFontSize(TEXT("RailgunTargetDistanceFontSize"));
inline const FName TargetNameFontPath(TEXT("RailgunTargetNameFontPath"));
inline const FName TargetNameTypeface(TEXT("RailgunTargetNameTypeface"));
inline const FName TargetDistanceFontPath(TEXT("RailgunTargetDistanceFontPath"));
inline const FName TargetDistanceTypeface(TEXT("RailgunTargetDistanceTypeface"));
inline const FName TargetNameFontObject(TEXT("RailgunTargetNameFontObject"));
inline const FName TargetDistanceFontObject(TEXT("RailgunTargetDistanceFontObject"));
inline const FName FilePath(TEXT("InPath")); // Voyage declaring signature.
inline constexpr TCHAR RelativePath[] = TEXT("Paks/Railgun.ini");
inline constexpr TCHAR Separator[] = TEXT("=");
inline constexpr TCHAR MouseKey[] = TEXT("OpticsMousePercent");
inline constexpr TCHAR YawKey[] = TEXT("YawLimitDegrees");
inline constexpr TCHAR PitchMinKey[] = TEXT("MinimumPitchDegrees");
inline constexpr TCHAR PitchMaxKey[] = TEXT("MaximumPitchDegrees");
inline constexpr TCHAR ShotVolumeKey[] = TEXT("ShotVolumePercent");
inline constexpr TCHAR HitDamageKey[] = TEXT("HitDamage");
inline constexpr TCHAR FullChargeEnergyKey[] = TEXT("FullChargeEnergyKJ");
inline constexpr TCHAR FullChargeTimeKey[] = TEXT("FullChargeTimeSeconds");
inline constexpr TCHAR StatusIconOpacityKey[] = TEXT("StatusIconOpacityPercent");
inline constexpr TCHAR TargetNameOffsetXKey[] = TEXT("TargetNameOffsetX");
inline constexpr TCHAR TargetNameOffsetYKey[] = TEXT("TargetNameOffsetY");
inline constexpr TCHAR TargetNameOpacityKey[] = TEXT("TargetNameOpacityPercent");
inline constexpr TCHAR TargetNameFontSizeKey[] = TEXT("TargetNameFontSize");
inline constexpr TCHAR TargetDistanceOffsetXKey[] = TEXT("TargetDistanceOffsetX");
inline constexpr TCHAR TargetDistanceOffsetYKey[] = TEXT("TargetDistanceOffsetY");
inline constexpr TCHAR TargetDistanceOpacityKey[] = TEXT("TargetDistanceOpacityPercent");
inline constexpr TCHAR TargetDistanceFontSizeKey[] = TEXT("TargetDistanceFontSize");
inline constexpr TCHAR TargetNameFontPathKey[] = TEXT("TargetNameFontPath");
inline constexpr TCHAR TargetNameTypefaceKey[] = TEXT("TargetNameTypeface");
inline constexpr TCHAR TargetDistanceFontPathKey[] = TEXT("TargetDistanceFontPath");
inline constexpr TCHAR TargetDistanceTypefaceKey[] = TEXT("TargetDistanceTypeface");
inline constexpr TCHAR MouseDefault[] = TEXT("35.0");
inline constexpr TCHAR YawDefault[] = TEXT("80.0");
inline constexpr TCHAR ShotVolumeDefault[] = TEXT("600.0");
inline constexpr TCHAR HitDamageDefault[] = TEXT("200.0");
inline constexpr TCHAR FullChargeEnergyDefault[] = TEXT("500.0");
inline constexpr TCHAR FullChargeTimeDefault[] = TEXT("2.0");
inline constexpr TCHAR StatusIconOpacityDefault[] = TEXT("100.0");
inline constexpr TCHAR TargetNameOffsetXDefault[] = TEXT("0.0");
inline constexpr TCHAR TargetNameOffsetYDefault[] = TEXT("72.0");
inline constexpr TCHAR TargetDistanceOffsetXDefault[] = TEXT("0.0");
inline constexpr TCHAR TargetDistanceOffsetYDefault[] = TEXT("100.0");
inline constexpr TCHAR TargetOpacityDefault[] = TEXT("100.0");
inline constexpr TCHAR TargetFontSizeDefault[] = TEXT("20.0");
inline constexpr TCHAR TargetFontPathDefault[] = TEXT("/Engine/EngineFonts/Roboto.Roboto");
inline constexpr TCHAR TargetTypefaceDefault[] = TEXT("Bold");
inline constexpr TCHAR OriginalPerPercent[] = TEXT("0.0128");
inline constexpr TCHAR PercentMultiplier[] = TEXT("0.01");
inline constexpr TCHAR PercentMin[] = TEXT("1.0");
inline constexpr TCHAR PercentMax[] = TEXT("100.0");
inline constexpr TCHAR OpacityMin[] = TEXT("0.0");
inline constexpr TCHAR OffsetMin[] = TEXT("-4096.0");
inline constexpr TCHAR OffsetMax[] = TEXT("4096.0");
inline constexpr TCHAR FontSizeMin[] = TEXT("1.0");
inline constexpr TCHAR FontSizeMax[] = TEXT("200.0");
inline constexpr TCHAR YawMax[] = TEXT("170.0");
inline constexpr TCHAR PitchLowerBound[] = TEXT("-89.0");
inline constexpr TCHAR PitchUpperBound[] = TEXT("89.0");
inline constexpr TCHAR DamageMin[] = TEXT("0.0");
inline constexpr TCHAR PositiveGameplayMin[] = TEXT("0.001");
inline constexpr TCHAR GameplayMax[] = TEXT("1000000000.0");
inline constexpr TCHAR ChargeTimeMax[] = TEXT("86400.0");
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

struct FNumericDisplaySetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
    const TCHAR* Minimum;
    const TCHAR* Maximum;
};
inline const FNumericDisplaySetting DisplayNumbers[] = {
    {StatusIconOpacityKey, StatusIconOpacity, StatusIconOpacityDefault, OpacityMin, PercentMax},
    {TargetNameOffsetXKey, TargetNameOffsetX, TargetNameOffsetXDefault, OffsetMin, OffsetMax},
    {TargetNameOffsetYKey, TargetNameOffsetY, TargetNameOffsetYDefault, OffsetMin, OffsetMax},
    {TargetNameOpacityKey, TargetNameOpacity, TargetOpacityDefault, OpacityMin, PercentMax},
    {TargetNameFontSizeKey, TargetNameFontSize, TargetFontSizeDefault, FontSizeMin, FontSizeMax},
    {TargetDistanceOffsetXKey, TargetDistanceOffsetX, TargetDistanceOffsetXDefault, OffsetMin, OffsetMax},
    {TargetDistanceOffsetYKey, TargetDistanceOffsetY, TargetDistanceOffsetYDefault, OffsetMin, OffsetMax},
    {TargetDistanceOpacityKey, TargetDistanceOpacity, TargetOpacityDefault, OpacityMin, PercentMax},
    {TargetDistanceFontSizeKey, TargetDistanceFontSize, TargetFontSizeDefault, FontSizeMin, FontSizeMax}
};
inline const FNumericDisplaySetting GameplayNumbers[] = {
    {HitDamageKey, ShotAttack::ConfiguredDamage, HitDamageDefault, DamageMin, GameplayMax},
    {FullChargeEnergyKey, Charge::ConfiguredEnergyKJ, FullChargeEnergyDefault, PositiveGameplayMin, GameplayMax},
    {FullChargeTimeKey, Charge::ConfiguredTimeSeconds, FullChargeTimeDefault, PositiveGameplayMin, ChargeTimeMax}
};

struct FTextDisplaySetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
};
inline const FTextDisplaySetting DisplayText[] = {
    {TargetNameFontPathKey, TargetNameFontPath, TargetFontPathDefault},
    {TargetNameTypefaceKey, TargetNameTypeface, TargetTypefaceDefault},
    {TargetDistanceFontPathKey, TargetDistanceFontPath, TargetFontPathDefault},
    {TargetDistanceTypefaceKey, TargetDistanceTypeface, TargetTypefaceDefault}
};
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
    G.Write(Settings::Mouse, nullptr, Settings::MouseDefault);
    G.Write(Settings::Yaw, nullptr, Settings::YawDefault);
    G.Write(Settings::PitchMin, nullptr, Aim::MinimumPitch);
    G.Write(Settings::PitchMax, nullptr, Aim::MaximumPitch);
    G.Write(ShotAudio::VolumePercent, nullptr, Settings::ShotVolumeDefault);
    for (const auto& Setting : Settings::GameplayNumbers) G.Write(Setting.Field, nullptr, Setting.Default);
    for (const auto& Setting : Settings::DisplayNumbers) G.Write(Setting.Field, nullptr, Setting.Default);
    for (const auto& Setting : Settings::DisplayText) G.Write(Setting.Field, nullptr, Setting.Default);
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
    ReadKey(Settings::MouseKey, Settings::Mouse, Settings::PercentMin, Settings::PercentMax);
    ReadKey(Settings::YawKey, Settings::Yaw, Settings::PercentMin, Settings::YawMax);
    ReadKey(Settings::PitchMinKey, Settings::PitchMin, Settings::PitchLowerBound, N::Zero);
    ReadKey(Settings::PitchMaxKey, Settings::PitchMax, N::Zero, Settings::PitchUpperBound);
    for (const auto& Setting : Settings::GameplayNumbers) ReadKey(Setting.Key, Setting.Field, Setting.Minimum, Setting.Maximum);
    for (const auto& Setting : Settings::DisplayNumbers) ReadKey(Setting.Key, Setting.Field, Setting.Minimum, Setting.Maximum);
    G.Tail = NextKey;
    auto* VolumeMatch = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(Key, G.Pin(VolumeMatch, P::Binary::LeftOperand)); G.Default(VolumeMatch, P::Binary::RightOperand, Settings::ShotVolumeKey);
    G.Branch(G.Pin(VolumeMatch, P::ReturnValue));
    auto* NonnegativeVolume = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax));
    G.Link(G.Pin(Value, P::ReturnValue), G.Pin(NonnegativeVolume, P::Binary::LeftOperand)); G.Default(NonnegativeVolume, P::Binary::RightOperand, N::Zero);
    G.Write(ShotAudio::VolumePercent, G.Pin(NonnegativeVolume, P::ReturnValue));

    G.Tail = SettingType->GetThenPinGivenIndex(1);
    auto* NextTextKey = G.Tail;
    for (const auto& Setting : Settings::DisplayText)
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
    LoadFont(Settings::TargetNameFontPath, Settings::TargetNameFontObject);
    LoadFont(Settings::TargetDistanceFontPath, Settings::TargetDistanceFontObject);
}
