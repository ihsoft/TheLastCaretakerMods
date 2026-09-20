#pragma once
namespace EyeAim
{
inline const FName Yaw(TEXT("RailgunEyeYaw"));
inline const FName Pitch(TEXT("RailgunEyePitch"));
inline const FName Target(TEXT("RailgunEyeTarget"));
inline const FName EyeLocation(TEXT("OutLocation"));
inline const FName NewLocation(TEXT("NewLocation"));
inline const FName Direction(TEXT("Direction"));
inline const FName RotationVector(TEXT("InVec"));
inline constexpr TCHAR FirstPersonCameraName[] = TEXT("FirstPersonCamera");
}

// Follow only the observed first-person component's position. Rotation remains
// station-owned; do not activate the character camera or re-possess its pawn.
void UpdateEyeCameraPosition(FGraph& G)
{
    auto* Find = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetComponentsByClass));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Find, P::FunctionTarget)); G.Pin(Find, OP::ComponentClass)->DefaultObject = UCameraComponent::StaticClass();
    auto* Loop = ContextLoop(G, G.Pin(Find, P::ReturnValue));
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = UCameraComponent::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Loop, CE::ArrayElement), Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    auto* Name = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName));
    G.Link(Cast->GetCastResultPin(), G.Pin(Name, P::Object));
    auto* Equal = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(Name, P::ReturnValue), G.Pin(Equal, P::Binary::LeftOperand)); G.Default(Equal, P::Binary::RightOperand, EyeAim::FirstPersonCameraName);
    G.Branch(G.Pin(Equal, P::ReturnValue));
    auto* Set = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorLocation));
    G.Link(G.Read(DS::Camera), G.Pin(Set, P::FunctionTarget));
    G.Link(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), Cast->GetCastResultPin()), G.Pin(Set, EyeAim::NewLocation));
    G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
    G.Tail = G.Pin(Loop, CE::Completed);
}

void RotateEyeCamera(FGraph& G)
{
    auto* Rot = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Read(EyeAim::Yaw), G.Pin(Rot, SP::Yaw)); G.Link(G.Read(EyeAim::Pitch), G.Pin(Rot, SP::Pitch));
    auto* Set = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRelativeRotation));
    G.Link(G.Read(DS::Camera), G.Pin(Set, P::FunctionTarget)); G.Link(G.Pin(Rot, P::ReturnValue), G.Pin(Set, OP::RelativeRotation));
    G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
}

void PlaceModeCamera(FGraph& G, bool Wide)
{
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(G.Read(DS::Camera), G.Pin(Attach, P::FunctionTarget));
    G.Link(G.Read(Wide ? S::Anchor : DS::Sight), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, E::SnapToTarget); G.Default(Attach, E::RotationRule, E::SnapToTarget);
    G.Default(Attach, E::ScaleRule, N::KeepWorld); G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach);
    if (Wide)
    {
        // Native eye point is a fallback only; measured FirstPersonCamera wins.
        auto* Eyes = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetActorEyesViewPoint));
        G.Link(G.Read(N::OriginalPawn), G.Pin(Eyes, P::FunctionTarget));
        auto* Set = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorLocation));
        G.Link(G.Read(DS::Camera), G.Pin(Set, P::FunctionTarget)); G.Link(G.Pin(Eyes, EyeAim::EyeLocation), G.Pin(Set, EyeAim::NewLocation));
        G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
        UpdateEyeCameraPosition(G);
        G.Write(EyeAim::Yaw, G.Read(Aim::Yaw)); G.Write(EyeAim::Pitch, G.Read(Aim::Pitch));
        RotateEyeCamera(G);
    }
}

void ConvergeEyeAim(FGraph& G)
{
    auto* Start = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), G.Read(O::Camera));
    auto* Forward = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetForwardVector), G.Read(O::Camera));
    auto* Ray = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat));
    G.Link(Forward, G.Pin(Ray, P::Binary::LeftOperand)); G.Default(Ray, P::Binary::RightOperand, Range::MaximumCentimeters);
    G.Write(EyeAim::Target, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector), Start, G.Pin(Ray, P::ReturnValue)));
    auto* Railgun = ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor));
    auto* Trace = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LineTraceSingle));
    G.Link(Start, G.Pin(Trace, E::TraceStart)); G.Link(G.Read(EyeAim::Target), G.Pin(Trace, E::TraceEnd));
    G.Link(G.ActorArray(Railgun, G.Read(N::OriginalPawn)), G.Pin(Trace, E::ActorsToIgnore));
    G.Default(Trace, E::TraceChannel, N::VisibilityTrace); G.Default(Trace, E::TraceComplex, N::False); G.Default(Trace, E::IgnoreSelf, N::True); G.Exec(Trace);
    auto* Found = G.Branch(G.Pin(Trace, P::ReturnValue));
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult)); G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit));
    G.Write(EyeAim::Target, G.Pin(Hit, OP::ImpactPoint)); StationMerge(G, {G.Tail, G.Pin(Found, P::Else)});
    // Two fixed iterations account approximately for the sight moving with pitch.
    // Clamp unreachable directions; no forced rotation of mount/character.
    for (int32 Iteration = 0; Iteration < 2; ++Iteration)
    {
        auto* Sight = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), G.Read(DS::Sight));
        auto* Delta = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), G.Read(EyeAim::Target), Sight);
        auto* Local = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, InverseTransformDirection));
        G.Link(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentToWorld), G.Read(S::Anchor)), G.Pin(Local, E::Transform));
        G.Link(Delta, G.Pin(Local, EyeAim::Direction));
        auto* Rot = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_VectorToRotator)); G.Link(G.Pin(Local, P::ReturnValue), G.Pin(Rot, EyeAim::RotationVector));
        auto* Parts = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakRotator)); G.Link(G.Pin(Rot, P::ReturnValue), G.Pin(Parts, SP::RotationValue));
        G.Write(Aim::Yaw, ClampStationAim(G, true, G.Pin(Parts, SP::Yaw)));
        G.Write(Aim::Pitch, ClampStationAim(G, false, G.Pin(Parts, SP::Pitch)));
        DedicatedAim(G);
    }
}
