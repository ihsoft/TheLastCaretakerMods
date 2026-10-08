#pragma once
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// HC28: provider -> unchanged stock hint widget in OWN HUD. No input/physics edit.
namespace Hint
{
inline const FName HintsReady(TEXT("RailgunHintWidgetReady"));
inline const FName HintInstance(TEXT("RailgunActionHints"));
inline const FName Root(TEXT("RailgunHintHost"));
inline const FName Status(TEXT("RailgunHintStatus"));
inline const FName InputAction(TEXT("InputAction"));
inline const FName Name(TEXT("Name"));
inline const FName Category(TEXT("Category"));
inline const FName Text(TEXT("Text"));
inline const FName Enabled(TEXT("bEnabled"));
inline const FName Type(TEXT("Type"));
inline const FName WidgetType(TEXT("WidgetType"));
inline const FName OwningPlayer(TEXT("OwningPlayer"));
inline const FName Content(TEXT("Content"));
inline constexpr TCHAR StockWidget[] = TEXT("/Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget.BP_DynamicPlayerInputHorizontalWidget_C");
inline constexpr TCHAR ActionName[] = TEXT("RailgunExit");
inline constexpr TCHAR ActionCategory[] = TEXT("Railgun");
inline constexpr TCHAR ExitLabel[] = TEXT("Exit Railgun");
inline constexpr TCHAR ZoomName[] = TEXT("RailgunZoom");
inline constexpr TCHAR ZoomLabel[] = TEXT("Toggle scope");
inline constexpr TCHAR FireName[] = TEXT("RailgunFire");
inline constexpr TCHAR FireLabel[] = TEXT("Fire");
inline constexpr TCHAR Central[] = TEXT("EPlayerInputInterfaceActionType::Central");
}

void AddStationActions(UBlueprint* BP);

// UMG Construct occurs when attached: configure context before AddChild.
void AddStationHintConstruction(UWidgetBlueprint* Hud,
    UEdGraphPin* ConstructTail = nullptr);
}
