// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// executable SHA-256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: reviewed mapping plus the stock Diesel Generator Blueprint function
// contract. Revalidate on fingerprint change. Editor-only: never package this.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PersistentInterface.generated.h"

UINTERFACE(BlueprintType)
class VOYAGE_API UPersistentInterface : public UInterface
{
    GENERATED_BODY()
};

class VOYAGE_API IPersistentInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
    void OnPersistentActorPostLoad(int32 Version);
};
