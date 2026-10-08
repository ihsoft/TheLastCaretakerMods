#pragma once
#include "../../RailgunModelContract.h"
#include "RailgunInventoryNames.h"
#include "VoyageBaseInventoryComponent.h"
#include "VoyageModuleComponent.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Native swept projectile with a single direct-hit combat submission.
namespace Shot
{
inline constexpr TCHAR Package[] = TEXT("/Game/Mods/Railgun/Station/BP_RailgunTestShot");
inline constexpr TCHAR Speed[] = TEXT("200000.0"), Range[] = TEXT("99999.0"), Life[] = TEXT("4.0");
inline const FName Body(TEXT("ShotCollision")), Move(TEXT("ShotMovement"));
inline const FName Start(TEXT("ShotOrigin")), Direction(TEXT("ShotDirection")), Done(TEXT("ShotDone"));
inline const FName SpawnedThisPress(TEXT("ShotSpawnedThisPress"));
inline const FName AmmoSlot(TEXT("ShotAmmoSlot"));
inline const FName SpawnLocation(TEXT("ShotSpawnLocation"));
inline const FName SpawnRotation(TEXT("ShotSpawnRotation"));
inline const FName Railgun(TEXT("ShotRailgun")), Operator(TEXT("ShotOperator")), Station(TEXT("ShotStation"));
inline const FName LifePin(TEXT("InLifespan")), HitEvent(TEXT("ReceiveHit"));
inline const FName Other(TEXT("Other")), Velocity(TEXT("Velocity")), Sweep(TEXT("bSweepCollision"));
inline const FName Penetration(TEXT("bAllowPenetration")), Ricochet(TEXT("bAllowRicochet"));
inline const FName Updated(TEXT("NewUpdatedComponent")), Reset(TEXT("bReset"));
inline const FName IgnoreActor(TEXT("Actor")), ShouldIgnore(TEXT("bShouldIgnore"));
inline const FName Prerequisite(TEXT("PrerequisiteActor")), Transform(TEXT("T"));
inline const FName Location(TEXT("Location")), ActorRotation(TEXT("Rotation"));
inline const FName Slot(TEXT("Slot")), OutItemData(TEXT("OutItemData"));
inline const FName Item(TEXT("Item")), Data(TEXT("Data")), ItemCount(TEXT("ItemCount"));
inline const FName Count(TEXT("Count")), PreferredSlot(TEXT("PreferredSlot"));
inline const FName Notify(TEXT("bNotify"));
inline constexpr TCHAR One[] = TEXT("1");
inline constexpr TCHAR NoSlot[] = TEXT("-1");
inline UClass* Class=nullptr;
}
namespace ShotAttack
{
inline const FName Controller(TEXT("ShotController")), DamageClass(TEXT("ShotDamageType"));
inline const FName ConfiguredDamage(TEXT("RailgunHitDamage")), DamageAmount(TEXT("ShotDamageAmount"));
inline const FName GetController(TEXT("GetController")); // APawn also has a templated C++ overload.
inline const FName Damage(TEXT("Damage")), Variance(TEXT("DamageVariance")), Impulse(TEXT("ImpulseOverride"));
inline const FName Type(TEXT("AttackType")), Hit(TEXT("Hit")), Target(TEXT("Target"));
inline const FName Instigator(TEXT("Instigator")), Causer(TEXT("DamageCauser")), Id(TEXT("AttackID"));
inline const FName TypeClass(TEXT("DamageTypeClass")), Attack(TEXT("Attack")), Duration(TEXT("bAcceptDuration"));
inline const FName ClassPin(TEXT("Class")), Context(TEXT("ContextObject"));
inline constexpr TCHAR PhysicalType[] = TEXT("/Game/Blueprints/DamageTypes/BP_DamageTypePhysicalForce.BP_DamageTypePhysicalForce_C");
inline constexpr TCHAR Directional[] = TEXT("Directional");
}
namespace ShotAudio
{
inline constexpr TCHAR Package[] = TEXT("/Game/Mods/Railgun/Station/S_RailgunShotBlast");
inline constexpr TCHAR Asset[] = TEXT("S_RailgunShotBlast");
inline constexpr TCHAR SourceArgument[] = TEXT("ShotSound=");
inline const FName PlayAtLocation(TEXT("PlaySoundAtLocation"));
inline const FName SoundPin(TEXT("Sound"));
inline const FName VolumePercent(TEXT("RailgunShotVolumePercent"));
inline const FName VolumeMultiplierPin(TEXT("VolumeMultiplier"));
inline constexpr TCHAR PercentMultiplier[] = TEXT("0.01");
inline USoundWave* Wave = nullptr;
}
bool SaveDedicatedAsset(UObject* Asset);
UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void DestroyDeferredShot(FGraph& G, UEdGraphPin* Actor);

USoundWave* ImportShotSound(const FString& Filename);
UEdGraphPin* ShotLength(FGraph& G, UEdGraphPin* Vector);
UEdGraphPin* ShotScale(FGraph& G, UEdGraphPin* Vector, const TCHAR* Scalar, UEdGraphPin* Value=nullptr);
UK2Node_Event* ShotEvent(FGraph& G,FName Name);
void StopShot(FGraph& G);
UClass* CreateRailgunShot();

void AddRailgunFire(FGraph& G);
}
