// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 23962331
// (Unreal Engine 5.7.4; VoyageSteam-Win64-Shipping.exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D).
// Revalidate with ../../../../tools/Get-VoyageBuildFingerprint.ps1, current
// mappings, and cooked stock interface exports after a game update.
// Editor-only: never package this native definition.

#pragma once

#include "CoreMinimal.h"
#include "PlayerInputInterfaceAction.generated.h"

// Signature-only stand-in for /Script/Voyage.PlayerInputInterfaceAction.
// Current mappings expose 23 reflected fields. Stock cooked interface exports
// record an element size of 0x290 bytes. The Harpoon no-op interface marker
// never reads, writes, constructs, or serializes an action value, so retaining
// only the exact reflected identity, native size, and alignment is safer than
// inventing delegate signatures for the six mapped delegate-tail fields.
USTRUCT(BlueprintType)
struct VOYAGE_API FPlayerInputInterfaceAction
{
    GENERATED_BODY()

private:
    uint64 OpaqueSignatureStorage[82];
};

static_assert(sizeof(FPlayerInputInterfaceAction) == 0x290,
    "FPlayerInputInterfaceAction signature stand-in must match current cooked element size");
