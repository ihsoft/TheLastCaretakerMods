#pragma once

// Builds optical view switching, reticle, and release graphs.
namespace OpticalProbeNames
{
inline const FName Camera(TEXT("StationOpticalCamera"));
inline const FName BaselineFov(TEXT("BaselineOpticalFOV"));
inline const FName RequestedFov(TEXT("RequestedOpticalFOV"));
inline constexpr TCHAR Half[] = TEXT("0.5");
inline constexpr TCHAR Twice[] = TEXT("2.0");
inline constexpr TCHAR Magnification[] = TEXT("5.0");
inline constexpr TCHAR MinimumFov[] = TEXT("10.0");
inline constexpr TCHAR MaximumFov[] = TEXT("150.0");
inline constexpr TCHAR Visible[] = TEXT("HitTestInvisible");
inline constexpr TCHAR Collapsed[] = TEXT("Collapsed");
}
namespace O = OpticalProbeNames;
namespace OP = OpticalCameraGraphNames;

UEdGraphPin* OpticalSelf(FGraph& G)
{
    auto* Node = G.Node(NewObject<UK2Node_Self>(G.Graph)); return G.Pin(Node, P::FunctionTarget);
}
#include "StationAimNames.h"
#include "StationRangeGraph.h"
#include "StationHudNames.h"
