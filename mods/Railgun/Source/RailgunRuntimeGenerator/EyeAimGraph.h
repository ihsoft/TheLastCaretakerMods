#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds eye-origin and parallax aiming graphs for the dedicated station.
void BindFirstPersonCamera(FGraph& G);

// Follow only the observed first-person component's position. Rotation remains
// station-owned; do not activate the character camera or re-possess its pawn.
void UpdateEyeCameraPosition(FGraph& G);

void RotateEyeCamera(FGraph& G);

void PlaceModeCamera(FGraph& G, bool Wide);

void ConvergeEyeAim(FGraph& G);
}
