#pragma once

// Included after FGraph. Engine-only observation: NEVER writes player/boat state.
namespace HelmProbeNames
{
inline const FName SawEntry(TEXT("ObservedControlTransfer"));
inline const FName Complete(TEXT("ObservationComplete"));
inline const FName Carrier(TEXT("ObservedControlledPawn"));
inline const FName EntryLocal(TEXT("CharacterEntryLocalPosition"));
inline const FName CarrierStart(TEXT("CarrierEntryWorldPosition"));
inline const FName MaxDrift(TEXT("MaximumLocalDrift"));
inline constexpr float TickInterval = 0.2f;
inline const FVector2D PanelSize(740.0f, 440.0f);
inline constexpr TCHAR Title[] = TEXT("HC10 HELM OBSERVER - READ ONLY");
inline constexpr TCHAR Ready[] = TEXT("Enter the normal ship helm, then exit. No F8 needed.");
inline constexpr TCHAR Inside[] = TEXT("OTHER PAWN CONTROLLED: look around; then use normal exit");
inline constexpr TCHAR Finished[] = TEXT("DONE: take this screenshot BEFORE closing the game");
inline constexpr TCHAR Lost[] = TEXT("UNAVAILABLE: original character/controller lost");
inline constexpr TCHAR Changed[] = TEXT("STOP: switched to a different vehicle; do not infer results");
inline constexpr TCHAR Before[] = TEXT("BEFORE: ");
inline constexpr TCHAR Occupied[] = TEXT("INSIDE: ");
inline constexpr TCHAR After[] = TEXT("AFTER: ");
inline constexpr TCHAR Parent[] = TEXT("\n  root parent: ");
inline constexpr TCHAR Base[] = TEXT("\n  movement base: ");
inline constexpr TCHAR Mode[] = TEXT("\n  movement mode: ");
inline constexpr TCHAR ModeLegend[] = TEXT("Mode 0=None, 1=Walking, 2=NavWalking, 3=Falling, 4=Swimming, 5=Flying, 6=Custom");
inline constexpr TCHAR Waiting[] = TEXT("Not sampled yet (not zero)");
inline constexpr TCHAR Drift[] = TEXT("Max character-local drift (cm): ");
inline constexpr TCHAR Travel[] = TEXT("\nCarrier world travel since entry (cm): ");
inline constexpr TCHAR LocalAngle[] = TEXT("\nCharacter rotation relative to carrier: ");
}
namespace C = HelmProbeNames;
namespace CP = CharacterObservationGraphNames;

UEdGraphPin* ObserveCall(FGraph& G, UClass* Owner, FName Function, UEdGraphPin* Target)
{
    auto* Call = G.Call(Owner, Function);
    G.Link(Target, G.Pin(Call, P::FunctionTarget));
    if (Call->FindPin(P::Execute)) G.Exec(Call);
    return G.Pin(Call, P::ReturnValue);
}

UEdGraphPin* ObserveObjectString(FGraph& G, UEdGraphPin* Prior, const TCHAR* Prefix, UEdGraphPin* Object)
{
    auto* Build = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Object));
    if (Prior) G.Link(Prior, G.Pin(Build, CP::AppendTo));
    G.Default(Build, E::StringPrefix, Prefix); G.Link(Object, G.Pin(Build, CP::ObjectValue));
    return G.Pin(Build, P::ReturnValue);
}

void ObserveSnapshot(FGraph& G, FName Row, const TCHAR* Prefix, UEdGraphPin* Movement, UEdGraphPin* Parent, UEdGraphPin* Base)
{
    auto* Label = ObserveObjectString(G, nullptr, Prefix, G.Read(N::OriginalPawn));
    Label = ObserveObjectString(G, Label, C::Parent, Parent);
    Label = ObserveObjectString(G, Label, C::Base, Base);
    auto* Mode = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName ModeName = GET_MEMBER_NAME_CHECKED(UCharacterMovementComponent, MovementMode);
    Mode->VariableReference.SetExternalMember(ModeName, UCharacterMovementComponent::StaticClass()); G.Node(Mode);
    G.Link(Movement, G.Pin(Mode, P::FunctionTarget));
    auto* Integer = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_ByteToInt));
    G.Link(G.Pin(Mode, ModeName), G.Pin(Integer, CP::ByteValue));
    auto* Build = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Int));
    G.Link(Label, G.Pin(Build, CP::AppendTo)); G.Default(Build, E::StringPrefix, C::Mode);
    G.Link(G.Pin(Integer, P::ReturnValue), G.Pin(Build, CP::IntegerValue));
    auto* Text = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Link(G.Pin(Build, P::ReturnValue), G.Pin(Text, E::StringValue));
    G.Text(Row, nullptr, G.Pin(Text, P::ReturnValue));
}

UEdGraphPin* ObserveLocalPosition(FGraph& G, UEdGraphPin* CharacterLocation, UEdGraphPin* CarrierTransform)
{
    auto* Local = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, InverseTransformLocation));
    G.Link(CarrierTransform, G.Pin(Local, E::Transform)); G.Link(CharacterLocation, G.Pin(Local, E::Location));
    return G.Pin(Local, P::ReturnValue);
}
