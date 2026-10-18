#pragma once

#include "CoreMinimal.h"
#include "VoyageModSettingsTypes.h"

namespace StableGyroSettings
{
inline constexpr TCHAR RelativePath[] = TEXT("Paks/StableGyro.ini");
inline constexpr TCHAR Separator[] = TEXT("=");

using VoyageModSettings::ENumberStorage;
using VoyageModSettings::ESettingType;
using VoyageModSettings::FSetting;

#include "StableGyroSettings.generated.h"
}
