#pragma once

#include "UObject/NameTypes.h"

// Stable reflected names used while constructing Blueprint graphs in editor
// commandlets. This header is build-time only and is never shipped in a mod.
namespace BlueprintGraphNames::MathFunctions
{
inline const FName AddDouble(TEXT("Add_DoubleDouble"));
inline const FName ClampFloat(TEXT("FClamp"));
inline const FName EqualDouble(TEXT("EqualEqual_DoubleDouble"));
inline const FName LessDouble(TEXT("Less_DoubleDouble"));
inline const FName MultiplyDouble(TEXT("Multiply_DoubleDouble"));
inline const FName SelectFloat(TEXT("SelectFloat"));
inline const FName SubtractDouble(TEXT("Subtract_DoubleDouble"));
}

namespace BlueprintGraphNames::WidgetFunctions
{
// UPanelWidget::AddChild is overloaded, so GET_FUNCTION_NAME_CHECKED cannot
// disambiguate it without an explicit reflected name.
inline const FName AddChild(TEXT("AddChild"));
}

namespace BlueprintGraphNames::Events
{
inline const FName ActorReceiveTick(TEXT("ReceiveTick"));
inline const FName WidgetConstruct(TEXT("Construct"));
}

namespace BlueprintGraphNames::EngineAssets
{
inline constexpr TCHAR StandardMacrosObjectPath[] =
    TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
}

namespace BlueprintGraphNames::Detail
{
inline const FName PinA(TEXT("A"));
inline const FName PinB(TEXT("B"));
inline const FName PinPickA(TEXT("bPickA"));
}

namespace BlueprintGraphNames::Pins::Binary
{
inline const FName& LeftOperand = Detail::PinA;
inline const FName& RightOperand = Detail::PinB;
}

namespace BlueprintGraphNames::Pins::Select
{
inline const FName& WhenTrue = Detail::PinA;
inline const FName& WhenFalse = Detail::PinB;
inline const FName& Condition = Detail::PinPickA;
}

namespace BlueprintGraphNames::Pins
{
inline const FName Completed(TEXT("Completed"));
inline const FName Content(TEXT("Content"));
inline const FName DeltaSeconds(TEXT("DeltaSeconds"));
inline const FName Duration(TEXT("Duration"));
inline const FName FirstArrayElement(TEXT("[0]"));
inline const FName FirstIndex(TEXT("FirstIndex"));
inline const FName Index(TEXT("Index"));
inline const FName ItemToFind(TEXT("ItemToFind"));
inline const FName Key(TEXT("Key"));
inline const FName LastIndex(TEXT("LastIndex"));
inline const FName LoopBody(TEXT("LoopBody"));
inline const FName Max(TEXT("Max"));
inline const FName Min(TEXT("Min"));
inline const FName PlayerIndex(TEXT("PlayerIndex"));
inline const FName ReturnValue(TEXT("ReturnValue"));
inline const FName SecondArrayElement(TEXT("[1]"));
inline const FName TargetArray(TEXT("TargetArray"));
inline const FName Value(TEXT("Value"));
}

namespace BlueprintGraphNames::Macros
{
inline const FName ForLoop(TEXT("ForLoop"));
}
