// Editor-only /Script/Voyage identity from reviewed Steam 25982825 mappings, UE 5.8.
// Revalidate when the game schema changes. Never shipped as native code.
#pragma once
#include "CoreMinimal.h"
#include "VoyageItemResourceType.generated.h"

UENUM()
enum class EModuleResourceType : uint8
{
    Electricity,
    Petrol,
    Oil,
    SaltWater,
    FreshWater,
    PhysicalEnergy,
    Data,
    Custom1,
    Custom2,
    Custom3,
    Methane,
    Oxygen,
    Steam,
    Worker,
    Unknown,
    Mooring,
    Diesel,
    CarbonDioxide,
    Count
};
