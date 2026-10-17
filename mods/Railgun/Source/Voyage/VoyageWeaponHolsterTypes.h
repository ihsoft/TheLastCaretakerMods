#pragma once
// Tagged-data/native enum mirror. Steam25956314 / UE5.8.
// mapping 1B777468894C071262FE5821DAFC1B509EFD5B8481213CC197C1B4B74177542E.
// Reconstructed from mapped package fields and focused native inspection.
// Revalidate when the Steam build ID, reviewed mapping, or holster enum or
// property layout changes.
#include "CoreMinimal.h"
#include "VoyageWeaponHolsterTypes.generated.h"

UENUM(BlueprintType)
enum class EVoyageHolsterSlotType : uint8
{
    Any = 0,
    Primary = 1,
    Secondary = 2,
    Throw = 3,
    Consumable = 4,
    Overlay = 5
};
