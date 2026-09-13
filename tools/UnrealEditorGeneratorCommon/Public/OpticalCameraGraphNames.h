#pragma once
#include "UObject/NameTypes.h"

namespace OpticalCameraGraphNames
{
inline const FName FieldOfView(TEXT("InFieldOfView"));
inline const FName NewViewTarget(TEXT("NewViewTarget"));
inline const FName BlendTime(TEXT("BlendTime"));
inline const FName Sweep(TEXT("bSweep"));
inline const FName Visibility(TEXT("InVisibility"));
inline const FName Hidden(TEXT("bNewHidden"));
// Unary Kismet trig input. Consumers use its semantic role, not raw A.
inline const FName AngleValue(TEXT("A"));
inline const FName MouseHorizontal(TEXT("DeltaX"));
// Overloaded native API cannot use GET_FUNCTION_NAME_CHECKED in UE5.8.
inline const FName GetMouseDelta(TEXT("GetInputMouseDelta"));
inline const FName MouseVertical(TEXT("DeltaY"));
inline const FName RelativeLocation(TEXT("NewRelativeLocation"));
inline const FName RelativeRotation(TEXT("NewRelativeRotation"));
inline const FName NormalizeAngle(TEXT("Angle"));
inline const FName ClampValue(TEXT("Value"));
inline const FName ClampMinimum(TEXT("Min"));
inline const FName ClampMaximum(TEXT("Max"));
inline const FName ComponentClass(TEXT("ComponentClass"));
inline const FName FindComponent(TEXT("GetComponentByClass"));
inline const FName ComponentOwner(TEXT("GetOwner"));
inline const FName ImpactPoint(TEXT("ImpactPoint"));
inline const FName SoftClassReference(TEXT("SoftClass"));
inline const FName WidgetClass(TEXT("WidgetClass"));
inline const FName FoundWidgets(TEXT("FoundWidgets"));
inline const FName TopLevelOnly(TEXT("TopLevelOnly"));
inline const FName OwningPlayerGetter(TEXT("GetOwningPlayer"));
inline const FName IntegerValue(TEXT("InInt"));
inline const FName StringSuffix(TEXT("Suffix"));
}
