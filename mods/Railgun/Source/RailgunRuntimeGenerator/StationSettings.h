#pragma once
#include "RailgunShot.h"
#include "StationEnergy.h"
#include "StationEnergyHud.h"
#include "StationRangeGraph.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace Settings
{
inline const FName FilePath(TEXT("InPath")); // Voyage declaring signature.
inline constexpr TCHAR RelativePath[] = TEXT("Paks/Railgun.ini");
inline constexpr TCHAR Separator[] = TEXT("=");
inline constexpr TCHAR OriginalPerPercent[] = TEXT("0.0128");
inline constexpr TCHAR Negate[] = TEXT("-1.0");
inline constexpr TCHAR DisabledRecoilFallback[] = TEXT("0");
inline const FName FontObjectPin(TEXT("FontObject"));
inline const FName TypefaceFontNamePin(TEXT("TypefaceFontName"));
inline const FName FontSizePin(TEXT("Size"));
inline const FName FontInfoPin(TEXT("InFontInfo"));
inline const FName TranslationPin(TEXT("Translation"));
inline const FName OpacityPin(TEXT("InOpacity"));
inline const FName DisplayFontSizePin(TEXT("DisplayFontSize"));
inline const FName AssetPin(TEXT("Asset"));
inline const FName SoftObjectPathPin(TEXT("SoftObjectPath"));
inline const FName XPin(TEXT("X"));
inline const FName YPin(TEXT("Y"));
inline const FName MakeSlateFontInfoFunction(TEXT("MakeSlateFontInfo"));

struct FNumericSetting
{
    const TCHAR* Key;
    FName Field;
    const TCHAR* Default;
    const TCHAR* Minimum;
    const TCHAR* Maximum;
};

struct FFixedFont
{
    const TCHAR* ObjectPath;
    FName ObjectField;
};

inline const FFixedFont FixedHudFonts[] = {
    {Range::TargetNameFontPath, Range::TargetNameFontObject},
    {Range::TargetDistanceFontPath, Range::TargetDistanceFontObject},
    {EnergyHud::ChargeTextFontPath, EnergyHud::ChargeTextFontObject},
};

#include "StationSettings.generated.h"

const TCHAR* RuntimeFallback(const FNumericSetting& Setting);
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

UEdGraphPin* TrimSetting(FGraph& G, UEdGraphPin* Text);

UEdGraphPin* ClampStationAim(FGraph& G, bool Horizontal, UEdGraphPin* Value);

void ReadStationSettings(FGraph& G);
}
