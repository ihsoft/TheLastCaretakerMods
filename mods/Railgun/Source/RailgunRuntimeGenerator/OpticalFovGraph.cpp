#include "OpticalFovGraph.h"

#include "GraphCallHelpers.h"
#include "StationOpticsGraph.h"
#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
void CalculateNativeOpticalFov(FGraph& G)
{
    auto* Manager = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Require(G.Valid(G.Pin(Manager, P::ReturnValue)));
    G.Write(O::BaselineFov, ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetFOVAngle), G.Pin(Manager, P::ReturnValue)));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), G.Read(O::BaselineFov), O::MinimumFov),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble), G.Read(O::BaselineFov), O::MaximumFov)));
    // Angular magnification: 2*atan(tan(on-foot FOV/2)/5), not FOV/5.
    auto* Half = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(G.Read(O::BaselineFov), G.Pin(Half, P::Binary::LeftOperand)); G.Default(Half, P::Binary::RightOperand, O::Half);
    auto* Tangent = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, DegTan));
    G.Link(G.Pin(Half, P::ReturnValue), G.Pin(Tangent, OP::AngleValue));
    auto* Divide = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(G.Pin(Tangent, P::ReturnValue), G.Pin(Divide, P::Binary::LeftOperand)); G.Default(Divide, P::Binary::RightOperand, O::Magnification);
    auto* Atan = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, DegAtan));
    G.Link(G.Pin(Divide, P::ReturnValue), G.Pin(Atan, OP::AngleValue));
    auto* Twice = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(G.Pin(Atan, P::ReturnValue), G.Pin(Twice, P::Binary::LeftOperand)); G.Default(Twice, P::Binary::RightOperand, O::Twice);
    G.Write(O::RequestedFov, G.Pin(Twice, P::ReturnValue));
}
}
