#pragma once
#include "CoreMinimal.h"

namespace Railgun::Runtime
{
// Names and constants shared by station HUD graph construction.
namespace StationHudNames
{
inline const FName ScopePanel(TEXT("RailgunScopePanel"));
inline constexpr int32 TargetFontSize = 20;
}
namespace H = StationHudNames;
}
