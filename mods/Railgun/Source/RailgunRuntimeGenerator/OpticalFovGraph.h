#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Calculates the station optical FOV without possessing or mutating the character camera.
void CalculateNativeOpticalFov(FGraph& G);
}
