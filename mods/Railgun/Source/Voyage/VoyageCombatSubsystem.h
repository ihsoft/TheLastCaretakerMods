#pragma once
// Editor-only named-bytecode mirrors. Steam25191271 / UE5.8,
// EXE747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Reconstructed from reviewed mappings, stock PerformAttack and native registration.
// Revalidate on executable change. NEVER serialize FVoyageAttack defaults/CDOs,
// use sizeof as shipping ABI, or add padding for unreflected native trailing state.
// The shipping constructor owns all omitted fields and private state.
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Controller.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoyageCombatSubsystem.generated.h"

UCLASS(Const)
class VOYAGE_API UVoyageDamageType : public UDamageType { GENERATED_BODY() };

UENUM(BlueprintType)
enum class ECombatAttackType : uint8 { Unknown=0, Directional=1, Radial=2, AreaEffect=3, Force=4, Count=5 };

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageAttack
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) TSubclassOf<UVoyageDamageType> DamageTypeClass;
    UPROPERTY(BlueprintReadWrite) float Damage = 0;
    UPROPERTY(BlueprintReadWrite) float DamageVariance = 0;
    UPROPERTY(BlueprintReadWrite) float ImpulseOverride = 0;
    UPROPERTY(BlueprintReadWrite) ECombatAttackType AttackType = ECombatAttackType::Unknown;
    UPROPERTY(BlueprintReadWrite) FHitResult Hit;
    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Target;
    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AController> Instigator;
    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> DamageCauser;
    UPROPERTY(BlueprintReadWrite) int32 AttackID = 0;
};

UCLASS(Abstract)
class VOYAGE_API UVoyageCombatSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual TStatId GetStatId() const override { return TStatId(); }
    UFUNCTION(BlueprintCallable) void RegisterAttack(FVoyageAttack Attack, bool bAcceptDuration) {}
};
