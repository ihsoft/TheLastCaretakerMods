#pragma once
#include "CoreMinimal.h"
namespace RailgunInputNames
{
inline constexpr TCHAR LookYaw[] = TEXT("/Game/Game/Input/Character/IA_LookRight.IA_LookRight");
inline constexpr TCHAR LookPitch[] = TEXT("/Game/Game/Input/Character/IA_LookUp.IA_LookUp");
inline constexpr TCHAR Exit[] = TEXT("/Game/Game/Input/Vehicle/IAV_VehicleExit.IAV_VehicleExit");
inline constexpr TCHAR Zoom[] = TEXT("/Game/Game/Input/Character/IAV_ADS.IAV_ADS");
inline constexpr TCHAR Fire[] = TEXT("/Game/Game/Input/Character/IA_PrimaryAction.IA_PrimaryAction");
inline constexpr TCHAR Reload[] = TEXT("/Game/Game/Input/Character/IA_Reload.IA_Reload");
inline constexpr TCHAR CharacterContext[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunCharacterContext");
inline constexpr TCHAR ExitContext[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunExitContext");
inline constexpr TCHAR ServiceEquipment[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunADSServiceEquipment");
inline constexpr TCHAR ServiceEquipmentActor[] = TEXT("/Game/Mods/Railgun/Inputs/BP_RailgunADSServiceEquipment");
inline constexpr TCHAR CharacterContextObject[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunCharacterContext.DA_RailgunCharacterContext");
inline constexpr TCHAR ExitContextObject[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunExitContext.DA_RailgunExitContext");
inline constexpr TCHAR ServiceEquipmentObject[] = TEXT("/Game/Mods/Railgun/Inputs/DA_RailgunADSServiceEquipment.DA_RailgunADSServiceEquipment");
inline constexpr TCHAR ServiceEquipmentActorClass[] = TEXT("/Game/Mods/Railgun/Inputs/BP_RailgunADSServiceEquipment.BP_RailgunADSServiceEquipment_C");
inline constexpr TCHAR StockCharacterContext[] = TEXT("/Game/Game/Input/Character/DA_Input_Context_Character.DA_Input_Context_Character");
inline constexpr TCHAR StockVehicleContext[] = TEXT("/Game/Game/Input/Vehicle/DA_Input_Context_Vehicle.DA_Input_Context_Vehicle");
inline constexpr TCHAR ObjectPathSeparator[] = TEXT(".");
inline constexpr TCHAR LookYawDescription[] = TEXT("Look right / left");
inline constexpr TCHAR LookPitchDescription[] = TEXT("Look up / down");
inline constexpr TCHAR FireDescription[] = TEXT("Fire");
inline constexpr TCHAR ReloadDescription[] = TEXT("Reload");
inline constexpr TCHAR ZoomDescription[] = TEXT("Aim");
inline constexpr TCHAR ExitDescription[] = TEXT("Exit");
inline constexpr TCHAR ItemPrimaryAssetType[] = TEXT("Item");
inline constexpr int32 ContextPriority = 10;
inline constexpr int32 ContextPropertyCount = 8;
}
