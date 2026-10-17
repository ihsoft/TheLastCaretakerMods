#pragma once

// Editor-only function identity mirror. Never shipped as native code.
// Steam 25056839, UE5.8.1.
// Canonical live jmap reflection: VoyageGameInstance:IsPlaying, const pure bool().
// Revalidate on Steam build ID change.
#include "Engine/GameInstance.h"
#include "VoyageGameInstance.generated.h"

UCLASS()
class VOYAGE_API UVoyageGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Voyage Mirror")
    bool IsPlaying() const { return false; }
};
