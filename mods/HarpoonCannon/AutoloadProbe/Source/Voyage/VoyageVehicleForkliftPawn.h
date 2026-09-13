#pragma once
// TYPE/FIELD-REFERENCE ONLY mirror, never a cooked parent/CDO. Steam25191271, UE5.8,
// EXE747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Native ctor145409680 calls VehiclePawn ctor1454099f0. HC21 imports this
// identity and spawns the shipping class; no stock Forklift Blueprint is used.
#include "VoyageVehiclePawn.h"
#include "VoyageInputAction.h"
#include "VoyageVehicleForkliftPawn.generated.h"
UCLASS(BlueprintType)
class VOYAGE_API AVoyageVehicleForkliftPawn : public AVoyageVehiclePawn
{
    GENERATED_BODY()
public:
    // Native descriptor149b5b0d0, actor+0x558, VoyageInputAction ObjectProperty.
    // HC21 crash analysis: native action provider dereferences this without a guard.
    // Revalidate native descriptor/type and stock Forklift CDO on fingerprint change.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UVoyageInputAction> ExitAction;
    // Current mapping[20,21]; stock Forklift CDO references Engine InputActions.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UInputAction> LookRightInputAction;
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UInputAction> LookUpInputAction;
};
