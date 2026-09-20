#pragma once
#include "../../../HarpoonModelContract.h"
#include "StationSettings.h"
#include "StationEnergyHud.h"
namespace ZoomTest
{
inline const FName Wide(TEXT("HarpoonWideView"));
inline const FName Label(TEXT("HarpoonZoomLabel"));
inline const FName Mouse(TEXT("HarpoonActiveMousePercent"));
inline const FName Mask(TEXT("HarpoonOpticalMask"));
inline const FName MaskImage(TEXT("HarpoonOpticalMaskImage"));
inline constexpr TCHAR MaskPackage[] = TEXT("/Game/Mods/HarpoonCannon/Station/T_HarpoonOpticalMask");
inline constexpr TCHAR WideLabel[] = TEXT("x1");
inline constexpr TCHAR NarrowLabel[] = TEXT("x5");
inline constexpr TCHAR NormalMouse[] = TEXT("100.0");
inline constexpr TCHAR Hidden[] = TEXT("Collapsed");
inline constexpr TCHAR Shown[] = TEXT("HitTestInvisible");
inline const FName WideCenter(TEXT("HarpoonWideCenter"));
inline constexpr TCHAR WideCenterText[] = TEXT("\u25CB");
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

void DedicatedViewTarget(FGraph& G, UEdGraphPin* Target)
{
    auto* Call = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, SetViewTargetWithBlend));
    G.Link(G.Read(DS::Controller), G.Pin(Call, P::FunctionTarget));
    G.Link(Target, G.Pin(Call, OP::NewViewTarget)); G.Default(Call, OP::BlendTime, N::Zero); G.Exec(Call);
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void AimModelPivot(FGraph& G, FName Tag, FName Angle, FName Axis)
{
    G.Branch(G.Valid(G.Read(S::Anchor)));
    [[maybe_unused]] constexpr auto OwnerSignature = static_cast<AActor* (UActorComponent::*)() const>(&UActorComponent::GetOwner);
    auto* Owner = ObserveCall(G, UActorComponent::StaticClass(), ActorScanGraphNames::GetActorOwner, G.Read(S::Anchor));
    auto* Find = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(Owner, G.Pin(Find, P::FunctionTarget));
    G.Pin(Find, OP::ComponentClass)->DefaultObject = USceneComponent::StaticClass();
    G.Default(Find, ActorScanGraphNames::ComponentTag, *Tag.ToString());
    auto* Loop = ContextLoop(G, G.Pin(Find, P::ReturnValue));
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = USceneComponent::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Loop, CE::ArrayElement), Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    auto* Rotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Read(Angle), G.Pin(Rotation, Axis));
    auto* Set = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetRelativeRotation));
    G.Link(Cast->GetCastResultPin(), G.Pin(Set, P::FunctionTarget));
    G.Link(G.Pin(Rotation, P::ReturnValue), G.Pin(Set, SP::NewRotation));
    G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
    G.Tail = G.Pin(Loop, CE::Completed);
}

void DedicatedAim(FGraph& G)
{
    // Camera is a child of the descriptor's sight under pitch. It inherits
    // model motion once, without a second camera-local yaw/pitch rotation.
    // Visual articulation only: never rotate the physical mount, station or
    // character. Missing visual roles must not break entry/camera/exit.
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(G.Graph));
    G.Link(G.Tail, G.Pin(Work, P::Execute)); G.Tail = Work->GetThenPinGivenIndex(0);
    AimModelPivot(G, HarpoonModelContract::YawTag, Aim::Yaw, SP::Yaw);
    AimModelPivot(G, HarpoonModelContract::PitchTag, Aim::Pitch, SP::Pitch);
    G.Tail = Work->GetThenPinGivenIndex(1);
}

#include "ParallaxProbe.h"

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
    auto* SafetyAndEnergy = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(G.Tail, G.Pin(SafetyAndEnergy, P::Execute)); G.Tail = SafetyAndEnergy->GetThenPinGivenIndex(0);
    ContextStationSafety(G);
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
    ReadStationSettings(G);
    G.Write(ZoomTest::Wide, nullptr, N::True);
    G.Write(ZoomTest::Label, nullptr, ZoomTest::WideLabel);
    G.Write(ZoomTest::Mouse, nullptr, ZoomTest::NormalMouse);
    auto* FindSight = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor)), G.Pin(FindSight, P::FunctionTarget));
    G.Pin(FindSight, OP::ComponentClass)->DefaultObject = USceneComponent::StaticClass();
    G.Default(FindSight, ActorScanGraphNames::ComponentTag, *HarpoonModelContract::SightTag.ToString());
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
    DedicatedAim(G); PlaceModeCamera(G, true); DedicatedViewTarget(G, G.Read(DS::Camera)); G.Write(DS::ViewOwned, nullptr, N::True);

    // Independent sequence: misses/classification failures cannot block view setup or exit.
    G.Tail = OccupiedWork->GetThenPinGivenIndex(1);
    auto* OpticalWork = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(G.Tail, G.Pin(OpticalWork, P::Execute)); G.Tail = OpticalWork->GetThenPinGivenIndex(0);
    G.Branch(G.Read(DS::ViewOwned)); G.Branch(G.Valid(G.Read(O::Camera)));
    G.Branch(G.Valid(G.Read(N::OriginalPawn))); G.Branch(G.Valid(G.Read(S::Anchor)));
    auto* EyeMode = G.Branch(G.Read(ZoomTest::Wide));
    UpdateEyeCameraPosition(G); ConvergeEyeAim(G); StationMerge(G, {G.Tail, G.Pin(EyeMode, P::Else)});
    UpdateStationRange(G, true);

    // Action event, not polling: one toggle on RMB Started while possessed.
    auto* ZoomInput = LoadObject<UInputAction>(nullptr, HarpoonInputNames::Zoom); check(ZoomInput);
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
        G.Write(ZoomTest::Label, nullptr, ToWide ? ZoomTest::WideLabel : ZoomTest::NarrowLabel);
        G.Write(ZoomTest::Mouse, ToWide ? nullptr : G.Read(Settings::Mouse), ToWide ? ZoomTest::NormalMouse : nullptr);
        auto* Set = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
        G.Link(G.Read(O::Camera), G.Pin(Set, P::FunctionTarget));
        G.Link(G.Read(ToWide ? O::BaselineFov : O::RequestedFov), G.Pin(Set, OP::FieldOfView)); G.Exec(Set);
    };
    SetZoom(false);
    G.Tail = G.Pin(Wide, P::Else); SetZoom(true);

    AddCannonFire(G, G.Pin(Tick, P::DeltaSeconds), SafetyAndEnergy->GetThenPinGivenIndex(1));
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
    AddVariable(BP, Charge::Sampled, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Charge::Module, UEdGraphSchema_K2::PC_Object, UVoyageModuleComponent::StaticClass());
    for (FName Field : {Charge::Energy, Charge::Previous, Charge::Rate}) AddVariable(BP, Field, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, ZoomTest::Wide, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, EyeAim::Yaw, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, EyeAim::Pitch, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, EyeAim::Target, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    AddVariable(BP, ZoomTest::Mouse, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, ZoomTest::Label, UEdGraphSchema_K2::PC_Text);
    for (FName Field : {Settings::Mouse, Settings::Yaw, Settings::PitchMin, Settings::PitchMax, ShotAudio::VolumePercent}) AddVariable(BP, Field, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, DS::Sight, UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    for (FName Field : {CE::Ready, CE::InteractBlocks, CE::ProviderSeen, CE::CallbackSeen}) AddVariable(BP, Field, UEdGraphSchema_K2::PC_Boolean);
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
    // Authored alpha mask, 4:1 texture scaled uniformly to fill the viewport.
    // For ordinary landscape aspect ratios up to4:1 the aperture stays circular.
    constexpr int32 MaskHeight = 512, MaskWidth = MaskHeight * 4;
    constexpr float ApertureRadius = 0.42f, FeatherWidth = 0.045f;
    TArray<FColor> Pixels; Pixels.SetNumUninitialized(MaskWidth * MaskHeight);
    for (int32 Y = 0; Y < MaskHeight; ++Y) for (int32 X = 0; X < MaskWidth; ++X)
    {
        const float DX = (X + 0.5f - MaskWidth * 0.5f) / MaskHeight;
        const float DY = (Y + 0.5f - MaskHeight * 0.5f) / MaskHeight;
        const float T = FMath::Clamp((FMath::Sqrt(DX * DX + DY * DY) - ApertureRadius) / FeatherWidth, 0.0f, 1.0f);
        Pixels[Y * MaskWidth + X] = FColor(0, 0, 0, FMath::RoundToInt(255.0f * T * T * (3.0f - 2.0f * T)));
    }
    auto* MaskTexture = NewObject<UTexture2D>(CreatePackage(ZoomTest::MaskPackage), *FPackageName::GetLongPackageAssetName(ZoomTest::MaskPackage), RF_Public | RF_Standalone);
    MaskTexture->Source.Init(MaskWidth, MaskHeight, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
    MaskTexture->CompressionSettings = TC_EditorIcon; MaskTexture->MipGenSettings = TMGS_NoMipmaps;
    MaskTexture->NeverStream = true; MaskTexture->UpdateResource(); check(SaveDedicatedAsset(MaskTexture));
    auto* Mask = Hud->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), ZoomTest::Mask); Mask->bIsVariable = true;
    Mask->SetStretch(EStretch::ScaleToFill); Mask->SetClipping(EWidgetClipping::ClipToBounds);
    auto* MaskImage = Hud->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), ZoomTest::MaskImage);
    MaskImage->SetBrushFromTexture(MaskTexture, false);
    // Newly authored texture has no platform dimensions until cooked. Do not
    // derive the Slate desired size from GetSizeX/Y at this stage (returns0).
    FSlateBrush MaskBrush = MaskImage->GetBrush();
    MaskBrush.ImageSize = FVector2D(MaskWidth, MaskHeight);
    MaskImage->SetBrush(MaskBrush);
    Mask->AddChild(MaskImage);
    auto* MaskSlot = Canvas->AddChildToCanvas(Mask); MaskSlot->SetAnchors(FAnchors(0, 0, 1, 1)); MaskSlot->SetOffsets(FMargin(0));
    Mask->SetVisibility(ESlateVisibility::Collapsed);
    auto* Label = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ZoomTest::Label); Label->bIsVariable = true;
    Label->SetText(FText::FromString(ZoomTest::NarrowLabel)); Label->SetJustification(ETextJustify::Center);
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
    auto AddDiagnosticText = [&](FName Field, const TCHAR* Text, float Offset)
    {
        auto* Widget = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Field); Widget->bIsVariable = true;
        Widget->SetText(FText::FromString(Text)); Widget->SetJustification(ETextJustify::Left); Widget->SetFont(Font);
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Layout = Canvas->AddChildToCanvas(Widget); Layout->SetAnchors(FAnchors(0.0f, 0.0f));
        Layout->SetAlignment(FVector2D(0.0f, 0.0f)); Layout->SetPosition(FVector2D(EnergyHud::DiagnosticLeft, Offset)); Layout->SetAutoSize(true);
    };
    AddScopeText(O::Reticle, O::ReticleText, 0.0f, true);
    AddScopeText(Range::TargetName, N::EmptyText, DS::TargetNameOffsetY, true);
    AddScopeText(Range::TargetRange, N::EmptyText, DS::TargetRangeOffsetY, true);
    AddScopeText(ZoomTest::WideCenter, ZoomTest::WideCenterText, 0.0f, true);
    AddDiagnosticText(EnergyHud::Connection, EnergyHud::UnknownConnection, EnergyHud::ConnectionOffset);
    AddDiagnosticText(EnergyHud::Power, EnergyHud::UnknownPower, EnergyHud::PowerOffset);
    AddDiagnosticText(EnergyHud::Progress, EnergyHud::EmptyCharge, EnergyHud::ProgressOffset);
    AddDiagnosticText(EnergyHud::Rate, EnergyHud::EmptyRate, EnergyHud::RateOffset);
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
    for (FName Field : {Range::TargetName, Range::TargetRange, ZoomTest::Label})
    {
        auto* Value = ReadNativeInputField(HG, Station->GetCastResultPin(), BP->GeneratedClass, Field);
        auto* Set = HG.Call(UTextBlock::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
        HG.Link(HG.Read(Field), HG.Pin(Set, P::FunctionTarget)); HG.Link(Value, HG.Pin(Set, E::WidgetText)); HG.Exec(Set);
    }
    auto* WideHud = HG.Branch(ReadNativeInputField(HG, Station->GetCastResultPin(), BP->GeneratedClass, ZoomTest::Wide));
    auto SetOpticalVisibility = [&](const TCHAR* Visibility)
    {
        for (FName Field : {ZoomTest::Mask, O::Reticle, Range::TargetName, Range::TargetRange})
        {
            auto* Set = HG.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
            HG.Link(HG.Read(Field), HG.Pin(Set, P::FunctionTarget)); HG.Default(Set, OP::Visibility, Visibility); HG.Exec(Set);
        }
    };
    auto SetWideCenter = [&](const TCHAR* Visibility)
    {
        auto* Set = HG.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
        HG.Link(HG.Read(ZoomTest::WideCenter), HG.Pin(Set, P::FunctionTarget)); HG.Default(Set, OP::Visibility, Visibility); HG.Exec(Set);
    };
    SetOpticalVisibility(ZoomTest::Hidden); SetWideCenter(ZoomTest::Shown); auto* WideTail = HG.Tail;
    HG.Tail = HG.Pin(WideHud, P::Else); SetOpticalVisibility(ZoomTest::Shown); SetWideCenter(ZoomTest::Hidden);
    StationMerge(HG, {WideTail, HG.Tail});
    UpdateStationEnergyHud(HG, Station->GetCastResultPin(), BP->GeneratedClass);
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
