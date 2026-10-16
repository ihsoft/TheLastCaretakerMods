#pragma once
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace StockInput
{
inline const FName CharacterContext(TEXT("RailgunCharacterInputContext"));
inline const FName ExitContext(TEXT("RailgunExitInputContext"));
inline const FName ServiceItem(TEXT("RailgunADSServiceItem"));
inline const FName MappingSnapshot(TEXT("RailgunInputMappingSnapshot"));
inline const FName Ready(TEXT("RailgunStockInputReady"));
inline const FName FailureStage(TEXT("RailgunStockInputFailureStage"));
inline const FName CharacterKeyboard(TEXT("RailgunCharacterKeyboardSource"));
inline const FName CharacterGamepad(TEXT("RailgunCharacterGamepadSource"));
inline const FName ExitKeyboard(TEXT("RailgunExitKeyboardSource"));
inline const FName ExitGamepad(TEXT("RailgunExitGamepadSource"));
inline const FName ExitControls(TEXT("RailgunExitInputControls"));
inline const FName Prepare(TEXT("PrepareRailgunStockInput"));
inline const FName ContextMap(TEXT("InputMappingContext"));
inline const FName SwitchGamepadContext(TEXT("SwitchGamepadMappingContext"));
inline const FName DefaultMappings(TEXT("DefaultKeyMappings"));
inline const FName ProfileOverrides(TEXT("MappingProfileOverrides"));
inline const FName Mappings(TEXT("Mappings"));
inline const FName TargetMap(TEXT("TargetMap"));
inline const FName Key(TEXT("Key"));
inline const FName Value(TEXT("Value"));
inline const FName Object(TEXT("Obj"));
inline const FName Outer(TEXT("Outer"));
inline const FName Action(TEXT("Action"));
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR CloneStage[] = TEXT("Railgun input initialization failed while cloning stock contexts");
inline constexpr TCHAR SourceStage[] = TEXT("Railgun input initialization failed while capturing live stock device mappings");
inline constexpr TCHAR FilterStage[] = TEXT("Railgun input initialization failed while filtering cloned stock mappings");
inline constexpr TCHAR NoFailure[] = TEXT("");
inline constexpr TCHAR MouseKeyboard[] = TEXT("EInputDeviceType::MouseKeyboard");
inline constexpr TCHAR Gamepad[] = TEXT("EInputDeviceType::Gamepad");
}

void AddRailgunStockInputPreparation(UBlueprint* BP);
void AddRailgunStockInputComponents(UBlueprint* BP);
}
