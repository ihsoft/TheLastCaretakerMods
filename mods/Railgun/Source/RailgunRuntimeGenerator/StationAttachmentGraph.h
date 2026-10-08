#pragma once

#include "CharacterStationGraphNames.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds station attachment, movement/collision ownership, and release graphs.
namespace StationProbeNames
{
inline const FName Anchor(TEXT("StationAnchor"));
inline constexpr TCHAR WalkingByte[] = TEXT("1");
}
namespace S = StationProbeNames;
namespace SP = CharacterStationGraphNames;

UEdGraphPin* StationMode(FGraph& G, UEdGraphPin* Movement);

void StationMerge(FGraph& G, const TArray<UEdGraphPin*>& Paths);
}
