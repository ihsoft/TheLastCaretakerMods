#pragma once
// HC26: authored child of COMMON transport. TAGGED cook required, no Forklift.
// Included after graph helpers. Native entry/exit remains unmodified.

bool SaveDedicatedAsset(UObject* Asset)
{
    auto* Package = Asset->GetOutermost();
    FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *File, Args);
}

void DedicatedViewTarget(FGraph& G, UEdGraphPin* Target)
{
    auto* Call = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, SetViewTargetWithBlend));
    G.Link(G.Read(DS::Controller), G.Pin(Call, P::FunctionTarget));
    G.Link(Target, G.Pin(Call, OP::NewViewTarget)); G.Default(Call, OP::BlendTime, N::Zero); G.Exec(Call);
}

void DedicatedAim(FGraph& G)
{
    auto* Rotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Read(Aim::Yaw), G.Pin(Rotation, SP::Yaw)); G.Link(G.Read(Aim::Pitch), G.Pin(Rotation, SP::Pitch));
    auto* Set = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRelativeRotation));
    G.Link(G.Read(DS::Camera), G.Pin(Set, P::FunctionTarget)); G.Link(G.Pin(Rotation, P::ReturnValue), G.Pin(Set, OP::RelativeRotation));
    G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
}

void BuildDedicatedStationGraph(UBlueprint* BP)
{
    UEdGraph* Graph = BP->UbergraphPages[0];
    const auto DefaultNodes = Graph->Nodes;
    for (UEdGraphNode* Node : DefaultNodes) Node->DestroyNode();
    FGraph G(Graph, nullptr);
    auto Self = [&]() { return OpticalSelf(G); };
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(G.Tail, G.Pin(Work, P::Execute)); G.Tail = Work->GetThenPinGivenIndex(0);
    ContextStationSafety(G, G.Pin(Tick, P::DeltaSeconds));
    G.Tail = Work->GetThenPinGivenIndex(1);
    G.Branch(G.Valid(G.Read(S::Anchor)));
    auto* Controlled = G.Branch(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()));
    auto* OccupiedTail = G.Tail;
    G.Tail = G.Pin(Controlled, P::Else);
    G.Branch(G.Read(DS::ViewOwned)); G.Write(DS::ViewOwned, nullptr, N::False);
    G.Branch(G.Valid(G.Read(DS::Controller)));
    auto* CurrentView = ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, GetViewTarget), G.Read(DS::Controller));
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), CurrentView, G.Read(DS::Camera)));
    auto* CurrentPawn = ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), G.Read(DS::Controller));
    G.Branch(G.Valid(CurrentPawn)); DedicatedViewTarget(G, CurrentPawn);

    G.Tail = OccupiedTail;
    auto* OccupiedWork = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(G.Tail, G.Pin(OccupiedWork, P::Execute)); G.Tail = OccupiedWork->GetThenPinGivenIndex(0);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Read(DS::ViewOwned), N::False));
    [[maybe_unused]] constexpr auto ControllerSignature = static_cast<AController* (APawn::*)() const>(&APawn::GetController);
    auto* Controller = ObserveCall(G, APawn::StaticClass(), BlueprintGraphNames::ActorFunctions::GetController, Self());
    auto* Cast = NewObject<UK2Node_DynamicCast>(Graph); Cast->TargetType = APlayerController::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Controller, Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    G.Write(DS::Controller, Cast->GetCastResultPin());
    auto* HasCamera = G.Branch(G.Valid(G.Read(DS::Camera))); auto* CameraReady = G.Tail;
    G.Tail = G.Pin(HasCamera, P::Else);
    auto* Location = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), Self());
    auto* Rotation = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorRotation), Self());
    auto* Transform = G.Transform(Location, Rotation);
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
    G.Pin(Spawn, E::ActorClass)->DefaultObject = ACameraActor::StaticClass(); G.Link(Transform, G.Pin(Spawn, P::SpawnTransform));
    G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn); G.Exec(Spawn); G.Branch(G.Valid(G.Pin(Spawn, P::ReturnValue)));
    auto* CameraCast = NewObject<UK2Node_DynamicCast>(Graph); CameraCast->TargetType = ACameraActor::StaticClass(); CameraCast->SetPurity(false); G.Node(CameraCast);
    G.Link(G.Tail, G.Pin(CameraCast, P::Execute)); G.Link(G.Pin(Spawn, P::ReturnValue), CameraCast->GetCastSourcePin()); G.Tail = CameraCast->GetValidCastPin();
    G.Write(DS::Camera, CameraCast->GetCastResultPin());
    auto* Finish = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(G.Read(DS::Camera), G.Pin(Finish, P::Actor)); G.Link(Transform, G.Pin(Finish, P::SpawnTransform)); G.Exec(Finish);
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(G.Read(DS::Camera), G.Pin(Attach, P::FunctionTarget));
    G.Link(ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), Self()), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld); G.Default(Attach, E::ScaleRule, N::KeepWorld);
    G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach); G.Branch(G.Pin(Attach, P::ReturnValue));
    auto* Position = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRelativeLocation));
    G.Link(G.Read(DS::Camera), G.Pin(Position, P::FunctionTarget)); G.Default(Position, OP::RelativeLocation, DS::EyeOffset);
    G.Default(Position, OP::Sweep, N::False); G.Default(Position, E::Teleport, N::False); G.Exec(Position);
    auto* FindCamera = G.Call(AActor::StaticClass(), OP::FindComponent);
    G.Link(G.Read(DS::Camera), G.Pin(FindCamera, P::FunctionTarget));
    G.Pin(FindCamera, OP::ComponentClass)->DefaultObject = UCameraComponent::StaticClass();
    auto* ComponentCast = NewObject<UK2Node_DynamicCast>(Graph); ComponentCast->TargetType = UCameraComponent::StaticClass(); ComponentCast->SetPurity(false); G.Node(ComponentCast);
    G.Link(G.Tail, G.Pin(ComponentCast, P::Execute)); G.Link(G.Pin(FindCamera, P::ReturnValue), ComponentCast->GetCastSourcePin()); G.Tail = ComponentCast->GetValidCastPin();
    G.Write(O::Camera, ComponentCast->GetCastResultPin());
    auto* Fov = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
    G.Link(ComponentCast->GetCastResultPin(), G.Pin(Fov, P::FunctionTarget)); G.Link(G.Read(O::RequestedFov), G.Pin(Fov, OP::FieldOfView)); G.Exec(Fov);
    StationMerge(G, {G.Tail, CameraReady});
    // Re-entry uses current on-foot FOV, including when the camera already exists.
    auto* RefreshFov = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
    G.Link(G.Read(O::Camera), G.Pin(RefreshFov, P::FunctionTarget)); G.Link(G.Read(O::RequestedFov), G.Pin(RefreshFov, OP::FieldOfView)); G.Exec(RefreshFov);
    DedicatedAim(G); DedicatedViewTarget(G, G.Read(DS::Camera)); G.Write(DS::ViewOwned, nullptr, N::True);

    // Independent sequence: misses/classification failures cannot block view setup or exit.
    G.Tail = OccupiedWork->GetThenPinGivenIndex(1);
    G.Branch(G.Read(DS::ViewOwned)); G.Branch(G.Valid(G.Read(O::Camera)));
    G.Branch(G.Valid(G.Read(N::OriginalPawn))); G.Branch(G.Valid(G.Read(S::Anchor)));
    UpdateStationRange(G, true);

    // Real Enhanced Input events on the possessed station, not observer key polling.
    auto ActionNode = [&](const TCHAR* Package, FName Trigger)
    {
        auto* Action = LoadObject<UInputAction>(nullptr, Package); check(Action);
        auto* Event = NewObject<UK2Node_EnhancedInputAction>(Graph); Event->InputAction = Action; G.Node(Event);
        G.Tail = G.Pin(Event, Trigger);
        G.Branch(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()));
        return Event;
    };
    ActionNode(HarpoonInputNames::Exit, DS::Started);
    auto* Exit = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle));
    G.Link(Self(), G.Pin(Exit, P::FunctionTarget)); G.Exec(Exit);
    for (bool Horizontal : {true, false})
    {
        auto* Action = ActionNode(Horizontal ? HarpoonInputNames::LookYaw : HarpoonInputNames::LookPitch, DS::Triggered);
        G.Branch(G.Valid(G.Read(DS::Camera)));
        auto* Scale = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
        G.Link(G.Pin(Action, DS::ActionValue), G.Pin(Scale, P::Binary::LeftOperand)); G.Default(Scale, P::Binary::RightOperand, DS::MouseScale);
        const FName Value = Horizontal ? Aim::Yaw : Aim::Pitch;
        auto* Sum = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(Value), G.Pin(Scale, P::ReturnValue));
        if (Horizontal)
        {
            auto* Normalize = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NormalizeAxis));
            G.Link(Sum, G.Pin(Normalize, OP::NormalizeAngle)); G.Write(Value, G.Pin(Normalize, P::ReturnValue));
        }
        else
        {
            auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
            G.Link(Sum, G.Pin(Clamp, OP::ClampValue)); G.Default(Clamp, OP::ClampMinimum, Aim::MinimumPitch); G.Default(Clamp, OP::ClampMaximum, Aim::MaximumPitch);
            G.Write(Value, G.Pin(Clamp, P::ReturnValue));
        }
        DedicatedAim(G);
    }
}

UClass* CreateDedicatedStation()
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(AVoyageVehiclePawn::StaticClass(), CreatePackage(DS::OperatorPackage),
        *FPackageName::GetLongPackageAssetName(DS::OperatorPackage), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    for (FName Field : {CE::Ready, CE::InteractBlocks, CE::ProviderSeen, CE::CallbackSeen, CE::ExitSent}) AddVariable(BP, Field, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, CE::Age, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, O::BaselineFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, S::Movement, UEdGraphSchema_K2::PC_Object, UCharacterMovementComponent::StaticClass());
    AddVariable(BP, CE::EntryAction, UEdGraphSchema_K2::PC_Object, UInputAction::StaticClass());
    // Stock acquisition separates the interaction object from its hit shape.
    // Only Engine transform properties are authored on this native component.
    auto* Interaction = BP->SimpleConstructionScript->CreateNode(UInteractiveObjectComponent::StaticClass(), CE::Interaction);
    Interaction->ParentComponentOrVariableName = BP->ParentClass->GetDefaultObject<AActor>()->GetRootComponent()->GetFName();
    Interaction->bIsParentComponentNative = true;
    BP->SimpleConstructionScript->AddNode(Interaction);
    CastChecked<USceneComponent>(Interaction->ComponentTemplate)->SetRelativeLocation(CE::BoxOffset);
    auto* Query = BP->SimpleConstructionScript->CreateNode(UBoxComponent::StaticClass(), CE::QueryBox);
    Interaction->AddChildNode(Query);
    auto* QueryTemplate = CastChecked<UBoxComponent>(Query->ComponentTemplate);
    QueryTemplate->SetBoxExtent(CE::BoxExtent);
    QueryTemplate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    QueryTemplate->SetCollisionObjectType(ECC_GameTraceChannel2);
    QueryTemplate->SetCollisionResponseToAllChannels(ECR_Ignore);
    QueryTemplate->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    QueryTemplate->SetGenerateOverlapEvents(false); QueryTemplate->SetSimulatePhysics(false);
    AddVariable(BP, N::NativeHudRequested, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Hint::ProviderSeen, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, DS::ViewOwned, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, DS::Camera, UEdGraphSchema_K2::PC_Object, ACameraActor::StaticClass());
    AddVariable(BP, DS::Controller, UEdGraphSchema_K2::PC_Object, APlayerController::StaticClass());
    AddVariable(BP, O::Camera, UEdGraphSchema_K2::PC_Object, UCameraComponent::StaticClass());
    AddVariable(BP, O::RequestedFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, N::OriginalPawn, UEdGraphSchema_K2::PC_Object, APawn::StaticClass());
    AddVariable(BP, S::Anchor, UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    AddVariable(BP, Range::TargetName, UEdGraphSchema_K2::PC_Text);
    AddVariable(BP, Range::TargetRange, UEdGraphSchema_K2::PC_Text);
    AddVariable(BP, Aim::Yaw, UEdGraphSchema_K2::PC_Real); AddVariable(BP, Aim::Pitch, UEdGraphSchema_K2::PC_Real);
    FKismetEditorUtilities::CompileBlueprint(BP);
    auto* Hud = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UVoyageBaseUserWidget::StaticClass(),
        CreatePackage(DS::HudPackage), *FPackageName::GetLongPackageAssetName(DS::HudPackage), BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
    if (!Hud->WidgetTree) Hud->WidgetTree = NewObject<UWidgetTree>(Hud, N::HudTree);
    AddVariable(Hud, Hint::HintsReady, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(Hud, Hint::HintInstance, UEdGraphSchema_K2::PC_Object, UVoyageDynamicPlayerInputWidget::StaticClass());
    auto* Canvas = Hud->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), N::HudCanvas); Canvas->bIsVariable = false;
    Hud->WidgetTree->RootWidget = Canvas;
    auto* Label = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), DS::Label); Label->bIsVariable = false;
    Label->SetText(FText::FromString(DS::HudLabel)); Label->SetJustification(ETextJustify::Center);
    auto Font = Label->GetFont(); Font.Size = H::TargetFontSize; Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::Green)); Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* Slot = Canvas->AddChildToCanvas(Label); Slot->SetAnchors(FAnchors(0.5f, 0.5f)); Slot->SetAlignment(FVector2D(0.5f, 0.5f)); Slot->SetAutoSize(true);
    Slot->SetPosition(FVector2D(0, DS::ExitLabelOffsetY));
    auto AddScopeText = [&](FName Field, const TCHAR* Text, float Offset, bool Variable)
    {
        auto* Widget = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Field); Widget->bIsVariable = Variable;
        Widget->SetText(FText::FromString(Text)); Widget->SetJustification(ETextJustify::Center); Widget->SetFont(Font);
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Layout = Canvas->AddChildToCanvas(Widget); Layout->SetAnchors(FAnchors(0.5f, 0.5f));
        Layout->SetAlignment(FVector2D(0.5f, 0.5f)); Layout->SetPosition(FVector2D(0, Offset)); Layout->SetAutoSize(true);
    };
    AddScopeText(O::Reticle, O::ReticleText, 0.0f, false);
    AddScopeText(Range::TargetName, N::EmptyText, DS::TargetNameOffsetY, true);
    AddScopeText(Range::TargetRange, N::EmptyText, DS::TargetRangeOffsetY, true);
    AddScopeText(Hint::Status, Hint::NotReady, DS::HintStatusOffsetY, true);
    auto* Host = Hud->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Hint::Root); Host->bIsVariable = true;
    auto* HostSlot = Canvas->AddChildToCanvas(Host); HostSlot->SetAnchors(FAnchors(0.0f, 1.0f));
    HostSlot->SetAlignment(FVector2D(0.0f, 1.0f)); HostSlot->SetPosition(DS::HintHostOffset); HostSlot->SetAutoSize(true);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud); FKismetEditorUtilities::CompileBlueprint(Hud);
    check(Hud->Status != BS_Error);
    UEdGraph* HudGraph = Hud->UbergraphPages[0]; const auto Defaults = HudGraph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph HG(HudGraph, nullptr);
    auto* HudTick = NewObject<UK2Node_Event>(HudGraph);
    HudTick->EventReference.SetExternalMember(BlueprintGraphNames::Events::WidgetTick, UUserWidget::StaticClass());
    HudTick->bOverrideFunction = true; HG.Node(HudTick); HG.Tail = HG.Pin(HudTick, P::Then);
    auto* Player = HG.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerPawn));
    auto* Station = NewObject<UK2Node_DynamicCast>(HudGraph); Station->TargetType = BP->GeneratedClass; Station->SetPurity(false); HG.Node(Station);
    HG.Link(HG.Tail, HG.Pin(Station, P::Execute)); HG.Link(HG.Pin(Player, P::ReturnValue), Station->GetCastSourcePin()); HG.Tail = Station->GetValidCastPin();
    for (FName Field : {Range::TargetName, Range::TargetRange})
    {
        auto* Value = ReadNativeInputField(HG, Station->GetCastResultPin(), BP->GeneratedClass, Field);
        auto* Set = HG.Call(UTextBlock::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
        HG.Link(HG.Read(Field), HG.Pin(Set, P::FunctionTarget)); HG.Link(Value, HG.Pin(Set, E::WidgetText)); HG.Exec(Set);
    }
    UpdateStationHintMarker(HG, Station->GetCastResultPin(), BP->GeneratedClass);
    AddStationHintConstruction(Hud);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud); FKismetEditorUtilities::CompileBlueprint(Hud);
    check(Hud->Status != BS_Error);
    AddStationActions(BP);
    AddContextEntry(BP);
    AddNativeStationHudInterface(BP, Hud->GeneratedClass); BuildDedicatedStationGraph(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP); check(BP->Status != BS_Error);
    auto* CDO = CastChecked<APawn>(BP->GeneratedClass->GetDefaultObject());
    check(CDO->GetRootComponent() && CDO->GetRootComponent()->IsA<UVoyageFastSceneComponent>());
    check(BP->SimpleConstructionScript->GetAllNodes().Num() == 2);
    check(Interaction->GetChildNodes().Num() == 1 && Interaction->GetChildNodes()[0] == Query);
    check(Interaction->bIsParentComponentNative && Interaction->ParentComponentOrVariableName == CDO->GetRootComponent()->GetFName());
    CDO->PrimaryActorTick.bCanEverTick = true; CDO->PrimaryActorTick.bStartWithTickEnabled = true;
    CDO->PrimaryActorTick.TickGroup = TG_PostPhysics;
    CDO->AutoPossessPlayer = EAutoReceiveInput::Disabled; CDO->AutoPossessAI = EAutoPossessAI::Disabled;
    CDO->bUseControllerRotationYaw = false; CDO->bUseControllerRotationPitch = false; CDO->bUseControllerRotationRoll = false;
    check(SaveDedicatedAsset(BP)); check(SaveDedicatedAsset(Hud)); return BP->GeneratedClass;
}
