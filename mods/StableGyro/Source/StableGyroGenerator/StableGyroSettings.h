#pragma once

#include "CoreMinimal.h"

namespace StableGyroSettings
{
inline constexpr TCHAR RelativePath[] = TEXT("Paks/StableGyro.ini");
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

#include "StableGyroSettings.generated.h"
}
