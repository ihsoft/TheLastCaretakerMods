#pragma once

// Included inside the generator's anonymous namespace after FGraph.
// Engine-only input gate, not a character/world-position/physics freeze.
namespace CharacterProbeNames
{
inline const FName OwnerController(TEXT("InputGateController"));
inline const FName OwnsGate(TEXT("OwnsMoveInputGate"));
inline constexpr TCHAR ToggleKey[] = TEXT("F8"); // FKey ImportText reads a bare key token.
inline constexpr TCHAR Limit[] = TEXT("20.0");
inline constexpr float TickInterval = 0.0f; // Key edge must be sampled every frame.
inline const FVector2D PanelSize(620.0f, 155.0f);
inline constexpr TCHAR Title[] = TEXT("HC08: ORIGINAL CHARACTER / NO DRONE");
inline constexpr TCHAR Ready[] = TEXT("F8: block walking input for up to 20 seconds");
inline constexpr TCHAR Active[] = TEXT("BLOCKED: test walk, jump, look; F8 releases");
inline constexpr TCHAR Released[] = TEXT("RELEASED: check walking again; F8 repeats");
inline constexpr TCHAR Refused[] = TEXT("REFUSED: another movement lock is active");
inline constexpr TCHAR CharacterRequired[] = TEXT("Stand on deck ON FOOT, then press F8");
inline constexpr TCHAR Elapsed[] = TEXT("Blocked gameplay seconds (max 20): ");
inline constexpr TCHAR MoveYes[] = TEXT("Controller ignores movement: YES");
inline constexpr TCHAR MoveNo[] = TEXT("Controller ignores movement: NO");
inline constexpr TCHAR LookYes[] = TEXT("Controller ignores look: YES");
inline constexpr TCHAR LookNo[] = TEXT("Controller ignores look: NO");
}
namespace C = CharacterProbeNames;

void ReleaseCharacterGate(FGraph& G)
{
    auto* Held = G.Branch(G.Read(C::OwnsGate));
    G.Write(C::OwnsGate, nullptr, N::False); // Exactly one decrement for our increment.
    auto* ValidOwner = G.Branch(G.Valid(G.Read(C::OwnerController)));
    auto* Unlock = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, SetIgnoreMoveInput));
    G.Link(G.Read(C::OwnerController), G.Pin(Unlock, P::FunctionTarget));
    G.Default(Unlock, E::IgnoreMoveInputSetting, N::False); G.Exec(Unlock);
    // Merge all release outcomes, including already released/invalid controller.
    auto* Join = NewObject<UK2Node_IfThenElse>(G.Graph); G.Node(Join);
    G.Default(Join, P::Condition, N::True);
    G.Link(G.Tail, G.Pin(Join, P::Execute));
    G.Link(G.Pin(ValidOwner, P::Else), G.Pin(Join, P::Execute));
    G.Link(G.Pin(Held, P::Else), G.Pin(Join, P::Execute));
    G.Tail = G.Pin(Join, P::Then);
}

void BuildCharacterGraph(UBlueprint* BP, UClass* HudClass)
{
    UEdGraph* Graph = BP->UbergraphPages[0];
    const auto Defaults = Graph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);

    // A held gate is serviced BEFORE any current-pawn validity early return.
    auto* Held = G.Branch(G.Read(C::OwnsGate));
    auto* CurrentController = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    auto* Character = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCharacter));
    auto* SameCharacter = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(Character, P::ReturnValue), G.Read(N::OriginalPawn));
    auto* SameController = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(CurrentController, P::ReturnValue), G.Read(C::OwnerController));
    auto* StillOurs = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), G.Valid(G.Read(C::OwnerController)), G.Valid(G.Read(N::OriginalPawn))),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), SameCharacter, SameController)));
    auto* AddAge = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble));
    G.Link(G.Read(N::Age), G.Pin(AddAge, P::Binary::LeftOperand));
    G.Link(G.Pin(Tick, P::DeltaSeconds), G.Pin(AddAge, P::Binary::RightOperand));
    G.Write(N::Age, G.Pin(AddAge, P::ReturnValue));
    auto* Key = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Read(C::OwnerController), G.Pin(Key, P::FunctionTarget)); G.Default(Key, P::Key, C::ToggleKey);
    auto* Done = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Pin(Key, P::ReturnValue), G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Read(N::Age), C::Limit)));
    ReleaseCharacterGate(G); G.Text(N::FreezeStatus, C::Released);
    G.Tail = G.Pin(StillOurs, P::Else); ReleaseCharacterGate(G); G.Text(N::FreezeStatus, C::Released);
    G.Tail = G.Pin(Done, P::Else);
    G.Text(N::FreezeStatus, C::Active);
    G.Number(N::Clock, C::Elapsed, G.Read(N::Age));
    auto* Ignored = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsMoveInputIgnored));
    G.Link(G.Read(C::OwnerController), G.Pin(Ignored, P::FunctionTarget));
    G.BooleanText(N::Current, G.Pin(Ignored, P::ReturnValue), C::MoveYes, C::MoveNo);
    auto* Look = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsLookInputIgnored));
    G.Link(G.Read(C::OwnerController), G.Pin(Look, P::FunctionTarget));
    G.BooleanText(N::Control, G.Pin(Look, P::ReturnValue), C::LookYes, C::LookNo);

    G.Tail = G.Pin(Held, P::Else);
    G.Branch(G.Valid(G.Pin(CurrentController, P::ReturnValue)));
    auto* HudReady = G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* ReadyTail = G.Tail; G.Tail = G.Pin(HudReady, P::Else);
    auto* CreateHud = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Pin(CreateHud, E::WidgetType)->DefaultObject = HudClass;
    G.Link(G.Pin(CurrentController, P::ReturnValue), G.Pin(CreateHud, E::OwningPlayer)); G.Exec(CreateHud);
    auto* CastHud = NewObject<UK2Node_DynamicCast>(Graph);
    CastHud->TargetType = HudClass; CastHud->SetPurity(false); G.Node(CastHud);
    G.Link(G.Tail, G.Pin(CastHud, P::Execute)); G.Link(G.Pin(CreateHud, P::ReturnValue), CastHud->GetCastSourcePin());
    G.Tail = CastHud->GetValidCastPin(); G.Write(N::HudInstance, CastHud->GetCastResultPin());
    auto* AddHud = G.Call(UUserWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport));
    G.Link(G.Read(N::HudInstance), G.Pin(AddHud, P::FunctionTarget)); G.Default(AddHud, E::ZOrder, N::HudZOrder); G.Exec(AddHud);
    G.Tail = ReadyTail;
    auto* IdleMove = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsMoveInputIgnored));
    G.Link(G.Pin(CurrentController, P::ReturnValue), G.Pin(IdleMove, P::FunctionTarget));
    G.BooleanText(N::Current, G.Pin(IdleMove, P::ReturnValue), C::MoveYes, C::MoveNo);
    auto* IdleLook = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsLookInputIgnored));
    G.Link(G.Pin(CurrentController, P::ReturnValue), G.Pin(IdleLook, P::FunctionTarget));
    G.BooleanText(N::Control, G.Pin(IdleLook, P::ReturnValue), C::LookYes, C::LookNo);
    auto* StartKey = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Pin(CurrentController, P::ReturnValue), G.Pin(StartKey, P::FunctionTarget)); G.Default(StartKey, P::Key, C::ToggleKey);
    G.Branch(G.Pin(StartKey, P::ReturnValue));
    G.Require(G.Valid(G.Pin(Character, P::ReturnValue)), C::CharacterRequired);
    auto* AlreadyIgnored = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsMoveInputIgnored));
    G.Link(G.Pin(CurrentController, P::ReturnValue), G.Pin(AlreadyIgnored, P::FunctionTarget));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Pin(AlreadyIgnored, P::ReturnValue), N::False), C::Refused);
    G.Write(C::OwnerController, G.Pin(CurrentController, P::ReturnValue));
    G.Write(N::OriginalPawn, G.Pin(Character, P::ReturnValue));
    G.Write(N::Age, nullptr, N::Zero);
    auto* Lock = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, SetIgnoreMoveInput));
    G.Link(G.Read(C::OwnerController), G.Pin(Lock, P::FunctionTarget));
    G.Default(Lock, E::IgnoreMoveInputSetting, N::True); G.Exec(Lock);
    G.Write(C::OwnsGate, nullptr, N::True); G.Text(N::FreezeStatus, C::Active);

    auto* EndPlay = NewObject<UK2Node_Event>(Graph);
    EndPlay->EventReference.SetExternalMember(E::EndPlayEvent, AActor::StaticClass());
    EndPlay->bOverrideFunction = true; G.Node(EndPlay); G.Tail = G.Pin(EndPlay, P::Then);
    ReleaseCharacterGate(G);
    G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* RemoveHud = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, RemoveFromParent));
    G.Link(G.Read(N::HudInstance), G.Pin(RemoveHud, P::FunctionTarget)); G.Exec(RemoveHud);
}
