#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds optical target/range tracing and HUD text graphs.
namespace StationRangeNames
{
// Retain the existing widget/property identity; HC33 generalizes its contents.
inline const FName TargetName(TEXT("DetectedSharkName"));
inline const FName TargetRange(TEXT("OpticalTargetRange"));
inline const FName PendingTargetName(TEXT("RailgunPendingTargetName"));
inline const FName DisplayedTargetName(TEXT("RailgunDisplayedTargetName"));
inline const FName DisplayedTargetRange(TEXT("RailgunDisplayedTargetRange"));
inline const FName DisplayInitialized(TEXT("RailgunRangeDisplayInitialized"));
inline const FName TargetNameFontObject(TEXT("RailgunTargetNameFontObject"));
inline const FName TargetDistanceFontObject(
    TEXT("RailgunTargetDistanceFontObject"));
inline constexpr TCHAR TargetNameOffsetX[] = TEXT("0.0");
inline constexpr TCHAR TargetNameOffsetY[] = TEXT("330.0");
inline constexpr TCHAR TargetNameOpacity[] = TEXT("0.7");
inline constexpr TCHAR TargetNameFontSize[] = TEXT("20");
inline constexpr TCHAR TargetNameFontPath[] = TEXT(
    "/Game/UI/Terminal/Fonts/ShareTech/ShareTechMono-Regular_Font.ShareTechMono-Regular_Font");
inline constexpr TCHAR TargetNameTypeface[] = TEXT("Bold");
inline constexpr TCHAR TargetDistanceOffsetX[] = TEXT("0.0");
inline constexpr TCHAR TargetDistanceOffsetY[] = TEXT("185.0");
inline constexpr TCHAR TargetDistanceOpacity[] = TEXT("0.7");
inline constexpr TCHAR TargetDistanceFontSize[] = TEXT("25");
inline constexpr TCHAR TargetDistanceFontPath[] = TEXT(
    "/Game/UI/Terminal/Fonts/DSEG/DSEG7Classic-Bold_Font.DSEG7Classic-Bold_Font");
inline constexpr TCHAR TargetDistanceTypeface[] = TEXT("Bold");
inline constexpr TCHAR MaximumCentimeters[] = TEXT("100000.0");
inline constexpr TCHAR MaximumDisplayedMeters[] = TEXT("999.0");
inline constexpr TCHAR CentimetersPerMeter[] = TEXT("100.0");
inline constexpr TCHAR NoHitLabel[] = TEXT("---");
inline constexpr TCHAR IntegralDigits[] = TEXT("3");
inline const FName UseGroupingPin(TEXT("bUseGrouping"));
inline const FName MinimumIntegralDigitsPin(TEXT("MinimumIntegralDigits"));
inline const FName MaximumIntegralDigitsPin(TEXT("MaximumIntegralDigits"));
}
namespace Range = StationRangeNames;

UEdGraphPin* RangeLiteralText(FGraph& G, const TCHAR* Value);

UEdGraphPin* EqualRangeText(FGraph& G, UEdGraphPin* Left,
    UEdGraphPin* Right);

void WriteRangeTextIfChanged(FGraph& G, FName Field, UEdGraphPin* Value);

void ClearStationRangeDisplay(FGraph& G);

void UpdateStationRange(FGraph& G);
}
