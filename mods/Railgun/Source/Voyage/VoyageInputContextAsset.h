#pragma once
// Eight-property serialization mirror. Steam25956314 / UE5.8.
// Current reviewed mapping plus stock context JSON confirms property
// order/types. ../../ARCHITECTURE.md owns the runtime contract and limits.
// Editor-only, no runtime DLL. Revalidate complete schema on Steam build ID change.
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
    UPROPERTY(BlueprintReadOnly)
    TMap<EInputDeviceType, TObjectPtr<UInputMappingContext>> InputMappingContext;
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UInputMappingContext> SwitchGamepadMappingContext;
    UPROPERTY() EInputControlContextType ContextType = EInputControlContextType::Default;
    UPROPERTY() EInputControlBindType BindType = EInputControlBindType::Default;
    UPROPERTY() int32 InputPriorityOffset = 0;
    UPROPERTY() TObjectPtr<UVoyageInputAction> ShiftAction;
    UPROPERTY() TObjectPtr<UVoyageInputAction> CtrlAction;
    UPROPERTY() TObjectPtr<UVoyageInputAction> AltAction;
};
