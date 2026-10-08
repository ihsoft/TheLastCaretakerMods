#pragma once
#include "../../RailgunModelContract.h"

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds the dedicated stationary railgun pawn and its HUD, input, and model graphs.
namespace ZoomTest
{
inline const FName Wide(TEXT("RailgunWideView"));
inline const FName Mouse(TEXT("RailgunActiveMousePercent"));
inline const FName Mask(TEXT("RailgunOpticalMask"));
inline const FName MaskImage(TEXT("RailgunOpticalMaskImage"));
inline constexpr TCHAR MaskPackage[] = TEXT("/Game/Mods/Railgun/Station/T_RailgunOpticalMask");
inline constexpr TCHAR MaskAsset[] = TEXT("T_RailgunOpticalMask");
inline constexpr TCHAR OverlaySourceArgument[] = TEXT("ScopeOverlay=");
inline constexpr TCHAR NormalMouse[] = TEXT("100.0");
inline constexpr TCHAR Hidden[] = TEXT("Collapsed");
inline constexpr TCHAR Shown[] = TEXT("HitTestInvisible");
inline const FName WideCenter(TEXT("RailgunWideCenter"));
inline constexpr TCHAR WideCenterText[] = TEXT("\u25CB");
inline UTexture2D* OverlayTexture = nullptr;
}
// HC26: authored child of COMMON transport. TAGGED cook required, no Forklift.
// Included after graph helpers. Native entry/exit remains unmodified.

bool SaveDedicatedAsset(UObject* Asset);

UTexture2D* ImportUiTexture(const FString& Filename, const TCHAR* PackageName,
    const TCHAR* AssetName, bool RequireSquare);

void DedicatedViewTarget(FGraph& G, UEdGraphPin* Target);

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void AddRailgunAimBindingFunctions(UBlueprint* BP);

void AimModelPivot(FGraph& G, FName Component, FName Angle, FName Axis);

void DedicatedAim(FGraph& G);


void ApplyRailgunAimRecoil(FGraph& G);

void BuildDedicatedStationGraph(UBlueprint* BP);

UClass* CreateDedicatedStation();
}
