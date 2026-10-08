#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace RailgunWater
{
inline const FName ProcessSegment(TEXT("ProcessWaterSegment"));
inline const FName SegmentStart(TEXT("ShotWaterSegmentStart"));
inline const FName FirstCrossingDone(TEXT("FirstWaterCrossingDone"));
inline const FName Start(TEXT("Start"));
inline const FName End(TEXT("End"));
inline const FName World(TEXT("World"));
inline const FName Loc(TEXT("Loc"));
inline const FName IntegerInput(TEXT("InInt"));
inline const FName VectorInput(TEXT("InVec"));
inline const FName VectorZ(TEXT("Z"));
inline const FName Alpha(TEXT("Alpha"));
inline const FName BreakLoop(TEXT("Break"));
inline const FName ForLoopWithBreak(TEXT("ForLoopWithBreak"));

inline const FName PreviousPoint(TEXT("PreviousPoint"));
inline const FName PreviousValid(TEXT("PreviousValid"));
inline const FName PreviousBelow(TEXT("PreviousBelow"));
inline const FName InsidePoint(TEXT("InsidePoint"));
inline const FName OutsidePoint(TEXT("OutsidePoint"));
inline const FName BestPoint(TEXT("BestPoint"));
inline const FName BestResidual(TEXT("BestResidual"));
inline const FName BestValid(TEXT("BestValid"));

inline constexpr TCHAR InvalidHeight[] = TEXT("-100000.0");
inline constexpr TCHAR MaximumFiniteMagnitude[] = TEXT("1000000000.0");
inline constexpr TCHAR SampleStepCentimeters[] = TEXT("100.0");
inline constexpr TCHAR MaximumSegmentCentimeters[] = TEXT("100000.0");
inline constexpr TCHAR MaximumSamples[] = TEXT("1000");
inline constexpr TCHAR BisectionLastIndex[] = TEXT("7");
inline constexpr TCHAR MaximumResidualCentimeters[] = TEXT("5.0");
inline constexpr TCHAR MidpointAlpha[] = TEXT("0.5");
inline constexpr TCHAR InitialBestResidual[] = TEXT("1000000000.0");
inline constexpr TCHAR FirstLoopIndex[] = TEXT("1");
inline constexpr TCHAR MinimumSamples[] = TEXT("1");
}

struct FRailgunWaterSample
{
    UEdGraphPin* Height;
    UEdGraphPin* Valid;
    UEdGraphPin* Below;
    UEdGraphPin* Residual;
};

FEdGraphPinType RailgunWaterPinType(FName Category, UObject* Type = nullptr);

FBPVariableDescription AddRailgunWaterLocal(UK2Node_FunctionEntry* Entry,
    FName Name, const FEdGraphPinType& Type);

UEdGraphPin* ReadRailgunWaterLocal(FGraph& G,
    const FBPVariableDescription& Local);

void WriteRailgunWaterLocal(FGraph& G, const FBPVariableDescription& Local,
    UEdGraphPin* Value = nullptr, const TCHAR* Literal = nullptr);

UK2Node_MacroInstance* RailgunWaterLoop(FGraph& G, UEdGraphPin* LastIndex,
    const TCHAR* LiteralLastIndex, const TCHAR* FirstIndex);

UEdGraphPin* RailgunWaterVectorLerp(FGraph& G, UEdGraphPin* Start,
    UEdGraphPin* End, UEdGraphPin* Alpha, const TCHAR* LiteralAlpha = nullptr);

UEdGraphPin* GetRailgunWaterHeight(FGraph& G, UEdGraphPin* World,
    UEdGraphPin* Location);

FRailgunWaterSample SampleRailgunWater(FGraph& G, UEdGraphPin* World,
    UEdGraphPin* Point);

void ConsiderRailgunWaterCandidate(FGraph& G, UEdGraphPin* Point,
    const FRailgunWaterSample& Sample,
    const FBPVariableDescription& BestPoint,
    const FBPVariableDescription& BestResidual,
    const FBPVariableDescription& BestValid);

void AddRailgunWaterSegmentFunction(UBlueprint* BP);

void ProcessRailgunWaterSegment(FGraph& G, UClass* ShotClass,
    UEdGraphPin* SegmentStart, UEdGraphPin* SegmentEnd);
}
