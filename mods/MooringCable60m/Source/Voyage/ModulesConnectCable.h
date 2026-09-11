#pragma once
// Editor-only mirror, never shipped as a DLL. Steam 25191271, EXE SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: current reflected member registrations and stock asset JSON.
// Revalidate names/types/ownership on executable change. No binary layout claim.
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "ModulesConnectCable.generated.h"

UCLASS()
class VOYAGE_API UModuleSocketComponent : public UBoxComponent
{
    GENERATED_BODY()
};

UCLASS(Blueprintable)
class VOYAGE_API AUtilityCable : public AActor
{
    GENERATED_BODY()
public:
    AUtilityCable();
};

UCLASS(Blueprintable)
class VOYAGE_API AModulesConnectCable : public AUtilityCable
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxLength = 0;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UModuleSocketComponent> CachedSocketA;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UModuleSocketComponent> CachedSocketB;
};
