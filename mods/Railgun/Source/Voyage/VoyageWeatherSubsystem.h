#pragma once

// Game-derived UE 5.8 editor mirror. Provenance: Voyage Steam build 25191271.
// Reconstructed with Inspect-VoyageAsset.ps1 from the reviewed mapping and
// the stock projectile's cooked AddFluidImpulse call. Revalidate whenever that
// Steam build ID changes.

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoyageWeatherSubsystem.generated.h"

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageFluidImpulse
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Direction = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Strength = 0.0f;
};

UCLASS(Abstract)
class VOYAGE_API UVoyageWeatherSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual TStatId GetStatId() const override { return TStatId(); }

    UFUNCTION(BlueprintCallable)
    void AddFluidImpulse(FVoyageFluidImpulse Impulse) {}
};
