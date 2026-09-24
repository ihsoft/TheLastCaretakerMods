#pragma once

#include "CoreMinimal.h"

namespace GyroKeyboardSettings
{
inline constexpr TCHAR RelativePath[] = TEXT("Paks/GyroKeyboardControl.ini");
inline constexpr TCHAR Separator[] = TEXT("=");

struct FNumericSetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
    const TCHAR* Minimum;
    const TCHAR* Maximum;
};

struct FBooleanSetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
};

#include "GyroKeyboardControlSettings.generated.h"
}
