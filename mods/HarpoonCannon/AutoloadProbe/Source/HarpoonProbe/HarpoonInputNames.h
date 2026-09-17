#pragma once
#include "CoreMinimal.h"
namespace HarpoonInputNames
{
inline constexpr TCHAR LookYaw[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookYaw");
inline constexpr TCHAR LookPitch[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookPitch");
inline constexpr TCHAR Exit[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonExit");
inline constexpr TCHAR Zoom[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonZoom");
inline constexpr TCHAR Fire[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/IA_HarpoonFire");
inline constexpr TCHAR Keyboard[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/IMC_HarpoonKeyboard");
inline constexpr TCHAR Context[] = TEXT("/Game/Mods/HarpoonCannon/Inputs/DA_HarpoonInputContext");
inline constexpr int32 ContextPriority = 10;
inline constexpr int32 KeyboardMappingCount = 5;
inline constexpr int32 ContextPropertyCount = 7;
}
