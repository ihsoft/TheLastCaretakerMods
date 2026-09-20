// Editor-only reflection identity, Steam25191271 / UE5.8.
// EXE SHA256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Reconstructed with Inspect-VoyageAsset mappings-enum:EModuleResourceType.
// Revalidate after a fingerprint change; never ship this module.
#pragma once
#include "CoreMinimal.h"
#include "ModuleResourceType.generated.h"

UENUM()
enum class EModuleResourceType : uint8
{
    Electricity, Petrol, Oil, SaltWater, FreshWater, PhysicalEnergy, Data,
    Custom1, Custom2, Custom3, Methane, Oxygen, Steam, Worker, Unknown,
    Mooring, Diesel, CarbonDioxide, Count
};
