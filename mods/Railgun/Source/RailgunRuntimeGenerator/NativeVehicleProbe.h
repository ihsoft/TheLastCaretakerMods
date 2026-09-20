#pragma once
#include "AssetLoadingGraphNames.h"

namespace NativeVehicleNames
{
inline const FName Vehicle(TEXT("NativeStation"));
inline const FName Body(TEXT("NativeStationBody"));
inline const FName Occupied(TEXT("NativeEntryObserved"));
inline const FName Attempted(TEXT("NativeSpawnAttempted"));
inline const FName ExitSent(TEXT("NativeExitRequested"));
inline const FName PreparedFlag(TEXT("NativeStationPrepared"));
inline const FName NewPossessor(TEXT("NewPossessor"));
inline constexpr TCHAR Title[] = TEXT("HC19 NATIVE VEHICLE BASE / NO DRIVING / NO OPTICS");
inline constexpr TCHAR Ready[] = TEXT("HC19: aim at railgun base; F8 prepares station, next F8 enters");
inline constexpr TCHAR Prepared[] = TEXT("HC19 PREPARED: F8 enters native vehicle. Do not save.");
inline constexpr TCHAR Active[] = TEXT("HC19 INSIDE: F8 native exit (automatic attempt after20s). Do not save.");
inline constexpr TCHAR Returned[] = TEXT("HC19 RETURNED: check walking/actions; F8 repeats entry");
inline constexpr TCHAR Failed[] = TEXT("HC19 STOP: prerequisite/entry failed. Screenshot, quit without saving.");
inline constexpr TCHAR ExitFailed[] = TEXT("HC19 EXIT REQUESTED: if stuck, F8 retries. Do not save.");
inline constexpr TCHAR PhysicsYes[] = TEXT("Vehicle root simulates: YES - STOP TEST");
inline constexpr TCHAR PhysicsNo[] = TEXT("Vehicle root simulates: NO");
inline constexpr TCHAR ControlYes[] = TEXT("Controller pawn = native station: YES");
inline constexpr TCHAR ControlNo[] = TEXT("Controller pawn = native station: NO");
inline constexpr TCHAR AttachedYes[] = TEXT("Vehicle attached to railgun: YES");
inline constexpr TCHAR AttachedNo[] = TEXT("Vehicle attached to railgun: NO - STOP TEST");
inline constexpr TCHAR PlayerOnly[] = TEXT("Local player walking near a built railgun required");
inline constexpr TCHAR StationOffset[] = TEXT("X=0,Y=0,Z=140");
inline constexpr TCHAR Seconds[] = TEXT("Native control seconds (max20): ");
}
namespace V = NativeVehicleNames;

namespace CombinedVehicleNames
{
inline constexpr TCHAR Title[] = TEXT("HC24 NATIVE FIRST-PERSON REQUEST / x5 / HC23 CONTROLS");
inline constexpr TCHAR Ready[] = TEXT("HC24: aim at railgun base; F8 prepares, next F8 enters");
inline constexpr TCHAR Prepared[] = TEXT("HC24 PREPARED: camera/input references checked. F8 enters. Do not save.");
inline constexpr TCHAR Active[] = TEXT("HC24: check first-person/x5/mouse; E exits, F8 fallback (max20s).");
inline constexpr TCHAR Offset[] = TEXT("X=-170,Y=0,Z=140");
inline constexpr TCHAR OperatorTag[] = TEXT("HC21OperatorPoint");
inline const FName ExitTag = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, ExitComponentTag);
inline const FName ComponentTags = GET_MEMBER_NAME_CHECKED(UActorComponent, ComponentTags);
}

namespace ExitActionNames
{
inline const FName Expected(TEXT("ExpectedStockExitAction"));
inline const FName Field = GET_MEMBER_NAME_CHECKED(AVoyageVehicleForkliftPawn, ExitAction);
inline constexpr TCHAR Path[] = TEXT("/Game/Game/Input/Vehicle/IAV_VehicleExit.IAV_VehicleExit");
inline constexpr TCHAR Failed[] = TEXT("HC23 STOP: stock input reference load/type/readback failed. Entry blocked; screenshot and quit.");
}

UEdGraphPin* RequireExitActionCast(FGraph& G, UEdGraphPin* Object, UClass* Class)
{
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = Class; Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Object, Cast->GetCastSourcePin());
    G.Tail = Cast->GetInvalidCastPin(); G.Text(N::FreezeStatus, ExitActionNames::Failed);
    G.Tail = Cast->GetValidCastPin(); return Cast->GetCastResultPin();
}

UEdGraphPin* LoadStockInputReference(FGraph& G, const TCHAR* ObjectPath, UClass* Class)
{
    // Load stock data in preparation, not inside native possession/action collection.
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString, ObjectPath);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, AssetLoadingGraphNames::SoftObjectPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
    G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Load, AssetLoadingGraphNames::Asset)); G.Exec(Load);
    G.Require(G.Valid(G.Pin(Load, P::ReturnValue)), ExitActionNames::Failed);
    return RequireExitActionCast(G, G.Pin(Load, P::ReturnValue), Class);
}

void LoadStockExitAction(FGraph& G)
{
    G.Write(ExitActionNames::Expected, LoadStockInputReference(G, ExitActionNames::Path, UVoyageInputAction::StaticClass()));
}

void CheckStockExitAction(FGraph& G, bool Assign)
{
    G.Require(G.Valid(G.Read(ExitActionNames::Expected)), ExitActionNames::Failed);
    auto* Target = RequireExitActionCast(G, G.Read(V::Vehicle), AVoyageVehicleForkliftPawn::StaticClass());
    if (Assign)
    {
        auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
        Set->VariableReference.SetExternalMember(ExitActionNames::Field, AVoyageVehicleForkliftPawn::StaticClass());
        G.Node(Set); G.Link(Target, G.Pin(Set, P::FunctionTarget));
        G.Link(G.Read(ExitActionNames::Expected), G.Pin(Set, ExitActionNames::Field)); G.Exec(Set);
    }
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetExternalMember(ExitActionNames::Field, AVoyageVehicleForkliftPawn::StaticClass());
    G.Node(Get); G.Link(Target, G.Pin(Get, P::FunctionTarget));
    auto* Value = G.Pin(Get, ExitActionNames::Field);
    G.Require(G.Valid(Value), ExitActionNames::Failed);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Value, G.Read(ExitActionNames::Expected)), ExitActionNames::Failed);
}

namespace NativeInputNames
{
inline const FName Context(TEXT("ExpectedVehicleInputContext"));
inline const FName LookRight(TEXT("ExpectedVehicleLookRight"));
inline const FName LookUp(TEXT("ExpectedVehicleLookUp"));
inline const FName ControlsField = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, InputControls);
inline const FName ContextField = GET_MEMBER_NAME_CHECKED(UVoyageInputControlsComponent, InputContextAsset);
inline const FName RightField = GET_MEMBER_NAME_CHECKED(AVoyageVehicleForkliftPawn, LookRightInputAction);
inline const FName UpField = GET_MEMBER_NAME_CHECKED(AVoyageVehicleForkliftPawn, LookUpInputAction);
inline constexpr TCHAR ContextPath[] = TEXT("/Game/Game/Input/Vehicle/DA_Input_Context_Forklift.DA_Input_Context_Forklift");
inline constexpr TCHAR RightPath[] = TEXT("/Game/Game/Input/Character/IA_LookRight.IA_LookRight");
inline constexpr TCHAR UpPath[] = TEXT("/Game/Game/Input/Character/IA_LookUp.IA_LookUp");
}

void LoadNativeLookInputs(FGraph& G)
{
    G.Write(NativeInputNames::Context, LoadStockInputReference(G, NativeInputNames::ContextPath, UVoyageInputContextAsset::StaticClass()));
    G.Write(NativeInputNames::LookRight, LoadStockInputReference(G, NativeInputNames::RightPath, UInputAction::StaticClass()));
    G.Write(NativeInputNames::LookUp, LoadStockInputReference(G, NativeInputNames::UpPath, UInputAction::StaticClass()));
}

UEdGraphPin* ReadNativeInputField(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field)
{
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetExternalMember(Field, Owner); G.Node(Get);
    G.Link(Target, G.Pin(Get, P::FunctionTarget)); return G.Pin(Get, Field);
}

void NativeInputFieldGuard(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field, FName Expected, bool Assign)
{
    G.Require(G.Valid(G.Read(Expected)), ExitActionNames::Failed);
    if (Assign)
    {
        auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
        Set->VariableReference.SetExternalMember(Field, Owner); G.Node(Set);
        G.Link(Target, G.Pin(Set, P::FunctionTarget)); G.Link(G.Read(Expected), G.Pin(Set, Field)); G.Exec(Set);
    }
    auto* Value = ReadNativeInputField(G, Target, Owner, Field);
    G.Require(G.Valid(Value), ExitActionNames::Failed);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Value, G.Read(Expected)), ExitActionNames::Failed);
}

void CheckNativeLookInputs(FGraph& G, bool Assign)
{
    auto* Controls = ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeInputNames::ControlsField);
    G.Require(G.Valid(Controls), ExitActionNames::Failed);
    NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, Assign);
    auto* Forklift = RequireExitActionCast(G, G.Read(V::Vehicle), AVoyageVehicleForkliftPawn::StaticClass());
    NativeInputFieldGuard(G, Forklift, AVoyageVehicleForkliftPawn::StaticClass(), NativeInputNames::RightField, NativeInputNames::LookRight, Assign);
    NativeInputFieldGuard(G, Forklift, AVoyageVehicleForkliftPawn::StaticClass(), NativeInputNames::UpField, NativeInputNames::LookUp, Assign);
}

#include "NativeVehicleCameraProbe.h"

void ConfigureCombinedOperatorPoint(FGraph& G)
{
    // This new actor's nonphysical root is the operator point, behind the
    // railgun at character height. Preserve the shell and native entry/exit.
    auto* Tags = NewObject<UK2Node_VariableSet>(G.Graph);
    Tags->VariableReference.SetExternalMember(CombinedVehicleNames::ComponentTags, UActorComponent::StaticClass());
    G.Node(Tags); G.Link(G.Read(V::Body), G.Pin(Tags, P::FunctionTarget));
    auto* Values = G.Node(NewObject<UK2Node_MakeArray>(G.Graph));
    G.Link(Values->GetOutputPin(), G.Pin(Tags, CombinedVehicleNames::ComponentTags));
    G.Default(Values, Values->GetPinName(0), CombinedVehicleNames::OperatorTag); G.Exec(Tags);
    auto* Tag = NewObject<UK2Node_VariableSet>(G.Graph);
    Tag->VariableReference.SetExternalMember(CombinedVehicleNames::ExitTag, AVoyageVehiclePawn::StaticClass());
    G.Node(Tag); G.Link(G.Read(V::Vehicle), G.Pin(Tag, P::FunctionTarget));
    G.Default(Tag, CombinedVehicleNames::ExitTag, CombinedVehicleNames::OperatorTag); G.Exec(Tag);
    auto* HasTag = G.Call(UActorComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UActorComponent, ComponentHasTag));
    G.Link(G.Read(V::Body), G.Pin(HasTag, P::FunctionTarget));
    // Engine tag argument is its own semantic identity, not an opaque operand.
    const FName TagArgument(TEXT("Tag"));
    G.Default(HasTag, TagArgument, CombinedVehicleNames::OperatorTag);
    G.Require(G.Pin(HasTag, P::ReturnValue), V::Failed);
}

UEdGraphPin* NativeControlled(FGraph& G)
{
    return ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), G.Read(S::Controller));
}

void NativeExit(FGraph& G)
{
    // No manual Possess, character transform, movement, collision or HUD repair.
    G.Write(V::ExitSent, nullptr, N::True);
    auto* Call = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle));
    G.Link(G.Read(V::Vehicle), G.Pin(Call, P::FunctionTarget)); G.Exec(Call);
    G.Text(N::FreezeStatus, V::ExitFailed);
}

void BuildNativeVehicleGraph(UBlueprint* BP, UClass* HudClass, bool CombinedCandidate = false, UClass* DedicatedClass = nullptr)
{
    UEdGraph* Graph = BP->UbergraphPages[0]; const auto Defaults = Graph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Controller = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    G.Branch(G.Valid(G.Pin(Controller, P::ReturnValue)));
    auto* HasController = G.Branch(G.Valid(G.Read(S::Controller))); auto* OwnerTail = G.Tail;
    G.Tail = G.Pin(HasController, P::Else); G.Write(S::Controller, G.Pin(Controller, P::ReturnValue));
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
    // End this first tick; services below always have a valid diagnostic widget.
    G.Tail = ReadyTail;
    if (CombinedCandidate) G.Text(N::Title, CombinedVehicleNames::Title);
    if (DedicatedClass) G.Text(N::Title, DedicatedStationNames::Title);
    auto* Key = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, WasInputKeyJustPressed));
    G.Link(G.Read(S::Controller), G.Pin(Key, P::FunctionTarget)); G.Default(Key, P::Key, S::Key);
    auto* Exists = G.Branch(G.Valid(G.Read(V::Vehicle))); auto* ExistingTail = G.Tail;

    // Initial preparation, explicitly only once. No native character changes.
    G.Tail = G.Pin(Exists, P::Else); G.Branch(G.Pin(Key, P::ReturnValue));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Read(V::Attempted), N::False), V::Failed);
    auto* Character = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCharacter));
    G.Require(G.Valid(G.Pin(Character, P::ReturnValue)), V::PlayerOnly);
    G.Write(N::OriginalPawn, G.Pin(Character, P::ReturnValue));
    auto* Movement = ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, GetMovementComponent), G.Read(N::OriginalPawn));
    auto* MoveCast = NewObject<UK2Node_DynamicCast>(Graph); MoveCast->TargetType = UCharacterMovementComponent::StaticClass(); MoveCast->SetPurity(false); G.Node(MoveCast);
    G.Link(G.Tail, G.Pin(MoveCast, P::Execute)); G.Link(Movement, MoveCast->GetCastSourcePin()); G.Tail = MoveCast->GetValidCastPin(); G.Write(S::Movement, MoveCast->GetCastResultPin());
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), S::WalkingByte), V::PlayerOnly);
    G.Require(ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, HasAuthority), G.Read(S::Controller)), V::PlayerOnly);
    G.Require(ObserveCall(G, APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, IsLocalController), G.Read(S::Controller)), V::PlayerOnly);
    auto* Camera = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Require(G.Valid(G.Pin(Camera, P::ReturnValue)), V::Failed);
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
    G.Require(G.Pin(Trace, P::ReturnValue), S::WrongTarget);
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult)); G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit));
    auto* Actor = G.Pin(Hit, SP::HitActor); G.Require(G.Valid(Actor), S::WrongTarget);
    auto* Class = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetObjectClass)); G.Link(Actor, G.Pin(Class, P::Object));
    auto* SoftPath = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetSoftClassPath));
    G.Link(G.Pin(Class, P::ReturnValue), G.Pin(SoftPath, P::Class));
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, BreakSoftClassPath));
    G.Link(G.Pin(SoftPath, P::ReturnValue), G.Pin(Path, SP::SoftClassPathInput));
    auto* EqualClass = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(Path, E::PathString), G.Pin(EqualClass, P::Binary::LeftOperand)); G.Default(EqualClass, P::Binary::RightOperand, S::ClassPath); G.Require(G.Pin(EqualClass, P::ReturnValue), S::WrongTarget);
    auto* Root = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), Actor);
    G.Require(G.Valid(Root), S::WrongTarget); G.Write(S::Anchor, Root);
    auto* RootName = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName)); G.Link(G.Read(S::Anchor), G.Pin(RootName, P::Object));
    auto* EqualRoot = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(RootName, P::ReturnValue), G.Pin(EqualRoot, P::Binary::LeftOperand)); G.Default(EqualRoot, P::Binary::RightOperand, S::RootName); G.Require(G.Pin(EqualRoot, P::ReturnValue), S::WrongTarget);
    G.Require(G.Valid(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), G.Read(S::Anchor))), S::WrongTarget);
    if (CombinedCandidate) { LoadStockExitAction(G); LoadNativeLookInputs(G); }
    if (CombinedCandidate || DedicatedClass) CalculateNativeOpticalFov(G);
    if (DedicatedClass)
        G.Write(NativeInputNames::Context, LoadStockInputReference(G, DedicatedStationNames::ContextPath, UVoyageInputContextAsset::StaticClass()));
    auto* AnchorLocation = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), G.Read(S::Anchor));
    auto* AnchorRotation = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentRotation), G.Read(S::Anchor));
    auto* Transform = G.Transform(G.Offset(G.Transform(AnchorLocation, AnchorRotation), (CombinedCandidate || DedicatedClass) ? CombinedVehicleNames::Offset : V::StationOffset), AnchorRotation);
    G.Write(V::Attempted, nullptr, N::True);
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
    G.Pin(Spawn, E::ActorClass)->DefaultObject = DedicatedClass ? DedicatedClass : (CombinedCandidate ? AVoyageVehicleForkliftPawn::StaticClass() : AVoyageVehiclePawn::StaticClass()); G.Link(Transform, G.Pin(Spawn, P::SpawnTransform));
    G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn); G.Exec(Spawn); G.Require(G.Valid(G.Pin(Spawn, P::ReturnValue)), V::Failed);
    auto* Cast = NewObject<UK2Node_DynamicCast>(Graph); Cast->TargetType = AVoyageVehiclePawn::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Spawn, P::ReturnValue), Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin(); G.Write(V::Vehicle, Cast->GetCastResultPin());
    if (DedicatedClass)
    {
        auto* SeedCast = NewObject<UK2Node_DynamicCast>(Graph); SeedCast->TargetType = DedicatedClass; SeedCast->SetPurity(false); G.Node(SeedCast);
        G.Link(G.Tail, G.Pin(SeedCast, P::Execute)); G.Link(G.Read(V::Vehicle), SeedCast->GetCastSourcePin()); G.Tail = SeedCast->GetValidCastPin();
        for (FName Field : {N::OriginalPawn, S::Anchor, O::RequestedFov})
        {
            auto* Set = NewObject<UK2Node_VariableSet>(Graph); Set->VariableReference.SetExternalMember(Field, DedicatedClass); G.Node(Set);
            G.Link(SeedCast->GetCastResultPin(), G.Pin(Set, P::FunctionTarget)); G.Link(G.Read(Field), G.Pin(Set, Field)); G.Exec(Set);
        }
    }
    auto* Body = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), G.Read(V::Vehicle));
    auto* BodyCast = NewObject<UK2Node_DynamicCast>(Graph); BodyCast->TargetType = UPrimitiveComponent::StaticClass(); BodyCast->SetPurity(false); G.Node(BodyCast);
    G.Link(G.Tail, G.Pin(BodyCast, P::Execute)); G.Link(Body, BodyCast->GetCastSourcePin()); G.Tail = BodyCast->GetValidCastPin(); G.Write(V::Body, BodyCast->GetCastResultPin());
    auto* Physics = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetSimulatePhysics));
    G.Link(G.Read(V::Body), G.Pin(Physics, P::FunctionTarget)); G.Default(Physics, E::SimulatePhysics, N::False); G.Exec(Physics);
    auto* Collision = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    G.Link(G.Read(V::Vehicle), G.Pin(Collision, P::FunctionTarget)); G.Default(Collision, SP::CollisionEnabled, N::False); G.Exec(Collision);
    // Configure the existing native subobject BEFORE BeginPlay/possession.
    // Let the native lifecycle activate mappings and bind handlers; no guessed setup call.
    if (CombinedCandidate) CheckNativeLookInputs(G, true);
    if (CombinedCandidate) ConfigureNativeOpticalCamera(G);
    auto CheckDedicatedContext = [&](bool Assign)
    {
        auto* Controls = ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeInputNames::ControlsField);
        G.Require(G.Valid(Controls), ExitActionNames::Failed);
        NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, Assign);
    };
    if (DedicatedClass) CheckDedicatedContext(true);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(G.Read(V::Vehicle), G.Pin(Finish, P::Actor)); G.Link(Transform, G.Pin(Finish, P::SpawnTransform)); G.Exec(Finish);
    G.Require(G.Valid(G.Pin(Finish, P::ReturnValue)), V::Failed);
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(G.Read(V::Vehicle), G.Pin(Attach, P::FunctionTarget)); G.Link(G.Read(S::Anchor), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld); G.Default(Attach, E::ScaleRule, N::KeepWorld); G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach);
    G.Require(G.Pin(Attach, P::ReturnValue), V::Failed);
    if (DedicatedClass) { ConfigureCombinedOperatorPoint(G); CheckDedicatedContext(false); }
    if (CombinedCandidate)
    {
        ConfigureCombinedOperatorPoint(G);
        CheckStockExitAction(G, true);
        CheckNativeLookInputs(G, false);
        CheckNativeOpticalCamera(G);
    }
    G.Write(V::PreparedFlag, nullptr, N::True);
    G.Text(N::FreezeStatus, DedicatedClass ? DedicatedStationNames::Prepared : (CombinedCandidate ? CombinedVehicleNames::Prepared : V::Prepared));

    // Independent next-tick observation; never run this only on a raycast hit.
    G.Tail = ExistingTail;
    G.Require(G.Read(V::PreparedFlag), V::Failed);
    G.Require(G.Valid(G.Read(V::Body)), V::Failed);
    auto* Simulating = ObserveCall(G, UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, IsSimulatingPhysics), G.Read(V::Body));
    G.BooleanText(N::Physics, Simulating, V::PhysicsYes, V::PhysicsNo);
    auto* Parent = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), G.Read(V::Body));
    auto* Attached = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Parent, G.Read(S::Anchor));
    G.BooleanText(N::Drift, Attached, V::AttachedYes, V::AttachedNo);
    auto* Ours = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(V::Vehicle));
    G.BooleanText(N::Control, Ours, V::ControlYes, V::ControlNo);
    if (CombinedCandidate) ObserveNativeOpticalCamera(G);
    if (DedicatedClass)
    {
        auto* DedicatedCast = NewObject<UK2Node_DynamicCast>(G.Graph); DedicatedCast->TargetType = DedicatedClass; DedicatedCast->SetPurity(true); G.Node(DedicatedCast);
        G.Link(G.Read(V::Vehicle), DedicatedCast->GetCastSourcePin());
        auto* Observed = ReadNativeInputField(G, DedicatedCast->GetCastResultPin(), DedicatedClass, N::NativeHudRequested);
        G.BooleanText(N::Current, Observed, DedicatedStationNames::HudObserved, DedicatedStationNames::HudNotObserved);
    }
    auto* Controlled = G.Branch(Ours); auto* InsideTail = G.Tail;
    G.Tail = G.Pin(Controlled, P::Else);
    auto* WasInside = G.Branch(G.Read(V::Occupied)); G.Write(V::Occupied, nullptr, N::False);
    G.BooleanText(N::FreezeStatus, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(N::OriginalPawn)), V::Returned, V::Failed);
    StationMerge(G, {G.Tail, G.Pin(WasInside, P::Else)});
    G.Branch(G.Pin(Key, P::ReturnValue));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(N::OriginalPawn)), V::Failed);
    G.Require(G.Valid(G.Read(S::Movement)), V::Failed);
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), S::WalkingByte), V::PlayerOnly);
    auto* CharacterLocation = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation), G.Read(N::OriginalPawn));
    auto* AnchorNow = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), G.Read(S::Anchor));
    auto* Distance = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), CharacterLocation, AnchorNow), G.Pin(Distance, E::VectorLengthInput));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble), G.Pin(Distance, P::ReturnValue), S::Range), V::PlayerOnly);
    G.Require(Attached, V::Failed); G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), Simulating, N::False), V::Failed);
    G.Write(N::Age, nullptr, N::Zero); G.Write(V::ExitSent, nullptr, N::False);
    // Recheck before EVERY entry, including entry after a successful native exit.
    if (CombinedCandidate) { CheckStockExitAction(G, false); CheckNativeLookInputs(G, false); }
    if (CombinedCandidate) CheckNativeOpticalCamera(G);
    if (DedicatedClass) CheckDedicatedContext(false);
    auto* Enter = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnEnterVehicle));
    G.Link(G.Read(V::Vehicle), G.Pin(Enter, P::FunctionTarget)); G.Link(G.Read(S::Controller), G.Pin(Enter, V::NewPossessor)); G.Exec(Enter);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), NativeControlled(G), G.Read(V::Vehicle)), V::Failed);
    G.Write(V::Occupied, nullptr, N::True); G.Text(N::FreezeStatus, DedicatedClass ? DedicatedStationNames::Active : (CombinedCandidate ? CombinedVehicleNames::Active : V::Active));
    G.Tail = InsideTail;
    G.Write(N::Age, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(N::Age), G.Pin(Tick, P::DeltaSeconds)));
    G.Number(N::Clock, V::Seconds, G.Read(N::Age));
    auto* Unsafe = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), Simulating,
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), Attached, N::False));
    auto* AutoExit = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Read(V::ExitSent), N::False),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), Unsafe,
            G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Read(N::Age), S::Limit)));
    G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), G.Pin(Key, P::ReturnValue), AutoExit)); NativeExit(G);
}
