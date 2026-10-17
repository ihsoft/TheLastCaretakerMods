#pragma once
// Editor-only mirror, never shipped as a DLL. Steam 25956314, UE 5.8,
// Method: reviewed reflection mappings and stock cable/socket JSON.
// On a new build, use the producer's fresh-stock gates and a game test;
// investigate names/types/ownership if either fails. No binary layout claim.
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
