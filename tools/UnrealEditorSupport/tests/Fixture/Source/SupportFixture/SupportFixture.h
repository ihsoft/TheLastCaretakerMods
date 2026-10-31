#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SupportFixture.generated.h"

// Test-only native identity, independent of any game or Blueprint tooling.
UCLASS(BlueprintType)
class SUPPORTFIXTURE_API USupportFixtureAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Support Test")
    int32 SampleValue = 42;
};
