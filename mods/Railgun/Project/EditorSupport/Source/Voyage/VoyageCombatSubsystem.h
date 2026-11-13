#pragma once
// Editor-only named-bytecode mirrors. Steam25191271 / UE5.8,
// Reconstructed from reviewed mappings, stock PerformAttack and native registration.
// Revalidate on Steam build ID change. NEVER serialize FVoyageAttack defaults/CDOs.
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") TSubclassOf<UVoyageDamageType> DamageTypeClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float Damage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float DamageVariance = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float Velocity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float MaxRange = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float InnerRadius = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float OuterRadius = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") float ImpulseOverride = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") ECombatAttackType AttackType = ECombatAttackType::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") FHitResult Hit;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") TWeakObjectPtr<AActor> Target;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") TWeakObjectPtr<AController> Instigator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") TWeakObjectPtr<AActor> DamageCauser;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoyageAttack") int32 AttackID = 0;
};

UCLASS(Abstract)
class VOYAGE_API UVoyageCombatSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual TStatId GetStatId() const override { return TStatId(); }
    UFUNCTION(BlueprintCallable, Category="") void RegisterAttack(FVoyageAttack Attack, bool bAcceptDuration) {}
};
