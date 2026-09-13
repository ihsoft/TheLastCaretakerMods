#pragma once

// Included after HelmObservationProbe for its Engine getter helpers, not its graph.
namespace StationProbeNames
{
inline const FName Held(TEXT("OwnsStationFixation"));
inline const FName Controller(TEXT("StationController"));
inline const FName Movement(TEXT("StationMovement"));
inline const FName Root(TEXT("StationCharacterRoot"));
inline const FName Anchor(TEXT("StationAnchor"));
inline const FName CollisionBefore(TEXT("StationCollisionBefore"));
inline const FName RotationBefore(TEXT("StationRotationBefore"));
inline const FName EntryLocal(TEXT("StationEntryLocal"));
inline const FName AnchorStart(TEXT("StationAnchorStart"));
inline constexpr TCHAR ClassPath[] = TEXT("/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New.BP_Module_WindTurbine_Medium_New_C");
inline constexpr TCHAR RootName[] = TEXT("ModuleMountCollision");
inline constexpr TCHAR Key[] = TEXT("F8");
inline constexpr TCHAR Limit[] = TEXT("20.0");
inline constexpr TCHAR Range[] = TEXT("300.0");
inline constexpr TCHAR Walking[] = TEXT("MOVE_Walking");
inline constexpr TCHAR NoneMode[] = TEXT("MOVE_None");
inline constexpr TCHAR WalkingByte[] = TEXT("1");
inline constexpr float TickInterval = 0.0f;
inline const FVector2D PanelSize(730.0f, 230.0f);
inline constexpr TCHAR Title[] = TEXT("HC11 CANNON FIXATION - NO DRONE / NO OPTICS");
inline constexpr TCHAR Ready[] = TEXT("HC18 Native HUD | Aim at cannon base and press F8");
inline constexpr TCHAR Active[] = TEXT("FIXED for max20s: look / try walk; F8 RELEASES");
inline constexpr TCHAR Released[] = TEXT("RELEASED: check walking. F8 can repeat.");
inline constexpr TCHAR Refused[] = TEXT("REFUSED: stand normally on deck; no other control/lock");
inline constexpr TCHAR WrongTarget[] = TEXT("REFUSED: aim within3m at attached, BUILT Harpoon base");
inline constexpr TCHAR Changed[] = TEXT("STOP: ownership/anchor changed; release requested");
inline constexpr TCHAR Failed[] = TEXT("ATTACH FAILED: entry rolled back; send screenshot");
inline constexpr TCHAR Seconds[] = TEXT("Fixed seconds (max20): ");
inline constexpr TCHAR Drift[] = TEXT("Current cannon-local drift (cm): ");
inline constexpr TCHAR Travel[] = TEXT("Anchor world travel (cm): ");
inline constexpr TCHAR Attached[] = TEXT("Character attached to cannon: YES");
inline constexpr TCHAR Detached[] = TEXT("Character attached to cannon: NO");
inline constexpr TCHAR Instructions[] = TEXT("No helm / loot / dismantle / save during fixation. Exit first.");
}
namespace S = StationProbeNames;
namespace SP = CharacterStationGraphNames;

UEdGraphPin* StationMode(FGraph& G)
{
    auto* Node = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName Name = GET_MEMBER_NAME_CHECKED(UCharacterMovementComponent, MovementMode);
    Node->VariableReference.SetExternalMember(Name, UCharacterMovementComponent::StaticClass()); G.Node(Node);
    G.Link(G.Read(S::Movement), G.Pin(Node, P::FunctionTarget)); return G.Pin(Node, Name);
}

UEdGraphPin* StationParent(FGraph& G)
{
    return ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), G.Read(S::Root));
}

void StationSetMode(FGraph& G, const TCHAR* Mode)
{
    auto* Call = G.Call(UCharacterMovementComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCharacterMovementComponent, SetMovementMode));
    G.Link(G.Read(S::Movement), G.Pin(Call, P::FunctionTarget)); G.Default(Call, SP::NewMovementMode, Mode); G.Exec(Call);
}

void StationCollision(FGraph& G, bool Restore)
{
    auto* Call = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Call, P::FunctionTarget));
    if (Restore) G.Link(G.Read(S::CollisionBefore), G.Pin(Call, SP::CollisionEnabled));
    else G.Default(Call, SP::CollisionEnabled, N::False);
    G.Exec(Call);
}

void StationMerge(FGraph& G, const TArray<UEdGraphPin*>& Paths)
{
    auto* Join = G.Node(NewObject<UK2Node_IfThenElse>(G.Graph)); G.Default(Join, P::Condition, N::True);
    for (UEdGraphPin* Path : Paths) G.Link(Path, G.Pin(Join, P::Execute));
    G.Tail = G.Pin(Join, P::Then);
}

#include "StationOpticsProbe.h"

void ReleaseStation(FGraph& G)
{
    ReleaseStationControl(G);
    ReleaseOptics(G);
    auto* Held = G.Branch(G.Read(S::Held)); G.Write(S::Held, nullptr, N::False);
    auto* CharacterValid = G.Branch(G.Valid(G.Read(N::OriginalPawn)));
    auto* RootValid = G.Branch(G.Valid(G.Read(S::Root)));
    auto* Ours = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), StationParent(G), G.Read(S::Anchor)));
    auto* Detach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DetachFromActor));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Detach, P::FunctionTarget));
    G.Default(Detach, E::LocationRule, N::KeepWorld); G.Default(Detach, E::RotationRule, N::KeepWorld);
    G.Default(Detach, E::ScaleRule, N::KeepWorld); G.Exec(Detach);
    StationMerge(G, {G.Tail, G.Pin(Ours, P::Else)});
    // Do not detach a replacement owner's parent or force its movement mode.
    auto* Unparented = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Valid(StationParent(G)), N::False));
    auto* CurrentRotation = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorRotation), G.Read(N::OriginalPawn));
    auto* CurrentParts = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakRotator));
    G.Link(CurrentRotation, G.Pin(CurrentParts, SP::RotationValue));
    auto* OldParts = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakRotator));
    G.Link(G.Read(S::RotationBefore), G.Pin(OldParts, SP::RotationValue));
    auto* Rotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Pin(CurrentParts, SP::Yaw), G.Pin(Rotation, SP::Yaw));
    G.Link(G.Pin(OldParts, SP::Pitch), G.Pin(Rotation, SP::Pitch)); G.Link(G.Pin(OldParts, SP::Roll), G.Pin(Rotation, SP::Roll));
    auto* SetRotation = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRotation));
    G.Link(G.Read(N::OriginalPawn), G.Pin(SetRotation, P::FunctionTarget));
    G.Link(G.Pin(Rotation, P::ReturnValue), G.Pin(SetRotation, SP::NewRotation)); G.Default(SetRotation, SP::TeleportPhysics, N::False); G.Exec(SetRotation);
    auto* FreeTail = G.Tail;
    // Restore our collision flag on both paths. External takeover is unsupported;
    // do not silently leave a character non-colliding after relinquishing our gate.
    G.Tail = G.Pin(Unparented, P::Else); StationCollision(G, true); auto* ExternalTail = G.Tail;
    G.Tail = FreeTail; StationCollision(G, true);
    auto* MovementValid = G.Branch(G.Valid(G.Read(S::Movement)));
    auto* StillNone = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), N::Zero));
    StationSetMode(G, S::Walking); // Entry admits Walking only; native floor reacquisition.
    StationMerge(G, {G.Tail, G.Pin(StillNone, P::Else), G.Pin(MovementValid, P::Else), ExternalTail,
        G.Pin(RootValid, P::Else), G.Pin(CharacterValid, P::Else), G.Pin(Held, P::Else)});
}

void BuildStationGraph(UBlueprint* BP, UClass* HudClass)
{
    UEdGraph* Graph = BP->UbergraphPages[0]; const auto Defaults = Graph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph); Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Controller = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    auto* Character = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCharacter));
    // Release service runs before idle current-character/HUD early returns.
    auto* Held = G.Branch(G.Read(S::Held));
    auto* Retry = G.Branch(G.Read(Control::ReturnPending));
    auto* RetryKey = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Branch(G.Valid(G.Read(S::Controller)));
    G.Link(G.Read(S::Controller), G.Pin(RetryKey, P::FunctionTarget)); G.Default(RetryKey, P::Key, S::Key);
    G.Branch(G.Pin(RetryKey, P::ReturnValue)); ReleaseStation(G); G.Text(N::FreezeStatus, S::Released);
    G.Tail = G.Pin(Retry, P::Else);
    auto* OwnerValid = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), G.Valid(G.Read(N::OriginalPawn)), G.Valid(G.Read(S::Root))),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), G.Valid(G.Read(S::Movement)), G.Valid(G.Read(S::Anchor)))));
    auto* OwnerTail = G.Tail; G.Tail = G.Pin(OwnerValid, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, S::Changed); G.Tail = OwnerTail;
    auto* Same = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), StationControlledPawn(G), OpticalSelf(G)),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), G.Pin(Controller, P::ReturnValue), G.Read(S::Controller))));
    auto* SameTail = G.Tail; G.Tail = G.Pin(Same, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, S::Changed); G.Tail = SameTail;
    auto* Expected = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), StationParent(G), G.Read(S::Anchor)),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), N::Zero)));
    auto* ExpectedTail = G.Tail; G.Tail = G.Pin(Expected, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, S::Changed); G.Tail = ExpectedTail;
    G.Write(N::Age, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(N::Age), G.Pin(Tick, P::DeltaSeconds)));
    auto* Key = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Pin(Controller, P::ReturnValue), G.Pin(Key, P::FunctionTarget)); G.Default(Key, P::Key, S::Key);
    auto* Done = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), G.Pin(Key, P::ReturnValue),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Read(N::Age), S::Limit)));
    ReleaseStation(G); G.Text(N::FreezeStatus, S::Released);
    G.Tail = G.Pin(Done, P::Else); G.Text(N::FreezeStatus, S::Active);
    G.Number(N::Clock, S::Seconds, G.Read(N::Age));
    auto* Position = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), G.Read(N::OriginalPawn));
    auto* Transform = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentToWorld), G.Read(S::Anchor));
    auto* Local = ObserveLocalPosition(G, Position, Transform);
    auto* Drift = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), Local, G.Read(S::EntryLocal)), G.Pin(Drift, E::VectorLengthInput));
    G.Number(N::Drift, S::Drift, G.Pin(Drift, P::ReturnValue));
    auto* AnchorWorld = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), G.Read(S::Anchor));
    auto* Travel = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), AnchorWorld, G.Read(S::AnchorStart)), G.Pin(Travel, E::VectorLengthInput));
    G.Number(N::Control, S::Travel, G.Pin(Travel, P::ReturnValue)); G.Text(N::Current, S::Attached);
    UpdateOptics(G);

    G.Tail = G.Pin(Held, P::Else); G.Branch(G.Valid(G.Pin(Controller, P::ReturnValue)));
    auto* HudReady = G.Branch(G.Valid(G.Read(N::HudInstance))); auto* ReadyTail = G.Tail; G.Tail = G.Pin(HudReady, P::Else);
    auto* Create = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Pin(Create, E::WidgetType)->DefaultObject = HudClass; G.Link(G.Pin(Controller, P::ReturnValue), G.Pin(Create, E::OwningPlayer)); G.Exec(Create);
    auto* CastHud = NewObject<UK2Node_DynamicCast>(Graph); CastHud->TargetType = HudClass; CastHud->SetPurity(false); G.Node(CastHud);
    G.Link(G.Tail, G.Pin(CastHud, P::Execute)); G.Link(G.Pin(Create, P::ReturnValue), CastHud->GetCastSourcePin());
    G.Tail = CastHud->GetValidCastPin(); G.Write(N::HudInstance, CastHud->GetCastResultPin());
    auto* Add = G.Call(UUserWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport));
    G.Link(G.Read(N::HudInstance), G.Pin(Add, P::FunctionTarget)); G.Default(Add, E::ZOrder, N::HudZOrder); G.Exec(Add);
    G.Tail = ReadyTail; G.Text(N::Current, S::Detached);
    auto* Start = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Pin(Controller, P::ReturnValue), G.Pin(Start, P::FunctionTarget)); G.Default(Start, P::Key, S::Key); G.Branch(G.Pin(Start, P::ReturnValue));
    G.Require(G.Valid(G.Pin(Character, P::ReturnValue)), S::Refused);
    G.Write(N::OriginalPawn, G.Pin(Character, P::ReturnValue)); G.Write(S::Controller, G.Pin(Controller, P::ReturnValue));
    auto* MoveIgnored = ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, IsMoveInputIgnored), G.Read(S::Controller));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), MoveIgnored, N::False), S::Refused);
    auto* Movement = ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, GetMovementComponent), G.Read(N::OriginalPawn));
    auto* MoveCast = NewObject<UK2Node_DynamicCast>(Graph); MoveCast->TargetType = UCharacterMovementComponent::StaticClass(); MoveCast->SetPurity(false); G.Node(MoveCast);
    G.Link(G.Tail, G.Pin(MoveCast, P::Execute)); G.Link(Movement, MoveCast->GetCastSourcePin()); G.Tail = MoveCast->GetValidCastPin();
    G.Write(S::Movement, MoveCast->GetCastResultPin());
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), S::WalkingByte), S::Refused);
    auto* Base = ObserveCall(G, UCharacterMovementComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCharacterMovementComponent, GetMovementBaseObject), G.Read(S::Movement));
    G.Require(G.Valid(Base), S::Refused);
    auto* Root = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), G.Read(N::OriginalPawn));
    auto* RootCast = NewObject<UK2Node_DynamicCast>(Graph); RootCast->TargetType = UPrimitiveComponent::StaticClass(); RootCast->SetPurity(false); G.Node(RootCast);
    G.Link(G.Tail, G.Pin(RootCast, P::Execute)); G.Link(Root, RootCast->GetCastSourcePin()); G.Tail = RootCast->GetValidCastPin(); G.Write(S::Root, RootCast->GetCastResultPin());
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Valid(StationParent(G)), N::False), S::Refused);
    auto* Physics = ObserveCall(G, UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, IsSimulatingPhysics), G.Read(S::Root));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), Physics, N::False), S::Refused);
    auto* Camera = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Require(G.Valid(G.Pin(Camera, P::ReturnValue)), S::Refused);
    auto* CameraLocation = ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraLocation), G.Pin(Camera, P::ReturnValue));
    auto* CameraRotation = ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraRotation), G.Pin(Camera, P::ReturnValue));
    auto* Forward = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GetForwardVector)); G.Link(CameraRotation, G.Pin(Forward, SP::RotationValue));
    auto* Ray = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat));
    G.Link(G.Pin(Forward, P::ReturnValue), G.Pin(Ray, P::Binary::LeftOperand)); G.Default(Ray, P::Binary::RightOperand, S::Range);
    auto* Trace = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LineTraceSingle));
    G.Link(CameraLocation, G.Pin(Trace, E::TraceStart));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector), CameraLocation, G.Pin(Ray, P::ReturnValue)), G.Pin(Trace, E::TraceEnd));
    G.Link(G.ActorArray(G.Read(N::OriginalPawn), G.Read(N::OriginalPawn)), G.Pin(Trace, E::ActorsToIgnore));
    G.Default(Trace, E::TraceChannel, N::VisibilityTrace); G.Default(Trace, E::TraceComplex, N::False); G.Default(Trace, E::IgnoreSelf, N::True); G.Exec(Trace);
    G.Require(G.Pin(Trace, P::ReturnValue), S::WrongTarget);
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult)); G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit));
    G.Require(G.Valid(G.Pin(Hit, SP::HitActor)), S::WrongTarget);
    auto* Class = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetObjectClass)); G.Link(G.Pin(Hit, SP::HitActor), G.Pin(Class, P::Object));
    auto* SoftPath = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetSoftClassPath));
    G.Link(G.Pin(Class, P::ReturnValue), G.Pin(SoftPath, P::Class));
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, BreakSoftClassPath));
    G.Link(G.Pin(SoftPath, P::ReturnValue), G.Pin(Path, SP::SoftClassPathInput));
    auto* EqualClass = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(Path, E::PathString), G.Pin(EqualClass, P::Binary::LeftOperand)); G.Default(EqualClass, P::Binary::RightOperand, S::ClassPath); G.Require(G.Pin(EqualClass, P::ReturnValue), S::WrongTarget);
    auto* TargetRoot = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), G.Pin(Hit, SP::HitActor));
    G.Require(G.Valid(TargetRoot), S::WrongTarget); G.Write(S::Anchor, TargetRoot);
    auto* RootName = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName)); G.Link(G.Read(S::Anchor), G.Pin(RootName, P::Object));
    auto* EqualRoot = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(RootName, P::ReturnValue), G.Pin(EqualRoot, P::Binary::LeftOperand)); G.Default(EqualRoot, P::Binary::RightOperand, S::RootName); G.Require(G.Pin(EqualRoot, P::ReturnValue), S::WrongTarget);
    auto* AnchorParent = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), G.Read(S::Anchor));
    G.Require(G.Valid(AnchorParent), S::WrongTarget);
    G.Write(S::CollisionBefore, ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetActorEnableCollision), G.Read(N::OriginalPawn)));
    G.Write(S::RotationBefore, ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorRotation), G.Read(N::OriginalPawn)));
    G.Write(S::EntryLocal, Local); G.Write(S::AnchorStart, AnchorWorld); G.Write(N::Age, nullptr, N::Zero);
    PrepareOptics(G);
    G.Write(S::Held, nullptr, N::True); // Cleanup owns every subsequent partial-entry mutation.
    StationCollision(G, false); StationSetMode(G, S::NoneMode);
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Attach, P::FunctionTarget)); G.Link(G.Read(S::Anchor), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld); G.Default(Attach, E::ScaleRule, N::KeepWorld);
    G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach);
    auto* Attached = G.Branch(G.Pin(Attach, P::ReturnValue)); BeginOptics(G);
    G.Tail = G.Pin(Attached, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, S::Failed);

    auto* End = NewObject<UK2Node_Event>(Graph); End->EventReference.SetExternalMember(E::EndPlayEvent, AActor::StaticClass()); End->bOverrideFunction = true; G.Node(End);
    G.Tail = G.Pin(End, P::Then); ReleaseStation(G); G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* Remove = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, RemoveFromParent)); G.Link(G.Read(N::HudInstance), G.Pin(Remove, P::FunctionTarget)); G.Exec(Remove);
}
