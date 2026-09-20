#pragma once
// Editor-only reflection identities, Steam25191271 / UE5.8, EXE
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Public enum mappings + RemoveResource native thunk; revalidate on update.
#include "CoreMinimal.h"
#include "ModuleResourceType.generated.h"
UENUM(BlueprintType)
enum class EModuleResourceType : uint8
{
    Electricity, Petrol, Oil, SaltWater, FreshWater, PhysicalEnergy, Data,
    Custom1, Custom2, Custom3, Methane, Oxygen, Steam, Worker, Unknown,
    Mooring, Diesel, CarbonDioxide, Count
};
UENUM(BlueprintType)
enum class EModuleResourceRemovalType : uint8
{
    Consumption, ConsumptionAfterModifiers, Transfer, Overflow, Conversion,
    Leak, EfficiencyChange, Count
};
