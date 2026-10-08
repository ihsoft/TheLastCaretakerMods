#pragma once
#include "../../RailgunModelContract.h"
#include "StationSettings.h"
#include "StationEnergyHud.h"

// Builds the dedicated stationary railgun pawn and its HUD, input, and model graphs.
namespace ZoomTest
{
inline const FName Wide(TEXT("RailgunWideView"));
inline const FName Mouse(TEXT("RailgunActiveMousePercent"));
inline const FName Mask(TEXT("RailgunOpticalMask"));
inline const FName MaskImage(TEXT("RailgunOpticalMaskImage"));
inline constexpr TCHAR MaskPackage[] = TEXT("/Game/Mods/Railgun/Station/T_RailgunOpticalMask");
inline constexpr TCHAR MaskAsset[] = TEXT("T_RailgunOpticalMask");
inline constexpr TCHAR OverlaySourceArgument[] = TEXT("ScopeOverlay=");
inline constexpr TCHAR NormalMouse[] = TEXT("100.0");
inline constexpr TCHAR Hidden[] = TEXT("Collapsed");
inline constexpr TCHAR Shown[] = TEXT("HitTestInvisible");
inline const FName WideCenter(TEXT("RailgunWideCenter"));
inline constexpr TCHAR WideCenterText[] = TEXT("\u25CB");
inline UTexture2D* OverlayTexture = nullptr;
}
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

UTexture2D* ImportUiTexture(const FString& Filename, const TCHAR* PackageName,
    const TCHAR* AssetName, bool RequireSquare)
{
    auto* Task = NewObject<UAssetImportTask>();
    Task->Filename = Filename;
    Task->DestinationPath = FPackageName::GetLongPackagePath(PackageName);
    Task->DestinationName = AssetName;
    Task->bReplaceExisting = true;
    Task->bReplaceExistingSettings = true;
    Task->bAutomated = true;
    Task->bSave = false;
    Task->bAsync = false;
    auto* Factory = NewObject<UTextureFactory>();
    Factory->NoCompression = true;
    Factory->NoAlpha = false;
    Factory->bDeferCompression = false;
    Factory->CompressionSettings = TC_EditorIcon;
    Factory->MipGenSettings = TMGS_NoMipmaps;
    Factory->LODGroup = TEXTUREGROUP_UI;
    UTextureFactory::SuppressImportOverwriteDialog(true);
    Task->Factory = Factory;
    TArray<UAssetImportTask*> Tasks {Task};
    FAssetToolsModule::GetModule().Get().ImportAssetTasks(Tasks);
    const TArray<UObject*>& Imported = Task->GetObjects();
    checkf(Imported.Num() == 1, TEXT("Expected one imported UI texture, got %d"), Imported.Num());
    auto* Texture = CastChecked<UTexture2D>(Imported[0]);
    checkf(Texture->GetOutermost()->GetName() == PackageName,
        TEXT("UI texture package mismatch: %s"), *Texture->GetPathName());
    const int32 Width = Texture->Source.GetSizeX();
    const int32 Height = Texture->Source.GetSizeY();
    checkf(Width > 0 && Height > 0, TEXT("UI texture must be non-empty, got %dx%d"), Width, Height);
    checkf(!RequireSquare || Width == Height, TEXT("Scope overlay must be square, got %dx%d"), Width, Height);
    Texture->CompressionSettings = TC_EditorIcon;
    Texture->MipGenSettings = TMGS_NoMipmaps;
    Texture->LODGroup = TEXTUREGROUP_UI;
    Texture->NeverStream = true;
    Texture->SRGB = true;
    Texture->UpdateResource();
    check(SaveDedicatedAsset(Texture));
    return Texture;
}

void DedicatedViewTarget(FGraph& G, UEdGraphPin* Target)
{
    auto* Call = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, SetViewTargetWithBlend));
    G.Link(G.Read(DS::Controller), G.Pin(Call, P::FunctionTarget));
    G.Link(Target, G.Pin(Call, OP::NewViewTarget)); G.Default(Call, OP::BlendTime, N::Zero); G.Exec(Call);
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void AddRailgunAimBindingFunctions(UBlueprint* BP)
{
    auto AddFunction = [&](FName Name)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, Name,
            UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        return TPair<UEdGraph*, UK2Node_FunctionEntry*>(Graph, Entry);
    };

    auto ClearFunction = AddFunction(Aim::ClearReferences);
    FGraph Clear(ClearFunction.Key, nullptr);
    Clear.Tail = Clear.Pin(ClearFunction.Value, P::Then);
    Clear.Write(Aim::YawComponent, nullptr);
    Clear.Write(Aim::PitchComponent, nullptr);
    Clear.Write(Aim::ModelOwner, nullptr);
    Clear.Write(EyeAim::FirstPersonCamera, nullptr);
    Clear.Write(EyeAim::FirstPersonCameraOwner, nullptr);

    auto BindFunction = AddFunction(Aim::BindComponents);
    FGraph Bind(BindFunction.Key, nullptr);
    Bind.Tail = Bind.Pin(BindFunction.Value, P::Then);
    Bind.Write(Aim::YawComponent, nullptr);
    Bind.Write(Aim::PitchComponent, nullptr);
    Bind.Write(Aim::ModelOwner, nullptr);
    auto* AnchorValid = Bind.Branch(Bind.Valid(Bind.Read(S::Anchor)));
    auto* Owner = ObserveCall(Bind, UActorComponent::StaticClass(),
        ActorScanGraphNames::GetActorOwner, Bind.Read(S::Anchor));
    auto* OwnerValid = Bind.Branch(Bind.Valid(Owner));
    Bind.Write(Aim::ModelOwner, Owner);
    auto BindRole = [&](FName Tag, FName Field)
    {
        auto* Find = Bind.Call(AActor::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
        Bind.Link(Owner, Bind.Pin(Find, P::FunctionTarget));
        Bind.Pin(Find, OP::ComponentClass)->DefaultObject =
            USceneComponent::StaticClass();
        Bind.Default(Find, ActorScanGraphNames::ComponentTag,
            *Tag.ToString());
        auto* Loop = ContextLoop(Bind, Bind.Pin(Find, P::ReturnValue));
        auto* Cast = NewObject<UK2Node_DynamicCast>(Bind.Graph);
        Cast->TargetType = USceneComponent::StaticClass();
        Cast->SetPurity(false);
        Bind.Node(Cast);
        Bind.Link(Bind.Tail, Bind.Pin(Cast, P::Execute));
        Bind.Link(Bind.Pin(Loop, CE::ArrayElement),
            Cast->GetCastSourcePin());
        Bind.Tail = Cast->GetValidCastPin();
        Bind.Write(Field, Cast->GetCastResultPin());
        Bind.Tail = Bind.Pin(Loop, CE::Completed);
    };
    BindRole(RailgunModelContract::YawTag, Aim::YawComponent);
    BindRole(RailgunModelContract::PitchTag, Aim::PitchComponent);
    StationMerge(Bind, {Bind.Tail, Bind.Pin(OwnerValid, P::Else),
        Bind.Pin(AnchorValid, P::Else)});

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(Aim::BindComponents) &&
        BP->GeneratedClass->FindFunctionByName(Aim::ClearReferences));
}

void AimModelPivot(FGraph& G, FName Component, FName Angle, FName Axis)
{
    auto* ComponentValid = G.Branch(G.Valid(G.Read(Component)));
    auto* OwnerValid = G.Branch(G.Valid(G.Read(Aim::ModelOwner)));
    auto* Owner = ObserveCall(G, UActorComponent::StaticClass(),
        ActorScanGraphNames::GetActorOwner, G.Read(Component));
    auto* SameOwner = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject), Owner, G.Read(Aim::ModelOwner)));
    auto* Rotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Read(Angle), G.Pin(Rotation, Axis));
    auto* Set = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetRelativeRotation));
    G.Link(G.Read(Component), G.Pin(Set, P::FunctionTarget));
    G.Link(G.Pin(Rotation, P::ReturnValue), G.Pin(Set, SP::NewRotation));
    G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
    StationMerge(G, {G.Tail, G.Pin(SameOwner, P::Else),
        G.Pin(OwnerValid, P::Else), G.Pin(ComponentValid, P::Else)});
}

void DedicatedAim(FGraph& G)
{
    // Camera is a child of the descriptor's sight under pitch. It inherits
    // model motion once, without a second camera-local yaw/pitch rotation.
    // Visual articulation only: never rotate the physical mount, station or
    // character. Missing visual roles must not break entry/camera/exit.
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(G.Graph));
    G.Link(G.Tail, G.Pin(Work, P::Execute)); G.Tail = Work->GetThenPinGivenIndex(0);
    AimModelPivot(G, Aim::YawComponent, Aim::Yaw, SP::Yaw);
    AimModelPivot(G, Aim::PitchComponent, Aim::Pitch, SP::Pitch);
    G.Tail = Work->GetThenPinGivenIndex(1);
}

#include "EyeAimGraph.h"

void ApplyRailgunAimRecoil(FGraph& G)
{
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        Greater_DoubleDouble), G.Read(RailgunRecoil::CameraStrength),
        RailgunRecoil::MinimumEnabledStrength));
    auto* RandomAzimuth = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, RandomFloatInRange));
    G.Default(RandomAzimuth, P::Min, N::Zero);
    G.Default(RandomAzimuth, P::Max,
        RailgunRecoil::MaximumAzimuthRadians);
    G.Write(RailgunRecoil::AimAzimuth,
        G.Pin(RandomAzimuth, P::ReturnValue));

    auto* KickAngle = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble));
    G.Link(G.Read(RailgunRecoil::CameraStrength),
        G.Pin(KickAngle, P::Binary::LeftOperand));
    G.Default(KickAngle, P::Binary::RightOperand,
        RailgunRecoil::BaselineAimKickDegrees);
    auto* AzimuthCos = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Cos));
    G.Link(G.Read(RailgunRecoil::AimAzimuth),
        G.Pin(AzimuthCos, RailgunRecoil::VectorInputPin));
    auto* AzimuthSin = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Sin));
    G.Link(G.Read(RailgunRecoil::AimAzimuth),
        G.Pin(AzimuthSin, RailgunRecoil::VectorInputPin));
    UEdGraphPin* DeltaYaw = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble), G.Pin(AzimuthCos, P::ReturnValue),
        G.Pin(KickAngle, P::ReturnValue));
    UEdGraphPin* DeltaPitch = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble), G.Pin(AzimuthSin, P::ReturnValue),
        G.Pin(KickAngle, P::ReturnValue));

    auto* Wide = G.Branch(G.Read(ZoomTest::Wide));
    G.Write(EyeAim::Yaw, ClampStationAim(G, true,
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble), G.Read(EyeAim::Yaw), DeltaYaw)));
    G.Write(EyeAim::Pitch, ClampStationAim(G, false,
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble), G.Read(EyeAim::Pitch), DeltaPitch)));
    RotateEyeCamera(G);
    ConvergeEyeAim(G);
    UEdGraphPin* WideTail = G.Tail;

    G.Tail = G.Pin(Wide, P::Else);
    G.Write(Aim::Yaw, ClampStationAim(G, true,
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble), G.Read(Aim::Yaw), DeltaYaw)));
    G.Write(Aim::Pitch, ClampStationAim(G, false,
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble), G.Read(Aim::Pitch), DeltaPitch)));
    DedicatedAim(G);
    StationMerge(G, {WideTail, G.Tail});
}

void BuildDedicatedStationGraph(UBlueprint* BP)
{
    UEdGraph* Graph = BP->UbergraphPages[0];
    const auto DefaultNodes = Graph->Nodes;
    // Energy owns two isolated custom-event components that must survive
    // reconstruction of the station lifecycle graph. Preserve each complete
    // connected component, including its latent and pure dependency nodes.
    TSet<UEdGraphNode*> EnergyEventNodes;
    TArray<UEdGraphNode*> PendingEnergyNodes;
    for (UEdGraphNode* Node : DefaultNodes)
        if (Cast<UK2Node_CustomEvent>(Node)) PendingEnergyNodes.Add(Node);
    while (!PendingEnergyNodes.IsEmpty())
    {
        UEdGraphNode* Node = PendingEnergyNodes.Pop();
        if (!Node || EnergyEventNodes.Contains(Node)) continue;
        EnergyEventNodes.Add(Node);
        for (UEdGraphPin* Pin : Node->Pins)
            for (UEdGraphPin* Linked : Pin->LinkedTo)
                if (Linked && Linked->GetOwningNode())
                    PendingEnergyNodes.Add(Linked->GetOwningNode());
    }
    for (UEdGraphNode* Node : DefaultNodes)
        if (!EnergyEventNodes.Contains(Node)) Node->DestroyNode();
    FGraph G(Graph, nullptr);
    auto Self = [&]() { return OpticalSelf(G); };
    auto* BeginPlay = NewObject<UK2Node_Event>(Graph);
    BeginPlay->EventReference.SetExternalMember(TimerGraphNames::ActorBeginPlay, AActor::StaticClass());
    BeginPlay->bOverrideFunction = true; G.Node(BeginPlay); G.Tail = G.Pin(BeginPlay, P::Then);
    ReadStationSettings(G);
    auto* RefreshBeginPlayHudStyle = G.Call(BP->GeneratedClass,
        EnergyHud::RefreshStyleFunction);
    G.Exec(RefreshBeginPlayHudStyle);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    UEdGraphPin* TickTail = G.Tail;

    auto* Possessed = NewObject<UK2Node_Event>(Graph);
    Possessed->EventReference.SetExternalMember(
        ActorLifecycleGraphNames::ReceivePossessedEvent,
        APawn::StaticClass());
    Possessed->bOverrideFunction = true;
    G.Node(Possessed);
    G.Tail = G.Pin(Possessed, P::Then);
    G.Write(StationLifecycle::ExitPending, nullptr, N::False);
    G.Write(StationLifecycle::EntryPending, nullptr, N::True);
    auto* EntryDelay = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, DelayUntilNextTick));
    G.Exec(EntryDelay);
    G.Branch(G.Read(StationLifecycle::EntryPending));
    G.Write(StationLifecycle::EntryPending, nullptr, N::False);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_BoolBool), G.Read(StationLifecycle::TeardownPending),
        N::False));
    G.Branch(G.Read(CE::Ready));
    G.Branch(G.Valid(G.Read(S::Anchor)));
    G.Branch(ContextParentValid(G));
    G.Branch(ObserveCall(G, APawn::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()));
    [[maybe_unused]] constexpr auto ControllerSignature =
        static_cast<AController* (APawn::*)() const>(&APawn::GetController);
    auto* Controller = ObserveCall(G, APawn::StaticClass(),
        BlueprintGraphNames::ActorFunctions::GetController, Self());
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_ObjectObject), Controller,
        G.Pin(Possessed, ActorLifecycleGraphNames::NewController)));
    auto* Cast = NewObject<UK2Node_DynamicCast>(Graph); Cast->TargetType = APlayerController::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Controller, Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    G.Write(DS::Controller, Cast->GetCastResultPin());
    G.Branch(ObserveCall(G, APlayerController::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APlayerController, IsLocalController),
        G.Read(DS::Controller)));
    ReadStationSettings(G);
    auto* RefreshPossessedHudStyle = G.Call(BP->GeneratedClass,
        EnergyHud::RefreshStyleFunction);
    G.Exec(RefreshPossessedHudStyle);
    auto* RefreshEnergySettings = G.Call(BP->GeneratedClass,
        Charge::RefreshFunction);
    G.Default(RefreshEnergySettings, Charge::ForceDemandParameter, N::True);
    G.Exec(RefreshEnergySettings);
    auto* RefreshChargeIndicator = G.Call(BP->GeneratedClass,
        RailgunChargeIndicator::RefreshFunction);
    G.Exec(RefreshChargeIndicator);
    G.Write(ZoomTest::Wide, nullptr, N::True);
    G.Write(ZoomTest::Mouse, nullptr, ZoomTest::NormalMouse);
    BindFirstPersonCamera(G);
    ClearStationRangeDisplay(G);
    auto* FindSight = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor)), G.Pin(FindSight, P::FunctionTarget));
    G.Pin(FindSight, OP::ComponentClass)->DefaultObject = USceneComponent::StaticClass();
    G.Default(FindSight, ActorScanGraphNames::ComponentTag, *RailgunModelContract::SightTag.ToString());
    auto* SightLoop = ContextLoop(G, G.Pin(FindSight, P::ReturnValue));
    auto* SightCast = NewObject<UK2Node_DynamicCast>(Graph); SightCast->TargetType = USceneComponent::StaticClass(); SightCast->SetPurity(false); G.Node(SightCast);
    G.Link(G.Tail, G.Pin(SightCast, P::Execute)); G.Link(G.Pin(SightLoop, CE::ArrayElement), SightCast->GetCastSourcePin()); G.Tail = SightCast->GetValidCastPin();
    G.Write(DS::Sight, SightCast->GetCastResultPin()); G.Tail = G.Pin(SightLoop, CE::Completed);
    G.Branch(G.Valid(G.Read(DS::Sight)));
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
    G.Link(G.Read(DS::Sight), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, E::SnapToTarget); G.Default(Attach, E::RotationRule, E::SnapToTarget); G.Default(Attach, E::ScaleRule, N::KeepWorld);
    G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach); G.Branch(G.Pin(Attach, P::ReturnValue));
    auto* FindCamera = G.Call(AActor::StaticClass(), OP::FindComponent);
    G.Link(G.Read(DS::Camera), G.Pin(FindCamera, P::FunctionTarget));
    G.Pin(FindCamera, OP::ComponentClass)->DefaultObject = UCameraComponent::StaticClass();
    auto* ComponentCast = NewObject<UK2Node_DynamicCast>(Graph); ComponentCast->TargetType = UCameraComponent::StaticClass(); ComponentCast->SetPurity(false); G.Node(ComponentCast);
    G.Link(G.Tail, G.Pin(ComponentCast, P::Execute)); G.Link(G.Pin(FindCamera, P::ReturnValue), ComponentCast->GetCastSourcePin()); G.Tail = ComponentCast->GetValidCastPin();
    G.Write(O::Camera, ComponentCast->GetCastResultPin());
    auto* Fov = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
    G.Link(ComponentCast->GetCastResultPin(), G.Pin(Fov, P::FunctionTarget)); G.Link(G.Read(O::BaselineFov), G.Pin(Fov, OP::FieldOfView)); G.Exec(Fov);
    StationMerge(G, {G.Tail, CameraReady});
    // Re-entry uses current on-foot FOV, including when the camera already exists.
    auto* RefreshFov = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
    G.Link(G.Read(O::Camera), G.Pin(RefreshFov, P::FunctionTarget)); G.Link(G.Read(O::BaselineFov), G.Pin(RefreshFov, OP::FieldOfView)); G.Exec(RefreshFov);
    DedicatedAim(G);
    PlaceModeCamera(G, true);
    DedicatedViewTarget(G, G.Read(DS::Camera));
    G.Write(DS::ViewOwned, nullptr, N::True);
    auto* RefreshEntryHudMode = G.Call(BP->GeneratedClass,
        EnergyHud::RefreshModeFunction);
    G.Exec(RefreshEntryHudMode);
    auto* EnableRuntimeActivity = G.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    G.Exec(EnableRuntimeActivity);

    auto* Unpossessed = NewObject<UK2Node_Event>(Graph);
    Unpossessed->EventReference.SetExternalMember(
        ActorLifecycleGraphNames::ReceiveUnpossessedEvent,
        APawn::StaticClass());
    Unpossessed->bOverrideFunction = true;
    G.Node(Unpossessed);
    G.Tail = G.Pin(Unpossessed, P::Then);
    G.Write(StationLifecycle::EntryPending, nullptr, N::False);
    G.Write(StationLifecycle::ExitPending, nullptr, N::True);
    auto* ExitDelay = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, DelayUntilNextTick));
    G.Exec(ExitDelay);
    G.Branch(G.Read(StationLifecycle::ExitPending));
    G.Write(StationLifecycle::ExitPending, nullptr, N::False);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_BoolBool), ObserveCall(G, APawn::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()),
        N::False));
    UEdGraphPin* RemainingController = ObserveCall(G, APawn::StaticClass(),
        BlueprintGraphNames::ActorFunctions::GetController, Self());
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_BoolBool), G.Valid(RemainingController), N::False));
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_ObjectObject),
        G.Pin(Unpossessed, ActorLifecycleGraphNames::OldController),
        G.Read(DS::Controller)));
    RestoreDedicatedViewIfOwned(G);
    G.Write(EyeAim::FirstPersonCamera, nullptr);
    G.Write(EyeAim::FirstPersonCameraOwner, nullptr);
    ClearStationRangeDisplay(G);
    auto* RefreshRuntimeActivity = G.Call(BP->GeneratedClass,
        DS::RefreshActivity);
    G.Exec(RefreshRuntimeActivity);
    auto* Teardown = G.Branch(
        G.Read(StationLifecycle::TeardownPending));
    auto* FinalizeTeardown = G.Call(BP->GeneratedClass,
        StationLifecycle::FinalizeTeardown);
    G.Exec(FinalizeTeardown);
    G.Tail = G.Pin(Teardown, P::Else);

    // Independent sequence: misses/classification failures cannot block view setup or exit.
    G.Tail = TickTail;
    G.Branch(G.Valid(G.Read(S::Anchor)));
    G.Branch(ObserveCall(G, APawn::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()));
    auto* OpticalWork = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(G.Tail, G.Pin(OpticalWork, P::Execute)); G.Tail = OpticalWork->GetThenPinGivenIndex(0);
    G.Branch(G.Read(DS::ViewOwned)); G.Branch(G.Valid(G.Read(O::Camera)));
    G.Branch(G.Valid(G.Read(N::OriginalPawn))); G.Branch(G.Valid(G.Read(S::Anchor)));
    auto* EyeMode = G.Branch(G.Read(ZoomTest::Wide));
    UpdateEyeCameraPosition(G); ConvergeEyeAim(G); StationMerge(G, {G.Tail, G.Pin(EyeMode, P::Else)});
    UpdateStationRange(G);

    // Action event, not polling: one toggle on RMB Started while possessed.
    auto* ZoomInput = LoadObject<UInputAction>(nullptr, RailgunInputNames::Zoom); check(ZoomInput);
    auto* ZoomEvent = NewObject<UK2Node_EnhancedInputAction>(Graph); ZoomEvent->InputAction = ZoomInput; G.Node(ZoomEvent);
    G.Tail = G.Pin(ZoomEvent, DS::Started);
    G.Branch(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()));
    G.Branch(G.Read(DS::ViewOwned)); G.Branch(G.Valid(G.Read(O::Camera)));
    G.Branch(G.Valid(G.Read(DS::Controller)));
    auto* Wide = G.Branch(G.Read(ZoomTest::Wide));
    auto SetZoom = [&](bool ToWide)
    {
        if (!ToWide) ConvergeEyeAim(G);
        PlaceModeCamera(G, ToWide);
        G.Write(ZoomTest::Wide, nullptr, ToWide ? N::True : N::False);
        G.Write(ZoomTest::Mouse, ToWide ? nullptr : G.Read(Settings::Mouse), ToWide ? ZoomTest::NormalMouse : nullptr);
        auto* Set = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
        G.Link(G.Read(O::Camera), G.Pin(Set, P::FunctionTarget));
        G.Link(G.Read(ToWide ? O::BaselineFov : O::RequestedFov), G.Pin(Set, OP::FieldOfView)); G.Exec(Set);
        auto* RefreshHudMode = G.Call(BP->GeneratedClass,
            EnergyHud::RefreshModeFunction);
        G.Exec(RefreshHudMode);
    };
    SetZoom(false);
    G.Tail = G.Pin(Wide, P::Else); SetZoom(true);

    AddRailgunVfxCanaries(G, Self());
    AddRailgunFire(G);
    // Real Enhanced Input events on the possessed station, not observer key polling.
    auto ActionNode = [&](const TCHAR* Package, FName Trigger)
    {
        auto* Action = LoadObject<UInputAction>(nullptr, Package); check(Action);
        auto* Event = NewObject<UK2Node_EnhancedInputAction>(Graph); Event->InputAction = Action; G.Node(Event);
        G.Tail = G.Pin(Event, Trigger);
        G.Branch(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), Self()));
        return Event;
    };
    ActionNode(RailgunInputNames::Exit, DS::Started);
    auto* Exit = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle));
    G.Link(Self(), G.Pin(Exit, P::FunctionTarget)); G.Exec(Exit);
    for (bool Horizontal : {true, false})
    {
        auto* Action = ActionNode(Horizontal ? RailgunInputNames::LookYaw : RailgunInputNames::LookPitch, DS::Triggered);
        G.Branch(G.Read(DS::ViewOwned));
        G.Branch(G.Valid(G.Read(DS::Camera)));
        auto* Scale = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
        auto* Scalar = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
        G.Link(G.Read(ZoomTest::Mouse), G.Pin(Scalar, P::Binary::LeftOperand)); G.Default(Scalar, P::Binary::RightOperand, Settings::OriginalPerPercent);
        G.Link(G.Pin(Action, DS::ActionValue), G.Pin(Scale, P::Binary::LeftOperand)); G.Link(G.Pin(Scalar, P::ReturnValue), G.Pin(Scale, P::Binary::RightOperand));
        auto* EyeInput = G.Branch(G.Read(ZoomTest::Wide));
        const FName EyeValue = Horizontal ? EyeAim::Yaw : EyeAim::Pitch;
        auto* EyeSum = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(EyeValue), G.Pin(Scale, P::ReturnValue));
        G.Write(EyeValue, ClampStationAim(G, Horizontal, EyeSum)); RotateEyeCamera(G);
        G.Tail = G.Pin(EyeInput, P::Else);
        const FName Value = Horizontal ? Aim::Yaw : Aim::Pitch;
        auto* Sum = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(Value), G.Pin(Scale, P::ReturnValue));
        G.Write(Value, ClampStationAim(G, Horizontal, Sum)); DedicatedAim(G);
    }
}

UClass* CreateDedicatedStation()
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(AVoyageVehiclePawn::StaticClass(), CreatePackage(DS::OperatorPackage),
        *FPackageName::GetLongPackageAssetName(DS::OperatorPackage), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    AddVariable(BP, Shot::SpawnedThisPress, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Shot::AmmoSlot, UEdGraphSchema_K2::PC_Int);
    AddVariable(BP, Shot::SpawnLocation, UEdGraphSchema_K2::PC_Struct,
        TBaseStructure<FVector>::Get());
    AddVariable(BP, Shot::SpawnRotation, UEdGraphSchema_K2::PC_Struct,
        TBaseStructure<FRotator>::Get());
    AddVariable(BP, RailgunRecoil::AimAzimuth,
        UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, RailgunRecoil::SearchComponent,
        UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    AddArrayVariable(BP, RailgunRecoil::VisitedComponents,
        UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    AddVariable(BP, RailgunRecoil::ShotDirection,
        UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    AddVariable(BP, RailgunRecoil::ModuleLocation,
        UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    AddVariable(BP, Charge::Module, UEdGraphSchema_K2::PC_Object, UVoyageModuleComponent::StaticClass());
    AddVariable(BP, Charge::Energy, UEdGraphSchema_K2::PC_Real);
    for (FName Field : {Charge::DemandInitialized, Charge::DemandCharging,
        Charge::UpdateActive, Charge::UpdatePending,
        Charge::SocketConnected, Charge::PowerAvailable,
        Charge::OfflineDrainActive, Charge::OfflineDrainDebitActive,
        Charge::SupplyReconcilePending})
        AddVariable(BP, Field, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Charge::SupplyReconcileGeneration,
        UEdGraphSchema_K2::PC_Int);
    for (FName Field : {Charge::SupplyReconcileTime,
        Charge::OfflineDrainTimestamp, Charge::OfflineDrainRate,
        Charge::OfflineDrainElapsed})
        AddVariable(BP, Field, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, Charge::OfflineDrainTimerHandle,
        UEdGraphSchema_K2::PC_Struct, FTimerHandle::StaticStruct());
    for (FName Field : {Charge::OfflineDrainActive,
        Charge::OfflineDrainDebitActive,
        Charge::SupplyReconcilePending,
        Charge::SupplyReconcileGeneration,
        Charge::SupplyReconcileTime,
        Charge::OfflineDrainTimestamp,
        Charge::OfflineDrainRate,
        Charge::OfflineDrainElapsed,
        Charge::OfflineDrainTimerHandle})
        MarkVariableTransient(BP, Field);
    AddVariable(BP, RailgunChargeIndicator::Owner,
        UEdGraphSchema_K2::PC_Object, AActor::StaticClass());
    AddVariable(BP, RailgunChargeIndicator::Component,
        UEdGraphSchema_K2::PC_Object, UStaticMeshComponent::StaticClass());
    AddVariable(BP, RailgunChargeIndicator::Material,
        UEdGraphSchema_K2::PC_Object, UMaterialInstanceDynamic::StaticClass());
    AddVariable(BP, RailgunChargeIndicator::LastLevel,
        UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, RailgunChargeIndicator::BoundModule,
        UEdGraphSchema_K2::PC_Object,
        UVoyageModuleComponent::StaticClass());
    AddVariable(BP, ZoomTest::Wide, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, EyeAim::Yaw, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, EyeAim::Pitch, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, EyeAim::Target, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    AddVariable(BP, EyeAim::FirstPersonCamera,
        UEdGraphSchema_K2::PC_Object, UCameraComponent::StaticClass());
    AddVariable(BP, EyeAim::FirstPersonCameraOwner,
        UEdGraphSchema_K2::PC_Object, APawn::StaticClass());
    AddVariable(BP, ZoomTest::Mouse, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, RailgunVfxCanary::ExplosionSystem,
        UEdGraphSchema_K2::PC_Object, UNiagaraSystem::StaticClass());
    AddVariable(BP, RailgunVfxCanary::SplashSystem,
        UEdGraphSchema_K2::PC_Object, UNiagaraSystem::StaticClass());
    AddVariable(BP, RailgunVfxCanary::ExplosionComponent,
        UEdGraphSchema_K2::PC_Object, UNiagaraComponent::StaticClass());
    AddVariable(BP, RailgunVfxCanary::SplashComponent,
        UEdGraphSchema_K2::PC_Object, UNiagaraComponent::StaticClass());
    for (const auto& Setting : Settings::NumericSettings) AddVariable(BP, Setting.Field, UEdGraphSchema_K2::PC_Real);
    for (const auto& Setting : Settings::TextSettings) AddVariable(BP, Setting.Field, UEdGraphSchema_K2::PC_String);
    for (const auto& Setting : Settings::FontSettings)
        AddVariable(BP, Setting.ObjectField, UEdGraphSchema_K2::PC_Object, UObject::StaticClass());
    AddVariable(BP, DS::Sight, UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    for (FName Field : {CE::Ready, CE::InteractBlocks}) AddVariable(BP, Field, UEdGraphSchema_K2::PC_Boolean);
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
    AddVariable(BP, DS::ViewOwned, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, DS::Camera, UEdGraphSchema_K2::PC_Object, ACameraActor::StaticClass());
    AddVariable(BP, DS::Controller, UEdGraphSchema_K2::PC_Object, APlayerController::StaticClass());
    AddVariable(BP, StationLifecycle::ShellOwner,
        UEdGraphSchema_K2::PC_Object, AActor::StaticClass());
    for (FName Field : {StationLifecycle::EntryPending,
        StationLifecycle::ExitPending,
        StationLifecycle::TeardownPending})
        AddVariable(BP, Field, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, O::Camera, UEdGraphSchema_K2::PC_Object, UCameraComponent::StaticClass());
    AddVariable(BP, O::RequestedFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, N::OriginalPawn, UEdGraphSchema_K2::PC_Object, APawn::StaticClass());
    AddVariable(BP, S::Anchor, UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    AddVariable(BP, Range::TargetName, UEdGraphSchema_K2::PC_Text);
    AddVariable(BP, Range::TargetRange, UEdGraphSchema_K2::PC_Text);
    AddVariable(BP, Range::PendingTargetName,
        UEdGraphSchema_K2::PC_Text);
    MarkVariableTransient(BP, Range::PendingTargetName);
    AddVariable(BP, EnergyHud::ActiveHud, UEdGraphSchema_K2::PC_Object,
        UUserWidget::StaticClass());
    MarkVariableTransient(BP, EnergyHud::ActiveHud);
    AddVariable(BP, Aim::Yaw, UEdGraphSchema_K2::PC_Real); AddVariable(BP, Aim::Pitch, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, Aim::YawComponent, UEdGraphSchema_K2::PC_Object,
        USceneComponent::StaticClass());
    AddVariable(BP, Aim::PitchComponent, UEdGraphSchema_K2::PC_Object,
        USceneComponent::StaticClass());
    AddVariable(BP, Aim::ModelOwner, UEdGraphSchema_K2::PC_Object,
        AActor::StaticClass());
    for (FName RuntimeReference : {EyeAim::FirstPersonCamera,
        EyeAim::FirstPersonCameraOwner, Aim::YawComponent,
        Aim::PitchComponent, Aim::ModelOwner})
        MarkVariableTransient(BP, RuntimeReference);
    FKismetEditorUtilities::CompileBlueprint(BP);
    AddRailgunAimBindingFunctions(BP);
    auto* Hud = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UVoyageBaseUserWidget::StaticClass(),
        CreatePackage(DS::HudPackage), *FPackageName::GetLongPackageAssetName(DS::HudPackage), BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
    if (!Hud->WidgetTree) Hud->WidgetTree = NewObject<UWidgetTree>(Hud, N::HudTree);
    AddVariable(Hud, Hint::HintsReady, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(Hud, Hint::HintInstance, UEdGraphSchema_K2::PC_Object, UVoyageDynamicPlayerInputWidget::StaticClass());
    AddVariable(Hud, EnergyHud::AmmoInitialized, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(Hud, EnergyHud::Station, UEdGraphSchema_K2::PC_Object,
        BP->GeneratedClass);
    MarkVariableTransient(Hud, EnergyHud::Station);
    AddVariable(Hud, Range::DisplayedTargetName,
        UEdGraphSchema_K2::PC_Text);
    AddVariable(Hud, Range::DisplayedTargetRange,
        UEdGraphSchema_K2::PC_Text);
    AddVariable(Hud, Range::DisplayInitialized,
        UEdGraphSchema_K2::PC_Boolean);
    AddVariable(Hud, EnergyHud::StatusState, UEdGraphSchema_K2::PC_Int);
    AddVariable(Hud, EnergyHud::StatusInitialized,
        UEdGraphSchema_K2::PC_Boolean);
    for (FName RuntimeDisplayState : {Range::DisplayedTargetName,
        Range::DisplayedTargetRange, Range::DisplayInitialized,
        EnergyHud::StatusState, EnergyHud::StatusInitialized})
        MarkVariableTransient(Hud, RuntimeDisplayState);
    auto* Canvas = Hud->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), N::HudCanvas); Canvas->bIsVariable = false;
    Hud->WidgetTree->RootWidget = Canvas;
    check(ZoomTest::OverlayTexture);
    const int32 MaskWidth = ZoomTest::OverlayTexture->Source.GetSizeX();
    const int32 MaskHeight = ZoomTest::OverlayTexture->Source.GetSizeY();
    auto* Mask = Hud->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), ZoomTest::Mask); Mask->bIsVariable = true;
    // A square image is scaled uniformly inside the full viewport. ScaleBox
    // centers its child, so the authored center pixel remains the aim center.
    Mask->SetStretch(EStretch::ScaleToFit); Mask->SetClipping(EWidgetClipping::ClipToBounds);
    auto* MaskImage = Hud->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), ZoomTest::MaskImage);
    MaskImage->SetBrushFromTexture(ZoomTest::OverlayTexture, false);
    FSlateBrush MaskBrush = MaskImage->GetBrush();
    MaskBrush.ImageSize = FVector2D(MaskWidth, MaskHeight);
    MaskImage->SetBrush(MaskBrush);
    Mask->AddChild(MaskImage);
    auto* MaskSlot = Canvas->AddChildToCanvas(Mask); MaskSlot->SetAnchors(FAnchors(0, 0, 1, 1)); MaskSlot->SetOffsets(FMargin(0));
    Mask->SetVisibility(ESlateVisibility::Collapsed);
    auto AddStatusIcon = [&](FName Field, UTexture2D* Texture)
    {
        check(Texture);
        auto* Icon = Hud->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Field); Icon->bIsVariable = true;
        Icon->SetBrushFromTexture(Texture, false);
        FSlateBrush Brush = Icon->GetBrush();
        Brush.ImageSize = FVector2D(Texture->Source.GetSizeX(), Texture->Source.GetSizeY()); Icon->SetBrush(Brush);
        Icon->SetVisibility(ESlateVisibility::Collapsed);
        auto* IconSlot = Canvas->AddChildToCanvas(Icon); IconSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        IconSlot->SetAlignment(FVector2D(0.5f, 0.5f)); IconSlot->SetPosition(FVector2D(0.0f, EnergyHud::StatusOffsetY));
        IconSlot->SetAutoSize(true);
    };
    AddStatusIcon(EnergyHud::StatusCharging, EnergyHud::ChargingTexture);
    AddStatusIcon(EnergyHud::StatusOffline, EnergyHud::OfflineTexture);
    AddStatusIcon(EnergyHud::StatusReady, EnergyHud::ReadyTexture);
    auto* StatusBlinkAnimation = NewObject<UWidgetAnimation>(Hud,
        EnergyHud::StatusBlinkAnimation, RF_Transactional);
    StatusBlinkAnimation->MovieScene = NewObject<UMovieScene>(
        StatusBlinkAnimation, EnergyHud::StatusBlinkAnimation,
        RF_Transactional);
    UMovieScene* StatusBlinkScene = StatusBlinkAnimation->MovieScene;
    StatusBlinkScene->SetDisplayRate(FFrameRate(
        EnergyHud::BlinkDisplayFramesPerSecond, 1));
    const FFrameRate StatusBlinkTickResolution =
        StatusBlinkScene->GetTickResolution();
    const FFrameNumber StatusBlinkDurationTick =
        (EnergyHud::BlinkDurationSeconds *
            StatusBlinkTickResolution).RoundToFrame();
    const FFrameNumber StatusBlinkHiddenTick =
        (EnergyHud::BlinkHiddenTimeSeconds *
            StatusBlinkTickResolution).RoundToFrame();
    check(StatusBlinkHiddenTick > FFrameNumber(0) &&
        StatusBlinkDurationTick > StatusBlinkHiddenTick);
    StatusBlinkScene->SetPlaybackRange(FFrameNumber(0),
        StatusBlinkDurationTick.Value);
    const FGuid ChargingBinding = StatusBlinkScene->AddPossessable(
        EnergyHud::StatusCharging.ToString(), UImage::StaticClass());
    FWidgetAnimationBinding WidgetBinding;
    WidgetBinding.WidgetName = EnergyHud::StatusCharging;
    WidgetBinding.AnimationGuid = ChargingBinding;
    WidgetBinding.bIsRootWidget = false;
    StatusBlinkAnimation->AnimationBindings.Add(WidgetBinding);
    auto* OpacityTrack = StatusBlinkScene->AddTrack<UMovieSceneFloatTrack>(
        ChargingBinding);
    check(OpacityTrack);
    OpacityTrack->SetPropertyNameAndPath(EnergyHud::RenderOpacityProperty,
        EnergyHud::RenderOpacityProperty.ToString());
    auto* OpacitySection = CastChecked<UMovieSceneFloatSection>(
        OpacityTrack->CreateNewSection());
    OpacitySection->SetRange(TRange<FFrameNumber>(FFrameNumber(0),
        StatusBlinkDurationTick));
    OpacitySection->GetChannel().AddConstantKey(FFrameNumber(0),
        EnergyHud::StatusVisibleOpacity);
    OpacitySection->GetChannel().AddConstantKey(
        StatusBlinkHiddenTick,
        EnergyHud::StatusHiddenOpacity);
    OpacityTrack->AddSection(*OpacitySection);
    Hud->Animations.Add(StatusBlinkAnimation);
    auto* ChargeRadial = Hud->WidgetTree->ConstructWidget<URadialSlider>(URadialSlider::StaticClass(), EnergyHud::ChargeRadial);
    ChargeRadial->bIsVariable = true;
    ChargeRadial->Value = 0.0f;
    ChargeRadial->WidgetStyle.BarThickness = EnergyHud::ChargeGaugeBarThickness;
    ChargeRadial->SliderBarColor = EnergyHud::ChargeGaugeBarColor;
    ChargeRadial->SliderProgressColor = EnergyHud::ChargeGaugeProgressColor;
    ChargeRadial->ShowSliderHandle = false;
    ChargeRadial->ShowSliderHand = false;
    ChargeRadial->Locked = true;
    ChargeRadial->IsFocusable = false;
    ChargeRadial->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* ChargeRadialSlot = Canvas->AddChildToCanvas(ChargeRadial);
    ChargeRadialSlot->SetAnchors(FAnchors(1.0f, 1.0f));
    ChargeRadialSlot->SetAlignment(FVector2D(1.0f, 1.0f));
    ChargeRadialSlot->SetPosition(FVector2D(-EnergyHud::ChargeGaugeMargin, -EnergyHud::ChargeGaugeMargin));
    ChargeRadialSlot->SetSize(FVector2D(EnergyHud::ChargeGaugeSize, EnergyHud::ChargeGaugeSize));
    check(EnergyHud::AmmoIndicatorTexture);
    const float TextureWidth = EnergyHud::AmmoIndicatorTexture->Source.GetSizeX();
    const float TextureHeight = EnergyHud::AmmoIndicatorTexture->Source.GetSizeY();
    check(TextureWidth > EnergyHud::AmmoCropRight &&
        TextureHeight > EnergyHud::AmmoCropBottom);
    const float CropWidth = EnergyHud::AmmoCropRight - EnergyHud::AmmoCropLeft;
    const float CropHeight = EnergyHud::AmmoCropBottom - EnergyHud::AmmoCropTop;
    const FVector2D IndicatorSize(
        EnergyHud::AmmoIndicatorHeight * CropWidth / CropHeight,
        EnergyHud::AmmoIndicatorHeight);
    const FBox2f IndicatorUv(
        FVector2f(EnergyHud::AmmoCropLeft / TextureWidth,
            EnergyHud::AmmoCropTop / TextureHeight),
        FVector2f(EnergyHud::AmmoCropRight / TextureWidth,
            EnergyHud::AmmoCropBottom / TextureHeight));
    auto* ChargeBlock = Hud->WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), EnergyHud::ChargeBlock);
    ChargeBlock->bIsVariable = true;
    ChargeBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* AmmoRow = Hud->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), EnergyHud::AmmoIndicatorRow);
    for (FName Field : EnergyHud::AmmoIndicators)
    {
        auto* Indicator = Hud->WidgetTree->ConstructWidget<UImage>(
            UImage::StaticClass(), Field);
        Indicator->bIsVariable = true;
        Indicator->SetBrushFromTexture(EnergyHud::AmmoIndicatorTexture, false);
        FSlateBrush Brush = Indicator->GetBrush();
        Brush.ImageSize = IndicatorSize;
        Brush.SetUVRegion(IndicatorUv);
        Indicator->SetBrush(Brush);
        Indicator->SetColorAndOpacity(EnergyHud::ChargeGaugeBarColor);
        Indicator->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* IndicatorSlot = AmmoRow->AddChildToHorizontalBox(Indicator);
        IndicatorSlot->SetPadding(FMargin(
            EnergyHud::AmmoIndicatorGap * 0.5f, 0.0f));
        IndicatorSlot->SetHorizontalAlignment(HAlign_Center);
        IndicatorSlot->SetVerticalAlignment(VAlign_Center);
    }
    auto* AmmoRowSlot = ChargeBlock->AddChildToVerticalBox(AmmoRow);
    AmmoRowSlot->SetPadding(FMargin(
        0.0f, 0.0f, 0.0f, EnergyHud::AmmoIndicatorTextGap));
    AmmoRowSlot->SetHorizontalAlignment(HAlign_Center);
    AmmoRowSlot->SetVerticalAlignment(VAlign_Center);
    auto* ChargeText = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), EnergyHud::ChargeText);
    ChargeText->bIsVariable = true;
    ChargeText->SetText(FText::FromString(EnergyHud::EmptyChargeDisplay));
    ChargeText->SetJustification(ETextJustify::Center);
    auto ChargeFont = ChargeText->GetFont(); ChargeFont.Size = EnergyHud::ChargeGaugeFontSize; ChargeText->SetFont(ChargeFont);
    ChargeText->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* ChargeTextSlot = ChargeBlock->AddChildToVerticalBox(ChargeText);
    ChargeTextSlot->SetHorizontalAlignment(HAlign_Center);
    ChargeTextSlot->SetVerticalAlignment(VAlign_Center);
    auto* ChargeBlockSlot = Canvas->AddChildToCanvas(ChargeBlock);
    ChargeBlockSlot->SetAnchors(FAnchors(1.0f, 1.0f));
    ChargeBlockSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    ChargeBlockSlot->SetPosition(FVector2D(
        EnergyHud::ChargeGaugeCenterOffset,
        EnergyHud::ChargeGaugeCenterOffset));
    ChargeBlockSlot->SetAutoSize(true);
    const FVector2D ScopeIndicatorSize(
        EnergyHud::ScopeAmmoIndicatorHeight * CropWidth / CropHeight,
        EnergyHud::ScopeAmmoIndicatorHeight);
    auto* ScopeAmmoRow = Hud->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), EnergyHud::ScopeAmmoIndicatorRow);
    ScopeAmmoRow->bIsVariable = true;
    ScopeAmmoRow->SetVisibility(ESlateVisibility::Collapsed);
    for (FName Field : EnergyHud::ScopeAmmoIndicators)
    {
        auto* Indicator = Hud->WidgetTree->ConstructWidget<UImage>(
            UImage::StaticClass(), Field);
        Indicator->bIsVariable = true;
        Indicator->SetBrushFromTexture(EnergyHud::AmmoIndicatorTexture, false);
        FSlateBrush Brush = Indicator->GetBrush();
        Brush.ImageSize = ScopeIndicatorSize;
        Brush.SetUVRegion(IndicatorUv);
        Indicator->SetBrush(Brush);
        Indicator->SetColorAndOpacity(EnergyHud::ScopeAmmoInactiveTint);
        Indicator->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* IndicatorSlot = ScopeAmmoRow->AddChildToHorizontalBox(Indicator);
        IndicatorSlot->SetPadding(FMargin(
            EnergyHud::ScopeAmmoIndicatorGap * 0.5f, 0.0f));
        IndicatorSlot->SetHorizontalAlignment(HAlign_Center);
        IndicatorSlot->SetVerticalAlignment(VAlign_Center);
    }
    auto* ScopeAmmoRowSlot = Canvas->AddChildToCanvas(ScopeAmmoRow);
    ScopeAmmoRowSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    ScopeAmmoRowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    ScopeAmmoRowSlot->SetPosition(FVector2D(
        0.0f, EnergyHud::ScopeAmmoIndicatorOffsetY));
    ScopeAmmoRowSlot->SetAutoSize(true);
    auto AddScopeText = [&](FName Field, const TCHAR* Text, float Offset, bool Variable)
    {
        auto* Widget = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Field); Widget->bIsVariable = Variable;
        Widget->SetText(FText::FromString(Text)); Widget->SetJustification(ETextJustify::Center);
        auto Font = Widget->GetFont(); Font.Size = H::TargetFontSize; Widget->SetFont(Font);
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Layout = Canvas->AddChildToCanvas(Widget); Layout->SetAnchors(FAnchors(0.5f, 0.5f));
        Layout->SetAlignment(FVector2D(0.5f, 0.5f)); Layout->SetPosition(FVector2D(0, Offset)); Layout->SetAutoSize(true);
    };
    AddScopeText(Range::TargetName, N::EmptyText, 0.0f, true);
    AddScopeText(Range::TargetRange, N::EmptyText, 0.0f, true);
    AddScopeText(ZoomTest::WideCenter, ZoomTest::WideCenterText, 0.0f, true);
    auto* Host = Hud->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Hint::Root); Host->bIsVariable = true;
    auto* HostSlot = Canvas->AddChildToCanvas(Host); HostSlot->SetAnchors(FAnchors(0.0f, 1.0f));
    HostSlot->SetAlignment(FVector2D(0.0f, 1.0f)); HostSlot->SetPosition(DS::HintHostOffset); HostSlot->SetAutoSize(true);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud); FKismetEditorUtilities::CompileBlueprint(Hud);
    check(Hud->Status != BS_Error);
    UBlueprint* RailgunModule = LoadObject<UBlueprint>(
        nullptr, RailgunInventoryShared::ModuleObjectPath);
    check(RailgunModule && RailgunModule->GeneratedClass);
    auto AddHudFunction = [&](FName FunctionName, auto BuildBody)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(Hud,
            FunctionName, UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(Hud, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        FGraph Function(Graph, nullptr);
        Function.Tail = Function.Pin(Entry, P::Then);
        Function.Branch(Function.Valid(Function.Read(EnergyHud::Station)));
        BuildBody(Function, Function.Read(EnergyHud::Station));
    };
    auto AddStatusFunction = [&](FName FunctionName, bool HasStatusParameter,
        auto BuildBody)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(Hud,
            FunctionName, UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(Hud, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        UEdGraphPin* StatusParameter = nullptr;
        if (HasStatusParameter)
        {
            FEdGraphPinType StatusType;
            StatusType.PinCategory = UEdGraphSchema_K2::PC_Int;
            StatusParameter = Entry->CreateUserDefinedPin(
                EnergyHud::NewStatusParameter, StatusType, EGPD_Output);
            check(StatusParameter);
        }
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        FGraph Function(Graph, nullptr);
        Function.Tail = Function.Pin(Entry, P::Then);
        BuildBody(Function, StatusParameter);
    };
    auto StopStatusAnimation = [&](FGraph& Function)
    {
        auto* Stop = Function.Call(UUserWidget::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UUserWidget, StopAnimation));
        Function.Link(OpticalSelf(Function),
            Function.Pin(Stop, P::FunctionTarget));
        Function.Link(Function.Read(EnergyHud::StatusBlinkAnimation),
            Function.Pin(Stop, EnergyHud::AnimationPin));
        Function.Exec(Stop);
        auto* RestoreOpacity = Function.Call(UWidget::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UWidget, SetRenderOpacity));
        Function.Link(Function.Read(EnergyHud::StatusCharging),
            Function.Pin(RestoreOpacity, P::FunctionTarget));
        Function.Default(RestoreOpacity, Settings::OpacityPin,
            *FString::SanitizeFloat(EnergyHud::StatusVisibleOpacity));
        Function.Exec(RestoreOpacity);
    };
    auto SetStatusVisibility = [&](FGraph& Function, FName VisibleStatus)
    {
        for (FName Field : {EnergyHud::StatusCharging,
            EnergyHud::StatusOffline, EnergyHud::StatusReady})
        {
            auto* Set = Function.Call(UWidget::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
            Function.Link(Function.Read(Field),
                Function.Pin(Set, P::FunctionTarget));
            Function.Default(Set, OP::Visibility,
                Field == VisibleStatus ? EnergyHud::Shown : EnergyHud::Hidden);
            Function.Exec(Set);
        }
    };
    AddStatusFunction(EnergyHud::ResetStatusFunction, false,
        [&](FGraph& Function, UEdGraphPin*)
        {
            StopStatusAnimation(Function);
            SetStatusVisibility(Function, NAME_None);
            Function.Write(EnergyHud::StatusState, nullptr,
                EnergyHud::StatusHiddenState);
            Function.Write(EnergyHud::StatusInitialized, nullptr, N::False);
        });
    AddStatusFunction(EnergyHud::ApplyStatusFunction, true,
        [&](FGraph& Function, UEdGraphPin* NewStatus)
        {
            UEdGraphPin* SameStatus = Function.Binary(
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                    EqualEqual_IntInt), Function.Read(EnergyHud::StatusState),
                NewStatus);
            UEdGraphPin* InitializedAndSame = Function.Binary(
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
                Function.Read(EnergyHud::StatusInitialized), SameStatus);
            auto* Skip = Function.Branch(InitializedAndSame);
            UEdGraphPin* UnchangedTail = Function.Tail;
            Function.Tail = Function.Pin(Skip, P::Else);
            StopStatusAnimation(Function);
            SetStatusVisibility(Function, NAME_None);
            Function.Write(EnergyHud::StatusState, NewStatus);
            Function.Write(EnergyHud::StatusInitialized, nullptr, N::True);

            auto ApplyVisibleState = [&](const TCHAR* State, FName Widget)
            {
                auto* Matches = Function.Branch(Function.Compare(
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                        EqualEqual_IntInt), NewStatus, State));
                auto* Set = Function.Call(UWidget::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
                Function.Link(Function.Read(Widget),
                    Function.Pin(Set, P::FunctionTarget));
                Function.Default(Set, OP::Visibility, EnergyHud::Shown);
                Function.Exec(Set);
                UEdGraphPin* MatchTail = Function.Tail;
                Function.Tail = Function.Pin(Matches, P::Else);
                return MatchTail;
            };
            UEdGraphPin* OfflineTail = ApplyVisibleState(
                EnergyHud::StatusOfflineState, EnergyHud::StatusOffline);
            UEdGraphPin* ReadyTail = ApplyVisibleState(
                EnergyHud::StatusReadyState, EnergyHud::StatusReady);
            auto* Charging = Function.Branch(Function.Compare(
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                    EqualEqual_IntInt), NewStatus,
                EnergyHud::StatusChargingState));
            auto* ShowCharging = Function.Call(UWidget::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
            Function.Link(Function.Read(EnergyHud::StatusCharging),
                Function.Pin(ShowCharging, P::FunctionTarget));
            Function.Default(ShowCharging, OP::Visibility, EnergyHud::Shown);
            Function.Exec(ShowCharging);
            auto* Play = Function.Call(UUserWidget::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UUserWidget, PlayAnimation));
            Function.Link(OpticalSelf(Function),
                Function.Pin(Play, P::FunctionTarget));
            Function.Link(Function.Read(EnergyHud::StatusBlinkAnimation),
                Function.Pin(Play, EnergyHud::AnimationPin));
            Function.Default(Play, EnergyHud::AnimationStartTimePin,
                EnergyHud::AnimationStartTime);
            Function.Default(Play, EnergyHud::AnimationLoopCountPin,
                EnergyHud::AnimationLoopForever);
            Function.Default(Play, EnergyHud::AnimationPlayModePin,
                EnergyHud::AnimationForward);
            Function.Default(Play, EnergyHud::AnimationPlaybackSpeedPin,
                EnergyHud::AnimationPlaybackSpeed);
            Function.Default(Play, EnergyHud::AnimationRestoreStatePin,
                N::False);
            Function.Exec(Play);
            UEdGraphPin* ChargingTail = Function.Tail;
            Function.Tail = Function.Pin(Charging, P::Else);
            StationMerge(Function, {UnchangedTail, OfflineTail, ReadyTail,
                ChargingTail, Function.Tail});
        });
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud);
    FKismetEditorUtilities::CompileBlueprint(Hud);
    check(Hud->Status != BS_Error);
    AddHudFunction(EnergyHud::RefreshEnergyFunction,
        [&](FGraph& Function, UEdGraphPin* Station)
        {
            UpdateStationEnergyHud(Function, Station, BP->GeneratedClass);
            UpdateStationStatusHud(Function, Station, BP->GeneratedClass,
                Hud->GeneratedClass, ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, ZoomTest::Wide));
        });
    AddHudFunction(EnergyHud::RefreshAmmoFunction,
        [&](FGraph& Function, UEdGraphPin* Station)
        {
            UpdateStationAmmoHud(Function, Station, BP->GeneratedClass,
                RailgunModule->GeneratedClass);
        });
    AddHudFunction(EnergyHud::RefreshStyleFunction,
        [&](FGraph& Function, UEdGraphPin* Station)
        {
            auto ApplyTextStyle = [&](FName WidgetField, FName Opacity,
                FName FontSize, FName FontObject, FName Typeface)
            {
                auto* NormalizedOpacity = Function.Call(
                    UKismetMathLibrary::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                        Multiply_DoubleDouble));
                Function.Link(ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, Opacity), Function.Pin(
                        NormalizedOpacity, P::Binary::LeftOperand));
                Function.Default(NormalizedOpacity,
                    P::Binary::RightOperand, Settings::PercentMultiplier);
                auto* SetOpacity = Function.Call(UWidget::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UWidget, SetRenderOpacity));
                Function.Link(Function.Read(WidgetField),
                    Function.Pin(SetOpacity, P::FunctionTarget));
                Function.Link(Function.Pin(NormalizedOpacity,
                    P::ReturnValue), Function.Pin(SetOpacity,
                        Settings::OpacityPin));
                Function.Exec(SetOpacity);

                auto* SizeValue = ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, FontSize);
                auto* SetSize = Function.Call(UTextBlock::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UTextBlock, SetFontSize));
                Function.Link(Function.Read(WidgetField),
                    Function.Pin(SetSize, P::FunctionTarget));
                Function.Link(SizeValue, Function.Pin(SetSize,
                    Settings::DisplayFontSizePin));
                Function.Exec(SetSize);

                auto* FontValue = ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, FontObject);
                auto* HasFont = Function.Branch(Function.Valid(FontValue));
                auto* NoFont = Function.Pin(HasFont, P::Else);
                auto* TypefaceName = Function.Call(
                    UKismetStringLibrary::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary,
                        Conv_StringToName));
                Function.Link(ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, Typeface), Function.Pin(TypefaceName,
                        TextSettingsGraphNames::NumericString));
                auto* FontInfo = Function.Call(
                    USlateFontInfoBlueprintLibrary::StaticClass(),
                    Settings::MakeSlateFontInfoFunction);
                Function.Link(FontValue, Function.Pin(FontInfo,
                    Settings::FontObjectPin));
                Function.Link(Function.Pin(TypefaceName, P::ReturnValue),
                    Function.Pin(FontInfo, Settings::TypefaceFontNamePin));
                Function.Link(SizeValue, Function.Pin(FontInfo,
                    Settings::FontSizePin));
                auto* SetFont = Function.Call(UTextBlock::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UTextBlock, SetFont));
                Function.Link(Function.Read(WidgetField),
                    Function.Pin(SetFont, P::FunctionTarget));
                Function.Link(Function.Pin(FontInfo, P::ReturnValue),
                    Function.Pin(SetFont, Settings::FontInfoPin));
                Function.Exec(SetFont);
                StationMerge(Function, {Function.Tail, NoFont});
            };
            auto ApplyTargetStyle = [&](FName WidgetField, FName OffsetX,
                FName OffsetY, FName Opacity, FName FontSize,
                FName FontObject, FName Typeface)
            {
                auto* Position = Function.Call(
                    UKismetMathLibrary::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                        MakeVector2D));
                Function.Link(ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, OffsetX), Function.Pin(Position,
                        Settings::XPin));
                Function.Link(ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, OffsetY), Function.Pin(Position,
                        Settings::YPin));
                auto* Translation = Function.Call(UWidget::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UWidget,
                        SetRenderTranslation));
                Function.Link(Function.Read(WidgetField),
                    Function.Pin(Translation, P::FunctionTarget));
                Function.Link(Function.Pin(Position, P::ReturnValue),
                    Function.Pin(Translation, Settings::TranslationPin));
                Function.Exec(Translation);
                ApplyTextStyle(WidgetField, Opacity, FontSize, FontObject,
                    Typeface);
            };
            ApplyTextStyle(EnergyHud::ChargeText,
                Settings::ChargeTextOpacity, Settings::ChargeTextFontSize,
                Settings::ChargeTextFontObject,
                Settings::ChargeTextTypeface);
            ApplyTargetStyle(Range::TargetName,
                Settings::TargetNameOffsetX, Settings::TargetNameOffsetY,
                Settings::TargetNameOpacity, Settings::TargetNameFontSize,
                Settings::TargetNameFontObject,
                Settings::TargetNameTypeface);
            ApplyTargetStyle(Range::TargetRange,
                Settings::TargetDistanceOffsetX,
                Settings::TargetDistanceOffsetY,
                Settings::TargetDistanceOpacity,
                Settings::TargetDistanceFontSize,
                Settings::TargetDistanceFontObject,
                Settings::TargetDistanceTypeface);
            auto* NormalizedStatusOpacity = Function.Call(
                UKismetMathLibrary::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                    Multiply_DoubleDouble));
            Function.Link(ReadNativeInputField(Function, Station,
                BP->GeneratedClass, Settings::StatusIconOpacity),
                Function.Pin(NormalizedStatusOpacity,
                    P::Binary::LeftOperand));
            Function.Default(NormalizedStatusOpacity,
                P::Binary::RightOperand, Settings::PercentMultiplier);
            for (FName Field : {EnergyHud::StatusCharging,
                EnergyHud::StatusOffline, EnergyHud::StatusReady})
            {
                auto* Set = Function.Call(UImage::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UImage, SetOpacity));
                Function.Link(Function.Read(Field),
                    Function.Pin(Set, P::FunctionTarget));
                Function.Link(Function.Pin(NormalizedStatusOpacity,
                    P::ReturnValue), Function.Pin(Set,
                        Settings::OpacityPin));
                Function.Exec(Set);
            }
        });
    AddHudFunction(EnergyHud::RefreshModeFunction,
        [&](FGraph& Function, UEdGraphPin* Station)
        {
            auto* WideHud = Function.Branch(ReadNativeInputField(Function,
                Station, BP->GeneratedClass, ZoomTest::Wide));
            auto SetOpticalVisibility = [&](const TCHAR* Visibility)
            {
                for (FName Field : {ZoomTest::Mask, Range::TargetName,
                    Range::TargetRange, EnergyHud::ScopeAmmoIndicatorRow})
                {
                    auto* Set = Function.Call(UWidget::StaticClass(),
                        GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
                    Function.Link(Function.Read(Field),
                        Function.Pin(Set, P::FunctionTarget));
                    Function.Default(Set, OP::Visibility, Visibility);
                    Function.Exec(Set);
                }
            };
            auto SetWideCenter = [&](const TCHAR* Visibility)
            {
                auto* Set = Function.Call(UWidget::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
                Function.Link(Function.Read(ZoomTest::WideCenter),
                    Function.Pin(Set, P::FunctionTarget));
                Function.Default(Set, OP::Visibility, Visibility);
                Function.Exec(Set);
            };
            auto SetWideChargeVisibility = [&](const TCHAR* Visibility)
            {
                for (FName Field : {EnergyHud::ChargeRadial,
                    EnergyHud::ChargeBlock})
                {
                    auto* Set = Function.Call(UWidget::StaticClass(),
                        GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
                    Function.Link(Function.Read(Field),
                        Function.Pin(Set, P::FunctionTarget));
                    Function.Default(Set, OP::Visibility, Visibility);
                    Function.Exec(Set);
                }
            };
            SetOpticalVisibility(ZoomTest::Hidden);
            SetWideCenter(ZoomTest::Shown);
            SetWideChargeVisibility(ZoomTest::Shown);
            auto* WideTail = Function.Tail;
            Function.Tail = Function.Pin(WideHud, P::Else);
            SetOpticalVisibility(ZoomTest::Shown);
            SetWideCenter(ZoomTest::Hidden);
            SetWideChargeVisibility(ZoomTest::Hidden);
            StationMerge(Function, {WideTail, Function.Tail});
            UpdateStationStatusHud(Function, Station, BP->GeneratedClass,
                Hud->GeneratedClass, ReadNativeInputField(Function, Station,
                    BP->GeneratedClass, ZoomTest::Wide));
        });
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud);
    FKismetEditorUtilities::CompileBlueprint(Hud);
    check(Hud->Status != BS_Error);

    auto AddStationHudFunction = [&](FName FunctionName,
        FName HudFunctionName)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
            FunctionName, UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        FGraph Function(Graph, nullptr);
        Function.Tail = Function.Pin(Entry, P::Then);
        Function.Branch(Function.Valid(Function.Read(EnergyHud::ActiveHud)));
        auto* CastHud = NewObject<UK2Node_DynamicCast>(Graph);
        CastHud->TargetType = Hud->GeneratedClass;
        CastHud->SetPurity(false);
        Function.Node(CastHud);
        Function.Link(Function.Tail, Function.Pin(CastHud, P::Execute));
        Function.Link(Function.Read(EnergyHud::ActiveHud),
            CastHud->GetCastSourcePin());
        Function.Tail = CastHud->GetValidCastPin();
        auto* Refresh = Function.Call(Hud->GeneratedClass,
            HudFunctionName);
        Function.Link(CastHud->GetCastResultPin(),
            Function.Pin(Refresh, P::FunctionTarget));
        Function.Exec(Refresh);
    };
    auto AddHudBindingFunction = [&](FName FunctionName, bool Register)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
            FunctionName, UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        FEdGraphPinType HudType;
        HudType.PinCategory = UEdGraphSchema_K2::PC_Object;
        HudType.PinSubCategoryObject = Hud->GeneratedClass;
        UEdGraphPin* HudParameter = Entry->CreateUserDefinedPin(
            EnergyHud::HudParameter, HudType, EGPD_Output);
        check(HudParameter);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        FGraph Function(Graph, nullptr);
        Function.Tail = Function.Pin(Entry, P::Then);
        if (Register)
        {
            Function.Write(EnergyHud::ActiveHud, HudParameter);
        }
        else
        {
            Function.Branch(Function.Binary(
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                    EqualEqual_ObjectObject),
                Function.Read(EnergyHud::ActiveHud), HudParameter));
            auto* Reset = Function.Call(Hud->GeneratedClass,
                EnergyHud::ResetStatusFunction);
            Function.Link(HudParameter,
                Function.Pin(Reset, P::FunctionTarget));
            Function.Exec(Reset);
            Function.Write(EnergyHud::ActiveHud, nullptr);
        }
    };
    AddHudBindingFunction(EnergyHud::RegisterFunction, true);
    AddHudBindingFunction(EnergyHud::UnregisterFunction, false);
    AddStationHudFunction(EnergyHud::RefreshEnergyFunction,
        EnergyHud::RefreshEnergyFunction);
    AddStationHudFunction(EnergyHud::RefreshAmmoFunction,
        EnergyHud::RefreshAmmoFunction);
    AddStationHudFunction(EnergyHud::RefreshStyleFunction,
        EnergyHud::RefreshStyleFunction);
    AddStationHudFunction(EnergyHud::RefreshModeFunction,
        EnergyHud::RefreshModeFunction);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error);

    UEdGraph* HudGraph = Hud->UbergraphPages[0];
    const auto Defaults = HudGraph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph HG(HudGraph, nullptr);
    auto RefreshRangeDisplay = [&](UEdGraphPin* BoundStation, bool Force)
    {
        const TPair<FName, FName> DisplayFields[] = {
            {Range::TargetName, Range::DisplayedTargetName},
            {Range::TargetRange, Range::DisplayedTargetRange}
        };
        for (const auto& DisplayField : DisplayFields)
        {
            const FName StationField = DisplayField.Key;
            const FName CachedField = DisplayField.Value;
            auto* Value = ReadNativeInputField(HG, BoundStation,
                BP->GeneratedClass, StationField);
            UK2Node_IfThenElse* Skip = nullptr;
            if (!Force)
            {
                UEdGraphPin* Same = EqualRangeText(HG,
                    HG.Read(CachedField), Value);
                UEdGraphPin* InitializedAndSame = HG.Binary(
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                        BooleanAND),
                    HG.Read(Range::DisplayInitialized), Same);
                Skip = HG.Branch(InitializedAndSame);
                HG.Tail = HG.Pin(Skip, P::Else);
            }
            auto* Set = HG.Call(UTextBlock::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
            HG.Link(HG.Read(StationField),
                HG.Pin(Set, P::FunctionTarget));
            HG.Link(Value, HG.Pin(Set, E::WidgetText));
            HG.Exec(Set);
            HG.Write(CachedField, Value);
            if (Skip)
                StationMerge(HG, {HG.Tail, HG.Pin(Skip, P::Then)});
        }
        if (Force)
        {
            HG.Write(Range::DisplayInitialized, nullptr, N::True);
        }
        else
        {
            auto* AlreadyInitialized = HG.Branch(
                HG.Read(Range::DisplayInitialized));
            HG.Tail = HG.Pin(AlreadyInitialized, P::Else);
            HG.Write(Range::DisplayInitialized, nullptr, N::True);
            StationMerge(HG, {HG.Tail,
                HG.Pin(AlreadyInitialized, P::Then)});
        }
    };
    auto* HudTick = NewObject<UK2Node_Event>(HudGraph);
    HudTick->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::WidgetTick,
        UUserWidget::StaticClass());
    HudTick->bOverrideFunction = true;
    HG.Node(HudTick);
    HG.Tail = HG.Pin(HudTick, P::Then);
    UEdGraphPin* Station = HG.Read(EnergyHud::Station);
    HG.Branch(HG.Valid(Station));
    RefreshRangeDisplay(Station, false);

    auto* Construct = NewObject<UK2Node_Event>(HudGraph);
    Construct->EventReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UUserWidget, Construct),
        UUserWidget::StaticClass());
    Construct->bOverrideFunction = true;
    HG.Node(Construct);
    auto* ConstructWork = HG.Node(
        NewObject<UK2Node_ExecutionSequence>(HudGraph));
    HG.Link(HG.Pin(Construct, P::Then),
        HG.Pin(ConstructWork, P::Execute));
    HG.Tail = ConstructWork->GetThenPinGivenIndex(0);
    auto* ResetOnConstruct = HG.Call(Hud->GeneratedClass,
        EnergyHud::ResetStatusFunction);
    HG.Exec(ResetOnConstruct);
    auto* OldStationValid = HG.Branch(
        HG.Valid(HG.Read(EnergyHud::Station)));
    auto* UnregisterOld = HG.Call(BP->GeneratedClass,
        EnergyHud::UnregisterFunction);
    HG.Link(HG.Read(EnergyHud::Station),
        HG.Pin(UnregisterOld, P::FunctionTarget));
    HG.Link(OpticalSelf(HG), HG.Pin(UnregisterOld,
        EnergyHud::HudParameter));
    HG.Exec(UnregisterOld);
    StationMerge(HG, {HG.Tail, HG.Pin(OldStationValid, P::Else)});
    HG.Write(EnergyHud::Station, nullptr);
    auto* OwningPawn = HG.Call(UUserWidget::StaticClass(),
        EnergyHud::OwningPlayerPawnGetter);
    auto* OwningStation = NewObject<UK2Node_DynamicCast>(HudGraph);
    OwningStation->TargetType = BP->GeneratedClass;
    OwningStation->SetPurity(false);
    HG.Node(OwningStation);
    HG.Link(HG.Tail, HG.Pin(OwningStation, P::Execute));
    HG.Link(HG.Pin(OwningPawn, P::ReturnValue),
        OwningStation->GetCastSourcePin());
    HG.Tail = OwningStation->GetValidCastPin();
    HG.Write(EnergyHud::Station, OwningStation->GetCastResultPin());
    auto* RegisterHud = HG.Call(BP->GeneratedClass,
        EnergyHud::RegisterFunction);
    HG.Link(OwningStation->GetCastResultPin(),
        HG.Pin(RegisterHud, P::FunctionTarget));
    HG.Link(OpticalSelf(HG), HG.Pin(RegisterHud,
        EnergyHud::HudParameter));
    HG.Exec(RegisterHud);
    for (FName RefreshFunction : {EnergyHud::RefreshStyleFunction,
        EnergyHud::RefreshModeFunction, EnergyHud::RefreshAmmoFunction,
        EnergyHud::RefreshEnergyFunction})
    {
        auto* Refresh = HG.Call(Hud->GeneratedClass, RefreshFunction);
        HG.Exec(Refresh);
    }
    RefreshRangeDisplay(OwningStation->GetCastResultPin(), true);
    AddStationHintConstruction(Hud,
        ConstructWork->GetThenPinGivenIndex(1));

    auto* Destruct = NewObject<UK2Node_Event>(HudGraph);
    Destruct->EventReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UUserWidget, Destruct),
        UUserWidget::StaticClass());
    Destruct->bOverrideFunction = true;
    HG.Node(Destruct);
    HG.Tail = HG.Pin(Destruct, P::Then);
    auto* ResetOnDestruct = HG.Call(Hud->GeneratedClass,
        EnergyHud::ResetStatusFunction);
    HG.Exec(ResetOnDestruct);
    auto* DestructStationValid = HG.Branch(
        HG.Valid(HG.Read(EnergyHud::Station)));
    auto* Unregister = HG.Call(BP->GeneratedClass,
        EnergyHud::UnregisterFunction);
    HG.Link(HG.Read(EnergyHud::Station),
        HG.Pin(Unregister, P::FunctionTarget));
    HG.Link(OpticalSelf(HG), HG.Pin(Unregister,
        EnergyHud::HudParameter));
    HG.Exec(Unregister);
    StationMerge(HG, {HG.Tail, HG.Pin(DestructStationValid, P::Else)});
    HG.Write(EnergyHud::Station, nullptr);
    HG.Write(Range::DisplayInitialized, nullptr, N::False);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud); FKismetEditorUtilities::CompileBlueprint(Hud);
    check(Hud->Status != BS_Error);
    AddStationActions(BP);
    AddContextEntry(BP);
    AddNativeStationHudInterface(BP, Hud->GeneratedClass);
    AddRailgunEnergyFunctions(BP, Settings::OfflineDischarge);
    AddRailgunChargeIndicatorFunctions(BP);
    AddStationLifecycleFunctions(BP);
    BuildDedicatedStationGraph(BP);
    AddRailgunEnergyTeardown(BP);
    AddRailgunChargeIndicatorTeardown(BP);
    AddStationLifecycleTeardown(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP); check(BP->Status != BS_Error);
    auto* CDO = CastChecked<APawn>(BP->GeneratedClass->GetDefaultObject());
    check(CDO->GetRootComponent() && CDO->GetRootComponent()->IsA<UVoyageFastSceneComponent>());
    check(BP->SimpleConstructionScript->GetAllNodes().Num() == 2);
    check(Interaction->GetChildNodes().Num() == 1 && Interaction->GetChildNodes()[0] == Query);
    check(Interaction->bIsParentComponentNative && Interaction->ParentComponentOrVariableName == CDO->GetRootComponent()->GetFName());
    CDO->PrimaryActorTick.bCanEverTick = true;
    CDO->PrimaryActorTick.bStartWithTickEnabled = false;
    CDO->PrimaryActorTick.TickInterval = DS::ContinuousTickInterval;
    CDO->PrimaryActorTick.TickGroup = TG_PostPhysics;
    CDO->AutoPossessPlayer = EAutoReceiveInput::Disabled; CDO->AutoPossessAI = EAutoPossessAI::Disabled;
    CDO->bUseControllerRotationYaw = false; CDO->bUseControllerRotationPitch = false; CDO->bUseControllerRotationRoll = false;
    check(SaveDedicatedAsset(BP)); check(SaveDedicatedAsset(Hud)); return BP->GeneratedClass;
}
