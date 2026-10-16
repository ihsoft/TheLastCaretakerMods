#pragma once
// Tagged-data/native enum mirror. Steam25956314 / UE5.8, EXE
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A,
// mapping 1B777468894C071262FE5821DAFC1B509EFD5B8481213CC197C1B4B74177542E.
// Reconstructed from mapped package fields and focused native inspection.
// Revalidate when the game fingerprint, reviewed mapping, or holster enum or
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
