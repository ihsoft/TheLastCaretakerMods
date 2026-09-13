#pragma once
// Seven-property serialization mirror. Steam25191271 / UE5.8, EXE
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// HC25: public mapping plus stock context JSON confirms property order/types;
// reviewed mapping EnumName values recovered through published CUE4Parse after
// Inspector Describe omitted enum names. GAME_DERIVED_SOURCES.md owns evidence.
// Editor-only, no runtime DLL. Revalidate complete schema on fingerprint change.
#include "Engine/DataAsset.h"
#include "InputMappingContext.h"
#include "VoyageInputAction.h"
#include "VoyageInputContextAsset.generated.h"

UENUM(BlueprintType)
enum class EInputDeviceType : uint8 { Unknown = 0, MouseKeyboard = 1, Gamepad = 2 };
UENUM(BlueprintType)
enum class EInputControlContextType : uint8 { Default = 0, Interface = 1, System = 2 };
UENUM(BlueprintType)
enum class EInputControlBindType : uint8 { Default = 0, Elevated = 1, Demoted = 2, Ignore = 3 };

UCLASS(BlueprintType)
class VOYAGE_API UVoyageInputContextAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY() TMap<EInputDeviceType, TObjectPtr<UInputMappingContext>> InputMappingContext;
    UPROPERTY() EInputControlContextType ContextType = EInputControlContextType::Default;
    UPROPERTY() EInputControlBindType BindType = EInputControlBindType::Default;
    UPROPERTY() int32 InputPriorityOffset = 0;
    UPROPERTY() TObjectPtr<UVoyageInputAction> ShiftAction;
    UPROPERTY() TObjectPtr<UVoyageInputAction> CtrlAction;
    UPROPERTY() TObjectPtr<UVoyageInputAction> AltAction;
};
