#pragma once
// TYPE/FIELD-REFERENCE ONLY mirror, never a cooked parent/CDO. Steam25191271, UE5.8,
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
    // Revalidate native descriptor/type and stock Forklift CDO on Steam build ID change.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UVoyageInputAction> ExitAction;
    // Current mapping[20,21]; stock Forklift CDO references Engine InputActions.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UInputAction> LookRightInputAction;
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UInputAction> LookUpInputAction;
};
