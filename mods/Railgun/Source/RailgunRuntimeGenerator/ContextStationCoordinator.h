#pragma once
// Engine-only autoload manager. Exact shell class; no shell event or mutation.
UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values)
{
    auto* Macros = LoadObject<UBlueprint>(nullptr, CE::LoopPackage); check(Macros);
    UEdGraph* LoopGraph = nullptr; for (UEdGraph* Graph : Macros->MacroGraphs) if (Graph->GetFName() == CE::LoopGraph) LoopGraph = Graph;
    check(LoopGraph); auto* Loop = NewObject<UK2Node_MacroInstance>(G.Graph); Loop->SetMacroGraph(LoopGraph); G.Node(Loop);
    UEdGraphPin* Execute = nullptr;
    for (auto* Pin : Loop->Pins)
        if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) { check(!Execute); Execute = Pin; }
    check(Execute);
    G.Link(G.Tail, Execute); G.Link(Values, G.Pin(Loop, CE::Array)); G.Tail = G.Pin(Loop, CE::LoopBody); return Loop;
}

void ContextPrepareStation(FGraph& G, UClass* StationClass, UEdGraphPin* Shell)
{
    // Resolve before spawning: old/missing shell contract must not create an
    // active fallback query around the gun body or orphan replacement stations.
    G.Write(CE::ModelEntry, nullptr, N::EmptyText);
    auto* FindEntry = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(Shell, G.Pin(FindEntry, P::FunctionTarget));
    G.Pin(FindEntry, OP::ComponentClass)->DefaultObject = UBoxComponent::StaticClass();
    G.Default(FindEntry, ActorScanGraphNames::ComponentTag, *RailgunModelContract::EntryTag.ToString());
    auto* EntryLoop = ContextLoop(G, G.Pin(FindEntry, P::ReturnValue));
    auto* EntryCast = NewObject<UK2Node_DynamicCast>(G.Graph); EntryCast->TargetType = UBoxComponent::StaticClass(); EntryCast->SetPurity(false); G.Node(EntryCast);
    G.Link(G.Tail, G.Pin(EntryCast, P::Execute)); G.Link(G.Pin(EntryLoop, CE::ArrayElement), EntryCast->GetCastSourcePin()); G.Tail = EntryCast->GetValidCastPin();
    G.Write(CE::ModelEntry, EntryCast->GetCastResultPin()); G.Tail = G.Pin(EntryLoop, CE::Completed);
    G.Branch(G.Valid(G.Read(CE::ModelEntry)));
    auto* Root = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), Shell);
    G.Branch(G.Valid(Root)); G.Write(S::Anchor, Root);
    // Fabricator visuals are not this class; exact native-built mount guards
    // also exclude transient/unattached editor previews of the same class.
    auto* Name = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName)); G.Link(Root, G.Pin(Name, P::Object));
    auto* Match = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(Name, P::ReturnValue), G.Pin(Match, P::Binary::LeftOperand)); G.Default(Match, P::Binary::RightOperand, S::RootName); G.Branch(G.Pin(Match, P::ReturnValue));
    G.Branch(G.Valid(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), Root)));
    auto* Rotation = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentRotation), Root);
    auto* AnchorTransform = G.Transform(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), Root), Rotation);
    auto* Transform = G.Transform(G.Offset(AnchorTransform, CombinedVehicleNames::Offset), Rotation);
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
    G.Pin(Spawn, E::ActorClass)->DefaultObject = StationClass; G.Link(Transform, G.Pin(Spawn, P::SpawnTransform));
    G.Link(Shell, G.Pin(Spawn, CE::Owner)); G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn); G.Exec(Spawn);
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = StationClass; Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Spawn, P::ReturnValue), Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    auto* Station = Cast->GetCastResultPin(); G.Write(V::Vehicle, Station);
    ContextSet(G, Station, StationClass, S::Anchor, G.Read(S::Anchor));
    ContextSet(G, Station, StationClass, CE::EntryAction, G.Read(CE::EntryAction));
    auto* Body = NewObject<UK2Node_DynamicCast>(G.Graph); Body->TargetType = UPrimitiveComponent::StaticClass(); Body->SetPurity(false); G.Node(Body);
    G.Link(G.Tail, G.Pin(Body, P::Execute));
    G.Link(ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), Station), Body->GetCastSourcePin()); G.Tail = Body->GetValidCastPin(); G.Write(V::Body, Body->GetCastResultPin());
    auto* Physics = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetSimulatePhysics));
    G.Link(G.Read(V::Body), G.Pin(Physics, P::FunctionTarget)); G.Default(Physics, E::SimulatePhysics, N::False); G.Exec(Physics);
    auto* Collision = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetCollisionEnabled));
    const FName CollisionType(TEXT("NewType")); const TCHAR* NoCollision = TEXT("ECollisionEnabled::NoCollision");
    G.Link(G.Read(V::Body), G.Pin(Collision, P::FunctionTarget)); G.Default(Collision, CollisionType, NoCollision); G.Exec(Collision);
    auto* Controls = ReadNativeInputField(G, Station, AVoyageVehiclePawn::StaticClass(), NativeInputNames::ControlsField);
    G.Require(G.Valid(Controls), ExitActionNames::Failed);
    NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, true);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(Station, G.Pin(Finish, P::Actor)); G.Link(Transform, G.Pin(Finish, P::SpawnTransform)); G.Exec(Finish); G.Require(G.Valid(G.Pin(Finish, P::ReturnValue)), V::Failed);
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(Station, G.Pin(Attach, P::FunctionTarget)); G.Link(G.Read(S::Anchor), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld); G.Default(Attach, E::ScaleRule, N::KeepWorld);
    G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach); G.Require(G.Pin(Attach, P::ReturnValue), V::Failed);
    ConfigureCombinedOperatorPoint(G);
    NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, false);
    auto* Interaction = ReadNativeInputField(G, Station, StationClass, CE::Interaction);
    auto* Query = ReadNativeInputField(G, Station, StationClass, CE::QueryBox);
    G.Require(G.Valid(Interaction), V::Failed); G.Require(G.Valid(Query), V::Failed);
    auto* PlaceEntry = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetWorldTransform));
    G.Link(Interaction, G.Pin(PlaceEntry, P::FunctionTarget));
    G.Link(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentToWorld), G.Read(CE::ModelEntry)), G.Pin(PlaceEntry, ActorLifecycleGraphNames::NewTransform));
    G.Default(PlaceEntry, OP::Sweep, N::False); G.Exec(PlaceEntry);
    auto* ResizeEntry = G.Call(UBoxComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBoxComponent, SetBoxExtent));
    G.Link(Query, G.Pin(ResizeEntry, P::FunctionTarget));
    G.Link(ObserveCall(G, UBoxComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBoxComponent, GetUnscaledBoxExtent), G.Read(CE::ModelEntry)), G.Pin(ResizeEntry, CE::BoxExtentPin)); G.Exec(ResizeEntry);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), Query), Interaction), V::Failed);
    // Only own query box is enabled: native root remains NoCollision/nonphysical.
    auto* Enable = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    G.Link(Station, G.Pin(Enable, P::FunctionTarget)); G.Default(Enable, SP::CollisionEnabled, N::True); G.Exec(Enable);
    // Observe the effective response after FinishSpawning; do not repair it here.
    auto* Response = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetCollisionResponseToChannel));
    G.Link(Query, G.Pin(Response, P::FunctionTarget)); G.Default(Response, CE::CollisionChannelPin, CE::InteractChannelValue);
    ContextSet(G, Station, StationClass, CE::InteractBlocks,
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), G.Pin(Response, P::ReturnValue), CE::BlockResponseValue));
    ContextSet(G, Station, StationClass, CE::Ready, nullptr, N::True);
}

void BuildContextCoordinator(UBlueprint* BP, UClass* HudClass, UClass* StationClass)
{
    AddVariable(BP, CE::ModelEntry, UEdGraphSchema_K2::PC_Object, UBoxComponent::StaticClass());
    AddVariable(BP, CE::ShellClass, UEdGraphSchema_K2::PC_Class, AActor::StaticClass());
    AddVariable(BP, CE::EntryAction, UEdGraphSchema_K2::PC_Object, UInputAction::StaticClass());
    AddVariable(BP, CE::FoundPair, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, CE::ProviderSeen, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, CE::CallbackSeen, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, CE::Spawned, UEdGraphSchema_K2::PC_Real);
    FKismetEditorUtilities::CompileBlueprint(BP);
    UEdGraph* Graph = BP->UbergraphPages[0]; const auto Defaults = Graph->Nodes; for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph); Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    G.Branch(ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, HasAuthority), OpticalSelf(G)));
    auto* Player = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    G.Branch(G.Valid(G.Pin(Player, P::ReturnValue))); G.Write(S::Controller, G.Pin(Player, P::ReturnValue));
    auto* HasHud = G.Branch(G.Valid(G.Read(N::HudInstance))); auto* ReadyHud = G.Tail;
    G.Tail = G.Pin(HasHud, P::Else);
    auto* Create = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Pin(Create, E::WidgetType)->DefaultObject = HudClass; G.Link(G.Read(S::Controller), G.Pin(Create, E::OwningPlayer)); G.Exec(Create);
    auto* HudCast = NewObject<UK2Node_DynamicCast>(Graph); HudCast->TargetType = HudClass; HudCast->SetPurity(false); G.Node(HudCast);
    G.Link(G.Tail, G.Pin(HudCast, P::Execute)); G.Link(G.Pin(Create, P::ReturnValue), HudCast->GetCastSourcePin()); G.Tail = HudCast->GetValidCastPin();
    G.Write(N::HudInstance, HudCast->GetCastResultPin());
    // Keep legacy diagnostic sink private: never attach its panel/instructions
    // to the viewport. Station selection and action hints have their own HUD.
    G.Tail = ReadyHud;
    auto* ClassValid = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValidClass));
    G.Link(G.Read(CE::ShellClass), G.Pin(ClassValid, P::Class));
    auto* Loaded = G.Branch(G.Pin(ClassValid, P::ReturnValue)); auto* LoadedTail = G.Tail; G.Tail = G.Pin(Loaded, P::Else);
    G.Write(CE::EntryAction, LoadStockInputReference(G, CE::InteractActionPath, UInputAction::StaticClass()));
    G.Write(NativeInputNames::Context, LoadStockInputReference(G, DS::ContextPath, UVoyageInputContextAsset::StaticClass()));
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftClassPath)); G.Default(Path, CE::SoftClassPath, S::ClassPath);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassPathToSoftClassRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, CE::SoftClassReferencePath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadClassAsset_Blocking));
    const FName AssetClass(TEXT("AssetClass")); G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Load, AssetClass)); G.Exec(Load);
    auto* ClassCast = NewObject<UK2Node_ClassDynamicCast>(Graph); ClassCast->TargetType = AActor::StaticClass(); ClassCast->SetPurity(false); G.Node(ClassCast);
    G.Link(G.Tail, G.Pin(ClassCast, P::Execute)); G.Link(G.Pin(Load, P::ReturnValue), ClassCast->GetCastSourcePin()); G.Tail = ClassCast->GetValidCastPin();
    G.Write(CE::ShellClass, ClassCast->GetCastResultPin()); StationMerge(G, {G.Tail, LoadedTail});
    G.Text(N::FreezeStatus, CE::PreparedStatus); G.Write(CE::Spawned, nullptr, N::Zero);
    auto* Shells = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetAllActorsOfClass));
    G.Link(G.Read(CE::ShellClass), G.Pin(Shells, E::ActorClass)); G.Exec(Shells);
    auto* Outer = ContextLoop(G, G.Pin(Shells, CE::OutActors)); auto* Shell = G.Pin(Outer, CE::ArrayElement);
    G.Branch(G.Valid(Shell)); G.Write(CE::FoundPair, nullptr, N::False);
    // Owner, not proximity or attached-list alone: a failed/deferred station is
    // retained as a blocked pair rather than spawning an unbounded retry train.
    auto* Stations = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetAllActorsOfClass));
    G.Pin(Stations, E::ActorClass)->DefaultObject = StationClass; G.Exec(Stations);
    auto* Inner = ContextLoop(G, G.Pin(Stations, CE::OutActors));
    auto* Candidate = G.Pin(Inner, CE::ArrayElement); G.Branch(G.Valid(Candidate));
    [[maybe_unused]] constexpr auto OwnerSignature = static_cast<AActor* (AActor::*)() const>(&AActor::GetOwner);
    auto* Owner = ObserveCall(G, AActor::StaticClass(), ActorScanGraphNames::GetActorOwner, Candidate);
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Owner, Shell)); G.Write(CE::FoundPair, nullptr, N::True);
    auto* Cast = NewObject<UK2Node_DynamicCast>(Graph); Cast->TargetType = StationClass; Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Candidate, Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    auto* Station = Cast->GetCastResultPin();
    // Sticky ANY aggregation; never label the last enumerated station as target.
    G.Write(CE::ProviderSeen, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Read(CE::ProviderSeen), ReadNativeInputField(G, Station, StationClass, CE::ProviderSeen)));
    G.Write(CE::CallbackSeen, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Read(CE::CallbackSeen), ReadNativeInputField(G, Station, StationClass, CE::CallbackSeen)));
    G.Branch(ReadNativeInputField(G, Station, StationClass, CE::Ready));
    auto* Increment = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble));
    G.Link(G.Read(CE::Spawned), G.Pin(Increment, P::Binary::LeftOperand)); G.Default(Increment, P::Binary::RightOperand, CE::One); G.Write(CE::Spawned, G.Pin(Increment, P::ReturnValue));
    G.Tail = G.Pin(Inner, CE::Completed);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Read(CE::FoundPair), N::False)); ContextPrepareStation(G, StationClass, Shell);
    G.Tail = G.Pin(Outer, CE::Completed); G.Number(N::Clock, CE::CountLabel, G.Read(CE::Spawned));
    G.BooleanText(N::Current, G.Read(CE::ProviderSeen), CE::ProviderYes, CE::ProviderNo);
    G.BooleanText(N::Control, G.Read(CE::CallbackSeen), CE::CallbackYes, CE::CallbackNo);
}
