#include "StationSettings.h"

#include "RailgunStationInitialization.h"
#include "StationAimNames.h"
#include "StationEntryGraph.h"
#include "StationOpticsGraph.h"
#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
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
    UBlueprint* Blueprint = G.Graph->GetTypedOuter<UBlueprint>();
    check(Blueprint && Blueprint->GeneratedClass);
    UK2Node_CallFunction* Reload = G.Call(
        Blueprint->GeneratedClass, Settings::ReloadSettings);
    G.Exec(Reload);
    G.Write(Aim::Yaw, ClampStationAim(G, true, G.Read(Aim::Yaw)));
    G.Write(Aim::Pitch, ClampStationAim(G, false, G.Read(Aim::Pitch)));
    auto LoadFont = [&](const TCHAR* ObjectPath, FName ObjectField)
    {
        auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
        G.Default(Path, E::PathString, ObjectPath);
        auto* Reference = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef));
        G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Reference, Settings::SoftObjectPathPin));
        auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
        G.Link(G.Pin(Reference, P::ReturnValue), G.Pin(Load, Settings::AssetPin)); G.Exec(Load);
        G.Write(ObjectField, G.Pin(Load, P::ReturnValue));
    };
    for (const auto& Font : Settings::FixedHudFonts)
        LoadFont(Font.ObjectPath, Font.ObjectField);
}
}
