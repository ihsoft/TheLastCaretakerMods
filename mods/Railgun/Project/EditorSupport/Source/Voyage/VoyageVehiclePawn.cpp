#include "VoyageVehiclePawn.h"

AVoyageVehiclePawn::AVoyageVehiclePawn()
{
    static const FName NativeRootName(TEXT("VehicleMesh"));
    SetRootComponent(
        CreateDefaultSubobject<UVoyageFastSceneComponent>(NativeRootName));
}
