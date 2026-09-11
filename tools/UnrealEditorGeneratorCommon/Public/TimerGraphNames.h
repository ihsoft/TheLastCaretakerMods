#pragma once
#include "BlueprintGraphNames.h"

// Engine-owned Blueprint event/timer names. Build-time only.
namespace TimerGraphNames
{
inline const FName ActorBeginPlay(TEXT("ReceiveBeginPlay"));
inline const FName& Object = BlueprintGraphNames::Pins::Object;
inline const FName Function(TEXT("FunctionName"));
inline const FName Interval(TEXT("Time"));
inline const FName Looping(TEXT("bLooping"));
}
