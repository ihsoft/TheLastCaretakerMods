#pragma once

// HC20 observes an EXISTING forklift. No spawn, drive, physics, attachment,
// item substitution, native HUD writes or modification of DonkLift assets.
namespace ForkliftEntryNames
{
inline constexpr TCHAR Title[] = TEXT("HC20 EXISTING FORKLIFT / NATIVE ENTRY COMPARISON");
inline constexpr TCHAR Ready[] = TEXT("HC20: test normal entry first; exit, aim at forklift, F8 enters");
inline constexpr TCHAR Active[] = TEXT("HC20 F8 ENTRY: compare native HUD; F8 exits (max20s). No saving.");
inline constexpr TCHAR Returned[] = TEXT("HC20 RETURNED: check walking/actions; screenshot and quit");
inline constexpr TCHAR Failed[] = TEXT("HC20 STOP: prerequisite/entry/ownership failed; screenshot and quit");
inline constexpr TCHAR WrongTarget[] = TEXT("HC20: on foot, aim within3m at a parked built forklift");
inline constexpr TCHAR ExitRequested[] = TEXT("HC20 native EXIT requested; F8 retries if still inside");
inline constexpr TCHAR ClassPath[] = TEXT("/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C");
inline constexpr TCHAR CurrentPawn[] = TEXT("Controlled pawn: ");
inline constexpr TCHAR OwnedYes[] = TEXT("Our F8 entry target controlled: YES");
inline constexpr TCHAR OwnedNo[] = TEXT("Our F8 entry target controlled: NO");
inline constexpr TCHAR Seconds[] = TEXT("F8 control seconds (max20): ");
inline constexpr TCHAR Unchanged[] = TEXT("Forklift physics / HUD / inputs: unmodified by HC20");
}
namespace F = ForkliftEntryNames;

void ForkliftNativeExit(FGraph& G)
{
    G.Write(V::ExitSent, nullptr, N::True);
    auto* Exit = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle));
    G.Link(G.Read(V::Vehicle), G.Pin(Exit, P::FunctionTarget)); G.Exec(Exit);
    G.Text(N::FreezeStatus, F::ExitRequested);
}

void BuildForkliftEntryGraph(UBlueprint* BP, UClass* HudClass)
{
    UEdGraph* Graph = BP->UbergraphPages[0]; const auto Defaults = Graph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Controller = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    G.Branch(G.Valid(G.Pin(Controller, P::ReturnValue)));
    auto* HasOwner = G.Branch(G.Valid(G.Read(S::Controller))); auto* OwnerTail = G.Tail;
    G.Tail = G.Pin(HasOwner, P::Else); G.Write(S::Controller, G.Pin(Controller, P::ReturnValue));
    StationMerge(G, {OwnerTail, G.Tail});
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), G.Read(S::Controller), G.Pin(Controller, P::ReturnValue)));
    auto* HudReady = G.Branch(G.Valid(G.Read(N::HudInstance))); auto* ReadyTail = G.Tail;
    G.Tail = G.Pin(HudReady, P::Else);
    auto* Create = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Pin(Create, E::WidgetType)->DefaultObject = HudClass; G.Link(G.Read(S::Controller), G.Pin(Create, E::OwningPlayer)); G.Exec(Create);
    auto* HudCast = NewObject<UK2Node_DynamicCast>(Graph); HudCast->TargetType = HudClass; HudCast->SetPurity(false); G.Node(HudCast);
    G.Link(G.Tail, G.Pin(HudCast, P::Execute)); G.Link(G.Pin(Create, P::ReturnValue), HudCast->GetCastSourcePin());
    G.Tail = HudCast->GetValidCastPin(); G.Write(N::HudInstance, HudCast->GetCastResultPin());
    auto* Add = G.Call(UUserWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport));
    G.Link(G.Read(N::HudInstance), G.Pin(Add, P::FunctionTarget)); G.Default(Add, E::ZOrder, N::HudZOrder); G.Exec(Add);
    G.Tail = ReadyTail;
    auto* PawnText = ObserveObjectString(G, nullptr, F::CurrentPawn, NativeControlled(G));
    auto* AsText = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Link(PawnText, G.Pin(AsText, E::StringValue)); G.Text(N::Current, nullptr, G.Pin(AsText, P::ReturnValue));
    auto* Key = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Read(S::Controller), G.Pin(Key, P::FunctionTarget)); G.Default(Key, P::Key, S::Key);
    auto* Owned = G.Branch(G.Read(V::Occupied)); auto* InsideTail = G.Tail;

    // A normal E entry is observation-only: no timer or F8 exit ownership.
    G.Tail = G.Pin(Owned, P::Else); G.Branch(G.Pin(Key, P::ReturnValue));
    auto* Character = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCharacter));
    G.Require(G.Valid(G.Pin(Character, P::ReturnValue)), F::WrongTarget);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Pin(Character, P::ReturnValue)), F::WrongTarget);
    G.Require(ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, HasAuthority), G.Read(S::Controller)), F::Failed);
    G.Require(ObserveCall(G, APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, IsLocalController), G.Read(S::Controller)), F::Failed);
    G.Write(N::OriginalPawn, G.Pin(Character, P::ReturnValue));
    auto* Move = ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, GetMovementComponent), G.Read(N::OriginalPawn));
    auto* MoveCast = NewObject<UK2Node_DynamicCast>(Graph); MoveCast->TargetType = UCharacterMovementComponent::StaticClass(); MoveCast->SetPurity(false); G.Node(MoveCast);
    G.Link(G.Tail, G.Pin(MoveCast, P::Execute)); G.Link(Move, MoveCast->GetCastSourcePin()); G.Tail = MoveCast->GetValidCastPin(); G.Write(S::Movement, MoveCast->GetCastResultPin());
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), S::WalkingByte), F::WrongTarget);
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsMoveInputIgnored), G.Read(S::Controller)), N::False), F::WrongTarget);
    auto* Camera = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Require(G.Valid(G.Pin(Camera, P::ReturnValue)), F::Failed);
    auto* Location = ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraLocation), G.Pin(Camera, P::ReturnValue));
    auto* Rotation = ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraRotation), G.Pin(Camera, P::ReturnValue));
    auto* Forward = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GetForwardVector)); G.Link(Rotation, G.Pin(Forward, SP::RotationValue));
    auto* Ray = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat));
    G.Link(G.Pin(Forward, P::ReturnValue), G.Pin(Ray, P::Binary::LeftOperand)); G.Default(Ray, P::Binary::RightOperand, S::Range);
    auto* Trace = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LineTraceSingle));
    G.Link(Location, G.Pin(Trace, E::TraceStart));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector), Location, G.Pin(Ray, P::ReturnValue)), G.Pin(Trace, E::TraceEnd));
    G.Link(G.ActorArray(G.Read(N::OriginalPawn), G.Read(N::OriginalPawn)), G.Pin(Trace, E::ActorsToIgnore));
    G.Default(Trace, E::TraceChannel, N::VisibilityTrace); G.Default(Trace, E::TraceComplex, N::False); G.Default(Trace, E::IgnoreSelf, N::True); G.Exec(Trace);
    G.Require(G.Pin(Trace, P::ReturnValue), F::WrongTarget);
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult)); G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit));
    auto* Actor = G.Pin(Hit, SP::HitActor); G.Require(G.Valid(Actor), F::WrongTarget);
    auto* Class = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetObjectClass)); G.Link(Actor, G.Pin(Class, P::Object));
    auto* Soft = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetSoftClassPath)); G.Link(G.Pin(Class, P::ReturnValue), G.Pin(Soft, P::Class));
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, BreakSoftClassPath)); G.Link(G.Pin(Soft, P::ReturnValue), G.Pin(Path, SP::SoftClassPathInput));
    auto* Equal = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(Path, E::PathString), G.Pin(Equal, P::Binary::LeftOperand)); G.Default(Equal, P::Binary::RightOperand, F::ClassPath); G.Require(G.Pin(Equal, P::ReturnValue), F::WrongTarget);
    auto* Cast = NewObject<UK2Node_DynamicCast>(Graph); Cast->TargetType = AVoyageVehiclePawn::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Actor, Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin(); G.Write(V::Vehicle, Cast->GetCastResultPin());
    auto* Driver = ObserveCall(G, APawn::StaticClass(), BlueprintGraphNames::ActorFunctions::GetController, G.Read(V::Vehicle));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Valid(Driver), N::False), F::WrongTarget);
    G.Write(N::Age, nullptr, N::Zero); G.Write(V::ExitSent, nullptr, N::False);
    auto* Enter = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnEnterVehicle));
    G.Link(G.Read(V::Vehicle), G.Pin(Enter, P::FunctionTarget)); G.Link(G.Read(S::Controller), G.Pin(Enter, V::NewPossessor)); G.Exec(Enter);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(V::Vehicle)), F::Failed);
    G.Write(V::Occupied, nullptr, N::True); G.Text(N::FreezeStatus, F::Active);

    G.Tail = InsideTail;
    auto* Ours = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(V::Vehicle));
    G.BooleanText(N::Control, Ours, F::OwnedYes, F::OwnedNo);
    auto* StillInside = G.Branch(Ours); auto* ControlledTail = G.Tail;
    G.Tail = G.Pin(StillInside, P::Else); G.Write(V::Occupied, nullptr, N::False);
    G.BooleanText(N::FreezeStatus, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(N::OriginalPawn)), F::Returned, F::Failed);
    G.Tail = ControlledTail; G.Require(G.Valid(G.Read(V::Vehicle)), F::Failed);
    G.Write(N::Age, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(N::Age), G.Pin(Tick, P::DeltaSeconds)));
    G.Number(N::Clock, F::Seconds, G.Read(N::Age));
    auto* AutoExit = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Read(V::ExitSent), N::False),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Read(N::Age), S::Limit));
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), G.Pin(Key, P::ReturnValue), AutoExit)); ForkliftNativeExit(G);
}
