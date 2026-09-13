#pragma once

// Included after StationMerge. HC12 view session plus HC13 local aim frame.
namespace OpticalProbeNames
{
inline const FName Camera(TEXT("StationOpticalCamera"));
inline const FName Active(TEXT("OwnsOpticalView"));
inline const FName PreviousView(TEXT("PreviousOpticalViewTarget"));
inline const FName BaselineFov(TEXT("BaselineOpticalFOV"));
inline const FName RequestedFov(TEXT("RequestedOpticalFOV"));
inline const FName CharacterHidden(TEXT("CharacterHiddenBeforeOptics"));
inline const FName Reticle(TEXT("OpticalReticle"));
inline constexpr TCHAR Half[] = TEXT("0.5");
inline constexpr TCHAR Twice[] = TEXT("2.0");
inline constexpr TCHAR Magnification[] = TEXT("5.0");
inline constexpr TCHAR MinimumFov[] = TEXT("10.0");
inline constexpr TCHAR MaximumFov[] = TEXT("150.0");
inline constexpr TCHAR Visible[] = TEXT("HitTestInvisible");
inline constexpr TCHAR Collapsed[] = TEXT("Collapsed");
inline constexpr TCHAR ReticleText[] = TEXT("+");
inline constexpr int32 ReticleFontSize = 34;
inline const FVector2D ReticleSize(50.0f, 60.0f);
inline const FVector2D ReticleCenter(0.5f, 0.5f);
inline constexpr TCHAR Title[] = TEXT("HC17 PAWN HANDOFF / x5 / F8 EXIT");
inline constexpr TCHAR ActiveText[] = TEXT("RANGE: aim at a shark; F8 exits (max20s)");
inline constexpr TCHAR Refused[] = TEXT("OPTICS REFUSED: normal on-foot view, no scope/vehicle");
inline constexpr TCHAR Failure[] = TEXT("OPTICS FAILED: released; send this screenshot");
inline constexpr TCHAR Lost[] = TEXT("OPTICS LOST VIEW TARGET: released without taking it back");
inline constexpr TCHAR Requested[] = TEXT("Requested optical FOV (degrees): ");
inline constexpr TCHAR Actual[] = TEXT("Actual camera FOV (degrees): ");
inline constexpr TCHAR Instructions[] = TEXT("Empty hands. No aim/helm/save while fixed. Stock HUD retained.");
}
namespace O = OpticalProbeNames;
namespace OP = OpticalCameraGraphNames;
void ReleaseStation(FGraph& G);

UEdGraphPin* OpticalSelf(FGraph& G)
{
    auto* Node = G.Node(NewObject<UK2Node_Self>(G.Graph)); return G.Pin(Node, P::FunctionTarget);
}

#include "StationAimProbe.h"
#include "StationRangeProbe.h"
#include "StationHudProbe.h"
#include "StationControlProbe.h"

UEdGraphPin* OpticalView(FGraph& G)
{
    return ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, GetViewTarget), G.Read(S::Controller));
}

void OpticalSwitchView(FGraph& G, UEdGraphPin* Target)
{
    auto* Call = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, SetViewTargetWithBlend));
    G.Link(G.Read(S::Controller), G.Pin(Call, P::FunctionTarget));
    G.Link(Target, G.Pin(Call, OP::NewViewTarget)); G.Default(Call, OP::BlendTime, N::Zero); G.Exec(Call);
}

void OpticalReticle(FGraph& G, bool Visible)
{
    auto SetWidget = [&](FName Name, bool Show)
    {
    auto* Widget = NewObject<UK2Node_VariableGet>(G.Graph);
    Widget->VariableReference.SetExternalMember(Name, G.HudClass); G.Node(Widget);
    G.Link(G.Read(N::HudInstance), G.Pin(Widget, P::FunctionTarget));
    auto* Call = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
    G.Link(G.Pin(Widget, Name), G.Pin(Call, P::FunctionTarget));
    G.Default(Call, OP::Visibility, Show ? O::Visible : O::Collapsed); G.Exec(Call);
    };
    SetWidget(O::Reticle, Visible); SetWidget(H::ScopePanel, Visible); SetWidget(N::FreezeStatus, !Visible);
}

void ReleaseOptics(FGraph& G)
{
    auto* Active = G.Branch(G.Read(O::Active)); G.Write(O::Active, nullptr, N::False);
    auto* ControllerValid = G.Branch(G.Valid(G.Read(S::Controller)));
    auto* Ours = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), OpticalView(G), OpticalSelf(G)));
    auto* PreviousValid = G.Branch(G.Valid(G.Read(O::PreviousView)));
    OpticalSwitchView(G, G.Read(O::PreviousView)); auto* Restored = G.Tail;
    G.Tail = G.Pin(PreviousValid, P::Else);
    auto* Pawn = ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), G.Read(S::Controller));
    auto* PawnValid = G.Branch(G.Valid(Pawn)); OpticalSwitchView(G, Pawn);
    StationMerge(G, {G.Tail, G.Pin(PawnValid, P::Else), Restored, G.Pin(Ours, P::Else), G.Pin(ControllerValid, P::Else)});
    auto* CharacterValid = G.Branch(G.Valid(G.Read(N::OriginalPawn)));
    auto* Hidden = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorHiddenInGame));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Hidden, P::FunctionTarget)); G.Link(G.Read(O::CharacterHidden), G.Pin(Hidden, OP::Hidden)); G.Exec(Hidden);
    StationMerge(G, {G.Tail, G.Pin(CharacterValid, P::Else)});
    auto* Detach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DetachFromActor));
    G.Link(OpticalSelf(G), G.Pin(Detach, P::FunctionTarget));
    G.Default(Detach, E::LocationRule, N::KeepWorld); G.Default(Detach, E::RotationRule, N::KeepWorld); G.Default(Detach, E::ScaleRule, N::KeepWorld); G.Exec(Detach);
    auto* HudValid = G.Branch(G.Valid(G.Read(N::HudInstance))); OpticalReticle(G, false);
    G.Text(Range::TargetName, N::EmptyText);
    G.Text(Range::TargetRange, N::EmptyText);
    StationMerge(G, {G.Tail, G.Pin(HudValid, P::Else), G.Pin(Active, P::Else)});
}

void PrepareOptics(FGraph& G)
{
    auto* Manager = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Require(G.Valid(G.Pin(Manager, P::ReturnValue)), O::Refused);
    G.Write(O::PreviousView, OpticalView(G));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), G.Read(O::PreviousView), G.Read(N::OriginalPawn)), O::Refused);
    G.Write(O::BaselineFov, ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetFOVAngle), G.Pin(Manager, P::ReturnValue)));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), G.Read(O::BaselineFov), O::MinimumFov),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble), G.Read(O::BaselineFov), O::MaximumFov)), O::Refused);
    auto* Half = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(G.Read(O::BaselineFov), G.Pin(Half, P::Binary::LeftOperand)); G.Default(Half, P::Binary::RightOperand, O::Half);
    auto* Tangent = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, DegTan)); G.Link(G.Pin(Half, P::ReturnValue), G.Pin(Tangent, OP::AngleValue));
    auto* Divide = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(G.Pin(Tangent, P::ReturnValue), G.Pin(Divide, P::Binary::LeftOperand)); G.Default(Divide, P::Binary::RightOperand, O::Magnification);
    auto* Atan = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, DegAtan)); G.Link(G.Pin(Divide, P::ReturnValue), G.Pin(Atan, OP::AngleValue));
    auto* Twice = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(G.Pin(Atan, P::ReturnValue), G.Pin(Twice, P::Binary::LeftOperand)); G.Default(Twice, P::Binary::RightOperand, O::Twice); G.Write(O::RequestedFov, G.Pin(Twice, P::ReturnValue));
    auto* Hidden = NewObject<UK2Node_VariableGet>(G.Graph);
    Hidden->VariableReference.SetExternalMember(E::ActorHidden, AActor::StaticClass()); G.Node(Hidden);
    G.Link(G.Read(N::OriginalPawn), G.Pin(Hidden, P::FunctionTarget));
    G.Write(O::CharacterHidden, G.Pin(Hidden, E::ActorHidden));
    PrepareStationControl(G);
}

void BeginOptics(FGraph& G)
{
    G.Write(O::Active, nullptr, N::True); // Partial entry is owned before any mutation.
    auto* Fov = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
    G.Link(G.Read(O::Camera), G.Pin(Fov, P::FunctionTarget)); G.Link(G.Read(O::RequestedFov), G.Pin(Fov, OP::FieldOfView)); G.Exec(Fov);
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(OpticalSelf(G), G.Pin(Attach, P::FunctionTarget)); G.Link(G.Read(S::Anchor), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld); G.Default(Attach, E::ScaleRule, N::KeepWorld); G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach);
    auto* Attached = G.Branch(G.Pin(Attach, P::ReturnValue)); auto* Success = G.Tail;
    G.Tail = G.Pin(Attached, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, O::Failure); G.Tail = Success;
    BeginStationAim(G);
    // ViewTarget is now a camera actor, so the native owner-no-see test changes.
    // Hide only the original character's rendering; restore its exact prior flag.
    auto* Hide = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorHiddenInGame));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Hide, P::FunctionTarget)); G.Default(Hide, OP::Hidden, N::True); G.Exec(Hide);
    BeginStationControl(G);
    OpticalSwitchView(G, OpticalSelf(G)); OpticalReticle(G, true);
    G.Number(N::Physics, O::Requested, G.Read(O::RequestedFov)); G.Text(N::FreezeStatus, O::ActiveText);
}

void UpdateOptics(FGraph& G)
{
    auto* Ours = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), G.Read(O::Active),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), OpticalView(G), OpticalSelf(G))));
    auto* Good = G.Tail; G.Tail = G.Pin(Ours, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, O::Lost); G.Tail = Good;
    UpdateStationAim(G);
    auto* Manager = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    auto* Fov = ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetFOVAngle), G.Pin(Manager, P::ReturnValue));
    G.Number(N::Current, O::Actual, Fov); G.Text(N::FreezeStatus, O::ActiveText);
    // Range generation leaves the exec tail inside its successful shark-hit branch.
    // The callback marker must update even when the optical ray hits nothing.
    G.BooleanText(H::ScopeFooter, G.Read(N::NativeHudRequested), N::NativeHudYes, N::NativeHudNo);
    UpdateStationRange(G);
}
