#pragma once
// HC32: explicit Blueprint interface membership; HC31 acquisition/native entry unchanged.
namespace CE = ContextEntryNames;
namespace DS = DedicatedStationNames;

void ContextSet(FGraph& G, UEdGraphPin* Target, UClass* Class, FName Field, UEdGraphPin* Value, const TCHAR* Literal = nullptr)
{
    auto* Set = NewObject<UK2Node_VariableSet>(G.Graph); Set->VariableReference.SetExternalMember(Field, Class); G.Node(Set);
    G.Link(Target, G.Pin(Set, P::FunctionTarget));
    if (Value) G.Link(Value, G.Pin(Set, Field)); else G.Default(Set, Field, Literal);
    G.Exec(Set);
}

UEdGraphPin* ContextParentValid(FGraph& G)
{
    [[maybe_unused]] constexpr auto Signature = static_cast<AActor* (UActorComponent::*)() const>(&UActorComponent::GetOwner);
    auto* Owner = ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor));
    auto* Parent = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetAttachParentActor), OpticalSelf(G));
    return G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Owner, Parent);
}

void AddContextEntry(UBlueprint* BP)
{
    // Own callback copies the exact current native delegate signature.
    auto* DelegateProperty = FindFProperty<FDelegateProperty>(FPlayerInputInterfaceAction::StaticStruct(), CE::OnTriggered); check(DelegateProperty);
    auto* CallbackGraph = FBlueprintEditorUtils::CreateNewGraph(BP, CE::EnterFunction, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, CallbackGraph, true, DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* Entry = nullptr; UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : CallbackGraph->Nodes)
    { if (auto* It = Cast<UK2Node_FunctionEntry>(Node)) Entry = It; if (auto* It = Cast<UK2Node_FunctionResult>(Node)) Result = It; }
    check(Entry && Result); FGraph C(CallbackGraph, nullptr);
    C.Pin(Entry, P::Then)->BreakAllPinLinks(); C.Pin(Result, P::Execute)->BreakAllPinLinks(); C.Tail = C.Pin(Entry, P::Then);
    C.Write(CE::CallbackSeen, nullptr, N::True);
    C.Branch(C.Read(CE::Ready)); C.Branch(C.Valid(C.Read(S::Anchor))); C.Branch(ContextParentValid(C));
    C.Branch(C.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        ObserveCall(C, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(C)), N::False));
    auto* Instance = NewObject<UK2Node_BreakStruct>(CallbackGraph); Instance->StructType = FVoyageInputActionInstance::StaticStruct(); C.Node(Instance);
    UEdGraphPin* InstanceInput = nullptr;
    for (auto* Pin : Instance->Pins) if (Pin->Direction == EGPD_Input) { InstanceInput = Pin; break; }
    check(InstanceInput); C.Link(C.Pin(Entry, CE::ActionInstance), InstanceInput);
    auto* PC = NewObject<UK2Node_DynamicCast>(CallbackGraph); PC->TargetType = APlayerController::StaticClass(); PC->SetPurity(false); C.Node(PC);
    C.Link(C.Tail, C.Pin(PC, P::Execute)); C.Link(C.Pin(Instance, CE::Controller), PC->GetCastSourcePin()); C.Tail = PC->GetValidCastPin();
    C.Write(DS::Controller, PC->GetCastResultPin());
    C.Branch(ObserveCall(C, APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, IsLocalController), C.Read(DS::Controller)));
    C.Branch(ObserveCall(C, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, HasAuthority), OpticalSelf(C)));
    auto* Character = NewObject<UK2Node_DynamicCast>(CallbackGraph); Character->TargetType = ACharacter::StaticClass(); Character->SetPurity(false); C.Node(Character);
    C.Link(C.Tail, C.Pin(Character, P::Execute));
    C.Link(ObserveCall(C, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), C.Read(DS::Controller)), Character->GetCastSourcePin()); C.Tail = Character->GetValidCastPin();
    C.Write(N::OriginalPawn, Character->GetCastResultPin());
    auto* Movement = NewObject<UK2Node_DynamicCast>(CallbackGraph); Movement->TargetType = UCharacterMovementComponent::StaticClass(); Movement->SetPurity(false); C.Node(Movement);
    C.Link(C.Tail, C.Pin(Movement, P::Execute));
    C.Link(ObserveCall(C, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, GetMovementComponent), C.Read(N::OriginalPawn)), Movement->GetCastSourcePin()); C.Tail = Movement->GetValidCastPin();
    C.Write(S::Movement, Movement->GetCastResultPin());
    C.Branch(C.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(C), S::WalkingByte));
    auto* Difference = C.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector),
        ObserveCall(C, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), C.Read(N::OriginalPawn)),
        ObserveCall(C, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), C.Read(S::Anchor)));
    auto* Distance = C.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    C.Link(Difference, C.Pin(Distance, E::VectorLengthInput));
    C.Branch(C.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble), C.Pin(Distance, P::ReturnValue), S::Range));
    CalculateNativeOpticalFov(C);
    auto* Enter = C.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnEnterVehicle));
    C.Link(OpticalSelf(C), C.Pin(Enter, P::FunctionTarget)); C.Link(C.Read(DS::Controller), C.Pin(Enter, V::NewPossessor)); C.Exec(Enter);
    C.Link(C.Tail, C.Pin(Result, P::Execute));

    // Common native VehiclePawn does not implement this interface. Register it
    // on our Blueprint, then use the generated interface graph and exact owner.
    UClass* Interface = UInteractiveInterface::StaticClass();
    const FName Function = GET_FUNCTION_NAME_CHECKED(IInteractiveInterface, GetInteractiveProvidedActions);
    check(!BP->ParentClass->ImplementsInterface(Interface));
    check(Interface->FindFunctionByName(Function)->GetOuterUClass() == Interface);
    check(FBlueprintEditorUtils::ImplementNewInterface(BP, Interface->GetClassPathName()));
    UEdGraph* Graph = nullptr;
    for (const auto& Description : BP->ImplementedInterfaces)
        if (Description.Interface == Interface)
            for (UEdGraph* Candidate : Description.Graphs)
                if (Candidate->GetFName() == Function) Graph = Candidate;
    check(Graph);
    Entry = nullptr; Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    { if (auto* It = Cast<UK2Node_FunctionEntry>(Node)) Entry = It; if (auto* It = Cast<UK2Node_FunctionResult>(Node)) Result = It; }
    check(Entry && Result); FGraph G(Graph, nullptr);
    G.Pin(Entry, P::Then)->BreakAllPinLinks(); G.Pin(Result, P::Execute)->BreakAllPinLinks(); G.Tail = G.Pin(Entry, P::Then);
    // Modern-provider ownership is independent of action availability. Returning
    // false asks Voyage to invoke the legacy interface, which this K2-only
    // station does not implement. Every rejected entry query must return an
    // explicit handled/empty result, including the shell-destruction window.
    auto* EmptyResult = NewObject<UK2Node_FunctionResult>(Graph);
    EmptyResult->FunctionReference = Result->FunctionReference;
    G.Node(EmptyResult); G.Default(EmptyResult, P::ReturnValue, N::True);
    check(G.Pin(EmptyResult, CE::OutActions)->LinkedTo.IsEmpty());
    auto Available = [&](UEdGraphPin* Condition)
    {
        auto* Guard = G.Branch(Condition);
        G.Link(G.Pin(Guard, P::Else), G.Pin(EmptyResult, P::Execute));
    };
    G.Write(CE::ProviderSeen, nullptr, N::True);
    Available(G.Read(CE::Ready)); Available(G.Valid(G.Read(S::Anchor))); Available(ContextParentValid(G));
    Available(G.Valid(G.Read(CE::EntryAction))); Available(G.Valid(G.Pin(Entry, CE::MyCharacter)));
    Available(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), G.Pin(Entry, CE::Component), G.Read(CE::Interaction)));
    Available(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(G)), N::False));
    auto* Action = NewObject<UK2Node_MakeStruct>(Graph); Action->StructType = FPlayerInputInterfaceAction::StaticStruct(); Action->bMadeAfterOverridePinRemoval = true; G.Node(Action);
    G.Link(G.Read(CE::EntryAction), G.Pin(Action, Hint::InputAction));
    G.Default(Action, Hint::Name, CE::ActionName); G.Default(Action, Hint::Category, Hint::ActionCategory);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultText(*G.Pin(Action, Hint::Text), FText::FromString(CE::Label));
    G.Default(Action, Hint::Enabled, N::True); G.Default(Action, Hint::Type, Hint::Central);
    auto* Delegate = G.Node(NewObject<UK2Node_CreateDelegate>(Graph));
    G.Link(Delegate->GetDelegateOutPin(), G.Pin(Action, CE::OnTriggered)); Delegate->SetFunction(CE::EnterFunction);
    auto* Array = G.Node(NewObject<UK2Node_MakeArray>(Graph));
    UEdGraphPin* ActionOut = nullptr; for (auto* Pin : Action->Pins) if (Pin->Direction == EGPD_Output) ActionOut = Pin;
    check(ActionOut); G.Link(ActionOut, G.Pin(Array, Array->GetPinName(0)));
    G.Link(Array->GetOutputPin(), G.Pin(Result, CE::OutActions)); G.Default(Result, P::ReturnValue, N::True); G.Link(G.Tail, G.Pin(Result, P::Execute));
}

// Independent station safety tick. Never destroy a possessed pawn.
void ContextStationSafety(FGraph& G)
{
    auto* Self = OpticalSelf(G);
    auto Exit = [&]() { auto* Call = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle)); G.Link(Self, G.Pin(Call, P::FunctionTarget)); G.Exec(Call); };
    auto* AnchorValid = G.Branch(G.Valid(G.Read(S::Anchor))); auto* Normal = G.Tail;
    G.Tail = G.Pin(AnchorValid, P::Else); G.Write(CE::Ready, nullptr, N::False);
    // Stop new acquisition before camera/station destruction. A detector can
    // still hold an earlier result: handled/empty provider above covers it.
    auto* DisableQuery = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    G.Link(Self, G.Pin(DisableQuery, P::FunctionTarget)); G.Default(DisableQuery, SP::CollisionEnabled, N::False); G.Exec(DisableQuery);
    auto* Occupied = G.Branch(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self));
    Exit(); auto* Requested = G.Tail; G.Tail = G.Pin(Occupied, P::Else);
    auto* Camera = G.Branch(G.Valid(G.Read(DS::Camera))); auto* NoCamera = G.Pin(Camera, P::Else);
    auto* DestroyCamera = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    G.Link(G.Read(DS::Camera), G.Pin(DestroyCamera, P::FunctionTarget)); G.Exec(DestroyCamera); StationMerge(G, {G.Tail, NoCamera});
    auto* Destroy = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor)); G.Link(Self, G.Pin(Destroy, P::FunctionTarget)); G.Exec(Destroy);
    (void)Requested; // Exit branch deliberately waits until an independent next tick.
    G.Tail = Normal;
    G.Branch(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self));
    auto* Key = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Read(DS::Controller), G.Pin(Key, P::FunctionTarget)); G.Default(Key, P::Key, S::Key);
    G.Branch(G.Pin(Key, P::ReturnValue)); Exit();
}
