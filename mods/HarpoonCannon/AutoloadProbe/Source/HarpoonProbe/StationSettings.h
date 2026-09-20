#pragma once
namespace Settings
{
inline const FName Mouse(TEXT("HarpoonMousePercent"));
inline const FName Yaw(TEXT("HarpoonYawLimitDegrees"));
inline const FName PitchMin(TEXT("HarpoonMinimumPitchDegrees"));
inline const FName PitchMax(TEXT("HarpoonMaximumPitchDegrees"));
inline const FName FilePath(TEXT("InPath")); // Voyage declaring signature.
inline constexpr TCHAR RelativePath[] = TEXT("Paks/HarpoonCannon.ini");
inline constexpr TCHAR Separator[] = TEXT("=");
inline constexpr TCHAR MouseKey[] = TEXT("OpticsMousePercent");
inline constexpr TCHAR YawKey[] = TEXT("YawLimitDegrees");
inline constexpr TCHAR PitchMinKey[] = TEXT("MinimumPitchDegrees");
inline constexpr TCHAR PitchMaxKey[] = TEXT("MaximumPitchDegrees");
inline constexpr TCHAR ShotVolumeKey[] = TEXT("ShotVolumePercent");
inline constexpr TCHAR MouseDefault[] = TEXT("35.0");
inline constexpr TCHAR YawDefault[] = TEXT("80.0");
inline constexpr TCHAR ShotVolumeDefault[] = TEXT("600.0");
inline constexpr TCHAR OriginalPerPercent[] = TEXT("0.0128");
inline constexpr TCHAR PercentMin[] = TEXT("1.0");
inline constexpr TCHAR PercentMax[] = TEXT("100.0");
inline constexpr TCHAR YawMax[] = TEXT("170.0");
inline constexpr TCHAR PitchLowerBound[] = TEXT("-89.0");
inline constexpr TCHAR PitchUpperBound[] = TEXT("89.0");
inline constexpr TCHAR Negate[] = TEXT("-1.0");
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
    G.Tail = NextKey;
    auto* VolumeMatch = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(Key, G.Pin(VolumeMatch, P::Binary::LeftOperand)); G.Default(VolumeMatch, P::Binary::RightOperand, Settings::ShotVolumeKey);
    G.Branch(G.Pin(VolumeMatch, P::ReturnValue));
    auto* NonnegativeVolume = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax));
    G.Link(G.Pin(Value, P::ReturnValue), G.Pin(NonnegativeVolume, P::Binary::LeftOperand)); G.Default(NonnegativeVolume, P::Binary::RightOperand, N::Zero);
    G.Write(ShotAudio::VolumePercent, G.Pin(NonnegativeVolume, P::ReturnValue));
    // A missing file returns an empty array. Parsing must never block entry.
    G.Tail = Work->GetThenPinGivenIndex(1);
    G.Write(Aim::Yaw, ClampStationAim(G, true, G.Read(Aim::Yaw)));
    G.Write(Aim::Pitch, ClampStationAim(G, false, G.Read(Aim::Pitch)));
}
