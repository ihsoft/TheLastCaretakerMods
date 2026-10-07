#pragma once
// Builds station entry callbacks and the interaction interface graph.
namespace CE = ContextEntryNames;
namespace DS = DedicatedStationNames;
namespace StationLifecycle
{
inline const FName ShellOwner(TEXT("RailgunShellOwner"));
inline const FName EntryPending(TEXT("RailgunEntryPending"));
inline const FName ExitPending(TEXT("RailgunExitPending"));
inline const FName TeardownPending(TEXT("RailgunTeardownPending"));
inline const FName ActiveHud(TEXT("RailgunActiveHud"));
inline const FName BindShell(TEXT("BindRailgunShellLifecycle"));
inline const FName ShellEndPlayCallback(TEXT("OnRailgunShellEndPlay"));
inline const FName FinalizeTeardown(TEXT("FinalizeRailgunStationTeardown"));
inline constexpr TCHAR DestroyedReason[] = TEXT("0");
inline constexpr TCHAR RemovedFromWorldReason[] = TEXT("3");
}

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

FMulticastDelegateProperty* StationShellEndPlayDelegateProperty()
{
    auto* Property = FindFProperty<FMulticastDelegateProperty>(
        AActor::StaticClass(), ActorLifecycleGraphNames::OnEndPlay);
    check(Property && Property->SignatureFunction &&
        Property->SignatureFunction->NumParms == 2);
    auto* Actor = FindFProperty<FObjectProperty>(Property->SignatureFunction,
        ActorLifecycleGraphNames::EndPlayActor);
    auto* Reason = FindFProperty<FByteProperty>(Property->SignatureFunction,
        ActorLifecycleGraphNames::EndPlayReason);
    check(Actor && Actor->HasAnyPropertyFlags(CPF_Parm) &&
        Actor->PropertyClass == AActor::StaticClass());
    check(Reason && Reason->HasAnyPropertyFlags(CPF_Parm) && Reason->Enum &&
        Reason->Enum->GetPathName() == TEXT("/Script/Engine.EEndPlayReason"));
    return Property;
}

void RestoreDedicatedViewIfOwned(FGraph& G)
{
    auto* Owned = G.Branch(G.Read(DS::ViewOwned));
    G.Write(DS::ViewOwned, nullptr, N::False);
    auto* ControllerValid = G.Branch(G.Valid(G.Read(DS::Controller)));
    auto* CurrentView = ObserveCall(G, AController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AController, GetViewTarget),
        G.Read(DS::Controller));
    auto* Ours = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject), CurrentView, G.Read(DS::Camera)));
    auto* CurrentPawn = ObserveCall(G, AController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn),
        G.Read(DS::Controller));
    auto* PawnValid = G.Branch(G.Valid(CurrentPawn));
    auto* Restore = G.Call(APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, SetViewTargetWithBlend));
    G.Link(G.Read(DS::Controller), G.Pin(Restore, P::FunctionTarget));
    G.Link(CurrentPawn, G.Pin(Restore, OP::NewViewTarget));
    G.Default(Restore, OP::BlendTime, N::Zero);
    G.Exec(Restore);
    StationMerge(G, {G.Tail, G.Pin(PawnValid, P::Else),
        G.Pin(Ours, P::Else), G.Pin(ControllerValid, P::Else),
        G.Pin(Owned, P::Else)});
}

void AddStationLifecycleFunctions(UBlueprint* BP)
{
    using namespace StationLifecycle;
    check(BP && BP->GeneratedClass);
    FMulticastDelegateProperty* DelegateProperty =
        StationShellEndPlayDelegateProperty();

    UEdGraph* FinalizeGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        FinalizeTeardown, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, FinalizeGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* FinalizeEntry = nullptr;
    for (UEdGraphNode* Node : FinalizeGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            FinalizeEntry = Candidate;
    check(FinalizeEntry);
    FinalizeEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Finalize(FinalizeGraph, nullptr);
    Finalize.Tail = Finalize.Pin(FinalizeEntry, P::Then);
    Finalize.Branch(Finalize.Read(TeardownPending));
    Finalize.Branch(Finalize.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        ObserveCall(Finalize, APawn::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled),
            OpticalSelf(Finalize)), N::False));
    UEdGraphPin* CurrentController = ObserveCall(Finalize,
        APawn::StaticClass(), BlueprintGraphNames::ActorFunctions::GetController,
        OpticalSelf(Finalize));
    Finalize.Branch(Finalize.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        Finalize.Valid(CurrentController), N::False));
    Finalize.Write(TeardownPending, nullptr, N::False);
    Finalize.Write(ExitPending, nullptr, N::False);
    Finalize.Write(ActiveHud, nullptr);
    RestoreDedicatedViewIfOwned(Finalize);
    auto* CameraValid = Finalize.Branch(
        Finalize.Valid(Finalize.Read(DS::Camera)));
    auto* DestroyCamera = Finalize.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    Finalize.Link(Finalize.Read(DS::Camera),
        Finalize.Pin(DestroyCamera, P::FunctionTarget));
    Finalize.Exec(DestroyCamera);
    UEdGraphPin* CameraDestroyed = Finalize.Tail;
    Finalize.Tail = Finalize.Pin(CameraValid, P::Else);
    StationMerge(Finalize, {CameraDestroyed, Finalize.Tail});
    Finalize.Write(DS::Camera, nullptr);
    auto* DestroyStation = Finalize.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    Finalize.Link(OpticalSelf(Finalize),
        Finalize.Pin(DestroyStation, P::FunctionTarget));
    Finalize.Exec(DestroyStation);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(FinalizeTeardown));

    UEdGraph* CallbackGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        ShellEndPlayCallback, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, CallbackGraph, true,
        DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* CallbackEntry = nullptr;
    for (UEdGraphNode* Node : CallbackGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            CallbackEntry = Candidate;
    check(CallbackEntry);
    CallbackEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Callback(CallbackGraph, nullptr);
    Callback.Tail = Callback.Pin(CallbackEntry, P::Then);
    UEdGraphPin* EndingActor = Callback.Pin(CallbackEntry,
        ActorLifecycleGraphNames::EndPlayActor);
    Callback.Branch(Callback.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject), EndingActor, Callback.Read(ShellOwner)));
    Callback.Write(CE::Ready, nullptr, N::False);
    Callback.Write(EntryPending, nullptr, N::False);
    auto* DisableQuery = Callback.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    Callback.Link(OpticalSelf(Callback),
        Callback.Pin(DisableQuery, P::FunctionTarget));
    Callback.Default(DisableQuery, SP::CollisionEnabled, N::False);
    Callback.Exec(DisableQuery);
    UEdGraphPin* Reason = Callback.Pin(CallbackEntry,
        ActorLifecycleGraphNames::EndPlayReason);
    UEdGraphPin* Destroyed = Callback.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte),
        Reason, DestroyedReason);
    UEdGraphPin* Removed = Callback.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte),
        Reason, RemovedFromWorldReason);
    Callback.Branch(Callback.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        Destroyed, Removed));
    Callback.Write(TeardownPending, nullptr, N::True);
    auto* StopRuntimeActivity = Callback.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    Callback.Exec(StopRuntimeActivity);
    auto* Occupied = Callback.Branch(ObserveCall(Callback,
        APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn,
            IsPlayerControlled), OpticalSelf(Callback)));
    auto* Exit = Callback.Call(AVoyageVehiclePawn::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle));
    Callback.Link(OpticalSelf(Callback),
        Callback.Pin(Exit, P::FunctionTarget));
    Callback.Exec(Exit);
    UEdGraphPin* ExitRequested = Callback.Tail;
    Callback.Tail = Callback.Pin(Occupied, P::Else);
    auto* FinalizeNow = Callback.Call(BP->GeneratedClass,
        FinalizeTeardown);
    Callback.Exec(FinalizeNow);
    StationMerge(Callback, {ExitRequested, Callback.Tail});

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(ShellEndPlayCallback));

    UEdGraph* BindGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        BindShell, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, BindGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* BindEntry = nullptr;
    for (UEdGraphNode* Node : BindGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            BindEntry = Candidate;
    check(BindEntry);
    BindEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Bind(BindGraph, nullptr);
    Bind.Tail = Bind.Pin(BindEntry, P::Then);
    auto* CallbackDelegate = Bind.Node(
        NewObject<UK2Node_CreateDelegate>(BindGraph));
    CallbackDelegate->SetFunction(ShellEndPlayCallback);
    auto NewRemove = [&]()
    {
        auto* Remove = NewObject<UK2Node_RemoveDelegate>(BindGraph);
        Remove->SetFromProperty(DelegateProperty, false,
            AActor::StaticClass());
        Bind.Node(Remove);
        Bind.Link(CallbackDelegate->GetDelegateOutPin(),
            Remove->GetDelegatePin());
        return Remove;
    };
    auto* RemovePrevious = NewRemove();
    auto* RemoveCurrent = NewRemove();
    auto* Add = NewObject<UK2Node_AddDelegate>(BindGraph);
    Add->SetFromProperty(DelegateProperty, false, AActor::StaticClass());
    Bind.Node(Add);
    Bind.Link(CallbackDelegate->GetDelegateOutPin(), Add->GetDelegatePin());
    auto* PreviousValid = Bind.Branch(Bind.Valid(Bind.Read(ShellOwner)));
    Bind.Link(Bind.Read(ShellOwner),
        Bind.Pin(RemovePrevious, P::FunctionTarget));
    Bind.Exec(RemovePrevious);
    UEdGraphPin* PreviousRemoved = Bind.Tail;
    Bind.Tail = Bind.Pin(PreviousValid, P::Else);
    StationMerge(Bind, {PreviousRemoved, Bind.Tail});
    Bind.Write(ShellOwner, nullptr);
    Bind.Branch(Bind.Valid(Bind.Read(S::Anchor)));
    UEdGraphPin* Owner = ObserveCall(Bind, UActorComponent::StaticClass(),
        OP::ComponentOwner, Bind.Read(S::Anchor));
    Bind.Branch(Bind.Valid(Owner));
    Bind.Write(ShellOwner, Owner);
    Bind.Link(Owner, Bind.Pin(RemoveCurrent, P::FunctionTarget));
    Bind.Exec(RemoveCurrent);
    Bind.Link(Owner, Bind.Pin(Add, P::FunctionTarget));
    Bind.Exec(Add);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(BindShell));
}

void AddStationLifecycleTeardown(UBlueprint* BP)
{
    using namespace StationLifecycle;
    check(BP && BP->UbergraphPages.Num() == 1);
    UEdGraph* Graph = BP->UbergraphPages[0];
    UK2Node_Event* EndPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        auto* Event = Cast<UK2Node_Event>(Node);
        if (!Event || Event->EventReference.GetMemberName() !=
            ActorLifecycleGraphNames::EndPlayEvent) continue;
        check(!EndPlay);
        EndPlay = Event;
    }
    check(EndPlay);
    UEdGraphPin* EndPlayTail = EndPlay->FindPinChecked(P::Then);
    check(EndPlayTail->LinkedTo.Num() == 1);
    UEdGraphPin* ExistingWork = EndPlayTail->LinkedTo[0];
    EndPlayTail->BreakAllPinLinks();
    FGraph G(Graph, nullptr);
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(EndPlayTail, G.Pin(Work, P::Execute));
    G.Link(Work->GetThenPinGivenIndex(0), ExistingWork);
    G.Tail = Work->GetThenPinGivenIndex(1);
    auto* DelegateProperty = StationShellEndPlayDelegateProperty();
    auto* Callback = G.Node(NewObject<UK2Node_CreateDelegate>(Graph));
    Callback->SetFunction(ShellEndPlayCallback);
    auto* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false, AActor::StaticClass());
    G.Node(Remove);
    G.Link(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    auto* OwnerValid = G.Branch(G.Valid(G.Read(ShellOwner)));
    G.Link(G.Read(ShellOwner), G.Pin(Remove, P::FunctionTarget));
    G.Exec(Remove);
    UEdGraphPin* Removed = G.Tail;
    G.Tail = G.Pin(OwnerValid, P::Else);
    StationMerge(G, {Removed, G.Tail});
    G.Write(ShellOwner, nullptr);
    G.Write(EntryPending, nullptr, N::False);
    G.Write(ExitPending, nullptr, N::False);
    G.Write(TeardownPending, nullptr, N::False);
    G.Write(ActiveHud, nullptr);
}
