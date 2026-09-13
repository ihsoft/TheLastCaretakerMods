#pragma once
#include "UObject/NameTypes.h"

// Engine-reflected pins for diagnostic actor generators. Build-time only.
namespace ActorLifecycleGraphNames
{
inline const FName NewTransform(TEXT("NewTransform"));
inline const FName NewLocation(TEXT("NewLocation"));
inline const FName NewRelativeLocation(TEXT("NewRelativeLocation"));
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
inline const FName SimulatePhysics(TEXT("bSimulate"));
inline const FName TraceStart(TEXT("Start"));
inline const FName TraceEnd(TEXT("End"));
inline const FName TraceChannel(TEXT("TraceChannel"));
inline const FName TraceComplex(TEXT("bTraceComplex"));
inline const FName ActorsToIgnore(TEXT("ActorsToIgnore"));
inline const FName IgnoreSelf(TEXT("bIgnoreSelf"));
inline const FName OutHit(TEXT("OutHit"));
inline const FName Hit(TEXT("Hit"));
inline const FName HitComponent(TEXT("HitComponent"));
inline const FName AttachmentParent(TEXT("Parent"));
inline const FName LocationRule(TEXT("LocationRule"));
inline const FName RotationRule(TEXT("RotationRule"));
inline const FName ScaleRule(TEXT("ScaleRule"));
inline const FName WeldBodies(TEXT("bWeldSimulatedBodies"));
inline const FName DisplayObject(TEXT("Object"));
inline const FName WidgetType(TEXT("WidgetType"));
inline const FName OwningPlayer(TEXT("OwningPlayer"));
inline const FName ZOrder(TEXT("ZOrder"));
inline const FName WidgetText(TEXT("InText"));
inline const FName EndPlayEvent(TEXT("ReceiveEndPlay"));
inline const FName IgnoreMoveInputSetting(TEXT("bNewMoveInput"));
}
