#pragma once
#include "UObject/NameTypes.h"

// Engine-reflected pins for diagnostic actor generators. Build-time only.
namespace ActorLifecycleGraphNames
{
inline const FName NewTransform(TEXT("NewTransform"));
inline const FName NewLocation(TEXT("NewLocation"));
inline const FName ActorHidden(TEXT("bHidden"));
inline const FName Location(TEXT("Location"));
inline const FName Rotation(TEXT("Rotation"));
inline const FName Scale(TEXT("Scale"));
inline const FName Transform(TEXT("T"));
inline const FName LocalPosition = Location;
inline const FName Text(TEXT("Value"));
inline const FName StringValue(TEXT("InString"));
inline const FName DoubleValue(TEXT("InDouble"));
inline const FName StringPrefix(TEXT("Prefix"));
inline const FName ActorClass(TEXT("ActorClass"));
inline const FName AssetClass(TEXT("AssetClass"));
inline const FName PathString(TEXT("PathString"));
inline const FName SoftClassPath(TEXT("SoftClassPath"));
inline const FName CollisionHandling(TEXT("CollisionHandlingOverride"));
inline const FName Teleport(TEXT("bTeleport"));
inline const FName VectorLengthInput(TEXT("A"));
}
