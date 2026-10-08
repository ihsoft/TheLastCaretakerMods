#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace RailgunWaterWake
{
inline constexpr TCHAR ControllerPackage[] =
    TEXT("/Game/Mods/Railgun/Station/BP_RailgunWaterWakeController");
inline const FName AddSample(TEXT("AddWakeSample"));
inline const FName Finish(TEXT("FinishWake"));
inline const FName ProcessSegment(TEXT("ProcessWaterWakeSegment"));
inline const FName Controller(TEXT("WaterWakeController"));
inline const FName Locations(TEXT("WakeLocations"));
inline const FName Strengths(TEXT("WakeStrengths"));
inline const FName Expiries(TEXT("WakeExpiries"));
inline const FName Accepting(TEXT("WakeAcceptingSamples"));
inline const FName Alive(TEXT("WakeAliveThisTick"));
inline const FName DistanceToNext(TEXT("WaterWakeDistanceToNextSample"));
inline const FName PreviewDistance(TEXT("WaterWakePreviewDistance"));
inline const FName Start(TEXT("Start"));
inline const FName End(TEXT("End"));
inline const FName FlushEnd(TEXT("FlushEnd"));
inline const FName SampleLocation(TEXT("SampleLocation"));
inline const FName SampleStrength(TEXT("SampleStrength"));
inline const FName ContextObject(TEXT("ContextObject"));
inline const FName ClassPin(TEXT("Class"));
inline const FName Impulse(TEXT("Impulse"));
inline const FName Location(TEXT("Location"));
inline const FName Direction(TEXT("Direction"));
inline const FName Radius(TEXT("Radius"));
inline const FName Strength(TEXT("Strength"));
inline const FName X(TEXT("X"));
inline const FName Y(TEXT("Y"));
inline const FName Z(TEXT("Z"));
inline const FName ArrayItem(TEXT("Item"));
inline const FName NewItem(TEXT("NewItem"));

inline const FName LifePin(TEXT("InLifespan"));

inline constexpr TCHAR SampleSpacingCentimeters[] = TEXT("200.0");
inline constexpr TCHAR IntegerOne[] = TEXT("1");
inline constexpr TCHAR MaximumSamplesPerSegment[] = TEXT("256");
inline constexpr TCHAR MaximumSamples[] = TEXT("256");
inline constexpr TCHAR MaximumPreviewDistanceCentimeters[] = TEXT("50000.0");
inline constexpr TCHAR MaximumHeightAboveWaterCentimeters[] = TEXT("800.0");
inline constexpr TCHAR WaterQueryLoweringCentimeters[] =
    TEXT("(X=0.0,Y=0.0,Z=-800.0)");
inline constexpr TCHAR ExactEndToleranceCentimeters[] = TEXT("199.999");
inline constexpr TCHAR ImpulseRadiusCentimeters[] = TEXT("200.0");
inline constexpr TCHAR ZeroDirection[] = TEXT("(X=0.0,Y=0.0)");
inline constexpr TCHAR MaximumImpulseStrength[] = TEXT("0.2");
inline constexpr TCHAR SampleLifetimeSeconds[] = TEXT("0.25");
inline constexpr TCHAR ControllerMaximumLifetimeSeconds[] = TEXT("1.0");
inline UClass* ControllerClass = nullptr;
}

UEdGraphPin* RailgunWaterWakeIntWithLiteral(FGraph& G, FName Function,
    UEdGraphPin* Value, const TCHAR* Literal);

void QueueRailgunWaterWakeSample(FGraph& G, UEdGraphPin* Point,
    UEdGraphPin* World);

void AddRailgunWaterWakeSegmentFunction(UBlueprint* BP);

void ProcessRailgunWaterWakeSegment(FGraph& G, UClass* ShotClass,
    UEdGraphPin* Start, UEdGraphPin* End, bool FlushEnd);

UEdGraphPin* RailgunWaterWakeArrayLength(FGraph& G, FName ArrayName);

UEdGraphPin* RailgunWaterWakeArrayGet(FGraph& G, FName ArrayName,
    UEdGraphPin* Index);

void RailgunWaterWakeArrayAdd(FGraph& G, FName ArrayName,
    UEdGraphPin* Item);

void CreateRailgunWaterWakeFunction(UBlueprint* BP, FName Name,
    UEdGraph*& Graph, UK2Node_FunctionEntry*& Entry,
    UK2Node_FunctionResult*& Result);

void AddRailgunWaterWakeControllerFunctions(UBlueprint* BP);

void SubmitRailgunWaterWakeControllerImpulse(FGraph& G,
    UEdGraphPin* Weather, UEdGraphPin* Index);

UK2Node_Event* RailgunWaterWakeEvent(FGraph& G, FName Name);

UClass* CreateRailgunWaterWakeController();

void FinishRailgunWaterWake(FGraph& G);

void SpawnRailgunWaterWakeController(FGraph& G);
}
