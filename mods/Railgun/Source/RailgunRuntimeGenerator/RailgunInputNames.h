#pragma once
#include "CoreMinimal.h"
namespace RailgunInputNames
{
inline constexpr TCHAR LookYaw[] = TEXT("/Game/Mods/Railgun/Inputs/IA_RailgunLookYaw");
inline constexpr TCHAR LookPitch[] = TEXT("/Game/Mods/Railgun/Inputs/IA_RailgunLookPitch");
inline constexpr TCHAR Exit[] = TEXT("/Game/Mods/Railgun/Inputs/IA_RailgunExit");
inline constexpr TCHAR Zoom[] = TEXT("/Game/Mods/Railgun/Inputs/IA_RailgunZoom");
inline constexpr TCHAR Fire[] = TEXT("/Game/Mods/Railgun/Inputs/IA_RailgunFire");
inline constexpr TCHAR Keyboard[] = TEXT("/Game/Mods/Railgun/Inputs/IMC_RailgunKeyboard");
inline constexpr TCHAR Context[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunInputContext");
inline constexpr int32 ContextPriority = 10;
inline constexpr int32 KeyboardMappingCount = 5;
inline constexpr int32 ContextPropertyCount = 7;
}
