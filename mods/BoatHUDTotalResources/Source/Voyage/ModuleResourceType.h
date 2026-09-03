// EDITOR-ONLY IDENTITY MIRROR for Steam build 25056839 (UE 5.8.1).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "ModuleResourceType.generated.h"

UENUM(BlueprintType)
enum class EModuleResourceType : uint8
{
    Electricity = 0,
    Petrol = 1,
    Oil = 2,
    SaltWater = 3,
    FreshWater = 4,
    PhysicalEnergy = 5,
    Data = 6,
    Custom1 = 7,
    Custom2 = 8,
    Custom3 = 9,
    Methane = 10,
    Oxygen = 11,
    Steam = 12,
    Worker = 13,
    Unknown = 14,
    Mooring = 15,
    Diesel = 16,
    CarbonDioxide = 17,
    Count = 18,
    EModuleResourceType_MAX = 19 UMETA(Hidden)
};
