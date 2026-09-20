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

void BuildHelmObservationGraph(UBlueprint* BP, UClass* HudClass)
{
    UEdGraph* Graph = BP->UbergraphPages[0];
    const auto Defaults = Graph->Nodes; for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Done = G.Branch(G.Read(C::Complete)); G.Tail = G.Pin(Done, P::Else);
    auto* Controller = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    G.Branch(G.Valid(G.Pin(Controller, P::ReturnValue)));
    auto* HudReady = G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* ExistingHud = G.Tail; G.Tail = G.Pin(HudReady, P::Else);
    auto* Create = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Pin(Create, E::WidgetType)->DefaultObject = HudClass;
    G.Link(G.Pin(Controller, P::ReturnValue), G.Pin(Create, E::OwningPlayer)); G.Exec(Create);
    auto* HudCast = NewObject<UK2Node_DynamicCast>(Graph); HudCast->TargetType = HudClass; HudCast->SetPurity(false); G.Node(HudCast);
    G.Link(G.Tail, G.Pin(HudCast, P::Execute)); G.Link(G.Pin(Create, P::ReturnValue), HudCast->GetCastSourcePin());
    G.Tail = HudCast->GetValidCastPin(); G.Write(N::HudInstance, HudCast->GetCastResultPin());
    auto* Add = G.Call(UUserWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport));
    G.Link(G.Read(N::HudInstance), G.Pin(Add, P::FunctionTarget)); G.Default(Add, E::ZOrder, N::HudZOrder); G.Exec(Add);
    // Newly created HUD continues on the next sample, not through two exec owners.
    G.Tail = ExistingHud;
    auto* OriginalReady = G.Branch(G.Valid(G.Read(N::OriginalPawn)));
    auto* OriginalValid = G.Tail; G.Tail = G.Pin(OriginalReady, P::Else);
    auto* NeverEntered = G.Branch(G.Read(C::SawEntry));
    G.Text(N::FreezeStatus, C::Lost); G.Write(C::Complete, nullptr, N::True);
    G.Tail = G.Pin(NeverEntered, P::Else);
    auto* Character = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCharacter));
    G.Require(G.Valid(G.Pin(Character, P::ReturnValue)), C::Lost);
    G.Write(N::OriginalPawn, G.Pin(Character, P::ReturnValue));
    G.Tail = OriginalValid;
    auto* Pawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerPawn));
    G.Require(G.Valid(G.Pin(Pawn, P::ReturnValue)), C::Lost);
    auto* Movement = ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, GetMovementComponent), G.Read(N::OriginalPawn));
    auto* MovementCast = NewObject<UK2Node_DynamicCast>(Graph); MovementCast->TargetType = UCharacterMovementComponent::StaticClass();
    MovementCast->SetPurity(false); G.Node(MovementCast); G.Link(G.Tail, G.Pin(MovementCast, P::Execute));
    G.Link(Movement, MovementCast->GetCastSourcePin()); G.Tail = MovementCast->GetInvalidCastPin(); G.Text(N::FreezeStatus, C::Lost);
    G.Tail = MovementCast->GetValidCastPin(); Movement = MovementCast->GetCastResultPin();
    auto* Base = ObserveCall(G, UCharacterMovementComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCharacterMovementComponent, GetMovementBaseObject), Movement);
    auto* Root = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), G.Read(N::OriginalPawn));
    G.Require(G.Valid(Root), C::Lost);
    auto* Parent = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), Root);
    auto* SamePawn = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), G.Pin(Pawn, P::ReturnValue), G.Read(N::OriginalPawn));
    auto* OnFoot = G.Branch(SamePawn);
    auto* WasOccupied = G.Branch(G.Read(C::SawEntry));
    ObserveSnapshot(G, N::Clock, C::After, Movement, Parent, Base);
    G.Text(N::FreezeStatus, C::Finished); G.Write(C::Complete, nullptr, N::True);
    G.Tail = G.Pin(WasOccupied, P::Else);
    ObserveSnapshot(G, N::Current, C::Before, Movement, Parent, Base); G.Text(N::FreezeStatus, C::Ready);
    G.Tail = G.Pin(OnFoot, P::Else);
    auto* HasCarrier = G.Branch(G.Read(C::SawEntry)); auto* CarrierReady = G.Tail; G.Tail = G.Pin(HasCarrier, P::Else);
    G.Write(C::Carrier, G.Pin(Pawn, P::ReturnValue));
    auto* Location = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), G.Read(N::OriginalPawn));
    auto* CarrierTransform = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetTransform), G.Read(C::Carrier));
    auto* Local = ObserveLocalPosition(G, Location, CarrierTransform);
    G.Write(C::EntryLocal, Local);
    auto* CarrierLocation = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), G.Read(C::Carrier));
    G.Write(C::CarrierStart, CarrierLocation); G.Write(C::SawEntry, nullptr, N::True);
    G.Tail = CarrierReady;
    G.Require(G.Valid(G.Read(C::Carrier)), C::Lost);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), G.Read(C::Carrier), G.Pin(Pawn, P::ReturnValue)), C::Changed);
    ObserveSnapshot(G, N::Control, C::Occupied, Movement, Parent, Base); G.Text(N::FreezeStatus, C::Inside);
    auto* Distance = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), Local, G.Read(C::EntryLocal)), G.Pin(Distance, E::VectorLengthInput));
    G.Write(C::MaxDrift, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMax), G.Read(C::MaxDrift), G.Pin(Distance, P::ReturnValue)));
    auto* TravelLength = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), CarrierLocation, G.Read(C::CarrierStart)), G.Pin(TravelLength, E::VectorLengthInput));
    auto* DriftString = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Double));
    G.Default(DriftString, E::StringPrefix, C::Drift); G.Link(G.Read(C::MaxDrift), G.Pin(DriftString, E::DoubleValue));
    auto* TravelString = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Double));
    G.Link(G.Pin(DriftString, P::ReturnValue), G.Pin(TravelString, CP::AppendTo)); G.Default(TravelString, E::StringPrefix, C::Travel);
    G.Link(G.Pin(TravelLength, P::ReturnValue), G.Pin(TravelString, E::DoubleValue));
    auto* Rotation = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorRotation), G.Read(N::OriginalPawn));
    auto* LocalRotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, InverseTransformRotation));
    G.Link(CarrierTransform, G.Pin(LocalRotation, E::Transform)); G.Link(Rotation, G.Pin(LocalRotation, E::Rotation));
    auto* AngleString = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Rotator));
    G.Link(G.Pin(TravelString, P::ReturnValue), G.Pin(AngleString, CP::AppendTo)); G.Default(AngleString, E::StringPrefix, C::LocalAngle);
    G.Link(G.Pin(LocalRotation, P::ReturnValue), G.Pin(AngleString, CP::RotatorValue));
    auto* StatsText = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Link(G.Pin(AngleString, P::ReturnValue), G.Pin(StatsText, E::StringValue)); G.Text(N::Drift, nullptr, G.Pin(StatsText, P::ReturnValue));

    auto* End = NewObject<UK2Node_Event>(Graph); End->EventReference.SetExternalMember(E::EndPlayEvent, AActor::StaticClass());
    End->bOverrideFunction = true; G.Node(End); G.Tail = G.Pin(End, P::Then);
    G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* Remove = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, RemoveFromParent));
    G.Link(G.Read(N::HudInstance), G.Pin(Remove, P::FunctionTarget)); G.Exec(Remove);
}
