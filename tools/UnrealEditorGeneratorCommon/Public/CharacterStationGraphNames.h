#pragma once
#include "UObject/NameTypes.h"

// Engine-reflected parameter roles for reversible character station experiments.
namespace CharacterStationGraphNames
{
inline const FName NewMovementMode(TEXT("NewMovementMode"));
inline const FName CollisionEnabled(TEXT("bNewActorEnableCollision"));
inline const FName RotationValue(TEXT("InRot"));
inline const FName Pitch(TEXT("Pitch"));
inline const FName Roll(TEXT("Roll"));
inline const FName Yaw(TEXT("Yaw"));
inline const FName NewRotation(TEXT("NewRotation"));
inline const FName TeleportPhysics(TEXT("bTeleportPhysics"));
inline const FName HitActor(TEXT("HitActor"));
inline const FName SoftClassPathInput(TEXT("InSoftClassPath"));
inline const FName PossessionTarget(TEXT("InPawn"));
}
