#pragma once

// Mod-authored HC13 aiming frame; included after OpticalSelf.
namespace StationAimNames
{
inline const FName Yaw(TEXT("RailgunLocalAimYaw"));
inline const FName Pitch(TEXT("RailgunLocalAimPitch"));
inline constexpr TCHAR DegreesPerMouseUnit[] = TEXT("0.64");
inline constexpr TCHAR MinimumPitch[] = TEXT("-50.0");
inline constexpr TCHAR MaximumPitch[] = TEXT("10.0");
// Provisional clear optical point 180cm above shell root, not a muzzle socket.
// Above the accepted 159cm blockout; shape/materials/mesh pivots remain untouched.
inline constexpr TCHAR EyePosition[] = TEXT("0,0,180");
inline constexpr TCHAR YawLabel[] = TEXT("Railgun-local yaw (wrap +/-180): ");
inline constexpr TCHAR PitchLabel[] = TEXT("Railgun-local pitch (-50 to +10): ");
}
namespace Aim = StationAimNames;

void ApplyStationAim(FGraph& G)
{
    auto* Rotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Read(Aim::Yaw), G.Pin(Rotation, SP::Yaw)); G.Link(G.Read(Aim::Pitch), G.Pin(Rotation, SP::Pitch));
    auto* Set = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRelativeRotation));
    G.Link(OpticalSelf(G), G.Pin(Set, P::FunctionTarget)); G.Link(G.Pin(Rotation, P::ReturnValue), G.Pin(Set, OP::RelativeRotation));
    G.Default(Set, OP::Sweep, N::False); G.Default(Set, E::Teleport, N::False); G.Exec(Set);
}

void BeginStationAim(FGraph& G)
{
    G.Write(Aim::Yaw, nullptr, N::Zero); G.Write(Aim::Pitch, nullptr, N::Zero);
    auto* Position = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRelativeLocation));
    G.Link(OpticalSelf(G), G.Pin(Position, P::FunctionTarget)); G.Default(Position, OP::RelativeLocation, Aim::EyePosition);
    G.Default(Position, OP::Sweep, N::False); G.Default(Position, E::Teleport, N::False); G.Exec(Position);
    ApplyStationAim(G);
}

void UpdateStationAim(FGraph& G)
{
    // Raw per-frame deltas avoid native player-camera pitch clamps. No dt factor:
    // these values are displacement, not angular speed. No input ownership write.
    [[maybe_unused]] constexpr auto MouseSignature = static_cast<void (APlayerController::*)(float&, float&) const>(&APlayerController::GetInputMouseDelta);
    auto* Mouse = G.Call(APlayerController::StaticClass(), OP::GetMouseDelta);
    G.Link(G.Read(S::Controller), G.Pin(Mouse, P::FunctionTarget));
    if (Mouse->FindPin(P::Execute)) G.Exec(Mouse);
    auto Scaled = [&](FName Axis)
    {
        auto* Scale = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
        G.Link(G.Pin(Mouse, Axis), G.Pin(Scale, P::Binary::LeftOperand));
        G.Default(Scale, P::Binary::RightOperand, Aim::DegreesPerMouseUnit); return G.Pin(Scale, P::ReturnValue);
    };
    auto* Normalize = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NormalizeAxis));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(Aim::Yaw), Scaled(OP::MouseHorizontal)), G.Pin(Normalize, OP::NormalizeAngle));
    G.Write(Aim::Yaw, G.Pin(Normalize, P::ReturnValue));
    auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), G.Read(Aim::Pitch), Scaled(OP::MouseVertical)), G.Pin(Clamp, OP::ClampValue));
    G.Default(Clamp, OP::ClampMinimum, Aim::MinimumPitch); G.Default(Clamp, OP::ClampMaximum, Aim::MaximumPitch);
    G.Write(Aim::Pitch, G.Pin(Clamp, P::ReturnValue)); ApplyStationAim(G);
    G.Number(N::Control, Aim::YawLabel, G.Read(Aim::Yaw)); G.Number(N::Physics, Aim::PitchLabel, G.Read(Aim::Pitch));
}
