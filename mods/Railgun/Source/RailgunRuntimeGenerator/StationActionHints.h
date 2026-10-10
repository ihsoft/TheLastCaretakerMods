#pragma once
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// HC28: provider -> unchanged stock hint widget in OWN HUD. No input/physics edit.
namespace Hint
{
inline const FName HintInstance(TEXT("RailgunActionHints"));
inline const FName InvalidationRoot(TEXT("RailgunHudInvalidationRoot"));
inline const FName InputAction(TEXT("InputAction"));
inline const FName Name(TEXT("Name"));
inline const FName Category(TEXT("Category"));
inline const FName Text(TEXT("Text"));
inline const FName Enabled(TEXT("bEnabled"));
inline const FName Type(TEXT("Type"));
inline constexpr TCHAR StockWidgetPackage[] =
    TEXT("/Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget");
inline constexpr TCHAR StockWidgetAsset[] =
    TEXT("BP_DynamicPlayerInputHorizontalWidget");
inline constexpr TCHAR StockWidgetClass[] =
    TEXT("BP_DynamicPlayerInputHorizontalWidget_C");
inline constexpr TCHAR StockWidget[] =
    TEXT("/Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget.BP_DynamicPlayerInputHorizontalWidget_C");
inline constexpr TCHAR ActionName[] = TEXT("RailgunExit");
inline constexpr TCHAR ActionCategory[] = TEXT("Railgun");
inline constexpr TCHAR ZoomName[] = TEXT("RailgunZoom");
inline constexpr TCHAR FireName[] = TEXT("RailgunFire");
inline constexpr TCHAR ReloadName[] = TEXT("RailgunReload");
inline constexpr TCHAR Central[] = TEXT("EPlayerInputInterfaceActionType::Central");
}

void AddStationActions(UBlueprint* BP);
}
